#pragma once

#include <ev.h>
#include <map>
#include <string>
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libev.h"

#include <memory>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "log.h"
#include "Singleton.h"

enum
{
	e_ev_io,				// IO事件
	e_ev_timer,			// 定时器
	e_ev_periodic,		// 周期性，指定日期一类的定时器
	e_ev_signal,		// 信号处理
	e_ev_child,			// 子进程状态变化
	e_ev_stat,			// 文件属性变化
	e_ev_idle,			// 每次event loop空闲触发事件
	e_ev_prepare,		// 每次event loop之前事件
	e_ev_embed,
	e_ev_fork,			// 开辟进程
	e_ev_async,		// 线程同步
	e_ev_cleanup,		// event loop退出触发事件
	e_ev_check,		// 每次event loop之后事件
};

class RedisElement
{
public:
	RedisElement()
	{
		m_c = NULL;
		m_reply = NULL;
	}
	RedisElement(redisContext *c, redisReply *reply)
	{
		m_c = c;
		m_reply = reply;
	}
	/*	[Dec 26, 2014] huhu
	 *	this class not take charge of recovery redis reply,just use it.
	*/
	~RedisElement()
	{
		m_c = NULL;
		m_reply = NULL;
	}

	void init(redisContext *c, redisReply *reply)
	{
		m_c = c;
		m_reply = reply;
	}
private:
	redisReply *m_reply;
	redisContext *m_c;
};

template<typename Callback, typename ThisType>
struct Handler
{
public:
    Handler(Callback c, ThisType *t) : _c(c), _this(t)
	{}

    // [Dec 18, 2014] huhu 执行命令的回调
    static void callback(redisAsyncContext *c, void *reply, void *privdata)
    {
        (*(static_cast< Handler<Callback, ThisType> * > (privdata)))/*->operator()*/ (c,reply);
    }

    void operator() (redisAsyncContext *c, void *reply)
    {
        if (reply)
        {
        	// not a Redis::Reply, to avoid re-release of reply
        	RedisElement replyPtr(&c->c, static_cast<redisReply*>(reply));
        	(_this->*_c)(&replyPtr);
        }
        else
        {
        	(_this->*_c)(NULL);
        }
        delete(this);
    }

    void operator() (const redisAsyncContext *c, int status)
    {
    	(_this->*_c)(c, status);
        delete(this);
    }

    Callback _c;
    ThisType *_this;
};

class RedisConnectionAsync : public Singleton<RedisConnectionAsync>
{
public:
	typedef void (*connectfn)(const redisAsyncContext*,int);

	/*	[Dec 26, 2014] huhu
	 *	connect redis.
	*/
	void start(const std::string& host, int port, connectfn onconnectedHander, connectfn ondisconnectedHander, struct ev_loop *pLoop)
	{
		_host = host;
		_ac = NULL;
		_port = port;
		_reconnect = false;
		m_pLoop = pLoop;
		m_connectedhander = onconnectedHander;
		m_disconnectedhander = ondisconnectedHander;

		asyncConnect();
	}

	/*	[Dec 26, 2014] huhu
	 *	just stop and Free redis connection,don't need disconnect
	*/
    void stop()
    {
        if (_ac)
        {
        	ev_io_stop(m_pLoop, &((((redisLibevEvents*)(_ac->ev.data)))->rev));
        	ev_io_stop(m_pLoop, &((((redisLibevEvents*)(_ac->ev.data)))->wev));
        	redisAsyncFree(_ac);
            // TODO: should we close it or not???
            // close(_ac->c.fd);
        }
		if(m_pLoop)
		{
	    	ev_loop_destroy(m_pLoop);
	    	m_pLoop = NULL;
		}
    }
private:
	// 传入的handler指针在析构时会自动删除
    RedisConnectionAsync()
		: _ac(NULL)
		, _port(0)
		, _reconnect(false)
		, m_pLoop(NULL)
		, m_connectedhander(NULL)
		, m_disconnectedhander(NULL)
    {}
    ~RedisConnectionAsync()
    {
    }
    friend class Singleton<RedisConnectionAsync>;
public:

    static void onConnected(const redisAsyncContext *c, int status)
    {
        void *data = c->data;
        if(((RedisConnectionAsync*)(data))->m_connectedhander)
        	((RedisConnectionAsync*)(data))->m_connectedhander(c, status);
    }

    static void onDisconnected(const redisAsyncContext *c, int status)
    {
        void *data = c->data;
        if(((RedisConnectionAsync*)(data))->m_disconnectedhander)
        	((RedisConnectionAsync*)(data))->m_disconnectedhander(c, status);
    }

    void disconnect()
    {
        if (_ac)
        {
            redisAsyncDisconnect(_ac);
        }
    }

    int execAsyncCommand(const char *cmd, ...)
    {
        va_list vl;
        va_start( vl, cmd );
        va_end(vl);
        if (_ac==NULL || (_ac->c.flags & (REDIS_DISCONNECTING | REDIS_FREEING)))
        {
//            	throw RedisException("Can't execute a command, disconnecting or freeing");
        	return -2;
        }
        int result = REDIS_ERR;
        result = redisvAsyncCommand(_ac, NULL, NULL, cmd, vl);
        return result;
    }

    // 回调函数模板，回调函数的对象模板
	template<typename commandfn, typename ThisType>
    int execAsyncCommand(commandfn handler, ThisType *t, const char *cmd, ...)
    {
        va_list vl;
        va_start( vl, cmd );
        va_end(vl);
        if (_ac==NULL || (_ac->c.flags & (REDIS_DISCONNECTING | REDIS_FREEING)))
        {
//            	throw RedisException("Can't execute a command, disconnecting or freeing");
        	return -2;
        }

        int result = REDIS_ERR;
        if(handler && t)
        {
            Handler<commandfn, ThisType> *hand = new Handler<commandfn, ThisType>(handler, t);
            result = redisvAsyncCommand(_ac, Handler<commandfn, ThisType>::callback, hand, cmd, vl);

            if (result == REDIS_ERR)
            {
                delete hand;
    //                throw RedisException("Can't execute a command, REDIS ERROR");
            }
        }
        else
        {
            result = redisvAsyncCommand(_ac, NULL, NULL, cmd, vl);
        }
        return result;
    }

    // 回调函数模板，回调函数的对象模板
    int asyncConnect()
    {
    	if(!m_pLoop)
    	{
    		return -4;
    	}
        _ac = redisAsyncConnect(_host.c_str(), _port);
        _ac->data = (void*)this;

        if (_ac->err)
        {
            //throw RedisException((std::string)"RedisAsyncConnect: "+_ac->errstr);
        	return -1;
        }
        if (redisAsyncSetConnectCallback(_ac, RedisConnectionAsync::onConnected)!=REDIS_OK
        		|| redisAsyncSetDisconnectCallback(_ac, RedisConnectionAsync::onDisconnected)!=REDIS_OK)
        {
            //throw RedisException("RedisAsyncConnect: Can't register callbacks");
        	return -2;
        }

        if (redisLibevAttach(m_pLoop, _ac)!=REDIS_OK)
        {
            //throw RedisException("redisLibevAttach: nothing should be attached when something is already attached");
        	return -3;
        }

        // actually start io proccess
        ev_io_start(m_pLoop, &((((redisLibevEvents*)(_ac->ev.data)))->rev));
        ev_io_start(m_pLoop, &((((redisLibevEvents*)(_ac->ev.data)))->wev));
        return 0;
    }

private:
    std::string        _host;
    redisAsyncContext* _ac;
    uint16_t           _port;
    bool               _reconnect;
    struct ev_loop *m_pLoop;
public:
    connectfn m_connectedhander;
    connectfn m_disconnectedhander;
};

class rediscommander
{
public:
	rediscommander(redisContext *c, const char *command, ...)
	{
		if(!command)
			return;
		m_pR = NULL;
		m_berror = false;

	    static char szBuffer[1024] = {0};
		memset( szBuffer, 0, sizeof( szBuffer ) );

	    va_list vl;
	    va_start( vl, command );
	    vsnprintf(szBuffer , 1024, command, vl );
	    va_end(vl);

	    m_pR = (redisReply *)redisCommand(c, command);
	    if(!m_pR)
	    	m_berror = true;
	}
	bool iserror()
	{
		return m_berror;
	}

#define getRediserrstr(c) c->errstr

	redisReply *getReply()
	{
		return m_pR;
	}

	~rediscommander()
	{
		freeReplyObject(m_pR);
		m_pR = NULL;
	}
private:
	redisReply *m_pR;
	bool m_berror;
};

// [Dec 23, 2014] huhu unblock redis connection for write to master
#define m_pAsyncRedis RedisConnectionAsync::sharedPtr()
