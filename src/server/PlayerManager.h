//--------------------------------------------------------------------------------
//
// Filename    : PlayerManager.h
// Written by  : reiot@ewestsoft.com
// Description : 
//
//--------------------------------------------------------------------------------

#ifndef __PLAYER_MANAGER_H__
#define __PLAYER_MANAGER_H__

#include "Types.h"
#include "Exception.h"
#include "Timeval.h"
#include "SocketAPI.h"
#include "Mutex.h"

class Player;
class Packet;

//--------------------------------------------------------------------------------
//
// class PlayerManager;
//
// �÷��̾ �����ϴ� ��ü�̴�. ���� �ӵ��� ���ؼ� socket descriptor
// �� �ε����� �ϴ� �迭�� ����Ѵ�. �� �迭�� ũ��� ���� �������� 
// ó���� �� �ִ� �ִ� �÷��̾��� ����(������ �ִ� ����)�̴�.
// ��� �޸� ���� �ֱ� ������.. ������ �� ���� �����̴�.
//
// �� ���׷쿡 ��� 100���� �÷��̾ �ִٸ�, 
//
//         900 x 4(byte) x 10(#ZoneGroup) = 36k 
//
// ������ ���� �ִ�.
//
//--------------------------------------------------------------------------------

class PlayerManager {
public:
    const static uint nMaxPlayers = 2000;

    PlayerManager() throw();
    virtual ~PlayerManager() throw();

    virtual void broadcastPacket(Packet * pPacket) throw(Error);

    virtual void addPlayer(Player * pPlayer) throw(DuplicatedException, Error);

    virtual void deletePlayer(SOCKET fd) throw(OutOfBoundException, NoSuchElementException, Error);

    virtual Player * getPlayer(SOCKET fd) throw(OutOfBoundException, NoSuchElementException, Error);

    virtual Player * getPlayerByPhoneNumber(PhoneNumber_t PhoneNumber ) throw(OutOfBoundException, NoSuchElementException, Error) { return NULL; }

    uint size() const throw() { return m_nPlayers; }

    void copyPlayers() throw();

protected:
    Player * m_pPlayers[nMaxPlayers];
    uint m_nPlayers;
    Player * m_pCopyPlayers[nMaxPlayers];
};

#endif
