#pragma once
#include <stdlib.h>

// [8/8/2014 huhu] 单键继承基类
// 非线程安全的。如果多线程使用该单键，最好还是弄个多线程安全版的。
// 不过。。为了跨平台。。如果要线程安全，那就得多写点代码了。。

// ps:这个单键是在程序退出时会自动调用析构的，会回收内存。
// 虽然当程序退出时回不回收，系统都会释放进程内存，但是有还是要完整些，也许会遇到一些特殊的情况导致一些特殊的问题。

template < typename T > class Singleton
{
public:
	static T &sharedInstance()
	{
		Init();
		return *instance_;
	}
	static T *sharedPtr()
	{
		Init();
		return instance_;
	}

private:

	Singleton(const Singleton &other);
	Singleton &operator=(const Singleton &other);

	static T * instance_;
protected:
	static void Init()
	{
		if (instance_ == 0)
		{
			instance_ = new T;
			atexit(Destroy);    //程序结束时调用注册的函数
		}
	}

	static void Destroy()
	{
		delete instance_;
	}
	// 允许子类自己构建
	Singleton()
	{}
	virtual ~Singleton()
	{}
};

template < typename T >
T * Singleton < T >::instance_ = 0;
