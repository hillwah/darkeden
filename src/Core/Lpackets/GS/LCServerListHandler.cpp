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
	#include "Cpackets/CLCreateServer.h"
	#include "Cpackets/CLDeleteServer.h"
	#include "Cpackets/CLSelectServer.h"
	#include "Cpackets/CLSelectBoard.h"

#endif

//----------------------------------------------------------------------
// 서버로부터 캐릭터 리스트를 받았다. 
// 이제 캐릭터 관리 인터페이스의 적절한 곳에 전송받은 값을 집어 넣어서
// 출력하자.
//----------------------------------------------------------------------
void LCServerListHandler::execute (LCServerList * pPacket , Player * pPlayer )
	 throw(ProtocolException , Error )
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

#ifdef __GAME_CLIENT__
#endif
		
	__END_DEBUG_EX __END_CATCH
}
