//////////////////////////////////////////////////////////////////////
//
// Filename    : HeartbeatManager.h
// Written by  : reiot@ewestsoft.com
// Description : �α��� ������ Ŭ���̾�Ʈ �Ŵ���
//
//////////////////////////////////////////////////////////////////////

#ifndef __HEARTBEAT_MANAGER_H__
#define __HEARTBEAT_MANAGER_H__

#include "Types.h"
#include "Exception.h"

class HeartbeatManager {
public:
    HeartbeatManager() throw(Error);
    ~HeartbeatManager() throw(Error);

    void init() throw(Error);
    void start() throw(Error);
    void stop() throw(Error);
    void run() throw(Error);
};

extern HeartbeatManager * g_pHeartbeatManager;

#endif
