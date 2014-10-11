//----------------------------------------------------------------------
//
// Filename    : GameWorldInfoManager.cpp
// Written By  : Reiot
// Description :
//
//----------------------------------------------------------------------

#include "GameWorldInfoManager.h"

#include "database/DatabaseManager.h"
#include "database/Connection.h"
#include "database/Statement.h"
#include "database/Result.h"


GameWorldInfoManager::~GameWorldInfoManager() throw() {
    // hashmap ���� �� pair �� second, �� GameWorldInfo ��ü���� �����ϰ�
    // pair ��ü�� �״�� �д�. (GameWorldInfo�� ���� �����Ǿ� �ִٴ� �Ϳ�
    // �����϶�. �� �ʻ������ �ؾ� �Ѵ�. �ϱ�, GSIM�� destruct �ȴٴ� ����
    // �α��� ������ �˴ٿ�ȴٴ� ���� �ǹ��ϴϱ�.. - -; )
    for (HashMapGameWorldInfo::iterator itr = m_GameWorldInfos.begin(); itr != m_GameWorldInfos.end(); itr++) {
        delete itr->second;
        itr->second = NULL;
    }

    // ���� �ؽ��ʾȿ� �ִ� ��� pair ���� �����Ѵ�.
    m_GameWorldInfos.clear();
}


void GameWorldInfoManager::init() throw(Error) {
    __BEGIN_TRY

    // just load data from GameWorldInfo table
    load();

    __END_CATCH
}


void GameWorldInfoManager::load() throw(Error) {
    __BEGIN_TRY

    clear();

    Statement * pStmt = NULL;

    try {
        pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
        Result * pResult = pStmt->executeQuery("SELECT `ID`, `Name`, `Stat` FROM `WorldInfo`");

        cout << "[GameWorldInfoManager] Loading..." << endl;

        while (pResult->next()) {
            GameWorldInfo * pGameWorldInfo = new GameWorldInfo();
            pGameWorldInfo->setID(pResult->getInt(1));
            pGameWorldInfo->setName(pResult->getString(2));
            pGameWorldInfo->setStatus((WorldStatus)pResult->getInt(3));
            addGameWorldInfo(pGameWorldInfo);
        }
        cout << "[GameWorldInfoManager] Loaded." << endl;

        SAFE_DELETE(pStmt);
    } catch (SQLQueryException & sqe) {
        SAFE_DELETE(pStmt);
        throw Error(sqe.toString());
    } catch (Throwable & t) {
        SAFE_DELETE(pStmt);
        cout << t.toString() << endl;
    }

    __END_CATCH
}


void GameWorldInfoManager::clear() throw(Error) {
    __BEGIN_TRY

    HashMapGameWorldInfo::iterator itr = m_GameWorldInfos.begin();
    for (; itr != m_GameWorldInfos.end(); itr++) {
        GameWorldInfo* pGameWorldInfo = itr->second;
        SAFE_DELETE(pGameWorldInfo);
    }

    m_GameWorldInfos.clear();

    __END_CATCH
}


void GameWorldInfoManager::addGameWorldInfo(GameWorldInfo * pGameWorldInfo) throw(DuplicatedException) {
    __BEGIN_TRY

    cout << pGameWorldInfo->toString() << endl;

    HashMapGameWorldInfo::iterator itr = m_GameWorldInfos.find(pGameWorldInfo->getID());

    if (itr != m_GameWorldInfos.end())
        throw DuplicatedException("duplicated gameserver nickname");

    m_GameWorldInfos[pGameWorldInfo->getID()] = pGameWorldInfo;

    __END_CATCH
}


void GameWorldInfoManager::deleteGameWorldInfo(const WorldID_t ID) throw(NoSuchElementException) {
    __BEGIN_TRY

    HashMapGameWorldInfo::iterator itr = m_GameWorldInfos.find(ID);

    if (itr != m_GameWorldInfos.end()) {
        delete itr->second;
        m_GameWorldInfos.erase(itr);
    }
    else
        throw NoSuchElementException();

    __END_CATCH
}


GameWorldInfo * GameWorldInfoManager::getGameWorldInfo(const WorldID_t ID) const throw(NoSuchElementException) {
    __BEGIN_TRY

    GameWorldInfo * pGameWorldInfo = NULL;

    HashMapGameWorldInfo::const_iterator itr = m_GameWorldInfos.find(ID);

    if (itr != m_GameWorldInfos.end())
        pGameWorldInfo = itr->second;
    else
        throw NoSuchElementException();

    return pGameWorldInfo;

    __END_CATCH
}


string GameWorldInfoManager::toString() const throw() {
    __BEGIN_TRY

    StringStream msg;

    msg << "GameWorldInfoManager(\n";

    if (m_GameWorldInfos.empty())
        msg << "EMPTY";
    else {
        //--------------------------------------------------
        // *OPTIMIZATION*
        //
        // for_each()�� ����� ��
        //--------------------------------------------------
        for (HashMapGameWorldInfo::const_iterator itr = m_GameWorldInfos.begin(); itr != m_GameWorldInfos.end(); itr++)
            msg << itr->second->toString() << '\n';
    }

    msg << ")";

    return msg.toString();

    __END_CATCH
}

GameWorldInfoManager * g_pGameWorldInfoManager = NULL;
