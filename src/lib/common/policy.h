#ifndef policy_h__
#define policy_h__

/************************************************************************/
/*  һЩ�������ܵ�ʵ��                                                  */
/************************************************************************/
#include "common/config.h"
#include "common/typedef.h"
#include "common/parameter.h"
#include "common/utility.h"

#ifdef P2ENGINE_DEBUG
#define MIX_ACCEPTOR_DBG(x) /*x*/
#else
#define MIX_ACCEPTOR_DBG(x)
#endif

namespace p2common{

	template<typename Base, typename Derived>
	struct get_derived_this
	{
		BOOST_STATIC_ASSERT((boost::is_base_and_derived<Base, Derived>::value));
		Derived* operator()(Base* ptr)const
		{
			Derived*p=(Derived*)(ptr);//!!DO NOT USING reinterpret_cast!!
			BOOST_ASSERT((uintptr_t)dynamic_cast<Derived*>(ptr)==(uintptr_t)(p));
			return p;
		}
	};

	/*! class ts2p_challenge_checker
	*  \brief ��֤.
	*
	*/
	class  ts2p_challenge_checker
	{
		typedef  ts2p_challenge_checker         this_type;
	public:
		//����challenge��Ϣ
		void send_challenge_msg(message_socket_sptr conn);
		//��֤�ظ�����Ϣ
		bool challenge_check(const p2ts_login_msg&  msg, const std::string& private_key);
		//��¼ʧ��
		void challenge_failed(message_socket_sptr conn, const int session, error_code_enum ec = e_unknown);
	protected:
		ts2p_challenge_checker();
		virtual ~ts2p_challenge_checker(){}
	protected:	
		std::string shared_key_signature(const std::string& pubkey);
	private:
		std::pair<std::string, std::string>    key_pair_;
	};

	class  p2ts_challenge_responser
	{
		typedef  p2ts_challenge_responser         this_type;
	public:
		//�ظ�challenge
		bool challenge_response(const ts2p_challenge_msg&  msg, client_param_sptr param, message_socket_sptr conn);
	protected:
		virtual ~p2ts_challenge_responser(){}
	};

	/************************************************************************/
	/* create(io_service&)                                                  */
	/************************************************************************/
#define  STATIC_IOS_CREATE_DECLARE\
	SHARED_ACCESS_DECLARE;\
public:\
	static boost::shared_ptr<this_type> create(io_service& svc) \
	{\
	boost::shared_ptr<this_type> obj(\
	new this_type(svc), shared_access_destroy<this_type>()\
	);\
	return obj;\
	};

#define  STATIC_VOID_CREATE_DECLARE\
	SHARED_ACCESS_DECLARE;\
public:\
	static boost::shared_ptr<this_type> create() \
	{\
	boost::shared_ptr<this_type> obj(\
	new this_type, shared_access_destroy<this_type>()\
	);\
	return obj;\
	};
}

#endif // policy_h__
