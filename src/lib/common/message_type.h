//
// message_type.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//

#ifndef common_message_type_h__
#define common_message_type_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "common/config.h"

namespace p2common{

	//broadcast��relay�Ľ�һ����װ
	//���磬һ���û����broadcast��Ϣ������װΪ[braodcast][USER_LEVEL_broadcast_msg][MSGTYPE]
	//��һ���װ��broadcast���ڶ����װΪUSER_LEVEL_broadcast_msg���������Ǿ������ʵ��Ϣ����
	//���ڵ��յ�һ��broadcast��Ϣʱ��Ҫ���ݵڶ����װ����������Ϣ����Ӧ�ò㴦���ǽ���ϵͳ�㴦��
	enum {
		SYSTEM_LEVEL=0, 
		USER_LEVEL
	};
	const static boost::uint8_t ROOM_INFO_BROADCAST_CHANNEL=0xf;
	const static boost::uint8_t TXT_BROADCAST_CHANNEL=0xe;
	const static boost::uint8_t FEC_MEDIA_CHANNEL=0xd;

	//////////////////////////////////////////////////////////////////////////
	//global_msg msgs def
	namespace global_msg{
		enum{
			GLOBAL_MSG=0, 
			broadcast, 
			relay, 
			join_channel, 
			peer_info, 
			media_request, 
			media, 
			media_sent_confirm, 
			no_piece, 
			fec_media, 
			pushed_media, 
			pulled_media, 
			cache_media
		};
	};

	//////////////////////////////////////////////////////////////////////////
	//tracker-peer msgs def
	namespace tracker_peer_msg{
		enum{
			TRACKER_PEER_MSG = 100, 
			login, 
			challenge, 
			login_reply, 
			logout, 
			logout_reply, 
			kickout, 
			local_info_report, 
			failure_report, 
			peer_request, 
			peer_reply, 
			cache_announce, 
			room_info, 
			play_quality_report
		};
	};


	//////////////////////////////////////////////////////////////////////////
	//server_peer_msg msgs def
	namespace server_peer_msg{
		enum{
			SERVER_PEER_MSG = 200, 
			recommend_seed, 
			info_report, 
			be_seed, 
			be_super_seed, 
			piece_notify
		};
	};
	//////////////////////////////////////////////////////////////////////////
	//peer_peer_msg msgs def
	namespace peer_peer_msg{
		enum{
			PEER_PEER_MSG = 300, 
			//����ά�����
			handshake_msg, 
			neighbor_table_exchange, 
			supervise_request, 
			keepalive, 
			//ý��ַ����
			media_subscription, 
			media_unsubscription, 
			buffermap_exchange, 
			buffermap_request, 
			punch_request
		};
	};

	//////////////////////////////////////////////////////////////////////////
	//server_tracker_msg msgs def
	namespace server_tracker_msg{
		enum{
			SERVER_TRACKER_MSG = 400, 
			//server����tracker
			create_channel, 
			info_report, 
			info_request, 
			//tracker����server
			distribute, 
			info_reply
		};
	};


	//time_server_msg msgs def
	namespace time_server_msg{
		enum{
			TIME_SERVER_MSG = 500, 
			//time_server����tracker
			daytime_reply
		};
	};

	namespace control_cmd_msg{
		enum{
			MDS_CMD_MSG = 600, 
			login_report, 
			cmd, 
			recvd_cmd, 
			cmd_reply, 
			alive_alarm, 
			auth_message
		};
	};

	namespace proanalytics_msg{
		enum{
			PROANANLYTICS_MSG = 700, 
			init_client_msg, 
			sample_msg_1, 
			sample_msg_2, 
			control_msg, 
			change_control_msg
		};
	};
}

#endif//common_message_type_h__

