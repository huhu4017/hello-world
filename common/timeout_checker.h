#pragma once
#include <string>
#include "log.h"

#ifdef _WINDOWS
	#include <time.h>
#else
	#include <sys/time.h>
#endif // _WINDOWS

class ctimeout_checker
{
public:
	ctimeout_checker(int interval_ms, const char *szlogfilename)
	{
		_interval = interval_ms;
		_strlogfilename = szlogfilename;
#ifdef _WINDOWS
		_start = 0;
		_end = 0;
		_start = timeGetTime();
#else
		_tend.tv_nsec = 0;
		_tend.tv_sec = 0;
		clock_gettime(CLOCK_REALTIME, &_tstart);
#endif
	}
	~ctimeout_checker()
	{
#ifdef _WINDOWS
	    if(_start + _interval < timeGetTime())
	    {
	    	cLog::fiPrint(_strlogfilename.c_str(), "%s处理超时 %I64u ms", _strlogfilename.c_str(), t1/10);
	    }
#else
		clock_gettime(CLOCK_REALTIME, &_tend);
		long long  t1, t2;
	    t1 =   _tstart.tv_sec*10000+_tstart.tv_nsec/100000;
	    t2 =  _tend.tv_sec*10000 + _tend.tv_nsec/100000;
	    t1 =  t2 - t1;
	    if(t1 > _interval*10)
	    {
	    	cLog::fiPrint(_strlogfilename.c_str(), "%s处理超时 %I64u ms", _strlogfilename.c_str(), t1/10);
	    }
#endif
	}

private:
	std::string _strlogfilename;
	// 单位毫秒
	int _interval;
#ifdef _WINDOWS
	unsigned int _start,_end;
#else
	timespec  _tstart, _tend;
#endif
};


