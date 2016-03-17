//
// parameter.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//

#ifndef common_parameter_h__
#define common_parameter_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "common/typedef.h"
#include "common/message.pb.h"

#include <p2engine/push_warning_option.hpp>
#include <vector>
#include <string>
#include <boost/filesystem/path.hpp>
#include <p2engine/pop_warning_option.hpp>

namespace p2common{

	enum error_code_enum
	{
		e_no_error=0, 

		__________login_error__________=1000, 

		e_already_login, 
		e_too_many_users, 
		e_unauthorized, 
		e_unreachable, 
		e_not_found, 


		e_unknown=2000
	};

	enum distribution_type{
		INTERACTIVE_LIVE_TYPE, 
		LIVE_TYPE, 
		VOD_TYPE, 
		BT_TYPE, 
		GLOBAL_CACHE_TYPE, 
		SHIFT_LIVE
	};

	inline bool is_interactive_category(int t)
	{
		switch(t)
		{
		case INTERACTIVE_LIVE_TYPE:
			return true;
		case LIVE_TYPE:
		case VOD_TYPE:
		case BT_TYPE:
			return false;
		default:
			BOOST_ASSERT(0);
			return false;
		}
	}
	inline bool is_live_category(int t)
	{
		switch(t)
		{
		case INTERACTIVE_LIVE_TYPE:
		case LIVE_TYPE:
			return true;
		case VOD_TYPE:
		case BT_TYPE:
			return false;
		default:
			BOOST_ASSERT(0);
			return false;
		}
	}
	inline bool is_shift_live_category(int t)
	{
		switch(t)
		{
		case INTERACTIVE_LIVE_TYPE:
		case LIVE_TYPE:
		case VOD_TYPE:
		case BT_TYPE:
			return false;
		case SHIFT_LIVE:
			return true;
		default:
			BOOST_ASSERT(0);
			return false;
		}
	}
	inline bool is_vod_category(int t)
	{
		switch(t)
		{
		case INTERACTIVE_LIVE_TYPE:
		case LIVE_TYPE:
			return false;
		case VOD_TYPE:
		case BT_TYPE:
		case SHIFT_LIVE:
			return true;
		default:
			BOOST_ASSERT(0);
			return false;
		}
	}
	
	inline bool is_bt_category(int t)
	{
		switch(t)
		{
		case INTERACTIVE_LIVE_TYPE:
		case LIVE_TYPE:
		case VOD_TYPE:
			return false;
		case BT_TYPE:
			return true;
		default:
			BOOST_ASSERT(0);
			return false;
		}
	}

	struct client_param_base{
		distribution_type type;
		std::string channel_uuid;
		std::string channel_link;
		std::string tracker_host;
		std::string user_info;
		std::string cache_directory;//����·��
		int64_t     offset; //��ʼ����ʱ��ƫ��

		//����������֤���
		std::string channel_key;
		std::string private_key;
		std::string public_key;
	};

	struct server_param_base{
		distribution_type type;
		boost::filesystem::path name;//����
		std::string channel_uuid;
		std::string channel_link;
		std::string tracker_ipport;//������������ַ
		std::string internal_ipport;
		std::string external_ipport;
		std::string multicast_ipport;//IP�鲥
		std::string stream_recv_url;//���ؽ�����url(��udp://127.0.0.1:1234 http://127.0.0.1/channelX)
		boost::filesystem::path media_directory;//ý���Ŀ¼
		std::vector<std::string>  cache_server_ipport;//ý�建���������ַ
		int film_duration;
		int film_length;

		bool enable_live_cache; //�Ƿ���ʱ��
		boost::filesystem::path live_cache_dir; //ֱ��cacheд��·��
		uint64_t max_duration; //ʱ����󳤶�, ��λΪsecond
		uint64_t max_length_per_file; //�����ļ���С

		std::string discription;
		std::string welcome;

		//����������֤���
		std::string channel_key;
	};

	struct tracker_param_base{
		tracker_param_base()
			:b_for_shunt(false){}

		distribution_type type;
		std::string internal_ipport;
		std::string external_ipport;
		//std::string supertracker_ipport;

		//����������֤���
		std::string aaa_key;

		//��Ϊshuntʹ�õģ�
		bool b_for_shunt;
	};


	struct client_param
		:client_param_base
	{
		p2message::peer_info local_info;

		//������ز���
		int delay_guarantee;//�ӳٱ�֤
		int back_fetch_duration;//�����ȡ���ݵ�ʱ����
		int max_push_delay;//��������ӳ�
		p2engine::time_duration tracker_peer_ping_interval;
		p2engine::time_duration server_seed_ping_interval;
		p2engine::time_duration member_table_exchange_interval;
		int stream_neighbor_peer_cnt;
		int hub_neighbor_peer_cnt;
		bool b_fast_push_to_player;//msec
		bool b_streaming_using_rtcp;
		bool b_streaming_using_rudp;
		bool b_tracker_using_rtcp;
		bool b_tracker_using_rudp;

		//ʵʱ��̬��Ϣ
		seqno_t smallest_seqno_i_care;
		seqno_t smallest_seqno_absenct;
	};

	struct server_param
		:server_param_base
	{
		int seed_peer_cnt;
		int min_super_seed_peer_cnt;
		p2engine::time_duration server_seed_ping_interval;
	};

	struct tracker_param
		:tracker_param_base
	{
		p2engine::time_duration tracker_peer_ping_interval;
		bool b_tracker_using_rtcp;
		bool b_tracker_using_rudp;
	};

	typedef boost::shared_ptr<client_param> client_param_sptr;
	typedef boost::shared_ptr<server_param> server_param_sptr;
	typedef boost::shared_ptr<tracker_param> tracker_param_sptr;

	client_param_sptr create_client_param_sptr(const client_param_base& param);
	server_param_sptr create_server_param_sptr(const server_param_base& param);
	tracker_param_sptr create_tracker_param_sptr(const tracker_param_base& param);

}

#endif//common_parameter_h__



