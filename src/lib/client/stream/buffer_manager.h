#ifndef buffer_manager_h__
#define buffer_manager_h__

#include "client/stream/absent_packet_list.h"
#include "asfio/async_dskcache.h"

namespace p2client{

	class buffer_manager
		:public basic_client_object
	{
	public:
		typedef packet_buffer::recent_packet_info recent_packet_info;
		
		buffer_manager(io_service& ios, client_param_sptr param, int64_t film_length=-1);
		virtual ~buffer_manager();

		absent_packet_list& get_absent_packet_list()
		{return absent_packet_list_;}
		const absent_packet_list& get_absent_packet_list()const
		{return absent_packet_list_;}

		packet_buffer& get_memory_packet_cache()
		{return memory_packet_cache_;}
		const packet_buffer& get_memory_packet_cache()const
		{return memory_packet_cache_;}

		boost::shared_ptr<asfio::async_dskcache>& get_disk_packet_cache()
		{return disk_packet_cache_;}
		const boost::shared_ptr<asfio::async_dskcache>& get_disk_packet_cache()const
		{return disk_packet_cache_;}

		const boost::optional<seqno_t>& get_bigest_sqno_i_know()const
		{return bigest_sqno_i_know_;}
		boost::optional<seqno_t>& get_bigest_sqno_i_know()
		{return bigest_sqno_i_know_;}

		const boost::optional<seqno_t>& get_average_current_server_seqno()const
		{return current_server_seqno_;}
		boost::optional<seqno_t>& get_average_current_server_seqno()
		{return current_server_seqno_;}

		int max_bitmap_range_cnt()const
		{return max_bitmap_range_cnt_;}

	public:
		//	���á��ڵ�����scheduling����ʱ��Ҫ���ñ�������
		void reset();

		//	���õ�ǰ��֪������Ƭ�κŵȡ��������������ȿ�ʼǰ���á�
		void set_bigest_seqno(seqno_t current_bigest_seq, int backfetch_cnt, timestamp_t now);

		//	��������һЩabsent seqno��VoD���͵ĵ��ȹ�������Ҫlocal��������absent seqno��
		//��ˣ�VoD������Ҫ�����Եĵ��ñ����������������ÿ��on_pull_timerʱ����á�
		void inject_absent_seqno(seqno_t smallest_seqno_i_care, int buffer_size, timestamp_t now);

		void get_absent_seqno_range(seqno_t& absent_first_seq, seqno_t& absent_last_seq, 
			const peer_sptr& per, int intervalTime, 
			const boost::optional<seqno_t>& smallest_seqno_i_care, 
			double src_packet_rate);

		//	��ȡlocal buffermap��д��MsgType�С�����remote����Ƭ����Ϣʱ���á�
		//��Ƶ��ʹ����Ӳ�̴洢ʱ��Ҫ����channel_uuid_for_erased_seqno_on_disk_cache��
		//����ʹ���һ��NULL��
		template<typename MsgType>
		void get_buffermap(MsgType& msg, int src_packet_rate, 
			bool using_bitset = false, bool using_longbitset = false, 
			const std::string* channel_uuid_for_erased_seqno_on_disk_cache=NULL
			)
		{
			buffermap_info* mutable_buffermap = msg.mutable_buffermap();
			get_buffermap(mutable_buffermap, src_packet_rate, using_bitset, 
				using_longbitset, channel_uuid_for_erased_seqno_on_disk_cache);
		}

		//	��ȡlocal buffermap��д��buffermap_info�С�����remote����Ƭ����Ϣʱ���á�
		//��Ƶ��ʹ����Ӳ�̴洢ʱ��Ҫ����channel_uuid_for_erased_seqno_on_disk_cache��
		//����ʹ���һ��NULL��
		void get_buffermap(buffermap_info* mutable_buffermap, int src_packet_rate, 
			bool using_bitset = false, bool using_longbitset = false, 
			const std::string* channel_uuid_for_erased_seqno_on_disk_cache=NULL
			);

		//	�����յ���buffermap����Я����ɾ��seqno����һ�������VoD����£�remote��ǰ
		//ͨ��buffermap��֪local�����ĳЩƬ�Σ���һ��ʱ�����ЩƬ��ɾ���ˡ�
		void process_erased_buffermap(peer_connection* conn, seqno_t seq_begin, 
			seqno_t seq_end, seqno_t smallest_seqno_i_care, timestamp_t now);

		//	�����յ���buffermap��
		typedef std::vector<boost::weak_ptr<peer_connection> > connection_vector;
		void process_recvd_buffermap(const buffermap_info& bufmap, 
			const connection_vector& in_substream, peer_connection* conn, 
			seqno_t smallest_seqno_i_care, timestamp_t now, bool add_to_owner);

		void process_recvd_buffermap(const std::vector<seqno_t>&seqnomap, 
			const connection_vector& in_substream, peer_connection* conn, 
			seqno_t bigest, seqno_t smallest_seqno_i_care, timestamp_t now, 
			bool add_to_owner, bool recentRecvdSeqno);

		void get_memory_packet_cache_buffermap(buffermap_info* mutableBuffermap, 
			int bitSize);
		void get_disk_packet_cache_buffermap(buffermap_info* mutableBuffermap, 
			const std::string& channel_uuid);

	private:
		int max_inject_count();
		void just_known(seqno_t seqno, const peer_connection_sptr& p, 
			seqno_t smallest_seqno_i_care, timestamp_t now, bool add_to_owner);

	protected:
		//���ݵ��ȵ���ʼλ�ò����ǵ�ǰ�����������Ƭ�Σ����Ǳȵ�ǰƬ�κ�Сbackfetch_cnt
		//��Ƭ�Ρ�
		int backfetch_cnt_;
		int max_backfetch_cnt_;
		int max_bitmap_range_cnt_;
		boost::optional<seqno_t> current_server_seqno_;

		typedef boost::optional<std::pair<seqno_t, seqno_t> > optional_range;
		optional_range				seqno_range_;//�㲥��BT�ȵ�seqno���з�Χ�޶���
		boost::optional<seqno_t>	bigest_sqno_i_know_;

		absent_packet_list absent_packet_list_;
		packet_buffer memory_packet_cache_;
		typedef asfio::async_dskcache async_dskcache;
		boost::shared_ptr<async_dskcache> disk_packet_cache_;

		//////////////////////////////////////////////////////////////////////////
		//Ϊ����Ƶ���������һЩ��ʱ��������Ϊ��ĳ�Ա����
		std::vector<char> buffermap_;
		std::vector<seqno_t> seqnomap_buffer_;
	};

}

#endif//buffer_manager_h__

