#include "common/smoother.h"
#include "common/utility.h"

NAMESPACE_BEGIN(p2common);

static const tick_type TIMER_TIME = 20;


//���մ���catchTime������push�ٶȣ�
//��push����һ�������ٹ�startTime��ʼ���ŵ�һ�����ݰ�;
//���ų�ȥ�ĸ���������ȵ�һ�����ų�ȥ�İ���ԭʼ�������ӳ�maxDellay��
smoother::smoother(const time_duration& longSpeedMeterTime, //ƽ�����ʼ����õĴ��ڴ�С
	const time_duration& smoothWindowTime, //ƽ������Ĵ�С
	const time_duration& preCatchTime, //Ԥ�Ȼ���ʱ��
	const time_duration& preDistribTime, //��ʼ���ͺ�ÿ�����������ǰ����ʱ�䣨�ӽ��뿪ʼ����״̬����
	const time_duration& maxDellay, //��ʼ���ͺ�ÿ��������󷢷��ӳ٣��ӽ��뿪ʼ����״̬����
	int lowSpeedThresh, //
	io_service& ios
	)
	: long_in_speed_meter_(longSpeedMeterTime)
	, smooth_in_speed_meter_(smoothWindowTime)
	, out_speed_meter_(milliseconds((std::min)(smoothWindowTime.total_milliseconds(), 10 * TIMER_TIME)))
	, low_speed_thresh_(lowSpeedThresh)
	, size_(0)
{
	start_time_delay_ = (int)preCatchTime.total_milliseconds();
	max_time_pre_ = (int)preDistribTime.total_milliseconds();
	max_time_delay_ = (int)maxDellay.total_milliseconds();
	timer_ = timer::create(ios);
	DEBUG_SCOPE(timer_->set_obj_desc("p2common::smoother::timer_"););
}

smoother::~smoother()
{
	stop();
}

void smoother::reset()
{
	size_ = 0;
	start_time_offset_.reset();
	first_push_time_.reset();
	last_push_time_.reset();
	send_handler_list_.clear();
	last_iterator_.reset();
	timer_->cancel();
}

void smoother::stop()
{
	reset();
}

void smoother::push(int64_t connectionID, const send_handler& h, size_t len)
{
	int64_t now = tick_now();
	int64_t outTime = now + start_time_delay_ + max_time_delay_;
	send_handler_list_[connectionID].push_back(element(h, len, outTime));
	long_in_speed_meter_ += len;
	smooth_in_speed_meter_ += len;
	size_ += 1;
	if (!first_push_time_)
	{
		first_push_time_ = now;
		last_push_time_.reset();
		BOOST_ASSERT(timer_);
		timer_->register_time_handler(boost::bind(&smoother::on_timer, this));
		timer_->async_keep_waiting(milliseconds(start_time_delay_), milliseconds(TIMER_TIME));
	}
}

void smoother::on_timer()
{
	int64_t now = tick_now();
	if (!first_push_time_ || (*first_push_time_ + start_time_delay_) > now)
	{
		return;
	}

	int64_t interval = (last_push_time_ ? (now - *last_push_time_) : TIMER_TIME);
	last_push_time_ = now;
	double speed = long_in_speed_meter_.bytes_per_second();
	double smoothSpeed = smooth_in_speed_meter_.bytes_per_second();
	double speedThresh = (std::max)(speed, smoothSpeed);
	//speedThresh+=1024;//���Գ���1K
	if (speedThresh < low_speed_thresh_)
		speedThresh = low_speed_thresh_;
	int maxSendLen = (int)(speedThresh*(interval + 5) / 1000);
	int sendLen = 0;
	double currSpeed = out_speed_meter_.bytes_per_second();
	std::set<int64_t, std::less<int64_t>, allocator<int64_t> > stopmark;
	task_map::iterator itr = last_iterator_ ? *last_iterator_ : send_handler_list_.begin();
	if (itr == send_handler_list_.end())
		itr = send_handler_list_.begin();
	for (; !send_handler_list_.empty();)
	{
		std::deque<element>& elementQue = itr->second;
		BOOST_ASSERT(!elementQue.empty());
		bool canSend = true;
		while (!elementQue.empty())
		{
			if (stopmark.find(itr->first)!=stopmark.end())
			{
				break;
			}
			element& ele = elementQue.front();
			int64_t t = ele.t;
			int timeOut = int(now - (t - max_time_delay_));
			if (timeOut <= -max_time_pre_)//�����ǰmaxPreTime��
			{
				canSend = false;
				break;
			}
			if (currSpeed <= speedThresh
				|| now >= t//��ʱ
				|| currSpeed<(1 + timeOut / double(max_time_delay_))*speedThresh//��ʱ�ˣ�ÿ��ʱmax_delay_�����������ٶ���ֵһ��
				)
			{
				size_ -= 1;
				out_speed_meter_ += ele.l;
				currSpeed += (double(ele.l) * 1000) / out_speed_meter_.time_window_millsec();
				sendLen += ele.l;
				ele.handler(timeOut);
				elementQue.pop_front();
				if (sendLen>maxSendLen
					&&now < t
					&&currSpeed >(speedThresh / 8)
					)
				{
					canSend = false;
					break;
				}
			}
			else
			{
				canSend = false;
				break;
			}
			if (send_handler_list_.size() > 1)
				break;//ֻ����һ��
		}

		if (elementQue.empty())
		{
			send_handler_list_.erase(itr++);
		}
		else
		{
			if (!canSend)
				stopmark.insert(itr->first);
			++itr;
		}
		if (itr == send_handler_list_.end())
			itr = send_handler_list_.begin();
		last_iterator_ = itr;

		if (sendLen >= maxSendLen || stopmark.size() >= send_handler_list_.size())
		{
			break;
		}

	}
	/*if (rand()%100<1)
	{
	std::cout<<"-----:    "<<timeOut<<",  "<<(speedThresh-currSpeed)<<"\n";
	}*/

	//std::cout<<"-----:    "<<timeOut<<",  "<<(speedThresh-currSpeed)<<"\n";
}


NAMESPACE_END(p2common)
