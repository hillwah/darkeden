//--------------------------------------------------------------------------------
// 
// Filename    : ResourceManager.h 
// Written By  : Reiot
// 
//--------------------------------------------------------------------------------

#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

// include files
#include "Resource.h"
#include "Assert1.h"
#include <list>

const uint maxResources = 1024;

//--------------------------------------------------------------------------------
//
// class ResourceManager
//
//--------------------------------------------------------------------------------

class ResourceManager{

public :

	// constructor
	ResourceManager () throw();

	// destructor
	~ResourceManager () throw();
	

public :

	// load from resource file
	void load (const string & filename) throw(Error);

	// save to resource file
	void save (const string & filename) const throw(Error);


public :

	// list methods
	void push_back (Resource* pResource) throw(Error) { Assert(pResource != NULL); m_Resources.push_back(pResource); }
	void pop_front () throw(Error) { Assert(!m_Resources.empty()); m_Resources.pop_front(); }
	Resource* front () const throw(Error) { Assert(!m_Resources.empty()); return m_Resources.front(); }
	bool empty () const throw() { return m_Resources.empty(); }

	// ������ ������ ���, ���� �ֽ� �������� �����ϰ� �������� ������ �����Ѵ�.
	void optimize () throw(Error);

	// get debug string
	string toString () const throw();


private :

	// list of Resource
	list< Resource* > m_Resources;

};

#endif
