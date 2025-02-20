//////////////////////////////////////////////////////////////////////////////
// Filename    : EffectVampireRelic.cpp
// Written by  : elca
// Description : 
//////////////////////////////////////////////////////////////////////////////

#include "EffectVampireRelic.h"
#include "ZoneGroupManager.h"
#include "Creature.h"
#include "Slayer.h"
#include "Vampire.h"
#include "Monster.h"
#include "MonsterCorpse.h"
#include "Player.h"
#include "StringPool.h"
#include "Gpackets/GCModifyInformation.h"
#include "Gpackets/GCStatusCurrentHP.h"
#include "Gpackets/GCRemoveEffect.h"
#include "Gpackets/GCAddEffect.h"
#include "Gpackets/GCSystemMessage.h"

#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
EffectVampireRelic::EffectVampireRelic(Creature* pCreature)
	throw(Error)
{
	__BEGIN_TRY

	m_HPRecovery = 1;

	setTarget(pCreature);

	__END_CATCH
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
EffectVampireRelic::EffectVampireRelic(Item* pItem)
	throw(Error)
{
	__BEGIN_TRY

	m_HPRecovery = 1;

	setTarget(pItem);

	__END_CATCH
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void EffectVampireRelic::affect(Creature* pCreature)
	throw(Error)
{
	__BEGIN_TRY
																															    	
	//Timeval      nextTime   = getNextTime();
	//Timeval      deadLine   = getDeadline();
	//Turn_t       RemainTime = deadLine.tv_sec - nextTime.tv_sec;

/*
	StringStream msg;

	if (pCreature->isSlayer())
	{
		Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);

		msg << pSlayer->getName();
	}
	else
	{
		Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);

		msg << pVampire->getName();
	}

	msg << " 님이 뱀파이어 성물을 가졌습니다.";
*/

    char msg[50];
    sprintf(msg, g_pStringPool->c_str(STRID_TAKE_VAMPIRE_RELIC ),
                    pCreature->getName().c_str());

    string sMsg(msg);

	GCSystemMessage gcSystemMessage;

	gcSystemMessage.setType(SYSTEM_MESSAGE_COMBAT);
	gcSystemMessage.setMessage(sMsg);

	g_pZoneGroupManager->broadcast(&gcSystemMessage);


	// Effect붙인다.
	GCAddEffect gcAddEffect;
	gcAddEffect.setObjectID(pCreature->getObjectID());
	gcAddEffect.setEffectID(getSendEffectClass());
	gcAddEffect.setDuration(65000);
	pCreature->getZone()->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcAddEffect);

	setNextTime(m_Tick);

	__END_CATCH
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void EffectVampireRelic::affect(Item* pItem)
	throw(Error)
{
	__BEGIN_TRY
																															    	
		/*
	//Timeval      nextTime   = getNextTime();
	//Timeval      deadLine   = getDeadline();
	//Turn_t       RemainTime = deadLine.tv_sec - nextTime.tv_sec;
	StringStream msg;

	if (pCreature->isSlayer())
	{
		Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);

		msg << pSlayer->getName() << " 님이 ";
	}
	else
	{
		Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);

		msg << pVampire->getName() << " 님이 ";
	}

	msg << " 뱀파이어 성물을 가지고 있습니다." << endl;

	GCSystemMessage gcSystemMessage;
	gcSystemMessage.setMessage(msg.toString());

	g_pZoneGroupManager->broadcast(&gcSystemMessage);

	setNextTime(m_Tick);
	*/

	__END_CATCH
}

void EffectVampireRelic::unaffect() 
	throw(Error)
{
	__BEGIN_TRY	

	
	switch (m_pTarget->getObjectClass())
	{
		case OBJECT_CLASS_CREATURE :
		{
			Creature* pCreature = dynamic_cast<Creature *>(m_pTarget);
			unaffect(pCreature);
		}
		break;

		case OBJECT_CLASS_ITEM :
		{
			Item* pItem = dynamic_cast<Item *>(m_pTarget);
			unaffect(pItem);
		}
		break;

		default :
			throw Error("Wrong Object Class");
	}


	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void EffectVampireRelic::unaffect(Creature* pCreature)
	throw(Error)
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	Assert(pCreature != NULL);

	// 능력치를 정상적으로 되돌리기 위해서는 플래그를 끄고,
	// initAllStat을 불러야 한다.
	pCreature->removeFlag(Effect::EFFECT_CLASS_VAMPIRE_RELIC);

	Zone* pZone = pCreature->getZone();
	Assert(pZone != NULL);

	GCRemoveEffect gcRemoveEffect;
	gcRemoveEffect.setObjectID(pCreature->getObjectID());
	gcRemoveEffect.addEffectList(Effect::EFFECT_CLASS_VAMPIRE_RELIC);
	pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcRemoveEffect);

	__END_DEBUG
	__END_CATCH
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void EffectVampireRelic::unaffect(Item* pItem)
	throw(Error)
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	Assert(pItem != NULL);

	Assert(pItem->getItemClass()==Item::ITEM_CLASS_CORPSE);
	Assert(pItem->getItemType()==MONSTER_CORPSE);

	MonsterCorpse* pCorpse = dynamic_cast<MonsterCorpse*>(pItem);

	pCorpse->removeFlag(Effect::EFFECT_CLASS_VAMPIRE_RELIC);

	Zone* pZone = pCorpse->getZone();
	Assert(pZone != NULL);

	GCRemoveEffect gcRemoveEffect;
	gcRemoveEffect.setObjectID(pItem->getObjectID());
	gcRemoveEffect.addEffectList(Effect::EFFECT_CLASS_VAMPIRE_RELIC);
	pZone->broadcastPacket(pCorpse->getX(), pCorpse->getY(), &gcRemoveEffect);

	__END_DEBUG
	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
string EffectVampireRelic::toString()
	const throw()
{
	__BEGIN_TRY

	StringStream msg;
	msg << "EffectVampireRelic("
		<< "ObjectID:" << getObjectID()
		<< ")";
	return msg.toString();

	__END_CATCH
}
