#include "asio_connect.h"
#include "log.h"
#include "asio_netModule.h"

unsigned short msgindex = 0;
using namespace std;

// [Nov 10, 2014] huhu 发送成功的消息数量
unsigned long long g_sentsucmsgcount_asio = 0;
// [Nov 10, 2014] huhu 调用发送的数量
unsigned long long g_sentmsgcount_asio = 0;

#define is_debug_net if(0)

extern void netevent(int iconnectid, int ievent/*e_event*/, int iparam);

connection_base::connection_base(boost::asio::io_service& io_service, const char *szname, asio_netModule *pModule)
: socket_(io_service)
, _strand(io_service)
, _sizebuffer(0)
, _bodybuffer(BODYBUFFER_LEN, 0)
, m_pNetModule(pModule)
{
	_strname = szname;
	_connectid = asio_netModule::makeconnectid();
	m_bconnected = 0;
}

connection_base::~connection_base()
{
	cLog::scPrint("connection_base ~connection_base %s %d %lu", _strname.c_str(), _connectid, pthread_self());
	boost::system::error_code er;
	CloseSocket(er);
}

void connection_base::sendMsg(const char *pMsg, size_t size, const char *szMsgDescription/* = NULL */)
{
	union
	{
		char bytes[4];
		unsigned int isize;
	}tempunion;

	stMsg tempmsg;
	tempmsg.iconnectid = getconnectid();
	tempmsg.msg.resize(size+4);
	tempunion.isize = size;
	for(size_t ihead = 0; ihead < 4; ++ihead)
	{
		tempmsg.msg[ihead] = tempunion.bytes[ihead];
	}
	memcpy(&tempmsg.msg[4], pMsg, size);
//	for(size_t i = 4; i < size + 4; ++i)
//	{
//		tempmsg.msg[i] = pMsg[i-4];
//	}
//	msgindex++;
//	tempmsg.msg[4+1] = msgindex>>8;
//	tempmsg.msg[4] = msgindex&0xff;
	async_write(socket_, boost::asio::buffer(tempmsg.msg, tempunion.isize+4),_strand.wrap(boost::bind(&connection_base::AfterSendString, shared_from_this()
			, boost::asio::placeholders::error
			, boost::asio::placeholders::bytes_transferred
			, tempunion.isize, size)));
	++g_sentmsgcount_asio;

//	m_pNetModule->pushsentmsg(tempmsg);
}

void connection_base::CloseSocket(boost::system::error_code const& ec)
{
	is_debug_net
	cLog::scPrint("%lu connection_base close socket_", pthread_self());
	try
	{
		if(socket_.is_open())
		{
			afterCloseSocket();
			netevent(getconnectid(), e_event_closed, ec.value());
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			socket_.close();
			m_bconnected = 0;
		}
	}
	catch (std::exception& e)
	{
		cLog::scPrint("%lu Connection connectid:%d %s", pthread_self(), _connectid, e.what());
	}
}

void connection_base::onconnected(boost::system::error_code const& ec)
{
	afterOnconnected(ec);
	if (ec)
	{
		m_bconnected = 0;
		cLog::scPrint("%lu [%s] onconnected false %s", pthread_self(), getname(), ec.message().c_str());
		return;
	}
	m_bconnected = 1;
	cLog::scPrint("%lu %s connected", pthread_self(), _strname.c_str());
	readmsg();
	netevent(getconnectid(), e_event_connected, 0);
}

void connection_base::AfterReadMsg(boost::system::error_code const & ec, size_t bytes_transferred)
{
	afterReadMsg(ec, bytes_transferred);
	is_debug_net
	cLog::scPrint("%lu connection_base enter AfterReadMsg", pthread_self());
	if (ec)
	{
		cLog::scPrint("%lu [%s] AfterReadMsg %s", pthread_self(), getname(), ec.message().c_str());
		return;
	}
	bool unpack_ok = unpacker_.on_recv(bytes_transferred, getconnectid(), m_pNetModule);
	if (unpack_ok)
		readmsg();
}

void connection_base::AfterSendString(boost::system::error_code const& ec, size_t bytes_transferred, unsigned int msghead, size_t realsize)
{
	g_sentsucmsgcount_asio++;
	afterSentMsg(ec, bytes_transferred, "");
	is_debug_net
	cLog::scPrint("%lu %s AfterSendString size:%u", pthread_self(), getname(), bytes_transferred);
	if(ec)
	{
		cLog::scPrint("%lu [%s] AfterSendString error %s", pthread_self(), getname(), ec.message().c_str());
		return;
	}
	if(!ec)
		m_pNetModule->aftersSentMsg(ec, bytes_transferred, msghead);
	netevent(getconnectid(), e_event_sentmsg, (int)bytes_transferred);
}

void connection_base::readmsg()
{
	size_t min_recv_len;
	boost::asio::mutable_buffers_1 recv_buff = unpacker_.prepare_next_recv(min_recv_len);
	async_read(socket_, recv_buff, boost::asio::transfer_at_least(min_recv_len),
			_strand.wrap(bind(&connection_base::AfterReadMsg, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void connection_base::setName(const char* szName)
{
	_strname = szName;
}

int connection_base::isconnected()
{
	return m_bconnected;
}

//void connection_base::readbody(size_t size)
//{
//	_bodybuffer.assign(BODYBUFFER_LEN, 0);
//
//	size_t min_recv_len;
//	boost::asio::mutable_buffers_1 recv_buff = unpacker_.prepare_next_recv(min_recv_len);
//	async_read(socket_, recv_buff, boost::asio::transfer_at_least(min_recv_len),
//			_strand.wrap(bind(&connection_base::AfterReadString, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, size)));
//}

connectAsclient::connectAsclient(boost::asio::io_service& io_service, const char *szname, asio_netModule *pModule)
: connection_base(io_service, szname, pModule)
{
}

connectAsclient::~connectAsclient()
{
}

void connectAsclient::connect(const char* szip, int iport)
{
	char szport[128] = "";
	sprintf(szport, "%d", iport);
	connect(szip, szport);
}

void connectAsclient::connect(const char* szip, const char* szport)
{
	_strip = szip;
	_strport = szport;

    tcp::resolver::query query(_strip, _strport);
	tcp::resolver _resolver(m_pNetModule->getio_service());
    tcp::resolver::iterator iterator = _resolver.resolve(query);

    boost::asio::async_connect(socket_, iterator, _strand.wrap(boost::bind(&connectAsclient::onconnected, shared_from_this(), boost::asio::placeholders::error)));
    m_bconnected = 2;
}

void connectAsclient::afterCloseSocket()
{
	m_pNetModule->removeConnect(getconnectid());
}

void connectAsclient::afterOnconnected(boost::system::error_code const& ec)
{
	if(!ec)
		m_pNetModule->afterclientconnected(shared_from_this(), ec);
}

void connectAsclient::afterReadMsg(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if(ec)
		CloseSocket(ec);
}

void connectAsclient::afterSentMsg(const boost::system::error_code& ec,
		size_t bytes_transferred, const char* szMsgDescription)
{
	if(ec)
		CloseSocket(ec);
}

void connectAsclient::check_reconnect(time_t second)
{
	static time_t lasttime = time(0);
	time_t nowtime = time(0);
	second = std::min((time_t)1, second);
	if(nowtime - lasttime > second)
	{
		lasttime = nowtime;
		if(!isconnected())
		{
			cLog::scPrint("check_reconnect %s %s", _strip.c_str(), _strport.c_str());
			connect(_strip.c_str(), _strport.c_str());
		}
	}
}

connectFromclient::connectFromclient(boost::asio::io_service& io_service, const char *szname, asio_netModule *pModule)
: connection_base(io_service, szname, pModule)
{
	_playerguid = 0;
	_iserverid = 0;
	_strServername = szname;
}

connectFromclient::~connectFromclient()
{
}

void connectFromclient::startAccept(tcp::acceptor& acceptor, const char *szservertip)
{
	// 加这个似乎是可以预防连接断开状态问题样，需要详查 when using resue_address(true), the CLOSE_WIAT state is gone
	acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor.async_accept(socket(), boost::bind(&connectFromclient::onconnected, shared_from_this(), boost::asio::placeholders::error));

	is_debug_net
	cLog::scPrint("%lu start_accept %s", pthread_self(), szservertip);
}

void connectFromclient::afterCloseSocket()
{
	m_pNetModule->removeConnect(getconnectid());
}

void connectFromclient::afterOnconnected(boost::system::error_code const& ec)
{
	if(!ec)
		m_pNetModule->afterserverconnected(shared_from_this(), ec);
	m_pNetModule->conntinueAccept(getClientlocalPort());
}

void connectFromclient::afterReadMsg(const boost::system::error_code& ec, size_t bytes_transferred)
{
	if(ec)
		CloseSocket(ec);
}

void connectFromclient::afterSentMsg(const boost::system::error_code& ec, size_t bytes_transferred, const char* szMsgDescription)
{
	if(ec)
		CloseSocket(ec);
}

void connectFromclient::setPlayerid(long long iplayerid)
{
	_playerguid = iplayerid;
}

void connectFromclient::setServerid(int iserverid)
{
	_iserverid = iserverid;
}

void connectFromclient::setServername(const char* szServername)
{
	_strServername = szServername;
}

bool unpacker::on_recv(size_t bytes_transferred, int iconnectid, asio_netModule *pNetModule)
{
	//len(unsigned int) + msg
	data_len += bytes_transferred;

	boost::array<char, MAX_MSG_LEN>::iterator pnext = raw_buff.begin();
	bool unpack_ok = true;
	std::vector<size_t> msglenvec;
	while (unpack_ok)
	{
		if ((size_t) -1 != total_data_len)
		{
			if (data_len >= total_data_len) //one msg received
			{
				vbuffer buffer;
				buffer.resize(total_data_len - HEAD_LEN);
				for(unsigned int i = 0; i < total_data_len - HEAD_LEN; ++ i)
				{
					buffer[i] = (pnext + HEAD_LEN)[i];
				}
				pNetModule->pushmsg(buffer, total_data_len - HEAD_LEN, iconnectid);
//				msg_list.push(
//					boost::shared_ptr<std::string>(new std::string(pnext + HEAD_LEN, total_data_len - HEAD_LEN)));
				data_len -= total_data_len;
				msglenvec.push_back(total_data_len);
				std::advance(pnext, total_data_len);
				total_data_len = -1;
			}
			else
				break;
		}
		else if (data_len >= HEAD_LEN) //the msg's head been received
		{
			total_data_len = (*(unsigned int*) pnext) + HEAD_LEN;
			if (total_data_len > MAX_MSG_LEN || total_data_len <= HEAD_LEN)
				unpack_ok = false;
		}
		else
			break;
	}

	if (!unpack_ok)
		reset_unpacker_state();
	else if (data_len > 0 && pnext > raw_buff.begin()) //left behind unparsed msg
		memcpy(raw_buff.begin(), pnext, data_len);

	return unpack_ok;
}
