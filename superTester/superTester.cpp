
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
void pushErrorMessage(const char *szMessage)
{
	if(!szMessage)
		return;
	errormessage = szMessage;
}

int main()
{
	bool ret = gamemanager->LoadGames("games.ini");
	if(!ret)
	{
		cout << getErrorMessage() << endl;
		return -1;
	}

	if(!gamemanager->run())
		cout << getErrorMessage() << endl;
	else
		cout << "正常退出。" << endl;
    return 0;
}
