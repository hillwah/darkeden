//----------------------------------------------------------------------
//
// Filename    : GameServerGroupInfoManager.h
// Written By  : reiot@ewestsoft.com
// Description :
//
//----------------------------------------------------------------------

#ifndef __GAME_SERVER_GROUP_INFO_MANAGER_H__
#define __GAME_SERVER_GROUP_INFO_MANAGER_H__

#include "Types.h"
#include "Exception.h"
#include "GameServerGroupInfo.h"
#include <map>

typedef map< ServerGroupID_t ,GameServerGroupInfo * > HashMapGameServerGroupInfo;

//----------------------------------------------------------------------
//
// class GameServerGroupInfoManager;
//
// ���� ������ ID �� Ű������ �ϴ� GameServerGroupInfo�� map �� 
// ���ο� ������ �ִ�.
//
//----------------------------------------------------------------------

class GameServerGroupInfoManager {
public:
    GameServerGroupInfoManager() throw() {};
    ~GameServerGroupInfoManager() throw();

    void init() throw(Error);
    void load() throw(Error);

    void addGameServerGroupInfo(GameServerGroupInfo * pGameServerGroupInfo, WorldID_t WorldID) throw(DuplicatedException);
    void deleteGameServerGroupInfo(const ServerGroupID_t ServerGroupID, WorldID_t WorldID) throw(NoSuchElementException);

    GameServerGroupInfo * getGameServerGroupInfo(const ServerGroupID_t ServerGroupID, WorldID_t WorldID) const throw(NoSuchElementException);

    uint getSize(WorldID_t WorldID) const throw() { return m_GameServerGroupInfos[WorldID].size(); }

    string toString() const throw();

private:
    // hash map of GameServerGroupInfo
    // key   : GameServerGroupID_t
    // value : GameServerGroupInfo *
    HashMapGameServerGroupInfo* m_GameServerGroupInfos;
    WorldID_t m_MaxWorldID;
};

extern GameServerGroupInfoManager * g_pGameServerGroupInfoManager;
#endif
