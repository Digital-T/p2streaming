//
// hub_topology.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 GuangZhu Wu (GuangZhuWu@gmail.com)
//
// All rights reserved. 
//
#ifndef peer_hub_topology_h__
#define peer_hub_topology_h__

#include "client/overlay.h"

namespace p2client
{
	class hub_scheduling;

	//���ݵ������˲�
	class hub_topology
		:public overlay
	{
		typedef hub_topology this_type;
		SHARED_ACCESS_DECLARE;
		typedef boost::shared_ptr<hub_scheduling> scheduling_sptr;
		typedef rough_timer timer;

	protected:
		hub_topology(client_service_sptr ovl);
		virtual ~hub_topology();

	public:
		static shared_ptr create(client_service_sptr ovl)
		{
			return shared_ptr(new this_type(ovl), 
				shared_access_destroy<this_type>());
		}

		//�̳���overlay
		virtual void start();
		virtual void stop(bool flush=false);

		virtual void try_shrink_neighbors(bool explicitShrink);
		virtual bool is_black_peer(const peer_id_t& id);//�Ƿ��ں�������
		virtual bool can_be_neighbor(peer_sptr p);
	};
}

#endif //peer_hub_topology_h__
