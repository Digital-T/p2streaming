//
// absent_packet_info.h
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//
#ifndef client_absent_packet_info_h__
#define client_absent_packet_info_h__

#include <utility>
#include <list>

#include "client/typedef.h"
#include "client/peer_connection.h"

#ifdef DEBUG_SCOPE_OPENED
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>
	using namespace boost::accumulators;
#endif

namespace p2client{

	class owner_map
	{
	public:
		owner_map() :owner_index_(NULL)
		{
			init();
		}
		~owner_map()
		{
			if (owner_index_) memory_pool::free(owner_index_);
		}
		size_t size()const
		{
			return owner_vec_.size();
		}
		bool empty()const
		{
			return owner_vec_.empty();
		}
		peer_connection_sptr select(size_t i);
		peer_connection_sptr random_select();
		void reset();
		void insert(const peer_connection_sptr& conn);
		void erase(const peer_connection_sptr& conn);
		peer_connection* find(const peer_connection_sptr& conn)const;
		bool dec_request_deadline(const peer_connection_sptr& conn);
		void dump();

	private:
		struct owner{
			enum { kRequestDeadline = 2 };
			owner() :id(INVALID_ID), uuid(INVALID_ID), request_deadline(kRequestDeadline)
				, is_link_local(0)
			{}
			owner(const peer_connection_sptr&_conn)
				: id(_conn->local_id()), uuid(_conn->local_uuid())
				, request_deadline(kRequestDeadline), is_link_local(_conn->is_link_local())
				, conn(_conn)
			{}
			~owner(){ reset(); }
			void assign(const peer_connection_sptr&_conn)
			{
				BOOST_ASSERT(
					id == INVALID_ID&&uuid == INVALID_ID
					|| (conn.expired() || !conn.lock()->is_open() || !conn.lock()->is_connected())
					|| request_deadline <= 0
					);
				id = _conn->local_id();
				uuid = _conn->local_uuid();
				request_deadline = kRequestDeadline;
				is_link_local = _conn->is_link_local();
				conn = _conn;
			}

			void reset()
			{
				id = uuid = INVALID_ID;
				request_deadline = 2;
				conn.reset();
			}
			boost::weak_ptr<peer_connection> conn;
			int32_t uuid;
			int8_t	id;
			uint8_t request_deadline : 4;
			uint8_t	is_link_local : 1;
		};
		void erase_owner(int id, const int* uuid = NULL);
		void init();
		//����ʹ��std::vector<char>ʱ����Щ��������resize��һ��һ���ĳ�ʼ��Ԫ��
		int8_t* owner_index_;
		std::vector<owner, p2engine::allocator<owner> > owner_vec_;
	};

	class absent_packet_info
		: public object_allocator
		, public basic_intrusive_ptr < absent_packet_info >
	{
	public:
		absent_packet_info()
			:inited__(false), m_seqno(0), m_server_request_deadline(6), m_pull_cnt(0)
		{
			DEBUG_SCOPE(
				total_info_object_cnt()++;
			);
		}
		~absent_packet_info()
		{
			DEBUG_SCOPE(
				total_info_object_cnt()--;
			);
		}

		bool is_this(seqno_t seqno, timestamp_t now)const
		{
			static const int kMaxTime = (sizeof(timestamp_t) <= 2 && sizeof(seqno_t) <= 2)
				? MAX_DELAY_GUARANTEE : (std::numeric_limits<int32_t>::max)() / 4;
			return(inited__&&m_seqno == seqno&&
				time_greater(m_first_known_this_piece_time + kMaxTime, now)
				);
		}
		void reset();
		void just_known(seqno_t seqno, timestamp_t now);
		void recvd(seqno_t seqno, const media_packet& pkt, seqno_t now);
		void request_failed(timestamp_t now, int reRequestDelay);

		//��Ϊ��һ�ṹ�彫���洢��һ��size�ܴ��vector�У����ԣ�Ҫ������С��һ�ṹ���С������ʹ��λ��
		owner_map				m_owners;//��������Ľڵ�
		peer_connection_wptr	m_peer_incharge;//��˭����������

		timestamp_t				m_first_known_this_piece_time;//��ʱ֪����һƬ�ε�
		timestamp_t				m_pull_time;//�������������ʱ��
		timestamp_t				m_pull_outtime;//���������������ȴ���ʱʱ��
		timestamp_t				m_last_check_peer_incharge_time;//���һ�μ��peer_incharge��ʱ��

		seqno_t					m_seqno;//���к�

		uint32_t					m_last_rto : 17;//�ϴ�У׼m_pull_outtimeʱ��peerincharge��rto
		uint32_t					m_priority : 3;//���ȼ�
		uint32_t					m_server_request_deadline : 6; enum{ server_request_deadline = 16 };
		uint32_t					m_pull_cnt : 6;

		bool					inited__ : 1;
		bool					m_dskcached : 1;
		bool					m_must_pull : 1;//ȷ��һ����ֻ��ͨ��pull��ʽ��ã���super_seed�����ã�


#ifdef DEBUG_SCOPE_OPENED
		static int& total_info_object_cnt()
		{
			static int  m_cnt = 0;
			return m_cnt;
		}
		struct endpoint_info{
			endpoint edp; timestamp_t t;
		};
		std::list<endpoint_info> m_pull_edps;
		static accumulator_set<double, stats<tag::mean > > s_pull_accumulator;
#endif
	};
}

#endif//client_absent_packet_info_h__


