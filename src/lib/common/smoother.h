//
// smoother.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//
#ifndef smoother_h__
#define smoother_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <deque>
#include <map>

#include "common/config.h"

namespace p2common{

	class smoother
	{
		typedef int time_offset_t;
		//��ǰ����time_offset_tΪ��
		typedef boost::function<void(time_offset_t)> send_handler;
		typedef precise_timer timer;
	public:
		//���մ���catchTime������push�ٶȣ�
		//��push����һ�������ٹ�startTime��ʼ���ŵ�һ�����ݰ�;
		//���ų�ȥ�ĸ���������ȵ�һ�����ų�ȥ�İ���ԭʼ�������ӳ�maxDellay��
		smoother(
			const time_duration& longSpeedMeterTime, //ƽ�����ʼ����õĴ��ڴ�С
			const time_duration& smoothWindowTime, //ƽ������Ĵ�С
			const time_duration& preCatchTime, //Ԥ�Ȼ���ʱ��
			const time_duration& preDistribTime, //��ʼ���ͺ�ÿ�����������ǰ����ʱ�䣨�ӽ��뿪ʼ����״̬����
			const time_duration& maxDellay, //��ʼ���ͺ�ÿ��������󷢷��ӳ٣��ӽ��뿪ʼ����״̬����
			int lowSpeedThresh, //��ͷ����ٶ�byte p s
			io_service& ios
			);
		virtual ~smoother();

		void reset();
		void stop();
		void push(int64_t connectionID, const send_handler& h, size_t len);
		size_t size()const{return size_;}

	protected:
		void on_timer();

	protected:
		struct element{
			send_handler handler;
			int64_t  t;
			size_t   l;
			element(const send_handler& hd, size_t len, boost::int64_t tm)
				:handler(hd), t(tm), l(len)
			{}
		};
		typedef std::map<int64_t, std::deque<element> > task_map;
		task_map send_handler_list_;
		size_t size_;
		boost::optional<task_map::iterator> last_iterator_;
		boost::optional<int64_t> start_time_offset_;
		boost::optional<int64_t> first_push_time_;
		boost::optional<int64_t> last_push_time_;
		int start_time_delay_;
		int max_time_pre_;
		int max_time_delay_;
		int low_speed_thresh_;
		rough_speed_meter long_in_speed_meter_;
		rough_speed_meter smooth_in_speed_meter_;
		rough_speed_meter out_speed_meter_;
		boost::shared_ptr<timer> timer_;
	};

}

#endif//smoother_h__

