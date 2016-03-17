//
// const_define.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//

#ifndef common_const_define_h__
#define common_const_define_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "common/config.h"
#include "common/typedef.h"

namespace p2common{
	extern const float P2P_VERSION;

	extern const int PIECE_SIZE;
	extern const int CHUNK_SIZE;
	extern const int PIECE_COUNT_PER_CHUNK;//piece�е�chunk����

	extern const int SMOOTH_TIME;//ƽ������ʱ����С
	extern const int UNSUBCRIP_DELAY;

	extern const int PLAYING_POINT_SLOT_DURATION;//��̬ʱ������ϵ�£��ڵ㶨λ�۳�ms

	extern const int DEFAULT_DELAY_GUARANTEE;//Ĭ���ӳٱ�֤
	extern const int DEFAULT_BACK_FETCH_DURATION;//Ĭ�����ȡ����ʱ��

	extern const int MAX_LIVE_DELAY_GUARANTEE;
	extern const int MAX_SUPPORT_BPS;//Ԥ��֧�ֵ��������
	extern const int MAX_PKT_BUFFER_SIZE;//packet buffer�洢�����packet����

	extern const int SUPERVISOR_CNT;//�ֲ�ʽ����̽�����������
	extern const int MAX_DELAY_GUARANTEE;//����ӳٱ�֤
	extern const int MIN_DELAY_GUARANTEE;//��С�ӳٱ�֤
	extern const int MAX_BACKFETCH_CNT;
	extern const int SUBSTREAM_CNT;//��������
	extern const int GROUP_LEN;
	extern const int GROUP_CNT;

	extern const int MAX_CLINET_UPLOAD_SPEED;
	extern const int MIN_CLINET_UPLOAD_SPEED;

	extern const int MAX_PEER_CNT_PER_ROOM;//�������ڵ����

	extern const time_duration SIMPLE_DISTRBUTOR_PING_INTERVAL;
	extern const time_duration LIVE_TRACKER_SERVER_PING_INTERVAL;
	extern const time_duration VOD_TRACKER_SERVER_PING_INTERVAL;
	extern const time_duration TRACKER_PEER_PING_INTERVAL;
	extern const time_duration SERVER_SEED_PING_INTERVAL;
	extern const time_duration SERVER_SUPER_SEED_PING_INTERVAL;
	extern const time_duration MEMBER_TABLE_EXCHANGE_INTERVAL;//peer���ھӽ�����ʱ����
	extern const time_duration STREAM_PEER_PEER_PING_INTERVAL;
	extern const time_duration BUFFERMAP_EXCHANGE_INTERVAL;

	extern const int FLOOD_OUT_EDGE;//flood ��Ϣ�ĳ���

	extern const int MIN_SUPER_SEED_PEER_CNT;
	extern const int SEED_PEER_CNT;
	extern const int STREAM_NEIGHTBOR_PEER_CNT;
	extern const int HUB_NEIGHTBOR_PEER_CNT;
	extern const int MAX_PUSH_DELAY;

	extern const double ALIVE_DROP_PROBABILITY;
	extern const double ALIVE_GOOD_PROBABILITY;
	extern const double ALIVE_VERY_GOOD_PROBABILITY;
	extern const double ALIVE_DROP_PROBABILITY;
	extern const double ALIVE_GOOD_PROBABILITY;
	extern const double ALIVE_VERY_GOOD_PROBABILITY;

	extern const time_duration HUB_PEER_PEER_PING_INTERVAL;

	extern const std::string ANTI_POLLUTION_CODE;
	extern const std::string AUTH_SERVER_HOST;
	extern const std::string TRACKER_SERVER_HOST;

	//////////////////////////////////////////////////////////////////////////
	//����ʽֱ���е������趨����
	extern const int DEFAULT_DELAY_GUARANTEE__INTERACTIVE;//����ʽĬ���ӳٱ�֤
	extern const int DEFAULT_BACK_FETCH_DURATION__INTERACTIVE;//����ʽĬ�����ȡ����ʱ��
	extern const time_duration TRACKER_PEER_PING_INTERVAL__INTERACTIVE;
	extern const time_duration MEMBER_TABLE_EXCHANGE_INTERVAL__INTERACTIVE;
	extern const time_duration SERVER_SEED_PING_INTERVAL__INTERACTIVE;
	extern const int SEED_PEER_CNT__INTERACTIVE;
	extern const int MIN_SUPER_SEED_PEER_CNT__INTERACTIVE;
	extern const int STREAM_NEIGHTBOR_PEER_CNT__INTERACTIVE;
	extern const int HUB_NEIGHTBOR_PEER_CNT__INTERACTIVE;
	extern const int MAX_PUSH_DELAY__INTERACTIVE;
	//
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	//VoD�����趨����
	extern const int DEFAULT_DELAY_GUARANTEE__VOD;//�ӳٱ�֤
	//
	//////////////////////////////////////////////////////////////////////////

	//non const param, ������Ҫ����ʹ��
	extern boost::optional<int>  g_delay_guarantee;//ms
	extern boost::optional<int>  g_back_fetch_duration;//ms

	extern boost::optional<bool> g_b_fast_push_to_player;//bool
	extern boost::optional<bool> g_b_streaming_using_rtcp;
	extern boost::optional<bool> g_b_streaming_using_rudp;
	extern boost::optional<bool> g_b_tracker_using_rtcp;
	extern boost::optional<bool> g_b_tracker_using_rudp;


	//domain define
	extern const std::string tracker_and_peer_demain;
	extern const std::string server_and_peer_demain;
	extern const std::string tracker_and_server_demain;
	extern const std::string cache_tracker_demain;
	extern const std::string cache_server_demain;
	extern const std::string tracker_time_domain;
}

#endif//common_const_define_h__
