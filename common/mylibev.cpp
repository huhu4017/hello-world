#include "mylibev.h"
#include "log.h"

static void timeout_cb (EV_P_ ev_timer *w, int revents)
{
	if(!w->data)
	{
		cLog::fiPrint("evtimerlog.txt", "有timer触发但是没找到对应的cevTimer");
		return;
	}
	cevTimer *pTimer = (cevTimer*)(w->data);
	pTimer->invoke(revents);
}

static void periodic_cb (EV_P_ ev_periodic *w, int revents)
{
	if(!w->data)
	{
		cLog::fiPrint("evtimerlog.txt", "有timer触发但是没找到对应的cevTimer");
		return;
	}
	cevPeriodic *pPeriodic = (cevPeriodic*)(w->data);
	pPeriodic->invoke(revents);
}


cevTimer::cevTimer(unsigned long long callerid, const char *szname)
: cevWatcher(callerid, szname)
{
}

cevTimer::~cevTimer()
{
}

bool cevTimer::create(void* buffer, int after, int repeat, cbf cb)
{
	cevWatcherMgr *pMgr = (cevWatcherMgr*)buffer;
	timeout_watcher.data = this;
	m_cb = cb;
    ev_timer_init (&timeout_watcher, timeout_cb, after, repeat);
    ev_timer_start (pMgr->getloop(), &timeout_watcher);
	return true;
}

bool cevTimer::setTime(int after, int repeat)
{
	ev_timer_set(&timeout_watcher, after, repeat);
	return true;
}

void cevTimer::stop(cevWatcherMgr *pMgr)
{
	if(!pMgr || !pMgr->getloop())
		return;
	ev_timer_stop(pMgr->getloop(), &timeout_watcher);
}

void cevTimer::again(cevWatcherMgr *pMgr)
{
	if(!pMgr || !pMgr->getloop())
		return;
	ev_timer_again(pMgr->getloop(), &timeout_watcher);
}

cevWatcherMgr::cevWatcherMgr()
{
	loop = ev_default_loop(0);
}
cevWatcherMgr::~cevWatcherMgr()
{
	for(mapWatcher::iterator it = m_mapWatchers.begin(); it != m_mapWatchers.end(); ++it)
	{
		if(it->second)
		{
			delete it->second;
		}
	}
	m_mapWatchers.clear();
}

bool cevWatcherMgr::create(void* buffer, long size)
{
	return true;
}

void cevWatcherMgr::update(void)
{
    ev_loop(loop, EVLOOP_NONBLOCK);
}

bool cevWatcherMgr::addWatcher(cevWatcher *pnewWatcher)
{
	mapWatcher::iterator it = m_mapWatchers.find(pnewWatcher->getName());
	if(it != m_mapWatchers.end())
	{
		return false;
	}
	m_mapWatchers[pnewWatcher->getName()] = pnewWatcher;
	return true;
}

cevWatcher *cevWatcherMgr::getWatcher(std::string strWatcher)
{
	mapWatcher::iterator it = m_mapWatchers.find(strWatcher);
	if(it == m_mapWatchers.end())
	{
		return NULL;
	}
	return it->second;
}

bool cevWatcherMgr::deleteWatcher(std::string strWatcher)
{
	mapWatcher::iterator it = m_mapWatchers.find(strWatcher);
	if(it == m_mapWatchers.end())
	{
		return false;
	}
	if(it->second)
	{
		delete it->second;
	}
	m_mapWatchers.erase(it);
	return true;
}

cevPeriodic::cevPeriodic(unsigned long long callerid, const char* szname)
: cevWatcher(callerid, szname)
{
}

cevPeriodic::~cevPeriodic()
{
}

bool cevPeriodic::create(void* buffer, int after, int repeat, cbf cb, cbperiodic cb_resc/* = 0*/)
{
	cevWatcherMgr *pMgr = (cevWatcherMgr*)buffer;
	periodic_watcher.data = this;
	m_cb = cb;
    ev_periodic_init (&periodic_watcher, periodic_cb, after, repeat, cb_resc);
    ev_periodic_start (pMgr->getloop(), &periodic_watcher);
	return true;
}

bool cevPeriodic::setTime(int after, int repeat, cbperiodic cb_resc/* = 0*/)
{
	ev_periodic_set(&periodic_watcher, after, repeat, cb_resc);
	return true;
}

void cevPeriodic::stop(cevWatcherMgr* pMgr)
{
	ev_periodic_stop(pMgr->getloop(), &periodic_watcher);
}

void cevPeriodic::again(cevWatcherMgr* pMgr)
{
	ev_periodic_again(pMgr->getloop(), &periodic_watcher);
}
