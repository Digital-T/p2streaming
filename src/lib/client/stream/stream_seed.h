#ifndef peer_stream_seed_h__
#define peer_stream_seed_h__

#include "client/stream/scheduling_typedef.h"

namespace p2client{

	class stream_seed
		:public basic_stream_scheduling
	{
		typedef stream_seed this_type;
		SHARED_ACCESS_DECLARE;

	public:
		static shared_ptr create(stream_scheduling& scheduling)
		{
			return shared_ptr(new this_type(scheduling), 
				shared_access_destroy<this_type>());
		}

		peer_connection_sptr get_connection()
		{
			return server_connection_;
		}

		bool local_is_seed()const
		{
			return server_connection_&&server_connection_->is_connected();
		}

	public:
		virtual void stop();
		virtual void start();
		virtual void reset();

		//ÿ��10ms��������stream_scheduling����һ�Σ���������һЩ��ʱ����
		//���Ƿ��Ǿ�ȷ��10ms����Ӳ���ϵͳʱ�侫�ȶ���ͬ����һЩarm-linuxƽ̨����20ms��
		//����cpuload
		virtual int on_timer(timestamp_t now);

	protected:
		stream_seed(stream_scheduling& scheduling);
		~stream_seed();

		void connect_server();
		void __connect_server(peer* srv_peer, error_code ec, coroutine coro);

		//�˴�Ψһ���ܶ�server_connection�Ĵ���
		void on_connected(peer_connection* conn, const error_code& ec);
		void on_disconnected(peer_connection* conn, const error_code& ec);
	private:
		peer_connection_sptr server_connection_tmp_;
		peer_connection_sptr server_connection_;
		boost::optional<timestamp_t> last_connect_server_time_;
		int connect_fail_count_;
	};
}


#endif//peer_stream_seed_h__
