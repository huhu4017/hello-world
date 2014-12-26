#pragma once
#include "basehead.h"
#include "../common/asio_netModule.h"
#include "../common/Singleton.h"
#include "cObject.h"
#include "../common/async_hiredis.h"

class cGame  : public cObject
{
public:
	cGame();
	~cGame();
public:
	/*	[Dec 15, 2014] huhu
	 *	create Game by the configfile
	 *	it will save the configfile
	 *	other component will create in function run,after main loop thread.
	 *	if success,return 0,else return minus
	*/
	int create(const char *configfile);

	/*	[Dec 15, 2014] huhu
	 *	get last time the loop lapse
	*/
	ev_tstamp getlapse()
	{
		return 0.;
	}

	/*	[Dec 15, 2014] huhu
	 *	start thread and create some Component like tcp net,redis connections
	*/
	void startrun();

	/*	[Dec 15, 2014] huhu
	 *
	*/
	void update();

	/*	[Dec 15, 2014] huhu
	 *	main loop enter
	*/
	void run();

	/*	[Dec 17, 2014] huhu
	 *	process gate messages
	*/
	void dispathGateMsg(void *pMsg, size_t size, int connectid);

	/*	[Dec 17, 2014] huhu
	 *	process log server messages
	*/
	void dispathDBLogMsg(void *pMsg, size_t size, int connectid);

	/*	[Dec 17, 2014] huhu
	 *	process dbplayerdata messages
	*/
	void dispathDBPlayerDataMsg(void *pMsg, size_t size, int connectid);

	/*	[Dec 24, 2014] huhu
	 *	set exit Flag
	 *	checked in function update
	*/
	void setExitFlag(bool b)
	{
		m_bExit = b;
	}
	/*	[Dec 17, 2014] huhu
	*/
	void stopAndexit();

	/*	[Dec 17, 2014] huhu
	 *	net message process loop
	*/
	void netUpdate();

	/*	[Dec 17, 2014] huhu
	 *	get ev loop in this game instance
	*/
	struct ev_loop *getLoop()
	{
		return m_loop;
	}

	/*	[Dec 24, 2014] huhu
	 *	callback of async redis connected
	*/
	static void onAsyncRedisConnected(const redisAsyncContext *ac, int status);

	/*	[Dec 24, 2014] huhu
	 *	callback of async redis disconnected
	*/
	static void onAsyncRedisDisConnected(const redisAsyncContext *ac, int status);

	/*	[Dec 24, 2014] huhu
	 *	net lib some event process
	*/
	void netevent(int iconnectid, int ievent/*e_event*/, int iparam);

	/*	[Dec 25, 2014] root
	 *	handler can be NULL,means not need callback,same time t will be NULL too.
	 *	this just run write data command,because of it connected remote redis server.
	 *	if need to read,recommend exe_redisreadCommand function.
	*/
	template<typename cb, class ThisType>
	bool exe_redisWriteCommand(cb handler, ThisType *t, const char *cmd, ...)
	{
		if(!m_pAsyncRedis)
		{
			pushErrorMessage("m_pAsyncRedis is empty!");
			return false;
		}
	    static char szBuffer[1024] = {0};
        va_list vl;
        va_start( vl, cmd );
        vsnprintf(szBuffer , 1024, cmd, vl );
        va_end(vl);
		m_pAsyncRedis->execAsyncCommand(handler, t, szBuffer);
		return true;
	}

	bool exe_redisWriteCommand(const char *cmd, ...)
	{
		if(!m_pAsyncRedis)
		{
			pushErrorMessage("m_pAsyncRedis is empty!");
			return false;
		}
	    static char szBuffer[1024] = {0};
        va_list vl;
        va_start( vl, cmd );
        vsnprintf(szBuffer , 1024, cmd, vl );
        va_end(vl);
		m_pAsyncRedis->execAsyncCommand(szBuffer);
		return true;
	}

	/*	[Dec 25, 2014] root
	 *	execute redis read command with connection from local redis server.
	 *	local redis server is read only.
	 *	this function is blocked on execute command,until server answered.
	*/
	bool exe_redisReadCommand(RedisElement *pE, const char *cmd, ...);

	/*	[Dec 25, 2014] huhu
	 *	test command cb function
	*/
	void testcommandcb(RedisElement *pRe);

	/*	[Dec 26, 2014] huhu
	 *	get the server's configuration file name
	*/
	const char *getConfigName()
	{
		return m_szConfig;
	}
private:
	/*	[Dec 15, 2014] huhu
	 *	create tcp
	*/
	bool createNet();

	/*	[Dec 23, 2014] huhu
	 *	create remote connection with redis server,most use for write commands
	 *	async operation
	*/
	bool createWriteRedis();

	/*	[Dec 23, 2014] huhu
	 *	create local connection with redis server
	 *	use for read
	*/
	bool createReadRedis();
private:
	// [Dec 17, 2014] huhu last lopp lapse
	ev_tstamp m_dlastloopLapse;
	// [Dec 17, 2014] huhu game main thread id
    pthread_t m_tid;
    // [Dec 17, 2014] huhu configurate file name
    char m_szConfig[32];
    // [Dec 17, 2014] huhu config reader instance
    class ConfigIniFile *m_pConfigfileReader;
    // connection pointer with gate server
    connectasclient_ptr m_pGateconnect;
    // connection pointer with log db server
    connectasclient_ptr m_pLogdbconnect;
    // connection pointer with player db server
    connectasclient_ptr m_pPlayerdatadbconnect;
    //
    struct ev_loop *m_loop;
    // timer for get net message
	ev_timer m_netmsgpoptimer;
	// game main logic loop timer
	ev_timer m_mainlooptimer;
	// [Dec 23, 2014] huhu local redis connection
	redisContext *m_pLocalRedisContext;
	// [Dec 24, 2014] huhu is need to exit from ev_loop
	bool m_bExit;

	// [Dec 26, 2014] huhu network module instance
	mul_asio_netModule m_asio_net_module;
	// redis connection callback function
	RedisConnectionAsync::connectfn m_pConnectedFn, m_pDisConnectedFn;
};

