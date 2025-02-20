//////////////////////////////////////////////////////////////////////////////
// Filename    : DuckingWallop.cpp
// Written by  : excel96
// Description : 
//////////////////////////////////////////////////////////////////////////////

#include "DuckingWallop.h"
#include "ZoneUtil.h"
#include "SimpleTileMissileSkill.h"
#include "RankBonus.h"
#include "Utility.h"

#include "Gpackets/GCSkillToTileOK1.h"
#include "Gpackets/GCSkillToTileOK2.h"
#include "Gpackets/GCSkillToTileOK3.h"
#include "Gpackets/GCSkillToTileOK4.h"
#include "Gpackets/GCSkillToTileOK5.h"
#include "Gpackets/GCSkillToTileOK6.h"


//////////////////////////////////////////////////////////////////////////////
// 생성자
// 마스크를 초기화한다.
//////////////////////////////////////////////////////////////////////////////
DuckingWallop::DuckingWallop()
	throw()
{
	__BEGIN_TRY

	const int width[7] = { 0, 1, 1, 1, 1, 1, 1 };

	TPOINT mask[8];
	mask[0].x = -1;
	mask[0].y =  0;
	mask[1].x = -1;
	mask[1].y =  1;
	mask[2].x =  0;
	mask[2].y =  1;
	mask[3].x =  1;
	mask[3].y =  1;
	mask[4].x =  1;
	mask[4].y =  0;
	mask[5].x =  1;
	mask[5].y = -1;
	mask[6].x =  0;
	mask[6].y = -1;
	mask[7].x = -1;
	mask[7].y = -1;

	for (int k = 0; k < 8; k++ )
	{
		int l = 0;
		for (int i = 1; i <= 6; i++ )
		{
			int x = 0;
			int y = 0;

			for (int j = 0; j <= width[i]; j++ )
			{
				x = mask[k].x * i;
				y = mask[k].y * i;

				if (j == 0 )
				{
					m_DamageRatio[l] = 75;
					m_pDuckingWallopMask[k][l++].set(x, y);
				}
				else 
				{
					int left  = (k % 2 == 0 ? (k + 2 ) % 8 : (k + 3 ) % 8);
					int right = (k % 2 == 0 ? (k + 6 ) % 8 : (k + 5 ) % 8);

					int xl = x + (mask[left].x * j);
					int yl = y + (mask[left].y * j);

					int xr = x + (mask[right].x * j);
					int yr = y + (mask[right].y * j);

					m_DamageRatio[l] = 100;
					m_pDuckingWallopMask[k][l++].set(xl, yl);
					m_DamageRatio[l] = 100;
					m_pDuckingWallopMask[k][l++].set(xr, yr);
				}
			}
		}
	}

	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
// 뱀파이어 오브젝트 핸들러
//////////////////////////////////////////////////////////////////////////////
void DuckingWallop::execute(Ousters* pOusters, ObjectID_t TargetObjectID, OustersSkillSlot* pOustersSkillSlot, CEffectID_t CEffectID)
    throw(Error)
{
    __BEGIN_TRY

	//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << "begin " << endl;

	Assert(pOusters != NULL);
	Assert(pOustersSkillSlot != NULL);

    try
    {
		Zone* pZone = pOusters->getZone();
		Assert(pZone != NULL);

		Creature* pTargetCreature = pZone->getCreature(TargetObjectID);
		//Assert(pTargetCreature != NULL);

		// NoSuch제거. by sigi. 2002.5.2
		if (pTargetCreature==NULL)
		{
			executeSkillFailException(pOusters, getSkillType());

			return;
		}

		execute(pOusters, pTargetCreature->getX(), pTargetCreature->getY(), pOustersSkillSlot, CEffectID);
    } 
	catch (Throwable & t) 
	{
		executeSkillFailException(pOusters, getSkillType());
        //cout << t.toString() << endl;
    }

	//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << " end " << endl;

    __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
// 뱀파이어 타일 핸들러
//////////////////////////////////////////////////////////////////////////////
void DuckingWallop::execute(Ousters* pOusters, ZoneCoord_t X, ZoneCoord_t Y, OustersSkillSlot* pOustersSkillSlot, CEffectID_t CEffectID)
	throw(Error)
{
	__BEGIN_TRY

	//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << "begin " << endl;
	SkillType_t SkillType = getSkillType();

	// Knowledge of Blood 가 있다면 hit bonus 10
	int HitBonus = 0;
	if (pOusters->hasRankBonus(RankBonus::RANK_BONUS_KNOWLEDGE_OF_BLOOD ) )
	{
		RankBonus* pRankBonus = pOusters->getRankBonus(RankBonus::RANK_BONUS_KNOWLEDGE_OF_BLOOD);
		Assert(pRankBonus != NULL);

		HitBonus = pRankBonus->getPoint();
	}

	try 
	{
		SkillInput input(pOusters, pOustersSkillSlot);
		SkillOutput output;
		computeOutput(input, output);

		Dir_t	dir		= getDirectionToPosition(pOusters->getX(), pOusters->getY(), X, Y);

		// 강제로 knockback시킬 확률
//		bool bForceKnockback = rand()%100 < output.ToHit;

		Player* pPlayer = pOusters->getPlayer();
		Zone* pZone = pOusters->getZone();
		Assert(pPlayer != NULL);
		Assert(pZone != NULL);

		VSRect rect(1, 1, pZone->getWidth()-2, pZone->getHeight()-2);
		if (!rect.ptInRect(X, Y ))
		{
			executeSkillFailException(pOusters, SkillType);
			return;
		}

		GCSkillToTileOK1 _GCSkillToTileOK1;
		GCSkillToTileOK2 _GCSkillToTileOK2;
//		GCSkillToTileOK3 _GCSkillToTileOK3;
//		GCSkillToTileOK4 _GCSkillToTileOK4;
		GCSkillToTileOK5 _GCSkillToTileOK5;
//		GCSkillToTileOK6 _GCSkillToTileOK6;

		SkillInfo*  pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);

		int  RequiredMP  = (int)pSkillInfo->getConsumeMP();
		bool bManaCheck  = hasEnoughMana(pOusters, RequiredMP);
		bool bTimeCheck  = verifyRunTime(pOustersSkillSlot);
		bool bRangeCheck = verifyDistance(pOusters, X, Y, pSkillInfo->getRange());

		ZoneCoord_t myX          = pOusters->getX();
		ZoneCoord_t myY          = pOusters->getY();
		int TargetX = (int)myX + dirMoveMask[dir].x*6;
		int TargetY = (int)myY + dirMoveMask[dir].y*6;

		bool bPassLine	 = isPassLine(pZone, myX, myY, TargetX, TargetY);

		if (bManaCheck && bTimeCheck && bRangeCheck && bPassLine && pZone->moveFastPC(pOusters, myX, myY, TargetX, TargetY, getSkillType() ))
		{
			// 마나를 떨어뜨린다.
			decreaseMana(pOusters, RequiredMP, _GCSkillToTileOK1);

			// 좌표와 방향을 구한다.
//			Dir_t       dir          = calcDirection(myX, myY, X, Y);

			list<Creature*> cList;

			// knockback 때문에 recursive 하게 데미지를 먹는 경우가 있다.
			// 그래서 제일 먼쪽에 있는 마스크부터 체크한다.
			for (int i = 17; i >= 0; i-- )
			{
				int tileX   = myX + m_pDuckingWallopMask[dir][i].x;
				int tileY   = myY + m_pDuckingWallopMask[dir][i].y;

				// 현재 타일이 존 내부이고, 안전지대가 아니라면 맞을 가능성이 있다.
				if (rect.ptInRect(tileX, tileY))
				{
					// 타일을 받아온다.
					Tile& tile = pZone->getTile(tileX, tileY);

					list<Creature*> targetList;
					if (tile.hasCreature(Creature::MOVE_MODE_WALKING))
					{
						Creature* pCreature = tile.getCreature(Creature::MOVE_MODE_WALKING);
						targetList.push_back(pCreature);
					}
					if (tile.hasCreature(Creature::MOVE_MODE_FLYING))
					{
						Creature* pCreature = tile.getCreature(Creature::MOVE_MODE_FLYING);
						targetList.push_back(pCreature);
					}
					if (tile.hasCreature(Creature::MOVE_MODE_BURROWING))
					{
						Creature* pCreature = tile.getCreature(Creature::MOVE_MODE_BURROWING);
						targetList.push_back(pCreature);
					}

					list<Creature*>::iterator itr = targetList.begin();
					for(; itr != targetList.end(); itr++)
					{
						Creature* pTargetCreature = (*itr);
						Assert(pTargetCreature != NULL);

						if (!canAttack(pOusters, pTargetCreature )
							|| pTargetCreature->isFlag(Effect::EFFECT_CLASS_COMA) )
						{
							continue;
						}

						if (pTargetCreature != pOusters)
						{
							bool bPK             = verifyPK(pOusters, pTargetCreature);
							bool bRaceCheck      = pTargetCreature->isSlayer() || pTargetCreature->isMonster() || pTargetCreature->isVampire();
							bool bZoneLevelCheck = checkZoneLevelToHitTarget(pTargetCreature);
							bool bHitRoll        = HitRoll::isSuccess(pOusters, pTargetCreature);

							// min : 20, max : 100
							if (bPK && bRaceCheck && bZoneLevelCheck && bHitRoll)
							{
								Damage_t Damage = computeDamage(pOusters, pTargetCreature);
								Damage += output.Damage;

								ObjectID_t targetObjectID = pTargetCreature->getObjectID();
								cList.push_back(pTargetCreature);

								_GCSkillToTileOK1.addCListElement(targetObjectID);
								_GCSkillToTileOK2.addCListElement(targetObjectID);
								_GCSkillToTileOK5.addCListElement(targetObjectID);

								// 일단 맞는 놈이 받을 패킷은 널 상태로 한 채로, 데미지를 준다.
								setDamage(pTargetCreature, Damage, pOusters, SkillType, NULL, &_GCSkillToTileOK1);
								computeAlignmentChange(pTargetCreature, Damage, pOusters, NULL, &_GCSkillToTileOK1);

								increaseAlignment(pOusters, pTargetCreature, _GCSkillToTileOK1);

								if (pTargetCreature->isDead())
								{
									int exp = computeCreatureExp(pTargetCreature, 100);
									shareOustersExp(pOusters, exp, _GCSkillToTileOK1);
								}
							}
						}
					}
				}
			}


			// 공격자의 아이템 내구성을 떨어뜨린다.
			decreaseDurability(pOusters, NULL, pSkillInfo, &_GCSkillToTileOK1, NULL);

			_GCSkillToTileOK1.setSkillType(SkillType);
			_GCSkillToTileOK1.setCEffectID(0);
			_GCSkillToTileOK1.setX(X);
			_GCSkillToTileOK1.setY(Y);
			_GCSkillToTileOK1.setRange(dir);
			_GCSkillToTileOK1.setDuration(0);

			_GCSkillToTileOK2.setObjectID(pOusters->getObjectID());
			_GCSkillToTileOK2.setSkillType(SkillType);
			_GCSkillToTileOK2.setX(X);
			_GCSkillToTileOK2.setY(Y);
			_GCSkillToTileOK2.setRange(dir);
			_GCSkillToTileOK2.setDuration(0);

			_GCSkillToTileOK5.setObjectID(pOusters->getObjectID());
			_GCSkillToTileOK5.setSkillType(SkillType);
			_GCSkillToTileOK5.setX(X);
			_GCSkillToTileOK5.setY(Y);
			_GCSkillToTileOK5.setRange(dir);
			_GCSkillToTileOK5.setDuration(0);

			pPlayer->sendPacket(&_GCSkillToTileOK1);

			// 이 기술에 의해 영향을 받는 놈들에게 패킷을 보내줘야 한다.
			for (list<Creature*>::const_iterator itr = cList.begin(); itr != cList.end(); itr++)
			{
				Creature * pTargetCreature = *itr;
				Assert(pTargetCreature != NULL);

				if (pTargetCreature->isPC())
				{
					_GCSkillToTileOK2.clearList();

					// HP의 변경사항을 패킷에다 기록한다.
					HP_t targetHP = 0;
					if (pTargetCreature->isSlayer())
					{
						targetHP = (dynamic_cast<Slayer*>(pTargetCreature))->getHP(ATTR_CURRENT);
					}
					else if (pTargetCreature->isVampire())
					{
						targetHP = (dynamic_cast<Vampire*>(pTargetCreature))->getHP(ATTR_CURRENT);
					}
					else if (pTargetCreature->isOusters())
					{
						targetHP = (dynamic_cast<Ousters*>(pTargetCreature))->getHP(ATTR_CURRENT);
					}

					_GCSkillToTileOK2.addShortData(MODIFY_CURRENT_HP, targetHP);

					// 아이템의 내구력을 떨어뜨린다.
					decreaseDurability(NULL, pTargetCreature, pSkillInfo, NULL, &_GCSkillToTileOK2);

					// 패킷을 보내준다.
					pTargetCreature->getPlayer()->sendPacket(&_GCSkillToTileOK2);
				}
				else if (pTargetCreature->isMonster())
				{
					// 당근 적으로 인식한다.
					Monster* pMonster = dynamic_cast<Monster*>(pTargetCreature);
					pMonster->addEnemy(pOusters);
				}
			}

			cList.push_back(pOusters);

			pZone->broadcastPacket(myX, myY, &_GCSkillToTileOK5 , cList);

			// set Next Run Time
			pOustersSkillSlot->setRunTime(output.Delay);
		}
		else 
		{
			executeSkillFailNormal(pOusters, SkillType, NULL);
		}
	}
	catch (Throwable & t) 
	{
		executeSkillFailException(pOusters, SkillType);
	}

//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << " end " << endl;

	__END_CATCH
}

DuckingWallop g_DuckingWallop;
