//----------------------------------------------------------------------
//
// Filename    : GameServerGroupInfo.h
// Written By  : Reiot
// Description : �α��� �������� ���� �ִ� �� ���� ������ ���� ����
//
//----------------------------------------------------------------------

#ifndef __GAME_SERVER_GROUP_INFO_H__
#define __GAME_SERVER_GROUP_INFO_H__

#include "Types.h"
#include "Exception.h"
#include "StringStream.h"

//----------------------------------------------------------------------
//
// class GameServerGroupInfo;
//
// GAME DB�� GameServerGroupInfo ���̺��� �о���� �� ���� ������ ������
// ���� Ŭ�����̴�.
//
//----------------------------------------------------------------------

class GameServerGroupInfo {
public:
    WorldID_t getWorldID() const throw() { return m_WorldID; }
    void setWorldID(WorldID_t WorldID) throw() { m_WorldID = WorldID; }

    ServerGroupID_t getGroupID() const throw() { return m_GroupID; }
    void setGroupID(ServerGroupID_t GroupID) throw() { m_GroupID = GroupID; }

    string getGroupName() const throw() { return m_GroupName; }
    void setGroupName(string GroupName) throw() { m_GroupName = GroupName; }

    string toString() const throw() {
        StringStream msg;
        msg << "(GameServerGroupInfo) WorldID: " << m_WorldID << ", ServerGroupID: " << m_GroupID << ", GroupName: " << m_GroupName;
        return msg.toString();
    }

private:
    WorldID_t m_WorldID;
    ServerGroupID_t m_GroupID;
    string m_GroupName;
};
#endif
