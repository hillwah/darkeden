//----------------------------------------------------------------------
//
// Filename    : LCServerListHandler.cpp
// Written By  : Reiot
// Description : 
//
//----------------------------------------------------------------------

// include files
#include "LCServerList.h"

#ifdef __GAME_CLIENT__
	#include "ClientPlayer.h"
	#include "CLCreateServer.h"
	#include "CLDeleteServer.h"
	#include "CLSelectServer.h"
	#include "CLSelectBoard.h"

#endif

//----------------------------------------------------------------------
// �����κ��� ĳ���� ����Ʈ�� �޾Ҵ�. 
// ���� ĳ���� ���� �������̽��� ������ ���� ���۹��� ���� ���� �־
// �������.
//----------------------------------------------------------------------
void LCServerListHandler::execute (LCServerList * pPacket , Player * pPlayer )
	 throw(ProtocolException , Error )
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

#ifdef __GAME_CLIENT__
#endif
		
	__END_DEBUG_EX __END_CATCH
}
