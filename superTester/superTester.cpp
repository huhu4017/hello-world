
#include "basehead.h"
#include "Game.h"

// [Dec 15, 2014] huhu 保存某些地方抛出的错误信息
std::string errormessage;
/*	[Dec 15, 2014] huhu
 *
*/
const char *getErrorMessage()
{
	return errormessage.c_str();
}

/*	[Dec 15, 2014] huhu
 *	塞错误信息进来
*/
void pushErrorMessage(const char *szMessage, ...)
{
	if(!szMessage)
		return;

    static char szBuffer[1024] = {0};
	memset( szBuffer, 0, sizeof( szBuffer ) );

    va_list vl;
    va_start( vl, szMessage );
    vsnprintf(szBuffer , 1024, szMessage, vl );
    va_end(vl);

	errormessage = szBuffer;
}

obj_gid gen_gid(cObject *p)
{
	static char gamegid = 0;
	if(dynamic_cast<cGame*>(p))
	{
		return ++gamegid;
	}
	else
		return 0;
}

int main()
{

	cGame game;
	int ret = game.create("game1.ini");
	if(ret)
	{
		cout << "create fail! " << endl << getErrorMessage() << endl;
		return -1;
	}

	game.run();
	cout << "正常退出。" << endl;
    return 0;
}
