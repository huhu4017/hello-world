#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <vector>
#include <list>
#include <queue>
#include <sys/types.h>


using boost::asio::ip::tcp;

typedef std::vector<char> vbuffer;

struct stMsg
{
	vbuffer msg;
	int iconnectid;
	stMsg()
	{
		iconnectid = 0;
	}
};
typedef std::queue<stMsg> msglist;
typedef std::map<unsigned short , stMsg> msgmap;

#ifndef SIZEBUFFER_LEN
#define SIZEBUFFER_LEN 4
#endif
#define BODYBUFFER_LEN 10240

#define MAX_MSG_LEN BODYBUFFER_LEN
#define HEAD_LEN 4

enum e_event
{
	e_event_connected,
	e_event_closed,
	e_event_sentmsg,

	e_event_max,
};

class unpacker
{
public:
	unpacker() {reset_unpacker_state();}

public:
	void reset_unpacker_state() {total_data_len = -1; data_len = 0;}

	bool on_recv(size_t bytes_transferred, int iconnectid);

	boost::asio::mutable_buffers_1 prepare_next_recv(size_t& min_recv_len)
	{
		assert(data_len < MAX_MSG_LEN);
		min_recv_len = ((size_t) -1 == total_data_len ? HEAD_LEN : total_data_len) - data_len;
		assert(0 < min_recv_len && min_recv_len <= MAX_MSG_LEN);
		return boost::asio::buffer(boost::asio::buffer(raw_buff) + data_len); //use mutable_buffer can protect raw_buff from accessing overflow
	}

private:
	boost::array<char, MAX_MSG_LEN> raw_buff;
	size_t total_data_len; //-1 means head has not received
	size_t data_len; //include head
};

class connection_base
  : public boost::enable_shared_from_this<connection_base>
{
public:
	connection_base(boost::asio::io_service& io_service, const char *szname);
	virtual ~connection_base();

	tcp::socket& socket()
	{
		return socket_;
	}

	inline int getconnectid()
	{
		return _connectid;
	}

	void sendMsg(const char *pMsg, size_t size, const char *szMsgDescription = NULL);

	/*	[Oct 21, 2014] huhu
	 *	关闭连接
	*/
	void CloseSocket(boost::system::error_code const& ec);

	/*	[Oct 24, 2014] huhu
	 *	关闭连接后通知子类
	*/
	virtual void afterCloseSocket(){}

	/*	[Oct 21, 2014] huhu
	 *	当连接后触发
	*/
	void onconnected(boost::system::error_code const& ec);

	/*	[Oct 24, 2014] huhu
	 *	通知子类连接完成
	*/
	virtual void afterOnconnected(boost::system::error_code const& ec){}

	/*	[Oct 21, 2014] huhu
	 *	读到长度消息之后的处理
	*/
	void AfterReadMsg(boost::system::error_code const& ec, size_t bytes_transferred);

	/*	[Nov 28, 2014] huhu
	 *	读取完成之后通知子类
	*/
	virtual void afterReadMsg(boost::system::error_code const& ec, size_t bytes_transferred){}

	/*	[Oct 21, 2014] huhu
	 *	发送消息体之后的处理
	*/
	void AfterSendString(boost::system::error_code const& ec, size_t bytes_transferred, unsigned int msghead, size_t realsize);

	/*	[Oct 24, 2014] huhu
	 *	通知子类消息发送完成
	*/
	virtual void afterSentMsg(boost::system::error_code const& ec, size_t bytes_transferred, const char *szMsgDescription){}

	/*	[Oct 26, 2014] huhu
	 *
	*/
	void setName(const char *szName);
	inline const char *getname()
	{
		return _strname.c_str();
	}

	/*	[Oct 27, 2014] huhu
	 *	是否已经连接了
	*/
	int isconnected();
private:
	void readmsg();

protected:
	tcp::socket socket_;
	// Strand to ensure the connection's handlers are not called concurrently.
	boost::asio::io_service::strand _strand;

	int _sizebuffer;

	// [Oct 26, 2014] huhu 消息buffer
	vbuffer _bodybuffer;

	// [Oct 25, 2014] huhu 连接号
	int _connectid;

	// [Oct 26, 2014] huhu 客户端过来的连接是自定义的名字，出去的连接表示客户端名字。
	std::string _strname;

	unpacker unpacker_;

	// [Nov 28, 2014] huhu 当前这个连接是否连接上了的
	// 0 没连上的，1连上了，2正在连接
	int m_bconnected;
};
typedef boost::shared_ptr<connection_base> connectbase_ptr;

/*	[Oct 24, 2014] huhu
 *	连接到一个服务器的连接，这个连接拥有者是个客户端
*/
class connectAsclient : public connection_base
{
public:
	connectAsclient(boost::asio::io_service& io_service, const char *szname);
	~connectAsclient();

	/*	[Oct 25, 2014] huhu
	 *	连接一个服务器
	*/
	void connect( const char *szip, int iport );

	void connect( const char *szip, const char *szport );

	/*	[Oct 24, 2014] huhu
	 *	关闭连接后通知子类
	*/
	virtual void afterCloseSocket();

	/*	[Oct 24, 2014] huhu
	 *	通知子类连接完成
	*/
	virtual void afterOnconnected(boost::system::error_code const& ec);

	/*	[Nov 28, 2014] huhu
	 *	读取完成之后通知子类
	*/
	virtual void afterReadMsg(boost::system::error_code const& ec, size_t bytes_transferred);

	/*	[Oct 24, 2014] huhu
	 *	通知子类消息发送完成
	*/
	virtual void afterSentMsg(boost::system::error_code const& ec, size_t bytes_transferred, const char *szMsgDescription);
private:
	// [Oct 25, 2014] huhu
	std::string _strip;
	std::string _strport;

protected:

};
typedef boost::shared_ptr<connectAsclient> connectasclient_ptr;

/*	[Oct 24, 2014] huhu
 *	连接到一个客户端的连接，这个连接拥有者是个服务器
*/
class connectFromclient : public connection_base
{
public:
	connectFromclient(boost::asio::io_service& io_service, const char *szname);
	~connectFromclient();

	/*	[Oct 25, 2014] huhu
	 *	监听
	*/
	void startAccept(tcp::acceptor &acceptor, const char *szservertip);

	/*	[Oct 24, 2014] huhu
	 *	关闭连接后通知子类
	*/
	virtual void afterCloseSocket();

	/*	[Oct 24, 2014] huhu
	 *	通知子类连接完成
	*/
	virtual void afterOnconnected(boost::system::error_code const& ec);

	/*	[Nov 28, 2014] huhu
	 *	读取完成之后通知子类
	*/
	virtual void afterReadMsg(boost::system::error_code const& ec, size_t bytes_transferred);

	/*	[Oct 24, 2014] huhu
	 *	通知子类消息发送完成
	*/
	virtual void afterSentMsg(boost::system::error_code const& ec, size_t bytes_transferred, const char *szMsgDescription);

	/*	[Oct 26, 2014] huhu
	 *	设置关联的玩家id
	*/
	void setPlayerid(long long iplayerid);
	inline long long getPlayerid()
	{
		return _playerguid;
	}

	/*	[Oct 26, 2014] huhu
	 *	服务器id
	*/
	void setServerid(int iserverid);
	inline int getServerid()
	{
		return _iserverid;
	}

	inline const char *getClientip()
	{
		return socket_.remote_endpoint().address().to_string().c_str();
	}

	inline int getClientlocalPort()
	{
		return socket_.local_endpoint().port();
	}

	/*	[Oct 26, 2014] huhu
	 *	服务器名字
	*/
	void setServername(const char *szServername);
	inline const char *getServername()
	{
		return _strServername.c_str();
	}
private:
	// [Oct 25, 2014] huhu 玩家的GUID
	long long _playerguid;
	// [Oct 25, 2014] huhu 所属服务器id
	int _iserverid;

	// [Oct 26, 2014] huhu 所属服务器的名字
	std::string _strServername;

protected:

};
typedef boost::shared_ptr<connectFromclient> connectfromclient_ptr;
