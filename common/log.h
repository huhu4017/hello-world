#pragma once
// 需要一个相对不太容易出问题，确实在其他地方无法创建实例的单键。
#include "Singleton.h"
#include <list>
// [Sep 29, 2014] huhu 新增跨平台多线程方式的log操作类
// 实现了windows和linux两个平台的多线程写本地log文件功能

// 如果都没有定义，则默认根据C++版本选择对应log方式
#ifndef Log_no
#ifndef Log_redis
	#ifndef Log_Thread_Local

		#ifndef WIN32
			#define Log_Thread_Local
			#pragma message("default log type : Log_Thread_Local")
		#else
			#define Log_no
			#pragma message("default log type : Log_no")
		#endif // #if __cplusplus >= 201103L

	#endif //#ifndef Log_Thread_Local
#endif //#ifndef Log_redis
#endif
enum log_type
{
	log_console,
	log_file,
};

#define OutputDebugStringA(info) std::cout << info;
// log类型 Log_redis Log_Thread_Local
//#define Log_redis
//#define Log_Thread_Local

// 定义接口
#ifdef Log_redis
#define log_interface cRedisLog::sharedPtr()
#endif

#ifdef Log_no
#define log_interface cLogno::sharedPtr()
#endif

#ifdef Log_Thread_Local
#define log_interface cThreadLog::sharedPtr()
#endif

// 单独开一个线程写在本地的log
#ifdef Log_Thread_Local

#include <pthread.h>
#include <string>
#include <list>

class cThreadLog : public Singleton<cThreadLog>
{
public:
	bool init();

	// 输出到文件的信息
	void fiPrint( const char *info, const char *filename = "Log.txt" );

	// 输出到服务器控制台的信息
	void scPrint( const char *info );

	// 输出到调试信息
	void dePrint(const char *info);

	// 输出到错误信息
	void erPrint(const char *info);

	/*	[10/9/2014 huhu]
	 *	处理所有列表里的log
	 *	返回false表示线程需要退出
	 */
	bool processlog();

	static void *update(void *args);
private:
	/*	[Sep 29, 2014] huhu
	 *	新增一个log
	 */
	void pushlog(const char *info, int type/*log_type*/, const char *filename = "log.txt");

	// 单键标配
private:
	cThreadLog();
	~cThreadLog();

	friend class Singleton<cThreadLog>;

public:
    pthread_t m_t;

    pthread_mutex_t m_mutex_list;

	pthread_cond_t m_cv; // 全局条件变量.

	typedef std::list< std::string > stringlist;
	stringlist m_listConsoleinfo;

	typedef std::list< std::pair<std::string, std::string> > stringpairlist;
	stringpairlist m_listfileinfo;

};
#endif

#ifdef Log_redis
#include "../redis/hiredis/hiredis.h"
class cRedisLog : public Singleton<cRedisLog>
{
public:
	bool init();

	// 输出到文件的信息
	// [Oct 13, 2014] huhu 注意！！filename不能有空格！！！
	void fiPrint( const char *info, const char *filename = "Log.txt" );

	// 输出到服务器控制台的信息
	void scPrint( const char *info );

	// 输出到调试信息
	void dePrint(const char *info);

	// 输出到错误信息
	void erPrint(const char *info);
private:
	// redis的连接
	redisContext *m_pRedisContext;
	// 返回值
	redisReply* _reply;

	// 单键标配
private:
	cRedisLog();
	~cRedisLog();

	friend class Singleton<cRedisLog>;

public:
};

#endif

#ifdef Log_no

class cLogno : public Singleton<cLogno>
{
public:
	bool init();

	// 输出到文件的信息
	// [Oct 13, 2014] huhu 注意！！filename不能有空格！！！
	void fiPrint( const char *info, const char *filename = "Log.txt" );

	// 输出到服务器控制台的信息
	void scPrint( const char *info );

	// 输出到调试信息
	void dePrint(const char *info);

	// 输出到错误信息
	void erPrint(const char *info);
private:

	// 单键标配
private:
	cLogno();
	~cLogno();

	friend class Singleton<cLogno>;

public:
};
#endif


//log head file


class cLog
{
public:
	cLog( void )  ;
	cLog( const char *logFile);
	virtual ~cLog( void );

	static void init()
	{
		log_interface->init();
	}

	/*	[Sep 24, 2014] huhu
	*	为了方便打log
	*/
	static const char *stringmaker(const char *info, ...);

	// 输出到文件的信息
	/*
        此函数主要是输出系统信息日志，具体规则参考redis.h文件的setSysLog函数

	*/
	static int fiPrint( const char *filename, const char *fmt, ... );

	/*	[Dec 2, 2014] huhu
	 *	马上写入文件，阻塞式的。
	*/
	static int fiPrintImmediately( const char *filename, const char *fmt, ... );

	// 输出到服务器控制台的信息
	static int scPrint( const char *fmt,  ... );

	// 输出到调试信息
	static void dePrint( const char *info );

	// 输出到错误信息
	static void erPrint(const char *info);

	// 在输入的字符串的前面加上个时间标签，返回一个新的字符串地址
	static const char *infowithtime(const char *info);

	/*	[Oct 13, 2014] huhu
	 *	字符串中的空格都转换成下划线
	 */
	static const char *space2underline(const char *info);

	/*	[Nov 17, 2014] huhu
	 *	获得距现在为止到第二天零点有多少秒
	*/
	static unsigned long long getsecond();
// [Nov 17, 2014] huhu 一天有多少秒
#define secondoneday 86400
};
