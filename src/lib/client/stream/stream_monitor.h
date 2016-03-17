//
// stream_monitor.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//
#ifndef peer_stream_monitor_h__
#define peer_stream_monitor_h__

#include "client/typedef.h"

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

namespace p2client{

	class timestamp_guess
	{
		enum{
			TIMESTAMP_GUESS_PKT_CNT = 8,
			TIMESTAMP_GUESS_SLOT_CNT = 1024
		};
		BOOST_STATIC_ASSERT((boost::is_unsigned<seqno_t>::value));
		BOOST_STATIC_ASSERT((~seqno_t(0) / TIMESTAMP_GUESS_PKT_CNT + (int64_t)1) % (TIMESTAMP_GUESS_SLOT_CNT) == 0);
		//��������һ��seqno��timestamp
		struct timestamp_element{
			timestamp_element() :m_inited(false){};
			void reset(){ m_inited = false; }
			seqno_t		m_seqno;
			timestamp_t m_timestamp;
			int			m_packet_rate;
			int			m_packet_rate_sum;
			int			m_packet_count;
			bool		m_inited;
		};
	public:
		timestamp_guess()
		{
			timestamp_guess_vector_.resize(TIMESTAMP_GUESS_SLOT_CNT, timestamp_element());
		}
		void set(seqno_t seqno, int packetRate, timestamp_t timestamp);
		bool get(seqno_t seqno, timestamp_t& timeStamp, int*thePacketRate)const;

	private:
		static unsigned int abs_middle_distance(seqno_t seqno)
		{
			BOOST_ASSERT(TIMESTAMP_GUESS_PKT_CNT);
			return std::abs<seqno_t>((seqno&(TIMESTAMP_GUESS_PKT_CNT - 1)) - (TIMESTAMP_GUESS_PKT_CNT / 2));
		}
		std::vector<timestamp_element> timestamp_guess_vector_;
		std::vector<char> bit_set_;
	};


	class stream_monitor
	{
		typedef int msec;
	public:
		stream_monitor(stream_scheduling& scheduling);
		enum dupe_state{ NOT_DUPE, PUSH_PULL_DUPE, PULL_PULL_DUPE };
		void incoming_pushed_media_packet(size_t len, seqno_t seq, int packetRate,
			timestamp_t timestamp, timestamp_t now, dupe_state dupe, bool timeout)
		{
			incoming_media_packet(len, seq, packetRate, timestamp, now, dupe, timeout, true);
		}
		void incoming_pulled_media_packet(size_t len, seqno_t seq, int packetRate,
			timestamp_t timestamp, timestamp_t now, dupe_state dupe, bool timeout)
		{
			incoming_media_packet(len, seq, packetRate, timestamp, now, dupe, timeout, false);
		}
		void outgoing_media_packet(size_t len, bool isPush)
		{
			out_speedmeter_long_ += len;
		}
		void reset_incoming_substeam(int sub_stream_id);
		void push_to_player(seqno_t seq, bool good);
		void reset_to_player();
		int get_push_to_player_packet_rate()
		{
			return (int)push_to_player_packet_speed_meter_.bytes_per_second();
		}
		double get_playing_quality();
		msec get_average_push_delay(int sub_stream_id);
		msec get_variance_push_delay(int sub_stream_id);
		msec get_average_push_delay();
		size_t get_average_packet_size();
		int get_bigest_push_delay_substream();
		double get_push_rate();
		double get_push_rate(int sub_stream_id);
		double get_duplicate_rate();
		double get_pull_pull_duplicate_rate();
		double get_push_pull_duplicate_rate(int sub_stream_id);
		double get_timeout_rate();
		size_t get_incoming_speed()
		{
			return (size_t)in_speedmeter_.bytes_per_second();
		}
		size_t get_incoming_packet_rate()
		{
			return (size_t)in_packet_rate_meter_.bytes_per_second();
		}
		int get_outgoing_speed()
		{
			return (int)out_speedmeter_long_.bytes_per_second();
		}
		int get_delay_thresh(int sub_stream_id)
		{
			return (int)delay_thresh_[sub_stream_id];
		}
		double get_dupe_thresh(int sub_stream_id)
		{
			return dupe_thresh_[sub_stream_id];
		}
		bool guess_timestamp(seqno_t seqno, timestamp_t& timeStamp,
			int* thePacketRate = NULL)const
		{
			return timestamp_guess_.get(seqno, timeStamp, thePacketRate);
		}
		bool piece_elapse(timestamp_t now, seqno_t seqno, int& t,
			const timestamp_t* pieceTimestamp = NULL)const;
		double urgent_degree(timestamp_t now, seqno_t seqno, int deadlineMsec)const;

	private:
		void recode_timestamp(seqno_t seqno, int packetRate, timestamp_t timestamp)
		{
			timestamp_guess_.set(seqno, packetRate, timestamp);
		}
		void parameter_adaptive(int subStreamID);
		void calc_rtt(boost::optional<msec>& srtt, int& rttVar, int delay);
		void incoming_media_packet(size_t len, seqno_t seq, int packetRate,
			timestamp_t timestamp, timestamp_t now, dupe_state dupe, bool timeout,
			bool isPush);

	public:
		//delay�������
		std::vector<boost::optional<msec> > pushed_in_substream_delay_;//���ش������ڵ����ﶩ�ĵ�����
		std::vector<msec>pushed_in_substream_delay_var_;//���ش������ڵ����ﶩ�ĵ�����

		//��ʱ����
		rough_speed_meter in_packet_rate_meter_long_;//���а����ʳ�ʱͳ��
		rough_speed_meter in_dupe_packet_rate_meter_long_;//�����ظ������ʳ�ʱ����
		rough_speed_meter in_pull_pull_dupe_packet_rate_meter_long_;
		rough_speed_meter out_speedmeter_long_;//���д���ʱ����
		rough_speed_meter in_push_packet_rate_meter_long_;//push�����ʳ�ʱ����

		rough_speed_meter in_packet_total_len_;  //ͳ��monitor���İ����ܳ���
		rough_speed_meter int_packet_total_count_; //ͳ��monitor���İ��ĸ���
		int               default_packet_size_;

		//�����ٶ����
		rough_speed_meter in_speedmeter_;//���д������
		rough_speed_meter in_packet_rate_meter_;//���а����ʲ���

		//����ͳ��
		std::vector<rough_speed_meter> in_substream_push_pull_dupe_packet_rate_meter_;//�ظ����ز���
		std::vector<rough_speed_meter> in_substream_packet_rate_meter_;//�ظ����ذ����ʲ���
		std::vector<rough_speed_meter> in_substream_pushed_packet_rate_meter_;//�ظ����ذ����ʲ���
		std::vector<double>  in_substream_push_pull_dupe_packet_rate_;

		rough_speed_meter  in_timeout_packet_rate_meter_;//��ʱƬ��

		std::vector<double> dupe_thresh_;
		std::vector<double> delay_thresh_;

		std::queue<seqno_t> push_to_player_seq_;
		rough_speed_meter push_to_player_packet_speed_meter_;
		boost::optional<seqno_t> bigest_push_to_playe_seq_;
		boost::optional<seqno_t> smallest_push_to_playe_seq_;
		double playing_quality_;

		timestamp_guess timestamp_guess_;

		//��������piece��time elapse�ã�ֻ�����������ڲ�������
		boost::optional<int> timestamp_offset_;

		stream_scheduling* scheduling_;

		const double  DELAY_INIT;
		const double  DELAY_MAX;
	};
}

#endif//peer_stream_monitor_h__