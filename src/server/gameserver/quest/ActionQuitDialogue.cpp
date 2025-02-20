////////////////////////////////////////////////////////////////////////////////
// Filename    : ActionQuitDialogue.cpp
// Written By  : 
// Description : 
////////////////////////////////////////////////////////////////////////////////

#include "ActionQuitDialogue.h"
#include "Creature.h"
#include "Gpackets/GCNPCResponse.h"
#include "GamePlayer.h"

////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
void ActionQuitDialogue::read (PropertyBuffer & propertyBuffer)
    throw(Error)
{
    __BEGIN_TRY

	// 이 액션은 NPC와 플레이어 간의 대화를 종료시키는 역할을 하므로
	// 특별히 읽어들여야 할 파라미터가 존재하지 않는다.

    __END_CATCH
}

////////////////////////////////////////////////////////////////////////////////
// 액션을 실행한다.
////////////////////////////////////////////////////////////////////////////////
void ActionQuitDialogue::execute (Creature * pCreature1 , Creature * pCreature2) 
	throw(Error)
{
	__BEGIN_TRY

	Assert(pCreature1 != NULL);
	Assert(pCreature2 != NULL);
	Assert(pCreature1->isNPC());
	Assert(pCreature2->isPC());

	Player* pPlayer = pCreature2->getPlayer();
	Assert(pPlayer != NULL);

	GCNPCResponse response;
	response.setCode(NPC_RESPONSE_QUIT_DIALOGUE);
	pPlayer->sendPacket(&response);

	__END_CATCH
}

////////////////////////////////////////////////////////////////////////////////
// get debug string
////////////////////////////////////////////////////////////////////////////////
string ActionQuitDialogue::toString () const 
	throw()
{
	__BEGIN_TRY

	StringStream msg;
	msg << "ActionQuitDialogue(" << ")";
	return msg.toString();

	__END_CATCH
}
