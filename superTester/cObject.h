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

	void setGid(obj_gid gid);
protected:
	virtual void onGidChange(obj_gid oldgid){;}
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

	cObject *getObj(obj_gid gid)
	{
		mapObj::iterator it = m_mapobjs.find(gid);
		if(it != m_mapobjs.end())
			return it->second;
		return NULL;
	}

	void removeall()
	{
		for(mapObj:: iterator it = m_mapobjs.begin(); it != m_mapobjs.end(); ++it)
		{
			SAFEDELETE(it->second);
		}
		m_mapobjs.clear();
	}
protected:
	mapObj m_mapobjs;
};

extern obj_gid gen_gid(cObject *p);

