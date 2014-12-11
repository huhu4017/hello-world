#pragma once
#include <pthread.h>
#include "Singleton.h"
#include "asio_connect.h"

class connection_base;

using boost::asio::ip::tcp;

typedef std::map<int , connectbase_ptr > connectmap;

// 线程列表
typedef std::list<pthread_t> pthreadid_list;

struct stAcceptorInfo
{
	tcp::acceptor _acceptor;
	char _szAcceptorname[128];
	stAcceptorInfo(boost::asio::io_service &io, const tcp::endpoint &endpoint)
	: _acceptor(io, endpoint)
	{
		memset(_szAcceptorname, 0, sizeof(_szAcceptorname));
	}
	~stAcceptorInfo()
	{

	}
};

// 线程，服务器接口
class asio_netModule : public Singleton<asio_netModule>
{
public:
	/*	[Oct 21, 2014] huhu
	*	运行服务器，在调用之前起码需要addserver一个服务器，才能正常运行。
	*/
	bool run(int ithreadcount = 1);

	/*	[Oct 21, 2014] huhu
	 *	全部都关闭，如果popmsg是wait中，则需要先push一个空消息进来，再调用stop
	*/
	void stop();

	// 为了个接口用的回调
	static void *threadhandler_static(void *args);

	/*	[Oct 21, 2014] huhu
	 *	新增一个服务器
	*/
	void addserver(const char *szServerName, const char *szip, int iport);

	/*	[Oct 25, 2014] huhu
	 *	添加一个连其他服务器的客户端连接
	*/
	connectasclient_ptr addclient(const char *szclientname, const char *szip, int iport);

	/*	[Oct 22, 2014] huhu
	 *	踢掉某个连接
	*/
	bool removeConnect(int iconnectid);

	/*	[Oct 22, 2014] huhu
	 *	发送消息
	 *	返回值并不能代表是否发送成功，只是代表做了发送这个操作
	*/
	bool sendMsg(int iconnectid, const char *pMsg, size_t size, const char *szMsgDescription = NULL);

	/*	[Oct 26, 2014] huhu
	 *	取一个消息出来
	*/
	bool popMsg(stMsg *pMsgout, bool bwait = false);

	/*	[Oct 26, 2014] huhu
	 *	接收消息到消息缓存里
	*/
	void pushmsg(vbuffer &buffer, size_t size, int iconnectid);

	/*	[Oct 26, 2014] huhu
	 *
	*/
	connectFromclient *getconnectFromclient(int iconnectid);

	connectAsclient *getconnectAsclient(int iconnectid);

	connectmap::iterator getconnectFromclientBegin()
	{
		return _mapConnects.begin();
	}

	connectmap::iterator getconnectFromclientEnd()
	{
		return _mapConnects.end();
	}

	connectmap::iterator getconnectAsclientBegin()
	{
		return _mapclientConnects.begin();
	}

	connectmap::iterator getconnectAsclientEnd()
	{
		return _mapclientConnects.end();
	}

	/*	[Nov 10, 2014] huhu
	 *	消息列表数量，加了锁的。
	*/
	size_t getmsglistsize();

	/*	[Nov 28, 2014] huhu
	 *	是否在运行中
	*/
	bool isIo_serviceRunning()
	{
		return !io_service.stopped();
	}
protected:
	friend class connection_base;
	friend class connectAsclient;
	friend class connectFromclient;
	friend class unpacker;

	/*	[Oct 25, 2014] huhu
	 *	连接连接完成时调用
	*/
	void afterserverconnected(connectbase_ptr pConnect, boost::system::error_code const& ec);

	/*	[Oct 25, 2014] huhu
	 *	连接连接完成时调用
	*/
	void afterclientconnected(connectbase_ptr pConnect, boost::system::error_code const& ec);

	/*	[Oct 29, 2014] huhu
	 *	发送掉一个消息之后的触发
	*/
	void aftersSentMsg(boost::system::error_code const& ec, size_t bytes_transferred, unsigned int msghead);

	/*	[Oct 22, 2014] huhu
	 *	获取io设备实例
	*/
	inline boost::asio::io_service& getio_service()
	{
		return io_service;
	}

	/*	[Oct 25, 2014] huhu
	 *	创建一个connectid
	*/
	static int makeconnectid();

	// 真正的线程回调函数
	void *threadhandler(void *args);

	/*	[Oct 27, 2014] huhu
	 *	新生成一个连接，继续等待接受某个端口的连接请求
	*/
	void conntinueAccept(int iport);

//	void pushsentmsg(stMsg &msg);
private:
	asio_netModule();
	virtual ~asio_netModule();
	friend class Singleton<asio_netModule>;
private:
	// [Nov 28, 2014] huhu 保持service：：run在没有任务时不退出。
	boost::shared_ptr<boost::asio::io_service::work> m_work;
	// ioservice
	boost::asio::io_service io_service;
	// io线程列表
	pthreadid_list _iorun_threadlist;

	// 服务器保存的客户端连接
	connectmap _mapConnects;

	// 连接到其他服务器的客户端连接
	connectmap _mapclientConnects;

	typedef boost::shared_ptr<stAcceptorInfo> acceptorinfo_ptr;
	// key:port
	typedef std::map<int, acceptorinfo_ptr> acceptormap;
	acceptormap _mapacceptors;

	msglist _msglist;

    pthread_mutex_t _packetQueueLock;
	pthread_cond_t m_netcv; // 全局条件变量.

    // 发送出去的消息的缓存
//    msgmap _sentmsglist;
};

#define asio_net_module asio_netModule::sharedPtr()
