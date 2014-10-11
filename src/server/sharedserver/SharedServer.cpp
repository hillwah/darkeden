//////////////////////////////////////////////////////////////////////
// 
// Filename    : SharedServer.cpp 
// Written By  : reiot@ewestsoft.com
// Description : ����� ������ ���� Ŭ����
// 
//////////////////////////////////////////////////////////////////////

#include "SharedServer.h"
#include "Assert1.h"
#include "GameServerInfoManager.h"
#include "GameServerGroupInfoManager.h"
#include "GameServerManager.h"
#include "HeartbeatManager.h"
#include "database/DatabaseManager.h"
#include "PacketFactoryManager.h"
#include "PacketValidator.h"
#include "GameWorldInfoManager.h"
#include "GuildManager.h"
#include "ResurrectLocationManager.h"
#include "StringPool.h"

//#include "LogClient.h"

#include "types/ServerType.h"
#ifdef __NETMARBLE_SERVER__
    #include "NetmarbleGuildRegisterThread.h"
#endif

SharedServer::SharedServer() throw(Error) {
    __BEGIN_TRY
    
    g_pDatabaseManager = new DatabaseManager();

    g_pGuildManager = new GuildManager();

    g_pGameServerInfoManager = new GameServerInfoManager();
    g_pGameServerGroupInfoManager = new GameServerGroupInfoManager();

    g_pPacketFactoryManager = new PacketFactoryManager();
    g_pPacketValidator = new PacketValidator();

    g_pGameServerManager = new GameServerManager();

    g_pHeartbeatManager = new HeartbeatManager();

    g_pGameWorldInfoManager = new GameWorldInfoManager();

    g_pResurrectLocationManager = new ResurrectLocationManager();

    g_pStringPool = new StringPool();

/*#ifdef __NETMARBLE_SERVER__
    g_pNetmarbleGuildRegisterThread = new NetmarbleGuildRegisterThread();
#endif*/

    __END_CATCH
}

SharedServer::~SharedServer() throw(Error) {
    __BEGIN_TRY
        
    SAFE_DELETE(g_pHeartbeatManager);
    SAFE_DELETE(g_pGameServerManager);
    SAFE_DELETE(g_pPacketValidator);
    SAFE_DELETE(g_pPacketFactoryManager);
    SAFE_DELETE(g_pGameServerInfoManager);
    SAFE_DELETE(g_pGameServerGroupInfoManager);
    SAFE_DELETE(g_pGuildManager);
    SAFE_DELETE(g_pDatabaseManager);
    SAFE_DELETE(g_pGameWorldInfoManager);
    SAFE_DELETE(g_pResurrectLocationManager);
    SAFE_DELETE(g_pStringPool);

/*#ifdef __NETMARBLE_SERVER__
    SAFE_DELETE(g_pNetmarbleGuildRegisterThread);
#endif*/

    __END_CATCH
}


void SharedServer::init() throw(Error) {
    __BEGIN_TRY

    cout << "[SharedServer] Initializing..." << endl;

    g_pDatabaseManager->init();

    g_pStringPool->load();

    g_pGuildManager->init();

    g_pGameServerInfoManager->init();
    g_pGameServerGroupInfoManager->init();
    
    g_pGameWorldInfoManager->init();

    g_pPacketFactoryManager->init();
    g_pPacketValidator->init();

    g_pGameServerManager->init();

    g_pResurrectLocationManager->init();

/*#ifdef __NETMARBLE_SERVER__
    g_pNetmarbleGuildRegisterThread->init();
#endif*/
    
    g_pHeartbeatManager->init();

    cout << "[SharedServer] Initialized." << endl;

    __END_CATCH
}


void SharedServer::start() throw(Error) {
    __BEGIN_TRY

    cout << "[SharedServer] Starting..." << endl;
    g_pGameServerManager->start();

    // �ݸ��� ��� ��� ������ ����
/*#ifdef __NETMARBLE_SERVER__
    g_pNetmarbleGuildRegisterThread->start();
#endif*/
        
    //
    // Ŭ���̾�Ʈ �Ŵ����� �����Ѵ�.
    //
    // *Reiot's Notes*
    //
    // ���� ���߿� ����Ǿ�� �Ѵ�. �ֳ��ϸ� ��Ƽ���������� �ƴ�
    // ���ѷ����� ���� �Լ��̱� �����̴�. ���� �� ������ �ٸ� �Լ���
    // ȣ���� ���, ������ ������ �ʴ���(�� ������ �߻����� �ʴ���)
    // �ٸ� �Ŵ����� ó�� ������ ������� �ʴ´�.    
    //
    g_pHeartbeatManager->start();
    cout << "[SharedServer] Started." << endl;

    __END_CATCH
}


void SharedServer::stop() throw(Error) {
    __BEGIN_TRY

    throw UnsupportedError();

    g_pHeartbeatManager->stop();
    
    g_pGameServerManager->stop();

    __END_CATCH
}

SharedServer * g_pSharedServer = NULL;
