//----------------------------------------------------------------------
//
// Filename    : GameServerInfo.h
// Written By  : Reiot
// Description : �α��� �������� ���� �ִ� �� ���� ������ ���� ����
//
//----------------------------------------------------------------------

#ifndef __GAME_SERVER_INFO_H__
#define __GAME_SERVER_INFO_H__

#include "Types.h"
#include "Exception.h"
#include "StringStream.h"


//----------------------------------------------------------------------
//
// class GameServerInfo;
//
// GAME DB�� GameServerInfo ���̺��� �о���� �� ���� ������ ������
// ���� Ŭ�����̴�.
//
//----------------------------------------------------------------------

class GameServerInfo {
public:
    GameServerInfo(): m_bNonPKServer(false) {}
    virtual ~GameServerInfo() {}

    ServerID_t getServerID() const throw() { return m_ServerID; }
    void setServerID(ServerID_t ServerID) throw() { m_ServerID = ServerID; }

    const string& getNickname() const throw() { return m_Nickname; }
    void setNickname(const string &nickname) throw() { m_Nickname = nickname; }

    const string& getIP() const throw() { return m_IP; }
    void setIP(const string &ip) throw() { m_IP = ip; }

    uint getTCPPort() const throw() { return m_TCPPort; }
    void setTCPPort(uint port) throw() { m_TCPPort = port; }

    uint getUDPPort() const throw() { return m_UDPPort; }
    void setUDPPort(uint port) throw() { m_UDPPort = port; }

    ServerGroupID_t getGroupID() const throw() { return m_GroupID; }
    void setGroupID(ServerGroupID_t GroupID) { m_GroupID = GroupID; }

    WorldID_t getWorldID() const throw() { return m_WorldID; }
    void setWorldID(WorldID_t WorldID) { m_WorldID= WorldID; }

    ServerStatus getServerStat() const throw() { return m_ServerStat; }
    void setServerStat(ServerStatus Stat) throw() { m_ServerStat = Stat; }

    bool isNonPKServer() const { return m_bNonPKServer; }
    void setNonPKServer() { m_bNonPKServer = true; }

    ServerID_t getCastleFollowingServerID() const { return m_CastleFollowingServerID; }
    void setCastleFollowingServerID(ServerID_t sID ) { m_CastleFollowingServerID = sID; }

    string toString() const throw() {
        StringStream msg;
        msg << "GameServerInfo("
            << "ServerID: " << (int)m_ServerID
            << ",Nickname:" << m_Nickname 
            << ",IP: " << m_IP
            << ",TCPPort:" << m_TCPPort
            << ",UDPPort:" << m_UDPPort
            << ",GroupID:" << (int)m_GroupID
            << ",WorldID:" << (int)m_WorldID
            << ",ServerStat:" << (int)m_GroupID
            << ")";
        return msg.toString();
    }

private:
    ServerID_t m_ServerID;
    string m_Nickname;
    string m_IP;
    uint m_TCPPort;
    uint m_UDPPort;
    ServerGroupID_t m_GroupID;
    WorldID_t m_WorldID;
    ServerStatus m_ServerStat;
    bool m_bNonPKServer;

    ServerID_t m_CastleFollowingServerID;
};

#endif
