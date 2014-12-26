#pragma once
#include <iostream>
#include "log.h"

// [Nov 7, 2014] huhu
// 适用于我们服务器的一个输入指令线程

using namespace std;

template<typename Tthis>
class cCinthread
{
	typedef bool (Tthis::*parseFunc)(std::string str);
public:
	cCinthread(parseFunc func, Tthis *pthis)
	{
		_pthis = pthis;
		_func = func;
		_cinthread = 0;
	}
	~cCinthread()
	{
		if(_cinthread)
			pthread_join(_cinthread, NULL);
	}

    static void *s_cin_pThread_handle( void *inFd )
    {
    	cCinthread *pthis = (cCinthread *)inFd;
    	char szBuffer[256] = "";
    	cCinthread::parseFunc func =  pthis->_func;
    	Tthis *funthis = pthis->_pthis;
        for (;;)
        {
        	cin >> szBuffer;
        	if(func && funthis)
        	{
        		if((funthis->*func)(szBuffer))
        			continue;
        		else
        			break;
        	}
        }
        cLog::scPrint("s_cin_pThread_handle exit");
        return NULL;
    }

	void run()
	{

	    if( 0 != pthread_create( &_cinthread, NULL, s_cin_pThread_handle, (void*)this ) )
	    {
	    	cLog::scPrint("创建命令处理线程失败！");
	        return;
	    }
	}
private:
	Tthis *_pthis;
	parseFunc _func;
    pthread_t _cinthread;
};

// [Nov 7, 2014] huhu 通用解释器
// 用cmdParserptr调用注册函数就可以了。
#include "Singleton.h"
#include "log.h"
#include <queue>
#include <map>
#include <string>
#include <sstream>

static void cmd_help(const char *szparam);
static void cmd_runtime(const char *szparam);
//static void cmd_exit(const char *szparam);

typedef void (*parsefunc_g)(const char *);
class cmdParser : public Singleton<cmdParser>
{
private:
	struct stParser
	{
		stParser()
		{
			func = 0;
		}
		void init(const char *cmd, const char *desc, parsefunc_g f)
		{
			strcmd = cmd;
			strdesc = desc;
			func = f;
		}
		std::string strcmd;
		std::string strdesc;
		parsefunc_g func;
	};
	typedef std::map<std::string, stParser> parserlist;
	parserlist m_listcmd;
	// [Nov 7, 2014] huhu 互斥锁
	pthread_mutex_t _commondmutex;
	// [Nov 7, 2014] huhu 命令队列
	std::queue<std::string> comstrqueue;

	// [Nov 10, 2014] huhu 程序开始时记录的时间
	time_t programstarttime;
public:
	/*	[Nov 7, 2014] huhu
	 *	循环调用，一有命令进来就处理，需要是对应的逻辑线程调用。
	*/
	void update(const char *szparam)
	{
		pthread_mutex_lock(&_commondmutex);
		std::queue<std::string> tempqueue(comstrqueue);
		while(comstrqueue.size())
			comstrqueue.pop();
		pthread_mutex_unlock(&_commondmutex);

		std::string str;
		while(tempqueue.size())
		{
			str = tempqueue.front();
			tempqueue.pop();
			parseCmdString( str );
		}
	}

	/*	[Nov 7, 2014] huhu
	 *	注册一个命令
	*/
	bool registercmd(const char *cmd, const char *desc, parsefunc_g f)
	{
		if(m_listcmd.find(cmd) != m_listcmd.end())
			return false;
		stParser &st = m_listcmd[cmd];
		st.init(cmd, desc, f);
		return true;
	}

	/*	[Nov 7, 2014] huhu
	 *
	 *	如果不管线程冲突，可以直接传入parseCmdString函数，在乎线程安全，就调用这个吧
	*/
	bool pushcommand(std::string str)
	{
		pthread_mutex_lock(&_commondmutex);
		comstrqueue.push(str);
		pthread_mutex_unlock(&_commondmutex);
		if(str == "exit")
			return false;
		else
			return true;
	}

	/*	[Nov 7, 2014] huhu
	 *	处理返回false表示退出线程
	 *	如果不管线程冲突，可以直接传入这个函数，在乎线程安全，就调用push吧
	*/
	bool parseCmdString(string cmd)
	{
		std::stringstream strStream(cmd);
		std::string str;
		if(!strStream.eof())
			strStream >> str;
		else
			return true;
		if(m_listcmd.find(str) == m_listcmd.end())
		{
			return true;
		}
		m_listcmd[str].func(cmd.c_str());

		if(str == "exit")
			return false;
		else
			return true;
	}

	void help()
	{
		for(parserlist::iterator it = m_listcmd.begin(); it != m_listcmd.end(); ++it)
		{
			stParser &per = it->second;
			cLog::scPrint("%s [%s]", per.strcmd.c_str(), per.strdesc.c_str());
		}
	}

	const time_t& getstarttime()  const
	{
		return programstarttime;
	}


private:
	cmdParser()
	{
		programstarttime = time(0);
		registercmd("help", "帮助信息", cmd_help);
		registercmd("runtime", "当前服务器运行了多长时间了", cmd_runtime);
//		registercmd("exit", "退出服务器", cmd_exit);
	}
	virtual ~cmdParser()
	{

	}
	friend class Singleton<cmdParser>;
};

#define cmdParserptr cmdParser::sharedPtr()

static void cmd_help(const char *szparam)
{
	cmdParserptr->help();
}

static void cmd_runtime(const char *szparam)
{
	time_t start = cmdParserptr->getstarttime();
	time_t now = time(NULL);
	time_t d = now - start;
	char tmpstart[128];
	strftime( tmpstart, sizeof(tmpstart), "%m/%d %X",localtime(&start));
	cLog::scPrint("开启时间：%s  开启时长：%I64u(秒)", tmpstart, d);
}

//static void cmd_exit(const char *szparam)
//{
//	cLog::scPrint("开始退出");
//	extern void g_cmd_exit();
//	g_cmd_exit();
//}

