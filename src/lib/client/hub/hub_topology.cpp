//
// hub_topology.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2008 GuangZhu Wu (WuGuangZhu@gmail.com)
//
// All rights reserved. 
//

#include "client/peer.h"
#include "client/peer_connection.h"
#include "client/client_service.h"
#include "client/hub/hub_scheduling.h"
#include "client/hub/hub_topology.h"

using namespace p2client;

//TODO�����¹���δʵ��
//////////////////////////////////////////////////////////////////////////
//��client_service��δʹ��DHT�Ƚϸ߼����������ˡ����������ҵľ������󣬴������˼·Ϊ:
//ÿ���ڵ��ڽ��������Һ���ͬ�����ڵ㽨�����ӣ����������
//MAX_MESSAGE_CONN_CNT=100��ping_interval����Ϊ�����(disable ping������tracker
//�ĵ��߹�����֪���ڵ������״̬)������Ҫ���ȶ�����һ�����ӣ����ǶԷ����ߣ���
//�������׶Ͽ����ӡ����ӵĽڵ�ÿ��INFO_EXCHANGE_INTERVAL=20s�໥������Ϣ����Ϣ��Ҫ����
//�������ŵĽڵ㣨����ʹ��������Ϣ��ʽ����overhead�����������ڵ����֪�����ھӽڵ�
//������Щ�ڵ�������
//��һ���ڵ�A��Ҫ����p2p��Ϣ����һ���ڵ�ʱDʱ��A���Ȳ鿴�Ƿ��D���������û������
//����A����A���ھӽڵ㣬��˭��D�������������B��D����������B·����Ϣ��D�����
//����δ�ܳɹ�����Aֱ������Tracker·�ɡ�����Ժܸߵĸ���ͨ��ֱ�ӷ��ͻ����ھ�ת��
//���p2p��Ϣ��ֻ��������Ϣ��Ҫͨ��Tracker·�ɣ�Ҳ������nat��Խ�ȵ�һЩ�鷳��
//������Ϣʹ��flood��ʽ��
//////////////////////////////////////////////////////////////////////////

#define GUARD_OVERLAY \
	client_service_sptr  ovl=client_service_.lock();\
	if (!ovl) {stop();return;}

namespace
{
	time_duration MAINTAINER_INTERVAL=milliseconds(500);
}

//��Ϣ������ע��
//void hub_topology::register_handler()
//{
//#define REGISTER_HANDLER(msgType, handler)\
//	msg_handler_map_.insert(std::make_pair(msgType, boost::bind(&this_type::handler, SHARED_OBJ_FROM_THIS, _1, _2) ) );
//
//	REGISTER_HANDLER(peer_peer_msg::handshake_msg, on_recvd_handshake);
//}

hub_topology::hub_topology(client_service_sptr ovl)
:overlay(ovl, HUB_TOPOLOGY)
{
	set_obj_desc("hub_topology");

	BOOST_ASSERT(ovl);
}

hub_topology::~hub_topology()
{
}

void hub_topology::start()
{
	const std::string& channelUUID = get_client_param_sptr()->channel_uuid;
	topology_acceptor_domain_base_=std::string("hub_topology")+"/"+channelUUID;
	overlay::start();
	
	//����������
	scheduling_=hub_scheduling::create(SHARED_OBJ_FROM_THIS);
	scheduling_->start();

	//����URDP��������������������
	DEBUG_SCOPE(
		std::cout<<"hub_topology udp_local_endpoint_:"<<udp_local_endpoint_<<std::endl;
	std::cout<<"hub_topology tcp_local_endpoint_:"<<tcp_local_endpoint_<<std::endl;
	);
}

void hub_topology::stop(bool flush)
{
	overlay::stop(flush);
}

//void hub_topology::on_accepted(peer_connection_sptr conn, const error_code& ec)
//{
//	if(!ec)
//	{
//		//����������δ�յ�handshakerǰ�ǲ��ŵ�neighbors�еģ�Ҳ������stream_scheduling����
//		BOOST_ASSERT(!conn->get_peer());//��ʱ��û�а󶨾���peer
//		keep_pending_passive_sockets(conn);
//		conn->register_message_handler(peer_peer_msg::handshake_msg).bind(
//			&this_type::on_recvd_handshake, this, conn.get(), _1);
//		conn->disconnected_signal().bind(&this_type::on_disconnected, this, conn.get(), _1);
//		conn->ping_interval(HUB_PEER_PEER_PING_INTERVAL);
//		conn->keep_async_receiving();
//	}
//	else
//	{
//		BOOST_ASSERT(!pending_passive_sockets_.is_keeped(conn));
//	}
//}
//
//void hub_topology::on_connected(peer_connection* connPtr, const error_code& ec)
//{
//	BOOST_ASSERT(connPtr->get_peer());
//
//	peer_connection_sptr conn=connPtr->shared_obj_from_this<peer_connection>();
//	if (!ec)
//	{
//		if(!pending_to_neighbor(conn, HUB_NEIGHTBOR_PEER_CNT))
//		{
//			conn->close();
//			return;
//		}
//
//		//����handshake�⣬��Ϣ����Ȩ����scheduling
//		scheduling_->register_message_handler(conn.get());
//		conn->register_message_handler(peer_peer_msg::handshake_msg).bind(
//			&this_type::on_recvd_handshake, this, conn.get(), _1);//handshake��Ϣ��SHARED_OBJ_FROM_THIS����
//		conn->disconnected_signal().bind(&this_type::on_disconnected, this, conn.get(), _1);
//		conn->keep_async_receiving();
//		conn->ping_interval(HUB_PEER_PEER_PING_INTERVAL);
//
//		//�����������ӶԷ���һ�����ӽ�����������Է�����һ��handshanke
//		scheduling_->send_handshake_to(conn.get());
//	}
//	else
//	{
//		BOOST_ASSERT(conn->get_peer());
//		conn->keep_async_receiving();
//		peer_sptr p=conn->get_peer();
//		if (p)
//		{
//			peer_id_t id=p->peer_info().peer_id();
//			pending_to_member(p);
//			//��ʱ������ȷ����һ�ڵ��Ƿ���߻��߲��ɴ���ԣ�����ɾ���˽ڵ㣬
//			//��һ��ʱ���ڲ��ٴ�������һ�ڵ�
//			low_capacity_peer_keeper_.try_keep(id, seconds(5));
//		}
//	}
//}
//
//void hub_topology::on_disconnected(peer_connection* conn, const error_code& ec)
//{
//	scheduling_->on_disconnected(conn, ec);
//	peer_sptr p=conn->get_peer();
//	if (p)
//	{
//		neighbor_to_member(conn->shared_obj_from_this<peer_connection>());
//	}
//}
//
//void hub_topology::on_recvd_handshake(peer_connection* connPtr, safe_buffer& buf)
//{
//	GUARD_OVERLAY;
//
//	peer_connection_sptr conn=connPtr->shared_obj_from_this<peer_connection>();
//
//	pending_passive_sockets_.erase(conn);
//
//	//����handshake_msg�������֪�����peer����뵽peer����
//	p2p_handshake_msg msg;
//	if(!parser(buf, msg))
//	{
//		PEER_LOGGER(warning)<<"can't parser p2p_handshake_msg";
//		conn->close();
//		return;
//	}
//	ovl->known_new_peer(msg.peer_info());
//
//	boost::shared_ptr<peer> p=ovl->find_peer(msg.peer_info().peer_id());
//	if (!p)//����ĳԭ��δ����client_service��ע�᱾�ڵ�
//	{
//		conn->close();
//		return;
//	}
//	//�����û������peer��˵����һ���������ӣ�ֻ����������²Ż���һ��handshake
//	bool needSendHandshake=(conn->get_peer().get()==NULL);
//	//��peer���ڱ�������
//	conn->set_peer(p);
//	//����members�ƶ���neighbors
//	if(!member_to_neighbor(conn, HUB_NEIGHTBOR_PEER_CNT))
//	{
//		conn->close();
//		return;
//	}
//
//	//��Ϊ�ھӽڵ�ɹ�, ����msg��Я����buffermap��pieceinfo��Ϣ
//	//if (msg.has_buffermap())
//	//{
//	//	scheduling_->process_recvd_buffermap(msg.buffermap(), conn.get());
//	//}
//	//if (msg.has_compressed_buffermap())
//	//{
//
//	//}
//	//for (int i=0;i<msg.pieceinfo_size();++i)
//	//{
//	//	stream_scheduling_->process_recvd_pieceinfo(msg.pieceinfo(i), conn.get());
//	//}
//
//	if (needSendHandshake)
//	{
//		//����Ϣ����Ȩ����stream_scheduling
//		scheduling_->register_message_handler(conn.get());
//
//		//�ظ�һ��handshake��ʹ�öԷ��˽Ȿ�ڵ������״̬
//		scheduling_->send_handshake_to(conn.get());
//	}
//}

bool hub_topology::is_black_peer(const peer_id_t& id)
{
	return low_capacity_peer_keeper_.is_keeped(id);
}

void hub_topology::try_shrink_neighbors(bool explicitShrink)
{
}

bool hub_topology::can_be_neighbor(peer_sptr p)
{
	return true;
}
