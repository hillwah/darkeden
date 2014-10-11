//////////////////////////////////////////////////////////////////////////////
// Filename    : Mutex.h
// Written By  : reiot@ewestsoft.com
// Description :
//////////////////////////////////////////////////////////////////////////////

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <pthread.h>
#include "Types.h"
#include "Exception.h"
#include "pthreadAPI.h"

class MutexAttr;

class Mutex {
public:
    Mutex(MutexAttr * attr = NULL) throw(Error);
    virtual ~Mutex() throw(Error);

    string getName(void) const { return m_Name; }
    void setName(string name) { m_Name = name; }

    void lock() throw(Error);
    void unlock() throw(Error);
    void trylock() throw(Error);

    pthread_mutex_t * getMutex() throw() { return &m_Mutex; }

private:
    pthread_mutex_t m_Mutex;  // ���ؽ� ��ü
    string m_Name;            // �� ���ؽ��� ������ Ŭ���� �̸�
    TID m_LockTID;            // ���� ���� �� ���� precoess id
};

#endif
