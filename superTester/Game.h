#pragma once
#include "basehead.h"
#include "../common/asio_netModule.h"
#include "../common/Singleton.h"

class cGame : public cObject
{
public:
	cGame();
	~cGame()
	{}
	/*	[Dec 15, 2014] huhu
	 *	根据配置文件创建一个服务器实例
	 *	成功返回0，失败小于0
	*/
	int create(const char *configfile);

	/*	[Dec 15, 2014] huhu
	 *	获得最近一次循环花费的时间
	*/
	ev_tstamp getlapse()
	{
		return 0.;
	}

	/*	[Dec 15, 2014] huhu
	 *	主循环入口
	*/
	void update();
private:
	/*	[Dec 15, 2014] huhu
	 *	创建网络线程
	*/
	bool createNet(const char *host, const char *port);
private:
	ev_tstamp m_dlastloopLapse;
};

class cGameManager : public Singleton<cGameManager>, public cManager
{
private:
	cGameManager();
	~cGameManager();
	friend class Singleton<cGameManager>;
public:
	/*	[Dec 15, 2014] huhu
	 *	根据配置加载各个服务器
	*/
	bool LoadGames(const char *configfile);

	/*	[Dec 15, 2014] huhu
	 *	开始运行，阻塞。default loop
	*/
	bool run();
private:
};

#define gamemanager cGameManager::sharedPtr()

