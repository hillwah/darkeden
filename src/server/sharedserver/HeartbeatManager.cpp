//////////////////////////////////////////////////////////////////////
//
// Filename    : HeartbeatManager.cpp
// Written by  : reiot@ewestsoft.com
// Description : �α��� ������ Ŭ���̾�Ʈ �Ŵ���
//
//////////////////////////////////////////////////////////////////////

#include "HeartbeatManager.h"
#include "Assert1.h"
#include <unistd.h>

HeartbeatManager::HeartbeatManager() throw(Error) {
    __BEGIN_TRY
    __END_CATCH
}


HeartbeatManager::~HeartbeatManager() throw(Error) {
    __BEGIN_TRY
    __END_CATCH
}


void HeartbeatManager::init() throw(Error) {
    __BEGIN_TRY
    __END_CATCH
}


void HeartbeatManager::start() throw(Error) {
    __BEGIN_TRY

    run();        // �ٷ� run() �޽�带 ȣ���Ѵ�. ^^;

    __END_CATCH
}


void HeartbeatManager::stop() throw(Error) {
    __BEGIN_TRY

    throw UnsupportedError("[HeartbeatManager] Stopping manager is not supported.");

    __END_CATCH
}


void HeartbeatManager::run() throw(Error) {
    __BEGIN_TRY

    while (true) {
        // *TODO
        // ���� HeartBeat���� ���⼭ ó���ϸ� �ȴ�.

        usleep(100);
    }

    __END_CATCH
}

HeartbeatManager * g_pHeartbeatManager = NULL;
