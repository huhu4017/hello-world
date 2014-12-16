/*
 * cObject.h
 *
 *  Created on: Dec 14, 2014
 *      Author: huhu
 */

#pragma once
#include "basehead.h"

class cObject
{
public:
	cObject();
	virtual ~cObject();

	obj_gid getGid()
	{
		return m_gid;
	}
private:
	obj_gid m_gid;
};

typedef map<obj_gid, cObject*> mapObj;

class cManager
{
public:
	cManager();
	virtual ~cManager();

	bool addObj(cObject*pObj);

	size_t getSize()
	{
		return m_mapobjs.size();
	}
private:
	mapObj m_mapobjs;
};

