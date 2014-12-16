/*
 * cObject.cpp
 *
 *  Created on: Dec 14, 2014
 *      Author: huhu
 */

#include "cObject.h"

cObject::cObject()
{
	// TODO Auto-generated constructor stub

}

cObject::~cObject()
{
	// TODO Auto-generated destructor stub
}

cManager::cManager()
{
	// TODO Auto-generated constructor stub

}

cManager::~cManager()
{
	// TODO Auto-generated destructor stub
}

bool cManager::addObj(cObject* pObj)
{
	if(!pObj)
	{
		pushErrorMessage("pointer pObj empty!");
		return false;
	}
	if(!pObj->getGid())
	{
		pushErrorMessage("obj gid == 0!");
		return false;
	}
	auto it = m_mapobjs.find(pObj->getGid());
	if(it != m_mapobjs.end())
	{
		pushErrorMessage("obj already in map, might gid repeated!");
		return false;
	}
	m_mapobjs[pObj->getGid()] = pObj;
	return true;
}
