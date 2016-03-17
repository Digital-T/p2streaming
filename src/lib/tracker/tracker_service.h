//
// tracker_service.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//
#ifndef tracker_tracker_service_h__
#define tracker_tracker_service_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "tracker/config.h"
#include "tracker/cache_service.h"
#include "tracker/member_service.h"
#include <p2engine/push_warning_option.hpp>
#include <boost/array.hpp>
#include <map>
#include <vector>
#include <p2engine/pop_warning_option.hpp>

namespace p2tracker{

	class tracker_service_logic_base;

	class tracker_service
		:public basic_engine_object
		, public basic_tracker_object
		, public basic_mix_acceptor<tracker_service, message_acceptor>
	{
		typedef tracker_service this_type;
		SHARED_ACCESS_DECLARE;

		friend class basic_mix_acceptor<tracker_service, message_acceptor>;

	public:
		typedef variant_endpoint endpoint;
		typedef member_table::peer peer;
		typedef rough_timer timer;

	protected:
		typedef boost::function<void(const s2ts_create_channel_msg&, message_socket_sptr)> 
			channel_creator;
		typedef std::map<distribution_type, channel_creator> channel_creator_map;

	protected:
		tracker_service(io_service& net_svc, tracker_param_sptr param);
		virtual ~tracker_service();

	public:
		static shared_ptr create(io_service& net_svc, tracker_param_sptr param)
		{
			return shared_ptr(new this_type(net_svc, param), 
				shared_access_destroy<this_type>());
		}
		void start(boost::shared_ptr<tracker_service_logic_base> tsLogic);
		void stop();
	protected:
		void __start();
		void __stop();
		//�����¼�����
	protected:
		void on_accepted(message_socket_sptr conn, const error_code& ec);

	protected:
		//������Ϣ����
		void register_message_handler(message_socket*);
		void on_recvd_create_channel(message_socket*sock, safe_buffer buf);//server��֪����һ��Ƶ��
		//	void on_recvd_info_report(message_socket*sock, safe_buffer&buf);

	protected:
		void on_check_pending_sockets_timer();

	protected:
		//helper functions
		void known_offline(message_socket*);//֪��ĳ�ڵ���߻��߳�����
		void create_member_service();
		void create_cache_service();

		//�󶨵ĺ���
		void live_start (const s2ts_create_channel_msg&, message_socket_sptr);
		void vod_start (const s2ts_create_channel_msg&, message_socket_sptr);
		void cache_start(const s2ts_create_channel_msg&, message_socket_sptr);
		void global_cache_start();
		void register_create_handler();

	private:
		//�����ã�������tracker_service_logic������tracker_service�ĳ�Ա�������γɵ�ѭ�����õ����ڴ�й¶[]
		boost::weak_ptr<tracker_service_logic_base> tracker_service_logic_;
		boost::shared_ptr<member_service> member_service_;
		boost::shared_ptr<cache_service> cache_service_;
		boost::shared_ptr<cache_service> global_cache_service_;
		timed_keeper_set<message_socket_sptr> pending_sockets_;//�ոս��������ӣ����һ����ʱʱ���ڿͻ���û������һ���ı��Ľ�����������ӽ����Զ��ر�
		boost::shared_ptr<timer> pending_sockets_clean_timer_;
		channel_creator_map       channel_creators_;
	};	
}

#endif//tracker_tracker_service_h__
