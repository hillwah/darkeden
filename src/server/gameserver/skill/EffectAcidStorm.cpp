#include "EffectAcidStorm.h"
#include "Zone.h"
#include "Slayer.h"
#include "Vampire.h"
#include "Monster.h"
#include "GamePlayer.h"
#include "SkillUtil.h"
#include "EffectStormAcid.h"

#include "Gpackets/GCModifyInformation.h"
#include "Gpackets/GCStatusCurrentHP.h"
#include "Gpackets/GCAddEffect.h"

EffectAcidStorm::EffectAcidStorm(Zone* pZone, ZoneCoord_t zoneX, ZoneCoord_t zoneY)
	throw(Error)
{

	__BEGIN_TRY

	//m_StormDuration = 1.8;  

	m_pZone = pZone;
	m_X = zoneX;
	m_Y = zoneY;

	m_bVampire = false;

	__END_CATCH
}


bool EffectAcidStorm::affectCreature(Creature* pTargetCreature, bool bAffectByMove)
	throw(Error)
{
	__BEGIN_TRY

	//cout << "EffectAcidStorm " << "affectCreature Begin " << endl;

	Assert(pTargetCreature != NULL);

	// 상대에게 이미 poison 이펙트가 걸려져 있는 경우에는 걸리지 않는다.
	if (pTargetCreature->isFlag(Effect::EFFECT_CLASS_STORM_ACID))
	{
		//cout << "EffectAcidStorm " << "affectCreature End(Already Effected) " << endl;
		return false;
	}
	
	Zone* pZone = pTargetCreature->getZone();

	Creature* pAttacker = pZone->getCreature(m_UserObjectID);
	// 상대방에게 미칠 독 데미지를 계산한다.
	int StormDamage = computeMagicDamage(pTargetCreature, m_Damage, SKILL_ACID_STORM, m_bVampire, pAttacker);

	if (StormDamage > 0)
	{
		//cout << "EffectAcidStorm(Damage:" << StormDamage << ") Affected" << endl;
		// 포이즌 이펙트를 생성해서, 타겟 크리쳐에 붙이고, 플래그를 켜준다.
		EffectStormAcid* pEffectStormAcid = new EffectStormAcid(pTargetCreature);
		pEffectStormAcid->setLevel(m_Level);
		pEffectStormAcid->setPoint(StormDamage/3);
		pEffectStormAcid->setDeadline(16); // 이부분 바꿔야 한다.
		pEffectStormAcid->setTick(5);             // 이부분도 바꿔야 한다.
		pEffectStormAcid->setUserObjectID(m_UserObjectID);
		pEffectStormAcid->affect(pTargetCreature);
		pTargetCreature->addEffect(pEffectStormAcid);
		pTargetCreature->setFlag(Effect::EFFECT_CLASS_STORM_ACID);

		// 이펙트가 붙었다고 주변에 알려준다.
		GCAddEffect gcAddEffect;
		gcAddEffect.setObjectID(pTargetCreature->getObjectID());
		gcAddEffect.setEffectID(Effect::EFFECT_CLASS_STORM_ACID);
		gcAddEffect.setDuration(m_Duration);
		pZone->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);
	}

	//cout << "EffectAcidStorm " << "affectCreature End " << endl;

	return true;

	__END_CATCH
}


void EffectAcidStorm::affect()
	throw(Error)
{
	__BEGIN_TRY
/*	
	Assert(m_pZone != NULL);

	// get tile 
	Tile& tile = m_pZone->getTile(m_X, m_Y);

	HP_t CurrentHP = 0;
	HP_t RemainHP = 0;

	//
	const list<Object*>& oList = tile.getObjectList();
	list<Object*>::const_iterator itr = oList.begin();

	for(; itr != oList.end(); itr++)
	{
		Assert(*itr != NULL);

		Object* pObject = *itr;
		Assert(pObject != NULL);

		if(pObject->getObjectClass() == Object::OBJECT_CLASS_CREATURE)
		{
			Creature* pCreature = dynamic_cast<Creature*>(pObject);
			Assert(pCreature != NULL);

			int AcidDamage = computeMagicDamage(pCreature, m_Damage, SKILL_ACID_STORM);

			if(pCreature->getMoveMode() != Creature::MOVE_MODE_FLYING)
			{
				if(pCreature->isSlayer())
				{
					Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);

					// 
					CurrentHP = pSlayer->getHP(ATTR_CURRENT);
                    RemainHP = max(0, CurrentHP-(int)AcidDamage);

					pSlayer->setHP(RemainHP, ATTR_CURRENT);
					 
					GCModifyInformation gcMI;
					gcMI.addShortData(MODIFY_CURRENT_HP, pSlayer->getHP(ATTR_CURRENT));

					Player* pPlayer = pSlayer->getPlayer();
					Assert(pPlayer != NULL);
					pPlayer->sendPacket(&gcMI);

					GCStatusCurrentHP pkt;
					pkt.setObjectID(pSlayer->getObjectID());
					pkt.setCurrentHP(RemainHP);
					m_pZone->broadcastPacket(pSlayer->getX(), pSlayer->getY(), &pkt);
				}
				else if(pCreature->isVampire())
				{
					Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);

					// 
					CurrentHP = pVampire->getHP(ATTR_CURRENT);
                    RemainHP = max(0, CurrentHP-(int)AcidDamage);

					pVampire->setHP(RemainHP, ATTR_CURRENT);
					 
					GCModifyInformation gcMI;
					gcMI.addShortData(MODIFY_CURRENT_HP, pVampire->getHP(ATTR_CURRENT));

					Player* pPlayer = pVampire->getPlayer();
					Assert(pPlayer != NULL);
					pPlayer->sendPacket(&gcMI);

					GCStatusCurrentHP pkt;
					pkt.setObjectID(pVampire->getObjectID());
					pkt.setCurrentHP(RemainHP);
					m_pZone->broadcastPacket(pVampire->getX(), pVampire->getY(), &pkt);
				}
				else if(pCreature->isMonster())
				{
					Monster* pMonster = dynamic_cast<Monster*>(pCreature);

					CurrentHP = pMonster->getHP(ATTR_CURRENT);
					RemainHP = max(0, CurrentHP-(int)AcidDamage);

					pMonster->setHP(RemainHP, ATTR_CURRENT);

					GCStatusCurrentHP pkt;
					pkt.setObjectID(pMonster->getObjectID());
					pkt.setCurrentHP(RemainHP);
					m_pZone->broadcastPacket(pMonster->getX(), pMonster->getY(), &pkt);
				}
			}
		}
		
	}
		
	setNextTime(m_Tick);
*/

	__END_CATCH
}

void EffectAcidStorm::affect(Creature* pCreature)
	throw(Error)
{
	__BEGIN_TRY
	__END_CATCH
}

void EffectAcidStorm::affect(Zone* pZone, ZoneCoord_t x, ZoneCoord_t y, Object* pObject) 
	throw(Error)
{
	__BEGIN_TRY
	__END_CATCH
}

void EffectAcidStorm::unaffect(Creature* pCreature)
	throw(Error)
{
	__BEGIN_TRY
	__END_CATCH
}

void EffectAcidStorm::unaffect()
	throw(Error)
{
	__BEGIN_TRY

	Tile& tile = m_pZone->getTile(m_X, m_Y);
    tile.deleteEffect(m_ObjectID);

	__END_CATCH
}

void EffectAcidStorm::unaffect(Zone* pZone, ZoneCoord_t x, ZoneCoord_t y, Object* pObjbect)
	throw(Error)
{
	__BEGIN_TRY
	__END_CATCH
}

string EffectAcidStorm::toString()
	const throw()
{
	__BEGIN_TRY

		StringStream msg;

	msg << "EffectAcidStorm("
		<< "ObjectID:" << getObjectID()
		<< ")";

	return msg.toString();

	__END_CATCH
}

EffectAcidStormLoader* g_pEffectAcidStormLoader = NULL;
