
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <netinet/in.h>
#include <strings.h>
#include "ev.h"
#include <fcntl.h>
#include <time.h>
#include "../common/asio_netModule.h"

#define PORT 8333
#define BUFFER_SIZE 1024

struct ev_loop *defaultloop = ev_default_loop(0);

//gcc test.c -lm ev.o
void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
static void timeout_cb (EV_P_ ev_timer *w, int revents) ;
static void periodic_timeout_cb (EV_P_ ev_periodic *w, int event) ;

static ev_tstamp my_scheduler_cb (struct ev_periodic *w, ev_tstamp now)
{
	time_t t_now = time(0);
	tm *pnowtm = localtime(&t_now);
	ev_tstamp re = 30. - fmod (now, 30.);
	printf("my_scheduler_cb %f %u\n", re, pnowtm->tm_sec);
	return re + now;
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

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

class cGame
{
public:
	cGame()
	{}
	~cGame()
	{}
	/*	[Dec 15, 2014] huhu
	 *	根据配置文件创建一个服务器实例
	 *	成功返回0，失败小于0
	*/
	int create(const char *configfile)
	{
		return 0;
	}

	/*	[Dec 15, 2014] huhu
	 *	获得最近一次循环花费的时间
	*/
	ev_tstamp getlapse();

	/*	[Dec 15, 2014] huhu
	 *	主循环入口
	*/
	void update();
private:

	bool createNet(const char *host, const char *port);
private:
	ev_tstamp m_dlastloopLapse;
};

typedef void timeout_cb_def (EV_P_ ev_timer *, int);

int main()
{

	// asio 网络，连接网关

	// 注册主循环
	ev_timer mainlooptimer;
	mainlooptimer.data = (void*)new cGame();
	if(!mainlooptimer.data)
	{
		std::cout << "new cGame fail! exit..." << std::endl;
		return -1;
	}
	int icreatemaingame_ret = ((cGame*)mainlooptimer.data)->create("gameconfig.ini");
	if( icreatemaingame_ret != 0)
	{
		std::cout << getErrorMessage() << std::endl;
		return icreatemaingame_ret;
	}
	// 开始主循环
    ev_timer_init (&mainlooptimer, timeout_cb, 5.5, 0.);
    ev_timer_start (defaultloop, &mainlooptimer);

    while (1)
    {
        ev_loop(defaultloop, 0);
    }
    return 0;
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sd;

    struct ev_io *w_client = (struct ev_io*) malloc (sizeof(struct ev_io));
    if(EV_ERROR & revents)
    {
        printf("error event in accept\n");
        return;
    }

    client_sd = accept(watcher->fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_sd < 0)
    {
        printf("accept error\n");
        return;
    }
    printf("someone connected.\n");

    ev_io_init(w_client, read_cb, client_sd, EV_READ);
    ev_io_start(loop, w_client);
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    char buffer[BUFFER_SIZE];
    ssize_t read;

    if(EV_ERROR & revents)
    {
        printf("error event in read");
        return;
    }

    read = recv(watcher->fd, buffer, BUFFER_SIZE, 0);
    if(read < 0)
    {
        printf("read error,errno:%d\n", errno);
        return;
    }
    if(read == 0)
    {
        printf("someone disconnected.errno:%d\n", errno);
        ev_io_stop(loop,watcher);
        free(watcher);
        return;
    }
    else
    {
        printf("get the message:%s\n",buffer);
    }
    send(watcher->fd, buffer, read, 0);
    bzero(buffer, read);
}
static void timeout_cb (EV_P_ ev_timer *w, int revents)
{
    puts ("timeout");
    //ev_break (EV_A_ EVBREAK_ONE);
}

static void periodic_timeout_cb (EV_P_ ev_periodic *w, int event)
{
	const char *pchar = (const char*)w->data;
	time_t t_now = time(0);
	tm *pnowtm = localtime(&t_now);
	printf("periodic_timeout_cb %u\n", pnowtm->tm_sec);
    //ev_break (EV_A_ EVBREAK_ONE);
}
