//////////////////////////////////////////////////////////////////////////////
// Filename    : EffectHarpoonBomb.cpp
// Written by  : excel96
// Description :
//////////////////////////////////////////////////////////////////////////////

#include "EffectHarpoonBomb.h"
#include "Slayer.h"
#include "Vampire.h"
#include "Monster.h"
#include "Player.h"
#include "SkillUtil.h"
#include "MonsterCorpse.h"

#include "Gpackets/GCModifyInformation.h"
#include "Gpackets/GCAddEffectToTile.h"
#include "Gpackets/GCStatusCurrentHP.h"
#include "Gpackets/GCRemoveEffect.h"
#include "Gpackets/GCSkillToObjectOK4.h"
#include "Gpackets/GCSkillToObjectOK2.h"

#include <list>

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
EffectHarpoonBomb::EffectHarpoonBomb(Creature* pCreature)
	throw(Error)
{
	__BEGIN_TRY

	m_UserObjectID = 0;
	setTarget(pCreature);
	m_bBroadcastingEffect = false;

	__END_CATCH
}

EffectHarpoonBomb::EffectHarpoonBomb(Zone* pZone, ZoneCoord_t X, ZoneCoord_t Y)
	throw(Error)
{
	__BEGIN_TRY

	m_pZone = pZone;
	m_X = X;
	m_Y = Y;
	m_UserObjectID = 0;
	m_bBroadcastingEffect = false;

	__END_CATCH
}

void EffectHarpoonBomb::crash(Zone* pZone, ZoneCoord_t X, ZoneCoord_t Y)
{
	//cout << "crashing " << X << ", " << Y << endl;
	Assert(pZone != NULL);
	Creature* pCastCreature = pZone->getCreature(m_UserObjectID);

	VSRect rect(0, 0, pZone->getWidth()-1, pZone->getHeight()-1);

	GCAddEffectToTile gcAE;
	gcAE.setEffectID(getSendEffectClass());
	gcAE.setObjectID(getObjectID());
	gcAE.setDuration(10);
	gcAE.setXY(X, Y);
	pZone->broadcastPacket(X, Y, &gcAE);

	for(int x = -1; x <= 1; x++)
	{
		for(int y= -1; y <= 1; y++)
		{
			if(!rect.ptInRect(X+x, Y+y)) continue;
			Tile& tile = pZone->getTile(X+x, Y+y);
			
			const list<Object*>& oList = tile.getObjectList();
			list<Object*>::const_iterator itr = oList.begin();
			for(; itr != oList.end(); itr++)
			{
				Object* pObject = *itr;
				Assert(pObject != NULL);

				if (pObject->getObjectClass() == Object::OBJECT_CLASS_CREATURE)
				{
					Creature* pCreature = dynamic_cast<Creature*>(pObject);
					Assert(pCreature != NULL);

					// 자신은 맞지 않는다
					// 무적상태 체크. by sigi. 2002.9.5
					if (pCreature->getObjectID()==m_UserObjectID
						|| !canAttack(pCastCreature, pCreature )
						|| pCreature->isFlag(Effect::EFFECT_CLASS_COMA)
						|| !checkZoneLevelToHitTarget(pCreature )
						|| pCreature->isDead()
						|| pCreature->isSlayer()
					)
					{
						continue;
					}

					if (pCastCreature != NULL && pCastCreature->isMonster() )
					{
						Monster* pMonster = dynamic_cast<Monster*>(pCastCreature);
						if (pMonster != NULL && !pMonster->isEnemyToAttack(pCreature ) ) continue;
					}

					//GCModifyInformation gcMI;
					GCModifyInformation gcAttackerMI;
					GCSkillToObjectOK2 gcSkillToObjectOK2;

					if (pCreature->isSlayer()) 
					{
						Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);

						::setDamage(pSlayer, m_Damage, pCastCreature, SKILL_HARPOON_BOMB, &gcSkillToObjectOK2, &gcAttackerMI);

/*						Player* pPlayer = pSlayer->getPlayer();
						Assert(pPlayer != NULL);
						pPlayer->sendPacket(&gcMI);*/

					} 
					else if (pCreature->isVampire())
					{
						Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);

						::setDamage(pVampire, m_Damage, pCastCreature, SKILL_HARPOON_BOMB, &gcSkillToObjectOK2, &gcAttackerMI);

/*						Player* pPlayer = pVampire->getPlayer();
						Assert(pPlayer != NULL);
						pPlayer->sendPacket(&gcMI);*/
					}
					else if (pCreature->isOusters())
					{
						Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);

						::setDamage(pOusters, m_Damage, pCastCreature, SKILL_HARPOON_BOMB, &gcSkillToObjectOK2, &gcAttackerMI);

/*						Player* pPlayer = pOusters->getPlayer();
						Assert(pPlayer != NULL);
						pPlayer->sendPacket(&gcMI);*/
					}
					else if (pCreature->isMonster())
					{
						Monster* pMonster = dynamic_cast<Monster*>(pCreature);

						::setDamage(pMonster, m_Damage, pCastCreature, SKILL_HARPOON_BOMB, NULL, &gcAttackerMI);

						if (pCastCreature != NULL ) pMonster->addEnemy(pCastCreature);
					}

					// user한테는 맞는 모습을 보여준다.
					if (pCreature->isPC())
					{
						gcSkillToObjectOK2.setObjectID(1);	// 의미 없다.
						gcSkillToObjectOK2.setSkillType(SKILL_ATTACK_MELEE);
						gcSkillToObjectOK2.setDuration(0);
						pCreature->getPlayer()->sendPacket(&gcSkillToObjectOK2);
					}

					GCSkillToObjectOK4 gcSkillToObjectOK4;
					gcSkillToObjectOK4.setTargetObjectID(pCreature->getObjectID());
					gcSkillToObjectOK4.setSkillType(SKILL_ATTACK_MELEE);
					gcSkillToObjectOK4.setDuration(0);

					pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcSkillToObjectOK4, pCreature);
				}
			}
		}
	}

	//cout << "crashed" << endl;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void EffectHarpoonBomb::affect()
	throw(Error)
{
	__BEGIN_TRY

	//cout << "harpoon bomb affect ";
	Creature* pCreature = dynamic_cast<Creature*>(m_pTarget);

	Zone* pZone = NULL;
	int cX, cY;

	if (pCreature == NULL )
	{
		//cout << "to zone" << endl;
		pZone = m_pZone;
		cX = m_X;
		cY = m_Y;
	}
	else
	{
		//cout << "to creature" << endl;
		pZone = pCreature->getZone();
		cX = pCreature->getX();
		cY = pCreature->getY();
	}

	Assert(pZone != NULL);

	VSRect rect(0, 0, pZone->getWidth()-1, pZone->getHeight()-1);

	for(int x = -1; x <= 1; x++)
	{
		for(int y= -1; y <= 1; y++)
		{
			int X = cX + x;
			int Y = cY + y;
			//cout << "check " << X << ", " << Y << endl;

			if(!rect.ptInRect(X, Y)) continue;
			Tile& tile = pZone->getTile(X, Y);

			const list<Object*>& oList = tile.getObjectList();
			list<Object*>::const_iterator itr = oList.begin();
			for(; itr != oList.end(); itr++)
			{
				Object* pObject = *itr;
				Assert(pObject != NULL);

				if(pObject->getObjectClass() == Object::OBJECT_CLASS_CREATURE)
				{
					Creature* pTargetCreature = dynamic_cast<Creature*>(pObject);
					Assert(pTargetCreature != NULL);

					if (pTargetCreature->isFlag(Effect::EFFECT_CLASS_COMA ) )
					{
						crash(pZone, X, Y);
						Effect* pComa = pTargetCreature->findEffect(Effect::EFFECT_CLASS_COMA);
						if (pComa != NULL )
						{
							pTargetCreature->setFlag(Effect::EFFECT_CLASS_HARPOON_BOMB);
							pComa->setDeadline(0);
						}
					}
				}
				else if (pObject->getObjectClass() == Object::OBJECT_CLASS_ITEM )
				{
					Item* pTargetItem = dynamic_cast<Item*>(pObject);
					if (pTargetItem->getItemClass() == Item::ITEM_CLASS_CORPSE )
					{
						crash(pZone, X, Y);

						// 시체를 삭제한다.
						if (pTargetItem->getItemType() == MONSTER_CORPSE )
						{
							bool bDelete = false;

							MonsterCorpse* pCorpse = dynamic_cast<MonsterCorpse*>(pTargetItem);
							Assert(pCorpse != NULL);
							// 성물보관함이 아니어야 한다.
							// 크리스마스 트리가 아니어야 한다.
							// 알림판이 아니어야 한다.
							// 성단이 아니어야 한다.
							if (!pCorpse->isFlag(Effect::EFFECT_CLASS_SLAYER_RELIC_TABLE)
							  && !pCorpse->isFlag(Effect::EFFECT_CLASS_VAMPIRE_RELIC_TABLE) 
							  && pCorpse->getMonsterType() != 482
							  && pCorpse->getMonsterType() != 650
							  && !pCorpse->isShrine() )
							{
								bDelete = true;
							}

							if (bDelete )
								pZone->deleteItemDelayed(pTargetItem, X, Y);
						}

//						SAFE_DELETE(pTargetItem);
					}
				}
			}
		}
	}

	setDeadline(0);

	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void EffectHarpoonBomb::unaffect()
	throw(Error)
{
	__BEGIN_TRY

	Creature* pCreature = dynamic_cast<Creature*>(m_pTarget);
//	unaffect(pCreature);

	if (pCreature == NULL )
	{
		Tile& tile = m_pZone->getTile(m_X, m_Y);
		tile.deleteEffect(m_ObjectID);
	}
	else
	{
		pCreature->removeFlag(Effect::EFFECT_CLASS_HARPOON_BOMB);
	}

	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
string EffectHarpoonBomb::toString() const 
	throw()
{
	__BEGIN_TRY

	StringStream msg;
	msg << "EffectHarpoonBomb("
		<< "ObjectID:" << getObjectID()
		<< ")";
	return msg.toString();

	__END_CATCH
}

