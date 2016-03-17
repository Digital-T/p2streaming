#include "tracker/member_service.h"
#include "tracker/tracker_service.h"
#include "tracker/tracker_service_logic.h"

using namespace p2tracker;

typedef member_table::peer peer;

static const time_duration CHANNEL_INFO_BROADCAST_INTERVAL=seconds(1);

void member_service::register_message_handler(message_socket* conn)
{
#define REGISTER_HANDLER(msgType, handler)\
	conn->register_message_handler(msgType, boost::bind(&this_type::handler, this, conn, _1));

	REGISTER_HANDLER(tracker_peer_msg::login, on_recvd_login);
}

member_service::member_service(io_service& net_svc, tracker_param_sptr param)
	: basic_engine_object(net_svc)
	, basic_tracker_object(param)
	, channel_change_cnt_(0)
{
	set_obj_desc("member_service");
}

member_service::~member_service()
{
	stop();
}

void member_service::__start(boost::shared_ptr<tracker_service_logic_base> tsLogic, 
	const std::string& channelID
	)
{
	BOOST_ASSERT(!urdp_acceptor_);
	BOOST_ASSERT(!trdp_acceptor_);

	tracker_service_logic_=tsLogic;

	//Ƶ��
	channel::shared_ptr this_channel = get_channel(channelID);
	if (!this_channel) 
		return;

	//�����������domain��tracker_and_peer_demain+"/"+channelID�������ӵ������ľͶ���channelID�Ķ���
	//member_serviceҪ���ܶ��Ƶ�������ӣ����в������ض���channelID.
	const std::string domain=tracker_and_peer_demain/*+"/"+channelID*/;
	endpoint edp = endpoint_from_string<endpoint>(
		get_tracker_param_sptr()->internal_ipport
		);

	error_code ec;
	start_acceptor(edp, domain, ec);
}

//����ֱ��Ƶ��
void member_service::start(boost::shared_ptr<tracker_service_logic_base>tsLogic, 
	const peer_info& serverInfo, 
	const message_socket_sptr server_socket, 
	const live_channel_info& channelInfo
	)
{
	const std::string& channelLink=channelInfo.channel_uuid();//live��link��uuidһ��
	start_any_channel(
		tsLogic, 
		channelLink, 
		serverInfo, 
		server_socket, 
		channelInfo
		);
	//����Ƶ���ӽ���ʱ�ͶԶ�ʱ������һ�θ���
	//update_channel_broadcast_timer();
}
//����һ���㲥Ƶ��
void member_service::start(boost::shared_ptr<tracker_service_logic_base>tsLogic, 
	const peer_info& serverInfo, 
	const message_socket_sptr server_socket, 
	const vod_channel_info& channelInfo)
{
	const std::string& channelLink=channelInfo.channel_link();
	start_any_channel(
		tsLogic, 
		channelLink, 
		serverInfo, 
		server_socket, 
		channelInfo
		);
}

template<typename ChannelInfoType>
void member_service::start_any_channel(boost::shared_ptr<tracker_service_logic_base>tsLogic, 
	const std::string& channelID, 
	const peer_info& serverInfo, 
	const message_socket_sptr server_socket, 
	const ChannelInfoType& channelInfo)
{
	if(!create_channel(channelID, server_socket))
		return;

	if (!urdp_acceptor_ && !trdp_acceptor_)
		__start(tsLogic, channelID);

	channel::shared_ptr this_channel = get_channel(channelID);
	if (!this_channel) 
		return;

	peer_info realServerInfo=serverInfo;
	if (server_socket->connection_category()==message_socket::UDP)
	{
		p2engine::error_code ec;
		if (get_tracker_param_sptr()->b_for_shunt)
		{
			BOOST_AUTO(edp, endpoint_from_string<variant_endpoint>(
				get_tracker_param_sptr()->external_ipport));
			realServerInfo.set_external_ip(edp.address().to_v4().to_ulong());
			realServerInfo.set_external_udp_port(edp.port());
			DEBUG_SCOPE(
				std::cout<<"----------"<<edp<<std::endl;
			std::cout<<"server from UDP( "<<edp<<" )"<<std::endl;
			);
		}
		else
		{
			BOOST_AUTO(edp, server_socket->remote_endpoint(ec));
			realServerInfo.set_external_udp_port(edp.port());
			DEBUG_SCOPE(
				std::cout<<"----------"<<edp<<std::endl;
			std::cout<<"server from UDP( "<<edp<<" )"<<std::endl;
			);
		}
	}
	else
	{
		p2engine::error_code ec;
		if (get_tracker_param_sptr()->b_for_shunt)
		{
			BOOST_AUTO(edp, endpoint_from_string<variant_endpoint>(
				get_tracker_param_sptr()->external_ipport));
			realServerInfo.set_external_ip(edp.address().to_v4().to_ulong());
			realServerInfo.set_external_tcp_port(edp.port());
			DEBUG_SCOPE(
				std::cout<<"----------"<<edp<<std::endl;
			std::cout<<"server from TCP( "<<edp<<" )"<<std::endl;
			);
		}
		else
		{
			BOOST_AUTO(edp, server_socket->remote_endpoint(ec));
			realServerInfo.set_external_tcp_port(edp.port());
			DEBUG_SCOPE(
				std::cout<<"----------"<<edp<<std::endl;
			std::cout<<"server from TCP( "<<edp<<" )"<<std::endl;
			);
		}
	}

	DEBUG_SCOPE(
		std::cout<<"server udp org internal: "<<internal_udp_endpoint(serverInfo)<<std::endl;
	std::cout<<"server udp org external: "<<external_udp_endpoint(serverInfo)<<std::endl;
	std::cout<<"server udp real external: "<<external_udp_endpoint(realServerInfo)<<std::endl;
	std::cout<<"server tcp org internal: "<<internal_tcp_endpoint(serverInfo)<<std::endl;
	std::cout<<"server tcp org external: "<<external_tcp_endpoint(serverInfo)<<std::endl;
	std::cout<<"server tcp real external: "<<external_tcp_endpoint(realServerInfo)<<std::endl;
	);
	/*if ("acedf2e2970b34bede00c839f697930e"==channelID)
	{
	p2engine::error_code e;
	BOOST_AUTO(edp, server_socket->remote_endpoint(e));

	FILE* fp=fopen("c:\\tracker.log", "a+");
	if (fp)
	{

	std::string str=std::string("\n\nserver udp org internal: ")+ endpoint_to_string(internal_udp_endpoint(serverInfo))
	+"server udp org external: "+ endpoint_to_string(external_udp_endpoint(serverInfo))
	+"server udp real external: "+ endpoint_to_string(external_udp_endpoint(realServerInfo))
	+"server tcp org internal: "+ endpoint_to_string(internal_tcp_endpoint(serverInfo))
	+"server tcp org external: "+ endpoint_to_string(external_tcp_endpoint(serverInfo))
	+"server tcp real external: "+ endpoint_to_string(external_tcp_endpoint(realServerInfo))
	+"--------?-------------: "+endpoint_to_string(edp)
	+"\n"+boost::lexical_cast<std::string>((int)timestamp_now());
	fwrite(str.c_str(), 1, str.size(), fp);
	fclose(fp);
	}
	}*/

	this_channel->start(realServerInfo, channelInfo);
}

void member_service::stop()
{
	//TODO:do what?
	//�ر�����Ƶ���Ķ�ʱ��
	BOOST_FOREACH(channel_pair_type& chn_pair, channel_set_)
	{
		chn_pair.second->stop();
	}
	close_acceptor();
	tracker_service_logic_.reset();
	//�������cache_service�Ը���Ƶ����member_table��������
	channel_set_.clear();
}
/*!
* \brief ���Ƶ���㲥�Ƿ�Ҫ����.
*
*/
bool member_service::broad_cast_condition(uint32_t min_channel_cnt/* = 100*/, 
	float change_thresh/* = 0.1*/)
{
	//Ƶ��������100�������ø��£��ۻ������ڵ�
	if(channel_set_.size() >= min_channel_cnt) 
		return false;

	//��¼���ӵ�Ƶ����
	++channel_change_cnt_;

	//Ƶ������100���ϣ������ӵĸ����ﵽ��ֵ������
	if(float(channel_change_cnt_) / (channel_set_.size()+1) >= change_thresh )
	{
		channel_change_cnt_ = 0;
		return true;
	}
	return false;
}

void member_service::known_offline(const peer& p)
{
	//��ȡƵ��
	boost::shared_ptr<tracker_service_logic_base> tslogic 
		= tracker_service_logic_.lock();
	if (tslogic)
		tslogic->known_offline(const_cast<peer*>(&p));
}

void member_service::remove_channel(const std::string& channelID)
{
	channel::shared_ptr this_channel = get_channel(channelID);
	if (!this_channel) 
		return;

	//�ж�server�Ƿ�Ͽ�
	error_code ec;
	//this_channel->m_server_socket->ping(ec);
	if (this_channel->server_socket() && 
		this_channel->server_socket()->is_connected())
	{
		boost::shared_ptr<empty_channel>& an_channel = empty_channels_[channelID];
		if(!an_channel)
		{
			an_channel.reset(new empty_channel(*this_channel));

			DEBUG_SCOPE(
				std::cout<<get_obj_desc()
				<<" channel id: "<<this_channel->channel_id()<<" cleared"<<std::endl;
			std::cout<<get_obj_desc()
				<<" before erase channel count: "<<channel_set_.size()<<std::endl;
			)
		}
	}
	this_channel->stop();
	channel_set_.erase(channelID);
	DEBUG_SCOPE(
		std::cout<<get_obj_desc()
		<<" after erase channel count: "<<channel_set_.size()<<std::endl;
	)
}

void member_service::recvd_peer_info_report(peer* p, const p2ts_quality_report_msg& msg)
{
	BOOST_AUTO(svc, tracker_service_logic_.lock());
	if(svc)
		svc->recvd_peer_info_report(p, msg);
}

void member_service::on_accepted(message_socket_sptr conn, const error_code& ec)
{
	if (ec) return;

	//����һ�����ݴ�����������ݴ泬ʱ��û���յ��Ϸ����ģ���ر����ӡ�
	//pending.challenge=boost::lexical_cast<std::string>(random<boost::uint64_t>(0ULL, 0xffffffffffffffffULL));
	pending_sockets_.try_keep(conn, seconds(20));
	register_message_handler(conn.get());
	conn->keep_async_receiving();

	//����challenge
	send_challenge_msg(conn);
}

//std::string member_service::shared_key_signature(const std::string& pubkey)
//{
//	std::string theStr=generate_shared_key(key_pair_.second, pubkey);
//
//	md5_byte_t digest[16];
//	md5_state_t pms;
//	md5_init(&pms);
//	md5_append(&pms, (const md5_byte_t *)theStr.c_str(), theStr.length());
//	md5_finish(&pms, digest);
//
//	return std::string((char*)digest, 16);
//}

//////////////////////////////////////////////////////////////////////////
//��Ϣ������ͬ��Ƶ���ڵ���������ɵ���ͬ��member_table
//�ڵ����ӵ�member_service
void member_service::on_recvd_login(message_socket* sockPtr, safe_buffer buf)
{
	p2ts_login_msg msg;
	if(!parser(buf, msg))
		return;

	const int session = (int)msg.session_id();
	const peer_info& peerInfo = msg.peer_info();
	const std::string& channelID = msg.channel_id();

	message_socket_sptr conn = sockPtr->shared_obj_from_this<message_socket>();
	pending_sockets_.erase(conn);

	if(!challenge_channel_check(conn, msg)) 
		return;

	channel::shared_ptr this_channel = get_channel(channelID);
	if (!this_channel) 
	{
		challenge_failed(conn, session, e_not_found);
		return;
	}

	const peer* p = this_channel->insert(conn, peerInfo);
	if (!p)
	{
		challenge_failed(conn, session, e_already_login);
	}
	else
	{	
		//��¼�ɹ������ͺ����ڵ��
		this_channel->login_reply(conn, msg, p);	
		conn->ping_interval(get_tracker_param_sptr()->tracker_peer_ping_interval);
	}
}

bool member_service::challenge_channel_check(message_socket_sptr conn, 
	p2ts_login_msg& msg)
{
	TODO("��ֹƵ����¼�Ĺ���");
	//error_code ec;
	//endpoint edp = conn->remote_endpoint(ec);
	//if (ec) 
	//	return false;

	//��֤certificate û�з�����challenge��ͨ����֤
	if (!challenge_check(msg, get_tracker_param_sptr()->aaa_key))
	{
		challenge_failed(conn, msg.session_id(), e_unauthorized);
		return false;
	}//���ԣ���ʱ����֤ȥ��*/

	return true;
}

void member_service::play_point_translate(const std::string& channelID, 
	peer_info& peerInfo)
{
	channel::shared_ptr this_channel = get_channel(channelID);
	if(!this_channel) 
		return;

	int pt = -1;
	if(VOD_TYPE == get_tracker_param_sptr()->type)
	{
		pt = this_channel->dynamic_play_point(peerInfo.relative_playing_point());

		peerInfo.set_relative_playing_point(pt);
	}
}

void member_service::kickout(const std::string& channelID, const peer_id_t& id)
{
	channel_map_type::iterator itr = channel_set_.find(channelID);
	if (itr!=channel_set_.end())
		itr->second->kickout(id);
}

channel::shared_ptr member_service::get_channel(
	const std::string& channelID)
{
	channel_map_type::iterator itr = channel_set_.find(channelID);
	if (itr != channel_set_.end())
		return (itr->second);

	//���û�нڵ��Ƶ��
	empty_channel_map::iterator it = empty_channels_.find(channelID);
	if(it != empty_channels_.end())
	{
		channel::shared_ptr newChannel=create_channel(channelID, it->second->server_sockt_);
		if(newChannel)
		{
			DEBUG_SCOPE(
				std::cout<<get_obj_desc()
				<<" channel id:"<<newChannel->channel_id()<<"restored"
				<<std::endl;
			);

			boost::shared_ptr<empty_channel> ept_channel = it->second;
			newChannel->set_server_info(ept_channel->server_info_);
			empty_channels_.erase(it);
			if(restore_channel(*newChannel, ept_channel))
				return newChannel;
			else
				channel_set_.erase(channelID);
		}
	}
	return channel::shared_ptr();
}

bool member_service::restore_channel(channel& des_channel, 
	boost::shared_ptr<empty_channel>& ept_channel)
{
	if(is_live_category(get_tracker_param_sptr()->type)
		&&ept_channel->live_channel_info_
		)
	{
		des_channel.set_channel_info(*(ept_channel->live_channel_info_));
		return true;
	}
	else if(ept_channel->vod_channel_info_)
	{
		des_channel.set_channel_info(*(ept_channel->vod_channel_info_));
		return true;
	}
	return false;
}
channel::shared_ptr member_service::create_channel(const std::string& channelID, message_socket_sptr conn)
{
	//��Ƶ���б��в��Ҹ�Ƶ���ǲ����Ѿ�������.
	channel_map_type::iterator itr = channel_set_.find(channelID);
	if (itr !=channel_set_.end())
	{
		//����server
		itr->second->set_server_socket(conn);
		return itr->second;
	}

	////�����б�
	std::pair<channel_map_type::iterator, bool> ret=channel_set_.insert(
		std::make_pair(channelID, channel::create(SHARED_OBJ_FROM_THIS, conn, channelID))
		);
	if(ret.second)
		return ret.first->second;
	return channel::shared_ptr();
}

bool member_service::pending_socket_check(message_socket_sptr conn)
{
	BOOST_AUTO(itr, pending_sockets_.find(conn));
	if (itr == pending_sockets_.end()) 
		return false;
	return true;
}

