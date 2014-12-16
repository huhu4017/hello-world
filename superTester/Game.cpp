#include "Game.h"
#include "../common/ConfigIniFile.h"

cGame::cGame()
{
	m_dlastloopLapse = 0.;
}

int cGame::create(const char* configfile)
{
	return 0;
}

void cGame::update()
{
	std::cout << "update" << std::endl;
}

bool cGame::createNet(const char* host, const char* port)
{
	return true;
}

cGameManager::cGameManager()
{
}

cGameManager::~cGameManager()
{
}

bool cGameManager::LoadGames(const char* configfile)
{
	if(!configfile)
	{
		pushErrorMessage("pointer configfile is empty!");
		return false;
	}

	ifstream stream(configfile);
	if (!stream.is_open())
		return false;

	char lineBuffer[1024];

	// 跳过表头
	stream.getline(lineBuffer, sizeof(lineBuffer));


	// 读护体数据
	while (!stream.eof() )
	{
		stream.getline( lineBuffer, sizeof( lineBuffer ) );
		if ( *lineBuffer == 0 || *( unsigned short )lineBuffer == '//' || *( unsigned short )lineBuffer == '--' || stream.eof() )
			continue;

		std::strstream lineStream( lineBuffer, ( int )strlen( lineBuffer ), ios_base::in );

		string strconfig;
		lineStream  >> strconfig;
		if(strconfig.find(".ini") == strconfig.npos)
			continue;
		cGame *g = new cGame;
		if(!g->create(strconfig.c_str()))
		{
			cLog::scPrint("游戏对象创建失败 %s %s", getErrorMessage(), strconfig.c_str());
			delete g;
			continue;
		}
		addObj(g);
	}

	stream.close();

	return true;
}

bool cGameManager::run()
{
	if(!getSize)
	{
		pushErrorMessage("game map is empty");
		return false;
	}
	for(auto it : m_mapobjs)
	{
		if(it.second)
			((cGame*)(it.second))->run();
	}
	return true;
}
