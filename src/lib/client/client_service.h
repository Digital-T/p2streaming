//
// client_service.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//
#ifndef peer_client_service_h__
#define peer_client_service_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/unordered_map.hpp>

#include "client/typedef.h"

namespace p2client{

	class client_service
		: public basic_engine_object
		, public basic_client_object
	{
		typedef client_service this_type;
		SHARED_ACCESS_DECLARE;
	public:
		typedef variant_endpoint endpoint;
		typedef boost::unordered_map<peer_id_t, peer_sptr> peer_map;

	protected:
		client_service(io_service& iosvc, const client_param_sptr& param);
		virtual ~client_service();

	public:
		static shared_ptr create(io_service& iosvc, const client_param_sptr& param)
		{
			return shared_ptr(new this_type(iosvc, param), 
				shared_access_destroy<this_type>()
				);
		}

	public:
		void start(client_service_logic_base_sptr clntLogic);
		void restart();
		virtual void stop(bool flush=false);
		void update_server_info();
		void bind_cache_serice();
		void set_play_offset(int64_t offset);

		virtual void on_recvd_media(const safe_buffer& buf, const peer_id_t& srcPeerID, 
			media_channel_id_t mediaChannelID, coroutine coro=coroutine()); //ԭ��logic�еĺ���
		virtual void on_recvd_room_info(const safe_buffer&);

	public:
		peer_nat_type local_nat_type();
		int external_udp_port()const{return extern_udp_port_;}
		int external_tcp_port()const{return extern_tcp_port_;}
		peer_key_t& local_peer_key(){return local_peer_key_;}
		int total_online_peer_cnt(){return online_peer_cnt_;}
		peer_map& peers(){return peers_;}
		peer_map& server_peers(){return server_peers_;}
		peer_sptr find_peer(const peer_id_t& id);
		bool is_offline(const peer_id_t& id){return offline_peers_keeper_.is_keeped(id);}
		client_service_logic_base_sptr get_client_service_logic(){return client_service_logic_.lock();}
		uint64_t generate_uuid();
		//�Ƿ��Ǳ�Ƶ��
	    bool is_same_channel(const std::string& channelID)const{
		    return channelID == get_client_param_sptr()->channel_uuid;
        }
		tracker_manager_sptr get_tracker_handler(){return member_tracker_handler_;}

		const boost::optional<vod_channel_info>& get_vod_channel_info()const{
			return vod_channel_info_;
		}
		const boost::optional<live_channel_info>& get_live_channel_info()const{
			return live_channel_info_;
		}

	public:
		void on_login_finished(error_code_enum e, const ts2p_login_reply_msg& msg);
		//��ͨ��tracker_handler_base/DHT/peerexchange��ʽ��֪�ڵ�ʱ�����øýӿ�
		void on_known_new_peers(const std::string& channelID, 
			const std::vector<const peer_info*>& peerInfos, bool sameChannel);
		void on_known_new_peer(const peer_info& peerInfo, bool sameChannel);
		//��֪ĳ�ڵ���ߣ����øýӿ�
		void on_known_offline_peer(const peer_id_t& peerID);
	
	public:
		void report_quality(const p2ts_quality_report_msg&);

	protected:
		//////////////////////////////////////////////////////////////////////////
		//��Ϣ�м�����ɢ
		void on_recvd_userlevel_relay(const relay_msg& msg, bool sameChannel);
		void on_recvd_broadcast(broadcast_msg& msg, const peer_id_t& whoFloodToMe);

		//������Ϣ����
		void register_message_handler(message_socket* conn);
		
		
	protected:
		void __start_client_service();
		void __topology_erase_offline_peer(const peer_id_t& peerID);
		void __stop_lowlayer(bool flush=false);

	protected:
		bool stoped_;

		client_service_logic_base_wptr client_service_logic_;

		//��¼��Ϣ
		tracker_manager_sptr member_tracker_handler_;//��tracker������

		boost::optional<vod_channel_info>	vod_channel_info_;
		boost::optional<live_channel_info>	live_channel_info_;

		int extern_udp_port_;
		int extern_tcp_port_;

		hub_topology_sptr  hub_topology_;//message client_service
		stream_topology_sptr stream_topology_;//ý�����ַ�������

		//client_service_wptr father_client_service_;//����
		//std::list<client_service_sptr>child_client_services_;//�Ӳ�

		//overlay
		peer_map server_peers_;//server�ڵ�
		peer_map peers_;//����chatroom��Ӧ����ȫ���ڵ㣻���ڴ��ģֱ�����ǲ��ֽڵ�
		int online_peer_cnt_;
		peer_key_t local_peer_key_;

		//��Ϣ����id��¼
		timed_keeper_set<peer_id_t> offline_peers_keeper_;

		peer_nat_type local_nat_type_;//

		bool bind_cache_serice_;
	};
}

#endif //peer_client_service_h__
