
#pragma once
#include "../common/commonDef.h"
#include "../common/log.h"
#include <map>
#include <list>
#include <string>
#include <algorithm>
#include <ev.h>
#include <iostream>
#include <pthread.h>
#include <stdarg.h>

using namespace std;

// [Dec 15, 2014] huhu 错误信息提示，提示最好用英文（为了避免语言版本问题）。
extern const char *getErrorMessage();
extern void pushErrorMessage(const char *szMessage, ...);

#define obj_gid unsigned long long
