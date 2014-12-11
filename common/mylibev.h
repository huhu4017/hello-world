#pragma once

#include <ev.h>
#include <map>
#include <string>

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

// [Dec 2, 2014] huhu 获取当前时间使用ev_now()

class cevWatcherMgr;

typedef int (*cbf)(unsigned long long , int);
typedef ev_tstamp (*cbperiodic)(ev_periodic* , ev_tstamp);

class cevWatcher
{
public:
	cevWatcher(unsigned long long callerid, const char *szname)
	: m_callerid(callerid)
	, m_strname(szname)
	{
		m_cb = NULL;
	}
	~cevWatcher()
	{	}
    inline const char *getName()
    {
    	return m_strname.c_str();
    }
protected:
    unsigned long long m_callerid;
    cbf m_cb;
    std::string m_strname;
};

class cevWatcherMgr
{

public:
    cevWatcherMgr();
	virtual ~cevWatcherMgr();

	// base object
	bool create(void *buffer, long size);
    void update(void);

    /*	[Dec 1, 2014] huhu
	 *	事件循环管理器
	*/
    struct ev_loop *getloop()
    {
    	return loop;
    }

    /*	[Dec 1, 2014] huhu
	 *	先手动生成一个，然后通过这个接口放进来。
	*/
    bool addWatcher(cevWatcher *pnewWatcher);

    /*	[Dec 1, 2014] huhu
	 *	通过Watcher名字获取Watcher对象
	*/
    cevWatcher *getWatcher(std::string strWatcher);

    /*	[Dec 1, 2014] huhu
	 *	通过Watcher名字删除Watcher对象，在函数中就会回收对象内存
	*/
    bool deleteWatcher(std::string strWatcher);
private:
    struct ev_loop *loop;
    typedef std::map<std::string , cevWatcher*> mapWatcher;
    mapWatcher m_mapWatchers;
};

// timer
class cevTimer : public cevWatcher
{
public:
    cevTimer(unsigned long long callerid, const char *szname);
    ~cevTimer();
public:
    /*	[Dec 1, 2014] huhu
	 *	设置数据
	*/
	bool create(void* buffer, int after, int repeat, cbf cb);

    /*	[Dec 1, 2014] huhu
	 *	修改触发时间和重复与否
	 *	Configure the timer to trigger after after seconds. If repeat is 0, then it will automatically be stopped once the timeout is reached.
	 *	If it is positive, then the timer will automatically be configured to trigger again repeat seconds later, again, and again,until stopped manually.
	 *	配置timer在after秒后触发。如果repeat参数是0，则这个触发器会在一次触发之后自动停止。如果是正数，那么这个timer会在被设置成在触发之后等repeat秒后重复触发，直到手动停止。
	*/
    bool setTime(int after, int repeat);

    /*	[Dec 1, 2014] huhu
	 *	停止这个计时器。
	*/
    void stop(cevWatcherMgr *pMgr);

    /*	[Dec 1, 2014] huhu
	 *	this function a little complicated.
	*/
    void again(cevWatcherMgr *pMgr);

    /*	[Nov 28, 2014] huhu
	 *	时间到了的时候调用的
	*/
    void invoke(int ievent)
    {
    	m_cb(m_callerid, ievent);
    }
protected:
private:

    ev_timer timeout_watcher;
};


class cevPeriodic : public cevWatcher
{
public:
	cevPeriodic(unsigned long long callerid, const char *szname);
    ~cevPeriodic();
public:
    /*	[Dec 1, 2014] huhu
	 *	设置数据
	*/
	bool create(void* buffer, int after, int repeat, cbf cb, cbperiodic cb_resc = 0);

    /*	[Dec 1, 2014] huhu
	 *	修改触发时间和重复与否
	 *	Configure the timer to trigger after after seconds. If repeat is 0, then it will automatically be stopped once the timeout is reached.
	 *	If it is positive, then the timer will automatically be configured to trigger again repeat seconds later, again, and again,until stopped manually.
	 *	配置timer在after秒后触发。如果repeat参数是0，则这个触发器会在一次触发之后自动停止。如果是正数，那么这个timer会在被设置成在触发之后等repeat秒后重复触发，直到手动停止。
	*/
    bool setTime(int after, int repeat, cbperiodic cb_resc = 0);

    /*	[Dec 1, 2014] huhu
	 *	停止这个计时器。
	*/
    void stop(cevWatcherMgr *pMgr);

    /*	[Dec 1, 2014] huhu
	 *	this function a little complicated.
	*/
    void again(cevWatcherMgr *pMgr);

    /*	[Nov 28, 2014] huhu
	 *	时间到了的时候调用的
	*/
    void invoke(int ievent)
    {
    	m_cb(m_callerid, ievent);
    }
protected:
private:

    ev_periodic periodic_watcher;
};

