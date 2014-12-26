#include "asio_netModule.h"
#include "asio_connect.h"
#include "log.h"

#define is_debug_net if(0)

// [Nov 10, 2014] huhu 收到了的消息数量
unsigned long long g_recvedmsgcount_asio = 0;

int asio_netModule::makeconnectid()
{
	static int id = 0;
	++id;
	return id;
}

asio_netModule::asio_netModule()
{
	m_work.reset(new boost::asio::io_service::work(io_service));
	pthread_mutex_init(&_packetQueueLock, NULL);
	pthread_cond_init(&m_netcv, NULL);
}

asio_netModule::~asio_netModule()
{
    pthread_mutex_destroy(&_packetQueueLock);
	pthread_cond_destroy(&m_netcv);
}

bool asio_netModule::removeConnect(int iconnectid)
{
	if(_mapConnects.find(iconnectid) != _mapConnects.end())
	{
		_mapConnects.erase(iconnectid);
	}
	else if(_mapclientConnects.find(iconnectid) != _mapclientConnects.end())
	{
		_mapclientConnects.erase(iconnectid);
	}
	else
	{
		cLog::scPrint("removeConnect 查询失败 %d", iconnectid);
		return false;
	}
	return true;
}

bool asio_netModule::sendMsg(int iconnectid, const char* pMsg, size_t size, const char *szMsgDescription /*= NULL*/)
{
	connectmap::iterator it = _mapConnects.find(iconnectid);
	connection_base *pconnect = NULL;
	if(it != _mapConnects.end())
	{
		pconnect = it->second.get();
	}
	it = _mapclientConnects.find(iconnectid);
	if(it != _mapclientConnects.end())
	{
		pconnect = it->second.get();
	}
	if(pconnect)
	{
		pconnect->sendMsg(pMsg, size, szMsgDescription);
		return true;
	}
	return false;
}

void asio_netModule::aftersSentMsg(const boost::system::error_code& ec, size_t bytes_transferred, unsigned int msghead)
{
}

void* asio_netModule::threadhandler_static(void* args)
{
	asio_netModule *pthis = (asio_netModule*)args;
	return pthis->threadhandler(NULL);
}

void* asio_netModule::threadhandler(void* args)
{
	try
	{
		cLog::scPrint("%lu threadhandler io_service.run", pthread_self());
		//while(!io_service.stopped())
			io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
	cLog::scPrint("%lu asio_netModule threadhandler out", pthread_self());
	return NULL;
}

void asio_netModule::stop()
{
	cLog::scPrint("%lu server_mgr::stop stop", pthread_self());
	// 先关闭各个服务器
	for(acceptormap::iterator itserver = _mapacceptors.begin(); itserver != _mapacceptors.end(); ++itserver)
	{
		acceptorinfo_ptr &pserver = itserver->second;
		pserver->_acceptor.close();
	}

	// 处理完消息
	pthread_mutex_lock(&_packetQueueLock);
	if(_msglist.size())
	{
		cLog::scPrint("%lu _msglist unclear when stop net size:%u", pthread_self(), _msglist.size());
		while( _msglist.size() )
			_msglist.pop();
	}
	pthread_mutex_unlock(&_packetQueueLock);

	_mapacceptors.clear();

	_mapConnects.clear();

	_mapclientConnects.clear();

	io_service.stop();

	m_work.reset();

	for(pthreadid_list::iterator itthreadlist = _iorun_threadlist.begin(); itthreadlist != _iorun_threadlist.end(); ++itthreadlist)
	{
		pthread_join(*itthreadlist, NULL);
	}
	_iorun_threadlist.clear();
}

bool asio_netModule::run(int ithreadcount/* = 1 */)
{
	if(!_mapacceptors.size())
	{
		cLog::scPrint("asio_netModule run on _mapacceptors is empty");
	}
	if(_iorun_threadlist.size())
	{
		cLog::scPrint("io线程列表里面有线程id了");
		return false;
	}
	if(ithreadcount)
	{
		// 线程句柄
		pthread_t _threadt;
		for(int i = 0; i < ithreadcount; ++i)
		{
			if( 0 != pthread_create(&_threadt, NULL, threadhandler_static, (void*)this))
			{
				cLog::scPrint("创建threadhandler_static线程失败！%u %d", _threadt, i);
			}
			_iorun_threadlist.push_back(_threadt);
		}
	}
	else
		threadhandler(NULL);

//	pthread_create(&_threadt, NULL, threadhandler_static, (void*)this);

	cLog::scPrint("asio_netModule success run");
	return true;
}

void asio_netModule::conntinueAccept(int iport)
{
	acceptormap::iterator it = _mapacceptors.find(iport);
	if(it == _mapacceptors.end())
	{
		cLog::scPrint("%lu conntinueAccept acceptor can not find with port:%d", pthread_self(), iport);
		return;
	}
	connectfromclient_ptr newconnect(new connectFromclient(io_service, it->second->_szAcceptorname, this));
	newconnect->startAccept(it->second->_acceptor, it->second->_szAcceptorname);
}

void asio_netModule::addserver(const char* szServerName, const char *szip, int iport)
{
	if(_mapacceptors.find(iport) != _mapacceptors.end())
	{
		conntinueAccept(iport);
		return;
	}

	acceptorinfo_ptr ptr;
	// 没给ip就分配一个本机ip
	if(strcmp(szip, "") == 0)
	{
		ptr = acceptorinfo_ptr(new stAcceptorInfo(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), iport)));
	}
	else
	{
		ptr = acceptorinfo_ptr(new stAcceptorInfo(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::address(boost::asio::ip::address_v4::from_string(szip)), iport)));
	}
	strcpy(ptr->_szAcceptorname, szServerName);
	connectfromclient_ptr newconnect(new connectFromclient(io_service, szServerName, this));
	newconnect->startAccept(ptr->_acceptor, szServerName);
	_mapacceptors[iport] = ptr;
}

void asio_netModule::afterserverconnected(connectbase_ptr pConnect, boost::system::error_code const& ec)
{
	if(ec)
		return;
	int iconnectid = pConnect->getconnectid();

	if(_mapConnects.find(iconnectid) != _mapConnects.end())
	{
		return;
	}
	_mapConnects[iconnectid] = pConnect;
}

connectasclient_ptr asio_netModule::addclient(const char* szclientname, const char* szip, int iport)
{
	connectasclient_ptr client(new connectAsclient(io_service, szclientname, this));
	client->connect(szip, iport);
	return client;
}

void asio_netModule::afterclientconnected(connectbase_ptr pConnect, boost::system::error_code const& ec)
{
	if(ec)
		return;
	int iconnectid = pConnect->getconnectid();

	if(_mapclientConnects.find(iconnectid) != _mapclientConnects.end())
	{
		return;
	}
	_mapclientConnects[iconnectid] = pConnect;
}

void asio_netModule::pushmsg(vbuffer& buffer, size_t size, int iconnectid)
{
	is_debug_net
	cLog::scPrint("%lu pushmsg buffer size:%u size:%u", pthread_self(), buffer.size(), size);
	stMsg temp;
	temp.msg.insert(temp.msg.begin(), buffer.begin(), buffer.begin() + size);
	temp.iconnectid = iconnectid;

   pthread_mutex_lock(&_packetQueueLock);
	_msglist.push(temp);
	pthread_mutex_unlock(&_packetQueueLock);
	pthread_cond_signal(&m_netcv);
	g_recvedmsgcount_asio++;
}

bool  asio_netModule::popMsg(stMsg *pMsgout, bool bwait /* = false */)
{
	if(!pMsgout)
		return false;

	pthread_mutex_lock(&_packetQueueLock);
	size_t msgsize = _msglist.size();
	pthread_mutex_unlock(&_packetQueueLock);
	if(!msgsize)
	{
		if(bwait)
		{
			pthread_mutex_lock(&_packetQueueLock);
			while(!_msglist.size() && !io_service.stopped())
				pthread_cond_wait(&m_netcv, &_packetQueueLock);
			pthread_mutex_unlock(&_packetQueueLock);
		}
		else
		{
			return false;
		}
	}
	pthread_mutex_lock(&_packetQueueLock);
	stMsg tempmsg = _msglist.front();
	_msglist.pop();

	pMsgout->iconnectid = tempmsg.iconnectid;
	pMsgout->msg.insert(pMsgout->msg.begin(), tempmsg.msg.begin(), tempmsg.msg.end());
	pthread_mutex_unlock(&_packetQueueLock);

	is_debug_net
	cLog::scPrint("%lu popMsg buffer pMsgoutsize:%u _msglistsize:%u", pthread_self(), pMsgout->msg.size(), msgsize);
	return true;
}

connectFromclient *asio_netModule::getconnectFromclient(int iconnectid)
{
	if(_mapConnects.find(iconnectid) != _mapConnects.end())
	{
		return (connectFromclient*)_mapConnects[iconnectid].get();
	}
	cLog::scPrint("getconnectFromclient 查询失败 %d", iconnectid);
	return NULL;
}

connectAsclient *asio_netModule::getconnectAsclient(int iconnectid)
{
	if(_mapclientConnects.find(iconnectid) != _mapclientConnects.end())
	{
		return (connectAsclient*)_mapclientConnects[iconnectid].get();
	}
	return NULL;
}

size_t asio_netModule::getmsglistsize()
{
	pthread_mutex_lock(&_packetQueueLock);
	size_t size = _msglist.size();
	pthread_mutex_unlock(&_packetQueueLock);
	return size;
}
//void asio_netModule::pushsentmsg(stMsg& msg)
//{
//	union
//	{
//		unsigned short index;
//		char szindex[2];
//	}un;
//	un.szindex[0] = msg.msg[4];
//	un.szindex[1] = msg.msg[4+1];
//	_sentmsglist[un.index] = msg;
//}
