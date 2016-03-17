//
// media_distributor.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//
#ifndef server_stream_distribution_h__
#define server_stream_distribution_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <p2engine/push_warning_option.hpp>
#include <vector>
#include <deque>
#include <boost/unordered_set.hpp>
#include <p2engine/pop_warning_option.hpp>

#include "server/config.h"
#include "server/seed_connection.h"

namespace p2server{
	namespace multi_index=boost::multi_index;

	class server;

	class media_distributor
		:public basic_engine_object
		, public basic_server_object
		, public basic_mix_acceptor<media_distributor, seed_acceptor>
	{
		typedef media_distributor this_type;
		SHARED_ACCESS_DECLARE;

	protected:
		typedef variant_endpoint endpoint;
		typedef rough_timer timer;
		typedef std::set<seed_connection_sptr, seed_connection::seed_connection_score_less> seed_peer_set;
		friend  class basic_mix_acceptor<media_distributor, seed_acceptor>;

	public:
		static shared_ptr create(io_service& net_svc, server_param_sptr param)
		{
			shared_ptr obj(new this_type(net_svc, param), 
				shared_access_destroy<this_type>());
			return obj;
		}

		void start(peer_info& local_info, error_code& ec);
		void stop();

	protected:
		media_distributor(io_service& net_svc, server_param_sptr param);
		virtual ~media_distributor();

	public:
		//�ַ�
		void distribute(const media_packet& pkt)
		{
			do_distribute(pkt, 0);
		}
		//ƽ����ַ�(����һ����ʮ����ӳ�)
		void smooth_distribute(const media_packet& pkt);
		//����ԭ�ⲻ���ķ��ͳ�ȥ
		void relay_distribute(const media_packet& pkt);
		//��ǰ�ַ�seqno
		seqno_t current_media_seqno()const
		{
			return media_pkt_seq_seed_;
		}
		timestamp_t current_time()const
		{
			return timestamp_now();
		}
		int packet_rate()const
		{
			return (int)media_packet_rate_long_.bytes_per_second();
		}
		double out_multiple()const
		{
			double media_bps=media_bps_.bytes_per_second();
			return total_out_bps_.bytes_per_second()/(media_bps+FLT_MIN);
		}
		int bitrate()const
		{
			double media_bps=media_bps_.bytes_per_second();
			return (int)(media_bps)*8/1024;//kb
		}
		int out_kbps()const
		{
			return (int)(total_out_bps_.bytes_per_second())*8/1024;//kb
		}
		int total_seed_count()const
		{
			return seed_peers_.size()+super_seed_peers_.size();
		}
		const std::deque<seqno_t>& iframe_list()const
		{
			return iframe_list_;
		}

	protected:
		//�����¼�����
		void on_accepted(seed_connection_sptr conn, const error_code& ec);
		void on_connected(seed_connection* conn, const error_code& ec);
		void on_disconnected(seed_connection* conn, const error_code& ec);
	protected:
		//������Ϣ����
		void register_message_handler(seed_connection_sptr con);
		void on_recvd_media_request(seed_connection*, safe_buffer);
		void on_recvd_info_report(seed_connection*, safe_buffer);
	protected:
		//��ʱ�¼�����
		void on_check_seed_peer_timer();
		void on_piece_notify_timer();

	protected:
		//helper functions
		void flood_message(safe_buffer& buf, int level);
		void distribution_push(media_packet& pkt);

	protected:
		void check_super_seed();
		bool try_accept_seed(seed_connection_sptr conn, seed_peer_set& peerset, 
			size_t maxCnt, bool shrink);

		//�ַ�
		void do_distribute(const media_packet& p, int ptsOffset)
		{
			__do_distribute(p, ptsOffset, false);
		}
		void do_pull_distribute(seed_connection_sptr conn, seqno_t seq, bool direct, 
			int smoothDelay);

	protected:
		//��¼��ǰ��seqno��
		void store_info();
		void write_packet(const media_packet& pkt);
		void __do_distribute(const media_packet& p, int ptsOffset, bool bFecPacket);

	protected:
		boost::weak_ptr<server> server_;

		//seed_acceptor_sptr urdp_acceptor_;//����seed_peer����������
		//seed_acceptor_sptr trdp_acceptor_;//����seed_peer����������

		seed_peer_set super_seed_peers_;//seed �ڵ�
		seed_peer_set seed_peers_;//seed �ڵ�
		uint8_t session_id_;
		timed_keeper_set<peer_id_t> undirect_peers_;//��ֱ��client����һclient���ܺͱ�serverֱ������ϣ�����ձ�server����
		timer::shared_ptr seed_peer_check_timer_;//���ڼ��seed�ڵ�
		timer::shared_ptr piece_notify_timer_;//���ڼ��seed�ڵ�
		uint32_t  piece_notify_times_;//������            

		packet_buffer packet_buffer_;//ý������� 
		seqno_t media_pkt_seq_seed_;//����ý������кŵ�����
		rough_speed_meter media_packet_rate_;//������
		rough_speed_meter media_packet_rate_long_;//������

		double current_push_multiple_;//�������ͱ���
		rough_speed_meter media_bps_;//ý�����ʲ���bit per s
		rough_speed_meter media_bps_short_;//ý�����ʲ���bit per s
		rough_speed_meter total_out_bps_;//�����д�������������������Ͳ�������bit per s
		rough_speed_meter push_out_bps_;//���ʹ����������������bit per s
		rough_speed_meter pull_out_bps_;//�ӽ��ڵ�ǰ���͵�������������͵��Զ�Ĳ��㣩
		ptime   last_modify_push_multiple_time_;
		double  last_alf_;

		std::string cas_string_;
		ptime   last_erase_super_seed_time_;
		ptime   last_change_super_seed_time_;
		ptime   last_recommend_seed_time_;

		boost::unordered_set<unsigned long> pushed_ips_;

		bool running_;

		boost::shared_ptr<boost::thread> store_info_thread_;

		std::deque<seqno_t> iframe_list_;
		boost::optional<seqno_t> last_iframe_seqno_;
		
		std::vector<double> send_multiples_;

		fec_encoder fec_encoder_;
		std::vector<media_packet> fec_results_;

		//smoother push_distrib_smoother_;
		smoother pull_distrib_smoother_, push_distrib_smoother_;
		boost::shared_ptr<media_cache_service> async_dskcache_; //live cache

	};
}

#endif//server_stream_distribution_h__
