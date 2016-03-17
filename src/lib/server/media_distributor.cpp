#include "server/media_distributor.h"
#include "common/tsparse.h"
#include <boost/foreach.hpp>

using namespace p2server;

namespace{

	void dummy_fun_1(const error_code&){}
	void dummy_fun_2(const error_code& ec, uint32_t){
		DEBUG_SCOPE(
			if (ec)
				std::cout << "dummy_func_2: " << ec.message() << std::endl;
		);
	}

	boost::function<void(const error_code&)>
		dummy_hundler_1(boost::bind(&dummy_fun_1, _1));
	boost::function<void(const error_code& ec, uint32_t)>
		dummy_hundler_2(boost::bind(&dummy_fun_2, _1, _2));
}
void media_distributor::register_message_handler(seed_connection_sptr conn)
{
#define REGISTER_HANDLER(msgType, handler)\
	conn->register_message_handler(msgType, boost::bind(&this_type::handler, this, conn.get(), _1));

	REGISTER_HANDLER(global_msg::media_request, on_recvd_media_request);
	REGISTER_HANDLER(server_peer_msg::info_report, on_recvd_info_report);

#undef REGISTER_HANDLER
}

media_distributor::media_distributor(io_service& net_svc, server_param_sptr param)
	:basic_engine_object(net_svc)
	, basic_server_object(param)
	, media_bps_(millisec(4500))
	, media_bps_short_(millisec(500))
	, total_out_bps_(millisec(4000))
	, push_out_bps_(millisec(4000))
	, pull_out_bps_(millisec(4000))
	, media_packet_rate_(millisec(500))
	, media_packet_rate_long_(seconds(35))
	, current_push_multiple_(1)
	, packet_buffer_(millisec(MAX_DELAY_GUARANTEE), is_vod_category(param->type))
	, running_(false)
	, last_alf_(1.0)
	, send_multiples_(is_interactive_category(param->type) ? 8 : 128, 0)//���뱻0xffff+1����
	, push_distrib_smoother_(millisec(5000), millisec(1000), millisec(SMOOTH_TIME / 2), millisec(SMOOTH_TIME / 2), millisec(SMOOTH_TIME / 2), 480 * 1024 / 8, net_svc)
	, pull_distrib_smoother_(millisec(2000), millisec(1000), millisec(SMOOTH_TIME), millisec(SMOOTH_TIME), millisec(SMOOTH_TIME), 480 * 1024 / 8, net_svc)
{
	set_obj_desc("media_distributor");

	last_change_super_seed_time_ =
		last_modify_push_multiple_time_ =
		last_recommend_seed_time_ = ptime_now();

	cas_string_ = security_policy::generate_shared_key(get_server_param_sptr()->channel_uuid, "");

	//��ȡseqno(���ܷ������쳣�˳������������������ݿ��seqno��Ҫ���������ԣ�
	//��ʹ��崻�ǰ�����߽ڵ���������������)
	if (!is_vod_category(get_server_param_sptr()->type))
	{
		std::string channelIdHex = string_to_hex(get_server_param_sptr()->channel_uuid);
		FILE* fp = fopen(channelIdHex.c_str(), "rb");
		if (fp)
		{
			fseek(fp, 0, SEEK_END);
			if (ftell(fp) >= int(sizeof(media_pkt_seq_seed_) + sizeof(session_id_)))
			{
				fseek(fp, 0, SEEK_SET);
				fread((char*)&session_id_, 1, sizeof(session_id_), fp);
				fread((char*)&media_pkt_seq_seed_, 1, sizeof(media_pkt_seq_seed_), fp);
				++session_id_;
			}
			fclose(fp);
		}
	}
	BOOST_ASSERT((0xffff + 1) % send_multiples_.size() == 0);
}

media_distributor::~media_distributor()
{
	stop();
}

void media_distributor::stop()
{
	if (!running_)
		return;
	running_ = false;

	close_acceptor();

	pull_distrib_smoother_.stop();
	push_distrib_smoother_.stop();

	if (seed_peer_check_timer_)
	{
		seed_peer_check_timer_->cancel();
		seed_peer_check_timer_.reset();
	}
	if (piece_notify_timer_)
	{
		piece_notify_timer_->cancel();
		piece_notify_timer_.reset();
	}
	if (store_info_thread_)
	{
		store_info_thread_->join();
		store_info_thread_.reset();
	}

	BOOST_FOREACH(seed_connection_sptr conn, super_seed_peers_)
	{
		conn->close();
	}
	super_seed_peers_.clear();
	BOOST_FOREACH(seed_connection_sptr conn, seed_peers_)
	{
		conn->close();
	}
	seed_peers_.clear();
}
void media_distributor::start(peer_info& local_info, error_code& ec)
{
	stop();

	running_ = true;

	//����acceptor
	//domain����ʽ��"tracker_service/channel_uuid"
	last_erase_super_seed_time_ = ptime_now();
	std::string domain = server_and_peer_demain + "/" + get_server_param_sptr()->channel_uuid;

	endpoint edp;
	edp.port(local_info.internal_udp_port());
	start_acceptor(edp, domain, ec);

	endpoint udp_edp = urdp_acceptor_->local_endpoint(ec);
	if (!ec)
	{
		if (!local_info.has_internal_udp_port() || local_info.internal_udp_port() == 0)
		{
			local_info.set_internal_udp_port(udp_edp.port());
			//local_info.set_internal_ip(udp_edp.address().to_v4().to_ulong());
		}
	}
	endpoint tcp_edp = trdp_acceptor_->local_endpoint(ec);
	if (!ec)
	{
		local_info.set_internal_tcp_port(tcp_edp.port());
		if (!local_info.has_external_tcp_port() || local_info.external_tcp_port() == 0)
		{
			local_info.set_external_tcp_port(tcp_edp.port());
			//local_info.set_external_ip(tcp_edp.address().to_v4().to_ulong());
		}
	}

	seed_peer_check_timer_ = timer::create(get_io_service());
	seed_peer_check_timer_->set_obj_desc("server::media_distributor::seed_peer_check_timer_");
	seed_peer_check_timer_->register_time_handler(boost::bind(&this_type::on_check_seed_peer_timer, this));
	seed_peer_check_timer_->async_keep_waiting(milliseconds(500), milliseconds(500));

	piece_notify_timer_ = timer::create(get_io_service());
	piece_notify_timer_->set_obj_desc("server::media_distributor::piece_notify_timer_");
	piece_notify_timer_->register_time_handler(boost::bind(&this_type::on_piece_notify_timer, this));
	piece_notify_timer_->async_keep_waiting(milliseconds(50), milliseconds(50));

	store_info_thread_.reset(new boost::thread(boost::bind(&this_type::store_info, this)));
	DEBUG_SCOPE(
		std::cout << "media_distributor listen port external:" << local_info.external_tcp_port()
		<< "port2:" << local_info.external_udp_port()
		<< "internal " << local_info.internal_udp_port()
		<< "port2 " << local_info.internal_tcp_port() << std::endl;
	);
}

bool media_distributor::try_accept_seed(seed_connection_sptr conn,
	seed_peer_set& peerset, size_t maxCnt, bool shrink)
{
	if (!running_)
		return false;

	ptime now = ptime_now();
	//����ڵ��Ѿ�����seed�ڵ����������ֵ����ɾ��һ���ڵ�
	if (peerset.size() >= maxCnt)
	{
		if (shrink)
		{
			//TODO: shrink
		}
	}
	if (peerset.size() < maxCnt)
	{
		//���Բ���
		if (!peerset.insert(conn).second)
			return false;

		//ע�������¼�����Ϣ�¼��Ĵ�����
		conn->register_connected_handler(boost::bind(&this_type::on_disconnected, this, conn.get(), _1));
		register_message_handler(conn);
		conn->keep_async_receiving();
		return true;
	}
	return false;
}

void media_distributor::on_accepted(seed_connection_sptr conn, const error_code& ec)
{
	if (!running_)
		return;

	if (!ec)
	{
		int super_seed_cnt = std::max(static_cast<int>(2 * current_push_multiple_)
			, get_server_param_sptr()->min_super_seed_peer_cnt);
		int seed_cnt = get_server_param_sptr()->seed_peer_cnt;

		////////////////////////////////////////////////////////////////////////
		//fortest
#ifdef TEST_MIN_SEED
		super_seed_cnt=1;
		seed_cnt = 1;
#endif

		ptime now = ptime_now();
		/*
		if(try_accept_seed(conn, super_seed_peers_, super_seed_cnt, false))
		{
		conn->set_super_seed(now, server_param_->server_seed_ping_interval);
		}
		else */
		if (try_accept_seed(conn, seed_peers_, seed_cnt, true))
		{
			conn->set_seed(now, get_server_param_sptr()->server_seed_ping_interval);
		}
		else
		{
			//std::cout<<__LINE__<<std::endl;
			conn->close();
		}
	}
	else
	{
		//std::cout<<__LINE__<<std::endl;
		if (conn)
			conn->close();
	}
}

void media_distributor::on_disconnected(seed_connection* conn, const error_code&ec)
{
	if (!running_)
		return;

	seed_connection_sptr ptr = conn->shared_obj_from_this<seed_connection>();
	if (conn->is_super_seed())
	{
		super_seed_peers_.erase(ptr);
	}
	else if (conn->is_normal_seed())//һ��seed
	{
		seed_peers_.erase(ptr);
	}
	else//normal?
	{
		super_seed_peers_.erase(ptr);
		seed_peers_.erase(ptr);
	}
}

void media_distributor::on_check_seed_peer_timer()
{
	if (!running_)
		return;

	//�Ż�����
	ptime now = ptime_now();
	if (last_change_super_seed_time_ + seconds(3) < now)
	{
		if (!super_seed_peers_.empty() && !seed_peers_.empty())
		{
			seed_peer_set::reverse_iterator seedItr = seed_peers_.rbegin();
			seed_peer_set::iterator superItr = super_seed_peers_.begin();
			seed_connection_sptr conn = (*seedItr);
			seed_connection_sptr superConn = (*superItr);
			if (conn->score() > superConn->score()*1.5
				&&superConn->be_super_seed_timestamp() + seconds(25) < now
				)
			{
				//����λ��
				seed_peers_.erase(--seedItr.base());
				super_seed_peers_.erase(superItr);

				super_seed_peers_.insert(conn);
				seed_peers_.insert(superConn);

				conn->set_super_seed(now, get_server_param_sptr()->server_seed_ping_interval);
				superConn->set_seed(now, get_server_param_sptr()->server_seed_ping_interval);

				last_change_super_seed_time_ = now;
			}
		}
	}

	if (seed_peers_.size() >= (size_t)get_server_param_sptr()->seed_peer_cnt//seed���㹻��
		|| seed_peers_.empty()//seed��Ϊ0
		)
		return;

	//�ڵ������������ѡ��һ���ڵ㷢��һ��seed�Ƽ�ָ��
#if 0//��һ������ʱδ����
	if(last_recommend_seed_time_+seconds(1)<now)
	{
		last_recommend_seed_time_=now;
		s2p_recommend_seed_msg msg;
		msg.set_ttl(6);
		if (seed_peers_.size()>0)
		{
			seed_peer_set::iterator itr=random_select(seed_peers_.begin(), seed_peers_.size());
			if((*itr)->m_socket&&(*itr)->m_socket->is_connected())
				(*itr)->m_socket->async_send_reliable(serialize(msg), server_peer_msg::recommend_seed);
			else
				seed_peers_.erase(itr);
		}
		else if (super_seed_peers_.size()>0)
		{		
			seed_peer_set::iterator itr=random_select(super_seed_peers_.begin(), super_seed_peers_.size());
			if((*itr)->m_socket&&(*itr)->m_socket->is_connected())
				(*itr)->m_socket->async_send_reliable(serialize(msg), server_peer_msg::recommend_seed);
			else
				super_seed_peers_.erase(itr);
		}
}
#endif
}

void media_distributor::on_piece_notify_timer()
{
	if (!running_)
		return;

	s2p_piece_notify msg;
	buffermap_info* info = msg.mutable_buffermap();
	if (is_interactive_category(get_server_param_sptr()->type))
	{
		info->add_recent_seqno(media_pkt_seq_seed_ - 2);
		info->set_bigest_seqno_i_know(media_pkt_seq_seed_ - 2);
	}
	else
	{
		info->add_recent_seqno(media_pkt_seq_seed_ - 64);//�����������µ�һЩƬ�Σ�����ͻ��˹�������
		info->set_bigest_seqno_i_know(media_pkt_seq_seed_ - 64);
	}

	safe_buffer sndbuf = serialize(msg);
	ptime now = ptime_now();
	seed_peer_set& peersSet = ((0 == ((++piece_notify_times_) % 3)) ? seed_peers_ : super_seed_peers_);
	seed_peer_set::iterator itr = peersSet.begin();
	for (; itr != peersSet.end();)
	{
		seed_connection& sp = *(*itr);
		if (!sp.is_connected())
		{
			sp.close(false);
			peersSet.erase(itr++);
			continue;
		}
		//notify piece
		sp.piece_notify(sndbuf, now);
		//confirm sent
		sp.piece_confirm(now);
		++itr;
	}
}

//////////////////////////////////////////////////////////////////////////
//ý��ַ�
void media_distributor::__do_distribute(const media_packet& p, int ptsOffset, bool beFecPacket)
{
	timestamp_t now = timestamp_now();
	media_packet& pkt = const_cast<media_packet&>(p);

	media_pkt_seq_seed_++;
	media_bps_ += pkt.buffer().length();
	media_packet_rate_ += 1;
	media_packet_rate_long_ += 1;

	///��������header
	int media_packet_rate = (int)media_packet_rate_.bytes_per_second();
	pkt.set_seqno(media_pkt_seq_seed_);
	pkt.set_time_stamp(now);
	pkt.set_session_id(session_id_);
	pkt.set_hop(0);
	pkt.set_packet_rate(media_packet_rate);

	//����keyframe
	if (!last_iframe_seqno_ || seqno_minus(media_pkt_seq_seed_, *last_iframe_seqno_) > 50)
	{
		ts_t ts;
		unsigned char* tsDataPtr = buffer_cast<unsigned char*>(pkt.payload());
		if (ts_parse().exist_keyframe(tsDataPtr, pkt.payload().size(), &ts) >= 0)
		{
			last_iframe_seqno_ = media_pkt_seq_seed_;
			pkt.set_priority(I_FRAME);
			iframe_list_.push_back(media_pkt_seq_seed_);
			if (iframe_list_.size() > 16)
				iframe_list_.pop_front();
		}
	}

	//cas
	security_policy::cas_mediapacket(pkt, cas_string_);
	//����anti pollution
	boost::int64_t sig = security_policy::signature_mediapacket(pkt, get_server_param_sptr()->channel_uuid);
	pkt.set_anti_pollution_signature(sig);

	//����Я����buffermap
	boost::uint64_t seqnoVec;
	char* pvec = (char*)&seqnoVec;
	for (size_t i = 0; i < sizeof(seqnoVec) / sizeof(seqno_t); ++i)
	{
		write_int_hton<seqno_t>(media_pkt_seq_seed_, pvec);
	}
	pkt.set_recent_seqno_map(seqnoVec);

	//��ý�����������
	packet_buffer_.insert(pkt, media_packet_rate, now);

	//�ַ�
	get_slot(send_multiples_, media_pkt_seq_seed_) = 0;
	distribution_push(pkt);

	//do FEC
	if (beFecPacket)
		return;
	fec_results_.clear();
	if (fec_encoder_(pkt, fec_results_))
	{
		for (size_t i = 0; i < fec_results_.size(); ++i)
		{
			__do_distribute(fec_results_[i], 0, true);
		}
	}
}

void media_distributor::write_packet(const media_packet& pkt)
{
	if (!running_)
		return;

	server_param_sptr paramSptr = get_server_param_sptr();
	if (!paramSptr->enable_live_cache)
		return;

	if (!async_dskcache_)
	{
		async_dskcache_ = media_cache_service::create(get_io_service());

		//�������ʼ����Ӧʱ����ļ���С
		async_dskcache_->open(paramSptr->name,
			paramSptr->live_cache_dir,
			paramSptr->channel_uuid,
			paramSptr->max_duration,
			paramSptr->max_length_per_file,
			dummy_hundler_1
			);
	}

	safe_buffer data = pkt.buffer().buffer_ref(media_packet::format_size());

	async_dskcache_->write_piece(pkt.get_seqno(), data, dummy_hundler_2);

	async_dskcache_->recal_cache_limit(pkt.get_packet_rate(), data.length());
}

void media_distributor::smooth_distribute(const media_packet& p)
{
	if (!running_)
		return;

	push_distrib_smoother_.push(0,
		boost::bind(&this_type::do_distribute, this, p, 0),
		p.buffer().length()
		);
}

void media_distributor::relay_distribute(const media_packet& p)
{
	if (!running_)
		return;

	media_packet& pkt = (media_packet&)p;
	//�������ʺͰ���
	media_bps_ += pkt.buffer().length();
	//media_bps_short_+=pkt.buffer().length();
	media_packet_rate_ += 1;
	media_packet_rate_long_ += 1;
	int media_packet_rate = (int)media_packet_rate_.bytes_per_second();
	//��¼seqno
	media_pkt_seq_seed_ = pkt.get_seqno();

	//��ý�����������
	packet_buffer_.insert(pkt, media_packet_rate);

	//�ַ�
	distribution_push(pkt);
}

void media_distributor::check_super_seed()
{
	ptime now = ptime_now();
	int minSize = (int)current_push_multiple_ + 1;
	int maxSize = 2 * (int)current_push_multiple_ + 1;
	if (minSize < get_server_param_sptr()->min_super_seed_peer_cnt)
		minSize = get_server_param_sptr()->min_super_seed_peer_cnt;
	if (maxSize < minSize)
		maxSize = minSize;

	////////////////////////////////////////////////////////////////////////
	//fortest
#ifdef TEST_MIN_SEED
	minSize = maxSize = 1;
#endif

	//super_seed_peers����������seed
	for (; (int)super_seed_peers_.size() < minSize;)
	{
		if (!seed_peers_.empty())
		{
			seed_peer_set::reverse_iterator itr = seed_peers_.rbegin();
			seed_connection_sptr conn = (*itr);
			if (conn->score() > 0)
			{
				seed_peers_.erase(--itr.base());
				super_seed_peers_.insert(conn);
				conn->set_super_seed(now, get_server_param_sptr()->server_seed_ping_interval);
			}
			else
				break;
		}
		else
			break;
	}
	if ((int)super_seed_peers_.size() > maxSize)
	{
		seed_peer_set::iterator itr = super_seed_peers_.begin();
		seed_connection_sptr conn = (*itr);
		if ((last_erase_super_seed_time_ + seconds(10)) < now
			&& (conn->be_super_seed_timestamp() + seconds(50)) < now
			)
		{
			super_seed_peers_.erase(itr);
			seed_peers_.insert(conn);
			conn->set_seed(now, get_server_param_sptr()->server_seed_ping_interval);
			last_erase_super_seed_time_ = now;
		}
	}
}

//ý��ַ�����˺����㷨��������
void media_distributor::distribution_push(media_packet& pkt)
{
	if (super_seed_peers_.empty() && seed_peers_.empty())
		return;

	pkt.set_is_push(1);//����
	seqno_t seqno = pkt.get_seqno();
	int packetLength = pkt.buffer().length();

	ptime now = ptime_now();
	double media_bps = media_bps_.bytes_per_second();
	//double media_bps_instance=media_bps_short_.bytes_per_second();//˲ʱ����
	double globalLocalToRemoteLostRate = global_local_to_remote_lost_rate();
	if (last_modify_push_multiple_time_ + millisec(500) < now)
	{
		last_modify_push_multiple_time_ = now;

		//���ȣ���������Ӧ���ͱ���
		//����ԭ���ǣ�����������������ƴ��������Сʱ���������Ʊ��ʣ���֮���͡�
		double push_out_bps = push_out_bps_.bytes_per_second();
		double total_out_bps = pull_out_bps_.bytes_per_second() + push_out_bps;//�������������out�ٶȣ�������Ϊ�˼������ͱ�����
		//total_out_bps*=(1.0-globalLocalToRemoteLostRate);
		double real_total_out_bps = total_out_bps_.bytes_per_second();
		//�����б�ʼ�����˼�ǣ���TOTAL_OUTС��PUSH�����A��ʱ���������ͱ���Ϊԭ����a��(���ʱ仯ϵ��=a).
		//�����������Ϊ���ƴ����B��ʱ������������Ϊԭ��b��(���ʱ仯ϵ��=b);
		//���������ͱ��ʱ仯ϵ����(�����/���ƴ���)����һֱ�ߣ�ֱ�߾���(x1, y1)(x2, y2)
		static const double A = 1.0, a = 0.5, B = 2.3, b = 1.5;
		static const double x1 = A, y1 = a, x2 = B, y2 = b;
		static const double slope = (y2 - y1) / (x2 - x1);//б��
		static const double ordinate = y1 - slope*x1;
		double alf = slope*(total_out_bps / (push_out_bps + 1)) + ordinate;
		alf = bound(0.5, alf, 2.0);
		alf = 0.7*alf + 0.3*last_alf_;//���ܱ仯̫��
		alf = bound(0.5, alf, 2.0);
		last_alf_ = alf;
		current_push_multiple_ = (push_out_bps / (media_bps + 1))*alf;
		double seed_cnt = seed_peers_.size() + super_seed_peers_.size();
		if (seed_cnt > 6)
			if (current_push_multiple_ < 3) current_push_multiple_ = 3;
		current_push_multiple_ = bound(2.0, current_push_multiple_, seed_cnt);
		DEBUG_SCOPE(
			if (random01() < 0.1)
			{
			std::cout << "********curent_push_multiple=" << current_push_multiple_
				<< ", totalout/pushout=" << int(total_out_bps / (push_out_bps + 1) * 100) / 100.0
				<< ", alf=" << alf << "(" << alf << ")"
				<< ", seqno=" << media_pkt_seq_seed_
				<< ", lostrate:" << globalLocalToRemoteLostRate
				<< "\nmedia_pps:" << media_packet_rate_.bytes_per_second()
				<< ", media_kbps:" << (int)(media_bps * 8 / 1000)
				<< ", total_out_kbps:" << (int)(real_total_out_bps * 8 / 1000)
				<< "    " << real_total_out_bps / (media_bps + FLT_MIN)
				<< "\n";

			LOG(
				double globalLocalLostRate = global_local_to_remote_lost_rate();
			LogInfo(
				"online_client_count:%d; "
				"media_kbps:%d; "
				"total_out_kbps:%d (%6.2f) "
				"global_local_to_remote_lost_rate:(%1.5f)"
				"\n"
				, (int)(super_seed_peers_.size() + seed_peers_.size())
				, (int)(media_bps * 8 / 1000)
				, (int)(real_total_out_bps * 8 / 1000), (real_total_out_bps / (media_bps + FLT_MIN))
				, globalLocalLostRate
				);
			);
			}
		);
	}

	check_super_seed();

	//��������Ӧ���ͱ����������
	//����˲ʱ����΢�����ͱ���
	//double instance_alf=std::min(std::max(media_bps_instance/(media_bps+FLT_MIN), 0.8), 1.2);
	//double instance_curent_push_multiple=current_push_multiple_*instance_alf;
	double instance_curent_push_multiple = current_push_multiple_;
	int N = (int)(instance_curent_push_multiple);
	if (instance_curent_push_multiple > N)
		N += (in_probability(instance_curent_push_multiple - N) ? 0 : 1);
	if (N < 2)
		N += (in_probability(globalLocalToRemoteLostRate) ? 1 : 0);
	int n = std::min(N, (int)super_seed_peers_.size() / 4 + 1);
	size_t totalSendLen = 0;
	double prob = (double)n / ((double)super_seed_peers_.size() + FLT_MIN);
	std::multimap<double, seed_connection*> candidates;
	pushed_ips_.clear();
	seed_peer_set::reverse_iterator itr = super_seed_peers_.rbegin();
	for (; itr != super_seed_peers_.rend() && n > 0;)
	{
		seed_connection_sptr conn = *itr;
		double aliveProbability = conn->alive_probability();

		if (!conn->is_connected())
		{
			super_seed_peers_.erase(--itr.base());
			conn->close();
			continue;
		}
		else if (aliveProbability <= FLT_MIN && (conn->be_super_seed_timestamp() + seconds(8)) < now)
		{
			super_seed_peers_.erase(--itr.base());
			conn->set_seed(now, get_server_param_sptr()->server_seed_ping_interval);
			seed_peers_.insert(conn);
			continue;
		}

		//������ͳɹ���--n;
		//���ͳ�����ζ�����ӶϿ��ˣ�message_soceket���Զ��ص���Ӧ���������������ﲻ��Ҫ���⴦�����
		error_code ec;
		unsigned long ip = conn->remote_endpoint(ec).address().to_v4().to_ulong();
		ip &= 0xffffff00UL;
		double remoteToLocalLostRate = conn->remote_to_local_lost_rate();
		double localToRemoteLostRate = conn->local_to_remote_lost_rate();
		double lostRate = (remoteToLocalLostRate*0.25 + localToRemoteLostRate*0.75);
		//std::cout<<"aliveProbability="<<aliveProbability<<", lostRate="<<lostRate<<"\n";
		double score = (1.0 - 0.5*lostRate)*(aliveProbability + FLT_MIN)*conn->score();
		if (aliveProbability >= ALIVE_GOOD_PROBABILITY//����
			&&lostRate < 0.02//�����ʸ߲�push
			)
		{
			bool alreadyPushedIP = (pushed_ips_.find(ip) != pushed_ips_.end());
			if (!alreadyPushedIP//���ڴ���ͬһ���������ڵĽڵ㣬�����ڽ�������һ��
				//&&((N-n)<N/4||in_probability(prob))
				)
			{
				--n;
				totalSendLen += packetLength;
				conn->send_media_packet(pkt.buffer(), seqno, now);
				pushed_ips_.insert(ip);
			}
			else
			{
				if (alreadyPushedIP)
					candidates.insert(std::make_pair(-0.5*score, conn.get()));//���ź�
				else
					candidates.insert(std::make_pair(-score, conn.get()));//
			}
		}
		else
			candidates.insert(std::make_pair(-0.9*score, conn.get()));//���ź�
		++itr;
	}
	for (; n > 0 && !candidates.empty();)
	{
		seed_connection_sptr conn = candidates.begin()->second
			->shared_obj_from_this<seed_connection>();
		if (conn->alive_probability() > ALIVE_DROP_PROBABILITY)
		{
			--n;
			totalSendLen += packetLength;
			conn->send_media_packet(pkt.buffer(), seqno, now);
		}
		candidates.erase(candidates.begin());
		////����ĩβ��һ��bad_candidate��super��ɾ��
		//if (candidates.empty()
		//	&&(int)super_seed_peers_.size()>current_push_multiple_+1
		//	&&seed_peers_.size()>0
		//	&&conn->alive_probability()<ALIVE_DROP_PROBABILITY
		//	)
		//{
		//	if (conn->be_super_seed_timestamp()+seconds(10)<now)
		//	{
		//		super_seed_peers_.erase(conn);

		//		conn->score(0);//��score����Ϊ0
		//		seed_peers_.insert(conn);

		//		conn->set_seed(now);

		//		last_erase_super_seed_time_=now;
		//	}
		//}
	}
	total_out_bps_ += totalSendLen;
	push_out_bps_ += totalSendLen;
	get_slot(send_multiples_, seqno) = (1.0 - globalLocalToRemoteLostRate)*(N - n);
}

void media_distributor::on_recvd_media_request(seed_connection* conn, safe_buffer buf)
{
	//GUARD_PALDEPORT;
	/*DEBUG_SCOPE(
		std::cout<<"media distributor: media request"<<std::endl;
		);*/
	if (!running_)
		return;

	media_request_msg msg;
	if (!parser(buf, msg))
		return;
	timestamp_t now = timestamp_now();

	//seed_peer_set::iterator itr;
	//if (conn->is_super_seed())
	//{
	//	itr=super_seed_peers_.find(conn->shared_obj_from_this<seed_connection>());
	//	if (itr==super_seed_peers_.end())
	//		return;
	//}
	//else
	//{
	//	itr=seed_peers_.find(conn->shared_obj_from_this<seed_connection>());
	//	if (itr==seed_peers_.end())
	//		return;
	//}
	seqno_t minseq, bigseq, seq;
	bool needSendWait = false;
	int waitTime = 0;
	boost::shared_ptr<seed_connection> connSptr
		= conn->shared_obj_from_this<seed_connection>();

	for (int i = 0; i < msg.seqno_size(); ++i)
	{
		seq = msg.seqno(i);
		if (packet_buffer_.has(seq))
		{
			pull_distrib_smoother_.push(((uintptr_t)conn) % 64,
				boost::bind(&this_type::do_pull_distribute, this,
				connSptr, seq, msg.direct_request() != 0, _1),
				1450
				);
		}
		else
		{
			if (packet_buffer_.smallest_seqno_in_cache(minseq)
				&& seqno_less(seq, minseq)
				)
			{
				needSendWait = true;
				waitTime = seqno_minus(minseq, seq) * 1000 / 50;
				break;
			}
		}
	}
	if (needSendWait)
	{
		no_piece_msg msg;
		msg.set_wait_time(waitTime);
		msg.set_min_seqno(minseq);
		if (packet_buffer_.bigest_seqno_in_cache(bigseq))
			msg.set_max_seqno(bigseq);
		msg.set_seqno(seq);
		conn->async_send_unreliable(serialize(msg), global_msg::no_piece);
	}
}

void media_distributor::do_pull_distribute(
	seed_connection_sptr sock,
	seqno_t seq,
	bool direct,
	int smoothDelay)
{
	(void)(smoothDelay);
	if (!sock->is_connected())
		return;
	ptime now = ptime_now();
	int seedCnt = (int)(seed_peers_.size() + super_seed_peers_.size());
	double remoteToLocalLostRate = (sock->remote_to_local_lost_rate());
	double localToRemoteLostRate = (sock->local_to_remote_lost_rate());
	double lostRate = (remoteToLocalLostRate*0.125 + localToRemoteLostRate*0.875);
	if (lostRate > 0.35&&seedCnt > 5000
		|| lostRate > 0.5&&seedCnt > 1000
		)//���ض���������������Ӧ
	{
		sock->media_have_sent(seq, now);
		return;
	}

	//�������أ���һ�����ʷ�������
	//double globalLocalLostRate=global_local_to_remote_lost_rate();
	if (localToRemoteLostRate > 0.60
		&&seedCnt*localToRemoteLostRate > 500
		&& in_probability(localToRemoteLostRate)
		)
	{
		sock->media_have_sent(seq, now);
		return;
	}

	//���ڶ�ʱ���ڴ�����Ӧͬһ��seqno������
	if (seqno_minus(media_pkt_seq_seed_, seq) <= (int)send_multiples_.size())
	{
		double maxmultiple = 2.0*current_push_multiple_;
		if (get_slot(send_multiples_, seq) <= maxmultiple
			|| in_probability(0.15)
			)
		{
			get_slot(send_multiples_, seq) += (1.0 - localToRemoteLostRate);
		}
		else
		{
			sock->media_have_sent(seq, now);
			return;
		}
	}
	media_packet pkt;
	if (packet_buffer_.get(pkt, seq))
	{
		//����Я��buffermap
		seqno_t seqForBufferMap = media_pkt_seq_seed_;
		if (!is_interactive_category(get_server_param_sptr()->type))
			seqForBufferMap -= 128;
		boost::uint64_t seqnoVec;
		char* pvec = (char*)&seqnoVec;
		for (size_t j = 0; j < sizeof(seqnoVec) / sizeof(seqno_t); ++j)
		{
			write_int_hton<seqno_t>(seqForBufferMap, pvec);
		}
		pkt.set_recent_seqno_map(seqnoVec);
		pkt.set_is_push(0);

		total_out_bps_ += pkt.buffer().length();
		if (sock->score()>0)
			pull_out_bps_ += pkt.buffer().length();

		if (direct)
		{//��ֱ��ͨ����ֱ�ӷ���
			//(*((*itr)->m_server_upload_speedmeter))+=pkt.buffer().length();
			sock->send_media_packet(pkt.buffer(), seq, now);
		}
		else
		{//û��ֱ��ͨ����ͨ��hub�м�
			BOOST_ASSERT(0 && "��δʵ��");
			//pdpt->get_hub()->relay_to(pkt.buffer(), msg.peer_id(), SYSTEM_LEVEL_RELAY_MSG);
		}
	}
	else
	{

	}
}

void media_distributor::on_recvd_info_report(seed_connection* sock, safe_buffer buf)
{
	p2s_info_report_msg msg;
	if (!parser(buf, msg))
		return;

	seed_connection_sptr conn = sock->shared_obj_from_this<seed_connection>();
	seed_peer_set& peersSet = (sock->is_super_seed() ? super_seed_peers_ : seed_peers_);
	seed_peer_set::iterator itr(peersSet.find(conn));
	if (itr != peersSet.end())
	{
		double scareValue = msg.upload_speed()*(1.0 - 0.5*msg.lost_rate());
		if (conn->score() != scareValue)
		{
			//���������£�ɾ�������룻�Դ�������
			peersSet.erase(itr);
			conn->score(scareValue);
			if (scareValue <= 0 && sock->is_super_seed())//��������Ϊsuperseed��
			{
				ptime now = ptime_now();
				conn->set_seed(now, get_server_param_sptr()->server_seed_ping_interval);
				seed_peers_.insert(conn);
			}
			else
				peersSet.insert(conn);
		}
	}
}

void media_distributor::store_info()
{
	if (is_vod_category(get_server_param_sptr()->type))
		return;

	int i = 0;
	int msec = 200;
	std::string channelIdHex = string_to_hex(get_server_param_sptr()->channel_uuid);
	FILE* fp = fopen(channelIdHex.c_str(), "wb");
	while (running_)
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(msec));
		++i;
		if (i > (2000 / msec))//ÿ����дһ��
		{
			if (!fp)
				fp = fopen(channelIdHex.c_str(), "wb");
			if (fp)
			{
				seqno_t seq = media_pkt_seq_seed_;
				while (seq != media_pkt_seq_seed_)
				{
					seq = media_pkt_seq_seed_;
				}
				fseek(fp, 0, SEEK_SET);
				fwrite(&seq, 1, sizeof(seq), fp);
				fwrite(&session_id_, 1, sizeof(session_id_), fp);
				fflush(fp);
				fclose(fp);
			}
		}
	}
}
