//
// client_service_logic.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//
#ifndef client_client_service_logic_h__
#define client_client_service_logic_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "client/typedef.h"

namespace p2client{

	class  client_service;

	class client_service_logic_base
		:public basic_engine_object
		, public basic_client_object
	{
		typedef client_service_logic_base this_type;
		SHARED_ACCESS_DECLARE;
	public:
		typedef variant_endpoint endpoint;
		typedef rough_timer timer;

	protected:
		client_service_logic_base(io_service& iosvc);
		virtual ~client_service_logic_base();

	public:
		void start_service(const client_param_base& param);
		void stop_service(bool flush=false);

		void set_play_offset(int64_t offset);

	public:
		//������Ϣ����
		virtual void register_message_handler(message_socket*)=0;

	public:
		//��ѹδ�����ý�����ݳ��ȣ�һ���ǲ���������������ᷢ����
		virtual int overstocked_to_player_media_size()=0;

	public:
		//��¼ʧ��
		virtual void on_login_failed(error_code_enum code, const std::string& errorMsg)=0;
		//��¼�ɹ�
		virtual void on_login_success()=0;
		//����
		virtual void on_droped()=0;

		//һ���½ڵ����ϵͳ
		virtual void on_join_new_peer(const peer_id_t& newPeerID, const std::string& userInfo)=0;
		//������һ�����ڱ��ڵ����ߵĽڵ�
		virtual void on_known_online_peer(const peer_id_t& newPeerID, const std::string& userInfo)=0;
		//�ڵ��뿪
		virtual void on_known_offline_peer(const peer_id_t& newPeerID)=0;

		//�ڵ���Ϣ�ı�
		virtual void on_user_info_changed(const peer_id_t& newPeerID, const std::string& oldUserInfo, 
			const std::string& newUserInfo)=0;

		virtual  void on_recvd_media(const safe_buffer& buf, 
			const peer_id_t& srcPeerID, media_channel_id_t mediaChannelID)=0;

		virtual void on_media_end(const peer_id_t& srcPeerID, media_channel_id_t mediaChannelID){}
		//�յ���Ϣ
		virtual void  on_recvd_msg(const std::string&msg, const peer_id_t& srcPeerID)=0;

	protected:
		boost::shared_ptr<client_service> client_service_;
	};
}

#endif//tracker_client_service_h__