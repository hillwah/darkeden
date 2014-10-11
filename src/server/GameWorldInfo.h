//----------------------------------------------------------------------
//
// Filename    : GameWorldInfo.h
// Written By  : Reiot
// Description : �α��� �������� ���� �ִ� �� ���� ������ ���� ����
//
//----------------------------------------------------------------------

#ifndef __GAME_WORLD_INFO_H__
#define __GAME_WORLD_INFO_H__

#include "Types.h"
#include "Exception.h"
#include "StringStream.h"


//----------------------------------------------------------------------
//
// class GameWorldInfo;
//
// GAME DB�� GameWorldInfo ���̺��� �о���� �� ���� ������ ������
// ���� Ŭ�����̴�.
//
//----------------------------------------------------------------------

class GameWorldInfo {
public:
    GameWorldInfo() {}
    virtual ~GameWorldInfo() {}

    WorldID_t getID() const throw() { return m_ID; }
    void setID(WorldID_t ID) throw() { m_ID = ID; }

    string getName() const throw() { return m_Name; }
    void setName(string Name) throw() { m_Name = Name; }

    WorldStatus getStatus() const throw() { return m_Status; }
    void setStatus(WorldStatus status ) throw() { m_Status = status; } 

    string toString() const throw() {
        StringStream msg;
        msg << "GameWorldInfo("
            << "WorldID: " << (int)m_ID
            << ", Name:" << m_Name
            << ")";
        return msg.toString();
    }

private :
    WorldID_t m_ID;
    string m_Name;
    WorldStatus m_Status;
};

#endif
