#include "commonDef.h"
#include "log.h"
#include <string.h>
#include <stdio.h>
#include <fstream>

#include <iostream>
#include <stdarg.h>
#include <time.h>

using namespace std;
int g_logswitch = 0;

cLog::cLog( void )
{
}

cLog::cLog( const char *logFile)
{

}

cLog::~cLog( void )
{

}

int  cLog::fiPrintImmediately( const char *filename, const char *fmt, ... )
{
    int ret = 0;
	if( fmt == NULL )
		return ret;

    static char szBuffer[1024] = {0};
	memset( szBuffer, 0, sizeof( szBuffer ) );

    va_list vl;
    va_start( vl, fmt );
    ret = vsnprintf(szBuffer , 1024, fmt, vl );
    va_end(vl);

   std::fstream file;
	file.open( filename, std::ios::app | std::ios::out );
	if( file.fail() )
        return ret;

    file << infowithtime(szBuffer) << "\n";

    file.close();
//    log_interface->fiPrint(infowithtime(szBuffer), filename);

    return ret;
}

int  cLog::fiPrint( const char *filename, const char *fmt, ... )
{
    int ret = 0;
	if( fmt == NULL )
		return ret;

    static char szBuffer[1024] = {0};
	memset( szBuffer, 0, sizeof( szBuffer ) );

    va_list vl;
    va_start( vl, fmt );
    ret = vsnprintf(szBuffer , 1024, fmt, vl );
    va_end(vl);

//   std::fstream file;
//	file.open( filename, std::ios::app | std::ios::out );
//	if( file.fail() )
//        return ret;
//
//    file << infowithtime(szBuffer) << "\n";
//
//    file.close();
    log_interface->fiPrint(infowithtime(szBuffer), filename);

    return ret;
}

int cLog::scPrint(  const char *fmt, ... )
{

    int ret = 0;
	if( fmt == NULL )
		return ret ;

    static char szBuffer[1024] = {0};
	memset( szBuffer, 0, sizeof( szBuffer ) );

    va_list vl;
    va_start( vl, fmt );
    ret = vsnprintf(szBuffer , 1024, fmt, vl );
    va_end(vl);

	log_interface->scPrint(infowithtime(szBuffer));
    return ret ;

	/*strcpy( szBuffer, info );
	int n = (int)strlen( szBuffer );
	szBuffer[n] = '\r';
	szBuffer[n+1] = '\n';
	szBuffer[n+2] = '\0';*/

  //  printf("%s",szBuffer);
	//void AddInfo(LPCSTR Info);

	//AddInfo( szBuffer );
}

const char* cLog::stringmaker(const char* info, ...)
{
	static char szTemp[1024] = "";

	va_list args;
	va_start(args, info);
	vsprintf(szTemp, info, args);
	va_end(args);

	return szTemp;
}

void cLog::dePrint( const char *info )
{
	if( info == NULL )
		return;

	//OutputDebugString( info );
	log_interface->dePrint(infowithtime(info));
}

void cLog::erPrint(const char* info)
{
	if( info == NULL )
		return;

	//OutputDebugString( info );
	log_interface->erPrint(infowithtime(info));
}

const char* cLog::infowithtime(const char* info)
{
	time_t t = time(0);
	char tmp[128];
	strftime( tmp, sizeof(tmp), "%m/%d %X",localtime(&t) );
	return stringmaker("[%s]%s", tmp, info);
}

const char* cLog::space2underline(const char* info)
{
	static char szTemp[1024] = "";
	size_t infosize = strlen(info)+1;
	for(size_t i = 0; i < 1024 && i < infosize; ++i)
	{
		szTemp[i] = info[i] == ' ' ? '_' : info[i];
	}
	return szTemp;
}

unsigned long long cLog::getsecond()
{
	time_t t_now = time(0);
	tm *ptm_now = localtime(&t_now);
	if(!ptm_now)
		return (unsigned long long)-1;
	tm tmnow = *ptm_now;
	tmnow.tm_hour = 23;
	tmnow.tm_min = 59;
	tmnow.tm_sec = 59;
	time_t dayout = mktime(&tmnow);

	return dayout - t_now + 1;
}

#ifdef Log_no
bool cLogno::init()
{
	return true;
}

void cLogno::fiPrint(const char* info, const char* filename)
{
	cout << info << "\n";
}

void cLogno::scPrint(const char* info)
{
	cout << info << "\n";
}

void cLogno::dePrint(const char* info)
{
	cout << info << "\n";
}

void cLogno::erPrint(const char* info)
{
	cout << info << "\n";
}

cLogno::cLogno()
{
}

cLogno::~cLogno()
{
}
#endif

#ifdef Log_Thread_Local
void *cThreadLog::update(void *args)
{
	cThreadLog *pthis = (cThreadLog*)args;
	while (true)
	{
		// 等待条件变量。
		{
			pthread_mutex_lock(&pthis->m_mutex_list);
			//cout << "准备进入等待 " << pthis->m_mutex_list.__align << endl;
			if(!pthis->m_listConsoleinfo.size() && !pthis->m_listfileinfo.size())
				pthread_cond_wait(&pthis->m_cv, &pthis->m_mutex_list);
			//cout << "退出等待 " << pthis->m_mutex_list.__align << endl;
			pthread_mutex_unlock(&pthis->m_mutex_list);
			//cout << "退出等待后解锁 " << pthis->m_mutex_list.__align << endl;
		}
		if(!pthis->processlog())
			break;
	}
	cout << "cThreadLog thread exit" << endl;
	return NULL;
}

cThreadLog::cThreadLog()
{
	pthread_mutex_init(&m_mutex_list, NULL);
	pthread_cond_init(&m_cv, NULL);

    if( 0 != pthread_create( &m_t, NULL, update, (void*)this ) )
    {
    	cLog::scPrint("创建cThreadLog线程失败！");
        return;
    }
}

bool cThreadLog::processlog()
{
	bool bexit = false;
	//cout << "处理log准备加锁 " << m_mutex_list.__align << endl;
	pthread_mutex_lock(&m_mutex_list);
	stringlist tempconsolelist = m_listConsoleinfo;
	m_listConsoleinfo.clear();
	pthread_mutex_unlock(&m_mutex_list);
	//cout << "拷贝完成解锁 " << m_mutex_list.__align << endl;
	for (stringlist::iterator it = tempconsolelist.begin(); it != tempconsolelist.end(); ++it)
	{
		if(strcmp("exit", it->c_str()) == 0)
		{
			bexit = true;
			continue;
		}
		std::cout << it->c_str() << "\n";
	}

	pthread_mutex_lock(&m_mutex_list);
	stringpairlist tempfilelist = m_listfileinfo;
	m_listfileinfo.clear();
	pthread_mutex_unlock(&m_mutex_list);
	std::ofstream file;
	string strlastfilename;
	for (stringpairlist::iterator itfile = tempfilelist.begin(); itfile != tempfilelist.end(); ++itfile)
	{
		if (itfile->second.empty())
			continue;
		if (strlastfilename != itfile->first)
		{
			file.close();
			file.open(itfile->first.c_str(), ios_base::app | ios_base::out);
			if (!file.is_open())
				continue;
			strlastfilename = itfile->first;
		}
		file << itfile->second.c_str() << '\n';
	}

	 file.flush();
	//if (!file.fail())
		file.close();
	return !bexit;
}


cThreadLog::~cThreadLog()
{
	pushlog("exit", log_console);
	pthread_join(m_t, NULL);
	pthread_mutex_destroy(&m_mutex_list);
	pthread_cond_destroy(&m_cv);
}

void cThreadLog::fiPrint(const char *info, const char *filename /*= "Log.txt" */)
{
	pushlog(info, log_file, filename);
}

void cThreadLog::scPrint(const char *info)
{
	pushlog(info, log_console);
	//std::cout << info << std::endl;
}

void cThreadLog::dePrint(const char *info)
{
	pushlog(info, log_console);
	pushlog(info, log_file, "debug");
}

void cThreadLog::erPrint(const char *info)
{
	pushlog(info, log_console);
	pushlog(info, log_file, "error.txt");
}

void cThreadLog::pushlog(const char *info, int type, const char *filename /* = "log.txt" */)
{
	if (type == log_console)
	{
		//cout << "在push中准备加锁 " << m_mutex_list.__align << endl;
		pthread_mutex_lock(&m_mutex_list);
		m_listConsoleinfo.push_back(info);
		pthread_mutex_unlock(&m_mutex_list);
		//cout << "在push中发送信号 " << m_mutex_list.__align << endl;
		pthread_cond_signal(&m_cv);
		//cout << "在push中发送信号后 " << m_mutex_list.__align << endl;
	}
	else if (type == log_file)
	{
		pthread_mutex_lock(&m_mutex_list);
		m_listfileinfo.push_back(pair<std::string, std::string>(filename, info));
		pthread_mutex_unlock(&m_mutex_list);
		pthread_cond_signal(&m_cv);
	}
}

bool cThreadLog::init()
{
	return true;
}

#endif

#ifdef Log_redis
#include "ConfigIniFile.h"

cRedisLog::cRedisLog()
{
	m_pRedisContext = NULL;
	_reply = NULL;
}

cRedisLog::~cRedisLog()
{
	if(_reply)
		freeReplyObject(_reply);
	if(m_pRedisContext)
		redisFree(m_pRedisContext);
    m_pRedisContext = NULL;
	_reply = NULL;
}

bool cRedisLog::init()
{
	// 连接redis服务器
	if(m_pRedisContext)
	{
		redisFree(m_pRedisContext);
		m_pRedisContext = NULL;
	}
	int iport = 6378;
	char szIp[32] = "127.0.0.1";
	ConfigIniFile gameini;
	if( gameini.OpenFile("gGame.ini") == 0 )
	{
		gameini.GetStringValue("redis", "logip", szIp, sizeof(szIp));
		gameini.GetIntValue("redis", "logport", iport);
	}
	m_pRedisContext = redisConnect(szIp, iport);
	if(!m_pRedisContext || m_pRedisContext->err)
	{
		if(m_pRedisContext)
		{
			cLog::scPrint("connect[%s][%d] error [%s]", szIp, iport, m_pRedisContext->errstr);
		}
		else
		{
			cLog::scPrint("connect[%s][%d] error [m_pRedisContext == NULL]", szIp, iport);
		}
		redisFree(m_pRedisContext);
	}

	return true;
}

void cRedisLog::fiPrint(const char* info, const char* filename)
{
	if(!m_pRedisContext)
		return;
	string strtempfilename = cLog::space2underline(filename);
	_reply = (redisReply*)redisCommand(m_pRedisContext, "set %s:%s log", strtempfilename.c_str(), cLog::space2underline(info));
	if(_reply)
	{
		freeReplyObject(_reply);
		_reply = NULL;
	}
}

void cRedisLog::scPrint(const char* info)
{
	cout << info << "\n";

	if(!m_pRedisContext)
		return;

	fiPrint(info, "scPrint.txt");
}

void cRedisLog::dePrint(const char* info)
{
	if(!m_pRedisContext)
		return;
	fiPrint(info, "debug.txt");
}

void cRedisLog::erPrint(const char* info)
{
	if(!m_pRedisContext)
		return;
	fiPrint(info, "error.txt");
}

#endif
