#include "Game.h"
#include "../common/ConfigIniFile.h"
#include <sstream>
#include <fstream>

void cincb(EV_P_ struct ev_io *w, int revents)
{
	char szTemp[128] = "";
	size_t sizeread = read(w->fd, (void*)szTemp, sizeof(szTemp));
	cLog::scPrint("cincb read data : %s %u", szTemp, (unsigned int)sizeread);
	string str = szTemp;
	cGame *game = (cGame*)w->data;
	if(str.find("exit") != str.npos)
	{
		cLog::scPrint("start stoping...");
	    ev_io_stop(game->getLoop(), w);
		game->stopAndexit();
	}
	else if(str.find("redis ") != str.npos)
	{
		RedisElement re;
		string strcmd = str.substr(str.find("redis ")+strlen("redis "));
		if(!game->exe_redisReadCommand(&re, strcmd.c_str()))
		{
			game->exe_redisWriteCommand(&cGame::testcommandcb, game, strcmd.c_str());
		}
	}
}

cGame *g_pGame = NULL;

void netevent(int iconnectid, int ievent/*e_event*/, int iparam)
{
	if(g_pGame)
		g_pGame->netevent(iconnectid, ievent, iparam);
}

void *gamepthreadmain(void *p)
{
	cGame *pG = (cGame*)p;
	if(!pG)
		return NULL;

	pG->run();

	return NULL;
}

cGame::cGame()
{
	m_dlastloopLapse = 0.;
	m_tid = 0;
	memset(m_szConfig, 0, sizeof(m_szConfig));
	m_pConfigfileReader = NULL;
	m_loop = ev_default_loop(0);
	m_pLocalRedisContext = NULL;
	m_bExit = false;
	g_pGame = this;
	m_pConnectedFn = NULL;
	m_pDisConnectedFn = NULL;
}

cGame::~cGame()
{
	if(m_pConfigfileReader)
	{
		delete m_pConfigfileReader;
		m_pConfigfileReader = NULL;
	}
	if(m_pLocalRedisContext)
	{
	    redisFree(m_pLocalRedisContext);
	    m_pLocalRedisContext = NULL;
	}

	try
	{
	    ev_default_destroy();
	}
	catch (std::exception& e)
	{
		printf("%lu ev_default_destroy %s", pthread_self(), e.what());
	}
	g_pGame = NULL;
    if(m_tid)
    	pthread_join(m_tid, NULL);
}

void cGame::stopAndexit()
{
	if(m_pGateconnect)
		m_pGateconnect.reset();
	if(m_pLogdbconnect)
		m_pLogdbconnect.reset();
	if(m_pPlayerdatadbconnect)
		m_pPlayerdatadbconnect.reset();
	m_asio_net_module.stop();
	if(m_pConfigfileReader)
		m_pConfigfileReader->CloseFile();
	m_pAsyncRedis->stop();
	ev_unloop(m_loop, EVUNLOOP_ALL);
	m_bExit = true;
	cLog::scPrint("ev_unloop EVUNLOOP_ALL %lu", pthread_self());
}

int cGame::create(const char* configfile)
{
	if(!configfile)
	{
		pushErrorMessage("param in game create function is empty");
		return -1;
	}
	strcpy(m_szConfig, configfile);
	if(m_pConfigfileReader)
	{
		pushErrorMessage("game configreader not empty");
		return -2;
	}
	m_pConfigfileReader = new ConfigIniFile;
	if(m_pConfigfileReader->OpenFile(configfile) || !m_pConfigfileReader->IsOpen())
	{
		pushErrorMessage("game %s open fail", configfile);
		delete m_pConfigfileReader;
		m_pConfigfileReader = NULL;
		return -3;
	}
	char szTemp[128] = "";
	if(!m_pConfigfileReader->GetStringValue("Game", "ID", szTemp, sizeof(szTemp)))
	{
		pushErrorMessage("game ID not in config %s", configfile);
		delete m_pConfigfileReader;
		m_pConfigfileReader = NULL;
		return -4;
	}
	setGid(atoll(szTemp));

	// redis
	// first create local,then execute "config get slaveof" to get master ip and port
	if(!createReadRedis())
	{
		string strerror = getErrorMessage();
		pushErrorMessage("createReadRedis fail %s", strerror.c_str());
		return -5;
	}
	// connect to master
	if(!createWriteRedis())
	{
		string strerror = getErrorMessage();
		pushErrorMessage("createWriteRedis fail %s", strerror.c_str());
		return -6;
	}
	if(!createNet())
	{
		cLog::scPrint("createNet fail %s", getErrorMessage());
		return -7;
	}
	return 0;
}

void cGame::netUpdate()
{
	// disconnect check to reconnect
	if(m_pGateconnect)
		m_pGateconnect->check_reconnect(10);
	if(m_pLogdbconnect)
		m_pLogdbconnect->check_reconnect(10);
	if(m_pPlayerdatadbconnect)
		m_pPlayerdatadbconnect->check_reconnect(10);
	// get nei messages
	stMsg msg;
	while(m_asio_net_module.popMsg(&msg, false))
	{
		// [Nov 10, 2014] huhu means need to exit
		if(!msg.msg.size())
			break;
		connectAsclient *pconnectasclient = m_asio_net_module.getconnectAsclient(msg.iconnectid);
		if(pconnectasclient)
		{
			if(pconnectasclient->getconnectid() == m_pGateconnect->getconnectid())
				dispathGateMsg((void*)&(*msg.msg.begin()), msg.msg.size(), msg.iconnectid);
			else if(m_pLogdbconnect && pconnectasclient->getconnectid() == m_pLogdbconnect->getconnectid())
				dispathDBLogMsg((void*)&(*msg.msg.begin()), msg.msg.size(), msg.iconnectid);
			else if(m_pPlayerdatadbconnect && pconnectasclient->getconnectid() == m_pPlayerdatadbconnect->getconnectid())
				dispathDBPlayerDataMsg((void*)&(*msg.msg.begin()), msg.msg.size(), msg.iconnectid);
		}
	}
}

void cGame::dispathGateMsg(void* pMsg, size_t size, int connectid)
{
	cLog::scPrint("recved gate message : %08x connectid:%d", *(unsigned long long*)(pMsg), connectid);
}

void netMsgTimer_cb(EV_P_ struct ev_timer *w, int revents)
{
	if(!w || !w->data)
	{
		cLog::scPrint("message loop had an empty watcher");
		return;
	}
	cGame *pGame = (cGame*)w->data;
	pGame->netUpdate();
	cLog::scPrint("netMsgTimer_cb %lu", pthread_self());
}

void mainloopTimer_cb(EV_P_ struct ev_timer *w, int revents)
{
	if(!w || !w->data)
	{
		cLog::scPrint("message loop had an empty watcher");
		return;
	}
	cGame *pGame = (cGame*)w->data;
	pGame->update();
	cLog::scPrint("mainloopTimer_cb %lu", pthread_self());
}

ev_timer exittimer;
void cGame::run()
{
	ev_io cinio;
	cinio.data = (void*)this;
	ev_io_init(&cinio, cincb, STDIN_FILENO, EV_READ);
	ev_io_start(m_loop, &cinio);

	if(!m_pConfigfileReader)
	{
		cLog::scPrint("m_pConfigfileReader is empty");
		return;
	}
	// 0 阻塞等待，直到调用unloop或者所有的watcher都退出了。
	//#define EVLOOP_NONBLOCK EVRUN_NOWAIT	非阻塞式，检查一遍以后就退出
	//#define EVLOOP_ONESHOT  EVRUN_ONCE			它会阻塞直到有一个新的事件触发，并且会在loop完成一次迭代后返回
	m_netmsgpoptimer.data = (void*)this;
	// 10毫秒一次的循环
	ev_timer_init(&m_netmsgpoptimer, netMsgTimer_cb, 10, 10.);
	ev_timer_start(m_loop, &m_netmsgpoptimer);

	m_mainlooptimer.data = (void*)this;
	// 20毫秒一次的循环
	ev_timer_init(&m_mainlooptimer, mainloopTimer_cb, 20, 20.);
	ev_timer_start(m_loop, &m_mainlooptimer);

	ev_set_timeout_collect_interval(m_loop,1);
	ev_loop(m_loop, 0);
//	timespec  _tstart, _tend;
//	while(!m_bExit)
//	{
//		clock_gettime(CLOCK_REALTIME, &_tstart);
//		ev_loop(m_loop, EVLOOP_ONESHOT);
//		clock_gettime(CLOCK_REALTIME, &_tend);
//		long long  t1, t2;
//	    t1 =   _tstart.tv_sec*10000+_tstart.tv_nsec/100000;
//	    t2 =  _tend.tv_sec*10000 + _tend.tv_nsec/100000;
//	    t1 =  (t2 - t1)*10;
//	    if(t1 < 100)
//	    	usleep(100-t1);
//	}

	std::cout << m_szConfig << " Game loop out !" << std::endl;
}

bool cGame::createNet()
{
	if(!m_pConfigfileReader)
	{
		pushErrorMessage("m_pConfigfileReader is empty");
		return false;
	}
	char szIp[64] = "";
	int iport = 0;
	// 连网关
	if(!m_pConfigfileReader->GetStringValue("gate", "logip", szIp, sizeof(szIp)))
	{
		pushErrorMessage("config get [gate] logip field error");
		return false;
	}
	if(!m_pConfigfileReader->GetIntValue("gate", "logport", iport))
	{
		pushErrorMessage("config get [gate] logip field error");
		return false;
	}
	char szName[64] = "";
	sprintf(szName, "%s_gate", m_szConfig);
	m_pGateconnect = m_asio_net_module.addclient(szName, szIp, iport);

	// dblog
	if(m_pConfigfileReader->GetStringValue("dblog", "logip", szIp, sizeof(szIp)) && m_pConfigfileReader->GetIntValue("dblog", "logport", iport))
	{
		sprintf(szName, "%s_dblog", m_szConfig);
		m_pLogdbconnect = m_asio_net_module.addclient(szName, szIp, iport);
	}

	// playerdatadb
	if(m_pConfigfileReader->GetStringValue("dbplayerdata", "logip", szIp, sizeof(szIp)) && m_pConfigfileReader->GetIntValue("dbplayerdata", "logport", iport))
	{
		sprintf(szName, "%s_dbplayerdata", m_szConfig);
		m_pPlayerdatadbconnect = m_asio_net_module.addclient(szName, szIp, iport);
	}
	// run
	m_asio_net_module.run();
	return true;
}

void cGame::onAsyncRedisConnected(const redisAsyncContext* ac, int status)
{
	std::string err = (ac && ac->errstr) ? ac->errstr : (status!=REDIS_OK ? "REDIS_ERR" : "REDIS_OK");
	cLog::scPrint("[%s] AsyncRedisConnected %s", g_pGame->getConfigName(), err.c_str());
	g_pGame->exe_redisWriteCommand(&cGame::testcommandcb, g_pGame, "set abc cba");
}

void cGame::onAsyncRedisDisConnected(const redisAsyncContext* ac, int status)
{
	std::string err = (ac && ac->errstr) ? ac->errstr : (status!=REDIS_OK ? "REDIS_ERR" : "REDIS_OK");
	cLog::scPrint("[%s] AsyncRedisDisConnected %s", g_pGame->getConfigName(), err.c_str());
}

void cGame::netevent(int iconnectid, int ievent, int iparam)
{

	if(ievent == e_event_connected)
	{
		connectAsclient *pclient = m_asio_net_module.getconnectAsclient(iconnectid);
		if(pclient)
		{
			string strname = pclient->getname();
			if(strname.find("_gate") != strname.npos)
			{
#pragma pack(push)
#pragma pack( 1 )
				struct Q_stGameRegisterToGate
				{
					unsigned char _serverProtocol;
					unsigned char _gameProtocol;
				    unsigned int flag;
					char szServerName[64];
				};
				Q_stGameRegisterToGate regMsg;
				regMsg._serverProtocol  = 2;
				regMsg._gameProtocol  = 1;
				regMsg.flag  = 1;
				strcpy( regMsg.szServerName, "game 1 line");
				pclient->sendMsg( (const char *)&regMsg, sizeof(Q_stGameRegisterToGate)+2, "test" );

#pragma pack(pop)
			}
		}
	}
}

void cGame::testcommandcb(RedisElement* pRe)
{
	cLog::scPrint("test command cb!");
}

bool cGame::createReadRedis()
{
	if(m_pLocalRedisContext)
	{
		pushErrorMessage("m_pLocalRedisContext not empty, might be had connection");
		return false;
	}
	if(!m_pConfigfileReader)
	{
		pushErrorMessage("m_pConfigfileReader empty, can not take config");
		return false;
	}
	char szTemp[128] = "";
	if(!m_pConfigfileReader->GetStringValue("localredis", "host", szTemp, sizeof(szTemp)))
	{
		sprintf(szTemp, "127.0.0.1");
		cLog::scPrint("can not take localredis host in config,set host default 127.0.0.1");
	}
	int port = 0;
	if(!m_pConfigfileReader->GetIntValue("localredis", "port", port))
	{
		port = 6379;
		cLog::scPrint("can not take localredis port in config,set port default 6379");
	}
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    m_pLocalRedisContext = redisConnectWithTimeout(szTemp, port, timeout);

    if (m_pLocalRedisContext == NULL || m_pLocalRedisContext->err)
    {
        if (m_pLocalRedisContext)
        {
        	pushErrorMessage("Connection error: %s\n", m_pLocalRedisContext->errstr);
            redisFree(m_pLocalRedisContext);
            m_pLocalRedisContext = NULL;
        }
        else
        {
        	pushErrorMessage("Connection error: can't allocate redis context\n");
        }
        return false;
    }
    else
		cLog::scPrint("connect local redis %s:%d ok!", szTemp, port);
	return true;
}

bool cGame::createWriteRedis()
{
	if(!m_pConfigfileReader)
	{
		pushErrorMessage("m_pConfigfileReader empty, can not take config");
		return false;
	}
	if(!m_pLocalRedisContext)
	{
		pushErrorMessage("m_pLocalRedisContext is empty, cant get master");
		return false;
	}
	// get master by local slave
	string strIp, strport;
	{
		rediscommander rc(m_pLocalRedisContext, "CONFIG GET slaveof");
		if(rc.iserror())
		{
			pushErrorMessage("rc.iserror() %s %s", "CONFIG GET slaveof", getRediserrstr(m_pLocalRedisContext));
			return false;
		}
		if(rc.getReply()->elements != 2)
		{
			char szTemp[128] = "";
			if(!m_pConfigfileReader->GetStringValue("localredis", "host", szTemp, sizeof(szTemp)))
			{
				sprintf(szTemp, "127.0.0.1");
				cLog::scPrint("can not take localredis host in config,set host default 127.0.0.1");
			}
			strIp = szTemp;
			if(!m_pConfigfileReader->GetStringValue("localredis", "port", szTemp, sizeof(szTemp)))
			{
				sprintf(szTemp, "6379");
				cLog::scPrint("can not take localredis port in config,set port default 6379");
			}
			strport = szTemp;
			cLog::scPrint("%s can not get slaveof,use local for master", "CONFIG GET slaveof");
		}
		else
		{
			string strRet = rc.getReply()->element[1]->str;
			//strRet = strRet.substr(strRet.find('\n')+1);
			int ipos = strRet.find(' ');
			strIp = strRet.substr(0, ipos);
			strport = strRet.substr(ipos+1);
		}
	}
	// 连接
	m_pConnectedFn = cGame::onAsyncRedisConnected;
	m_pDisConnectedFn = cGame::onAsyncRedisDisConnected;
	m_pAsyncRedis->start(strIp, atoi(strport.c_str()), m_pConnectedFn, m_pDisConnectedFn, ev_default_loop (0));
	return true;
}

void cGame::startrun()
{
	// 创建主线程
	//pthread_mutex_init(&m_mutex_list, NULL);
	//pthread_cond_init(&m_cv, NULL);

    if( 0 != pthread_create( &m_tid, NULL, gamepthreadmain, (void*)this ) )
    {
    	cLog::scPrint("创建%s线程失败！", m_szConfig);
        return;
    }

}

void cGame::update()
{
}

void cGame::dispathDBLogMsg(void* pMsg, size_t size, int connectid)
{
}

void cGame::dispathDBPlayerDataMsg(void* pMsg, size_t size, int connectid)
{
}

bool cGame::exe_redisReadCommand(RedisElement *pE, const char* cmd, ...)
{
	if(!m_pLocalRedisContext)
	{
		pushErrorMessage("m_pLocalRedisContext is empty");
		return false;
	}
	if(!pE)
	{
		pushErrorMessage("pE is empty");
		return false;
	}
    static char szBuffer[1024] = {0};
    va_list vl;
    va_start( vl, cmd );
    vsnprintf(szBuffer , 1024, cmd, vl );
    va_end(vl);
    rediscommander rc(m_pLocalRedisContext, szBuffer);
    if(rc.iserror())
    {
    	pushErrorMessage("%s executed with an error : %s", szBuffer, m_pLocalRedisContext->errstr);
    	return false;
    }
    pE->init(m_pLocalRedisContext, rc.getReply());
	return true;
}
