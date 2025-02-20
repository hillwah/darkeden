//////////////////////////////////////////////////////////////////////////////
// Filename    : JabbingVein.cpp
// Written by  : elca@ewestsoft.com
// Description :
//////////////////////////////////////////////////////////////////////////////

#include "JabbingVein.h"
#include "EffectJabbingVein.h"
#include "Gpackets/GCAttackArmsOK1.h"
#include "Gpackets/GCAttackArmsOK2.h"
#include "Gpackets/GCAttackArmsOK3.h"
#include "Gpackets/GCAttackArmsOK4.h"
#include "Gpackets/GCAttackArmsOK5.h"
#include "Gpackets/GCAddEffect.h"
#include "ItemUtil.h"

//////////////////////////////////////////////////////////////////////////////
// 슬레이어 오브젝트
//////////////////////////////////////////////////////////////////////////////
void JabbingVein::execute (Slayer* pSlayer, ObjectID_t TargetObjectID,  SkillSlot* pSkillSlot, CEffectID_t CEffectID)
	throw(Error)
{
	__BEGIN_TRY __BEGIN_DEBUG

	//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << " Begin" << endl;

	Assert(pSlayer != NULL);
	Assert(pSkillSlot != NULL);

	try 
	{
		Player* pPlayer = pSlayer->getPlayer();
		Zone* pZone = pSlayer->getZone();
		Assert(pPlayer != NULL);
		Assert(pZone != NULL);

		Creature* pTargetCreature = pZone->getCreature(TargetObjectID);
		//Assert(pTargetCreature != NULL);

		// NoSuch제거. by sigi. 2002.5.2
		if (pTargetCreature==NULL
			|| !canAttack(pSlayer, pTargetCreature )
			|| pTargetCreature->isNPC()) 
		{
			executeSkillFailException(pSlayer, getSkillType());
			//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << " End" << endl;
			return;
		}

		GCAttackArmsOK1 _GCAttackArmsOK1;
		GCAttackArmsOK2 _GCAttackArmsOK2;
		GCAttackArmsOK3 _GCAttackArmsOK3;
		GCAttackArmsOK4 _GCAttackArmsOK4;
		GCAttackArmsOK5 _GCAttackArmsOK5;

		// 들고 있는 무기가 없거나, 총 계열 무기가 아니라면 기술을 쓸 수 없다.
		// 총 계열 무기 중에서도 SG나 SR은 JabbingVein를 쓸 수가 없다.
		// SG, SR 도 이제 쓸 수 있다.
		// 2003. 1. 14  by bezz
		Item* pWeapon = pSlayer->getWearItem(Slayer::WEAR_RIGHTHAND);
		if (pWeapon == NULL || isArmsWeapon(pWeapon) == false )
//			pWeapon->getItemClass() == Item::ITEM_CLASS_SG || 
//			pWeapon->getItemClass() == Item::ITEM_CLASS_SR)
		{
			executeSkillFailException(pSlayer, getSkillType());
			//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << " End" << endl;
			return;
		}

		bool bIncreaseExp = pSlayer->isRealWearingEx(Slayer::WEAR_RIGHTHAND);

		SkillType_t       SkillType  = pSkillSlot->getSkillType();
		SkillInfo*        pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);
		SkillDomainType_t DomainType = pSkillInfo->getDomainType();
		SkillLevel_t      SkillLevel = pSkillSlot->getExpLevel();

		SkillInput input(pSlayer, pSkillSlot);
		SkillOutput output;

		if (pTargetCreature->isPC() )
		{
			input.TargetType = SkillInput::TARGET_PC;
		}
		else
		{
			input.TargetType = SkillInput::TARGET_MONSTER;
		}

		computeOutput(input, output);

		// 페널티 값을 계산한다.
		int ToHitPenalty = getPercentValue(pSlayer->getToHit(), output.ToHit); 

		int  RequiredMP  = (int)pSkillInfo->getConsumeMP();
		bool bManaCheck   = hasEnoughMana(pSlayer, RequiredMP);
		bool bTimeCheck   = verifyRunTime(pSkillSlot);
		bool bRangeCheck  = verifyDistance(pSlayer, pTargetCreature, pWeapon->getRange());
		bool bBulletCheck = (getRemainBullet(pWeapon) > 0) ? true : false;
		bool bHitRoll     = HitRoll::isSuccess(pSlayer, pTargetCreature, ToHitPenalty);
		bool bPK          = verifyPK(pSlayer, pTargetCreature);

		// 총알 숫자는 무조건 떨어뜨린다.
		Bullet_t RemainBullet = 0;
		if (bBulletCheck)
		{
			// 총알 숫자를 떨어뜨리고, 저장하고, 남은 총알 숫자를 받아온다.
			decreaseBullet(pWeapon);
			// 한발쓸때마다 저장할 필요 없다. by sigi. 2002.5.9
			//pWeapon->save(pSlayer->getName(), STORAGE_GEAR, 0, Slayer::WEAR_RIGHTHAND, 0);
			RemainBullet = getRemainBullet(pWeapon);
		}

		if (bManaCheck && bTimeCheck && bRangeCheck && bBulletCheck && bHitRoll && bPK)
		{
			//cout << pSlayer->getName().c_str() << " Attack OK" << endl;
			decreaseMana(pSlayer, RequiredMP, _GCAttackArmsOK1);

			_GCAttackArmsOK5.setSkillSuccess(true);
			_GCAttackArmsOK1.setSkillSuccess(true);

			bool bCriticalHit = false;

			// 데미지를 계산하고, quickfire 페널티를 가한다.
			// output.Damage가 음수이기 때문에, %값을 구해 더하면 결국 빼는 것이 된다.
			int Damage = computeDamage(pSlayer, pTargetCreature, SkillLevel/5, bCriticalHit);
			Damage += getPercentValue(Damage, output.Damage);
			Damage = max(0, Damage);

			//cout << "JabbingVeinDamage:" << Damage << endl;

			// 데미지를 세팅한다.
			setDamage(pTargetCreature, Damage, pSlayer, SkillType, &_GCAttackArmsOK2, &_GCAttackArmsOK1);
			computeAlignmentChange(pTargetCreature, Damage, pSlayer, &_GCAttackArmsOK2, &_GCAttackArmsOK1);

			bool bAffect = true;

			if (pTargetCreature->isMonster())
			{
				Monster* pMonster = dynamic_cast<Monster*>(pTargetCreature);
				if (pMonster->isMaster() ) bAffect = false;
			}

			if (bAffect && !pTargetCreature->isFlag(Effect::EFFECT_CLASS_JABBING_VEIN) && rand()%100 < output.Range )
			{
				EffectJabbingVein* pEffect = new EffectJabbingVein(pTargetCreature);
				pEffect->setDeadline(output.Duration);
				pTargetCreature->addEffect(pEffect);
				pTargetCreature->setFlag(pEffect->getEffectClass());

				GCAddEffect gcAddEffect;
				gcAddEffect.setObjectID(pTargetCreature->getObjectID());
				gcAddEffect.setEffectID(pEffect->getSendEffectClass());
				gcAddEffect.setDuration(output.Duration);

				pZone->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);
			}

			// 크리티컬 히트라면 상대방을 뒤로 물러나게 한다.
			if (bCriticalHit)
			{
				knockbackCreature(pZone, pTargetCreature, pSlayer->getX(), pSlayer->getY());
			}

			if(!pTargetCreature->isSlayer() ) 
			{
				if (bIncreaseExp)
				{
					shareAttrExp(pSlayer, Damage , 1, 8, 1, _GCAttackArmsOK1);
					increaseDomainExp(pSlayer, DomainType, pSkillInfo->getPoint(), _GCAttackArmsOK1, pTargetCreature->getLevel());
					increaseSkillExp(pSlayer, DomainType,  pSkillSlot, pSkillInfo, _GCAttackArmsOK1);
				}

				increaseAlignment(pSlayer, pTargetCreature, _GCAttackArmsOK1);
			}
			//}

			if (pTargetCreature->isPC()) 
			{
				Player* pTargetPlayer = pTargetCreature->getPlayer();
				if (pTargetPlayer != NULL) 
				{ 
					_GCAttackArmsOK2.setObjectID(getSkillType());
					_GCAttackArmsOK2.setObjectID(pSlayer->getObjectID());
					pTargetPlayer->sendPacket(&_GCAttackArmsOK2);
				}
			} 
			else if (pTargetCreature->isMonster())
			{
				Monster* pMonster = dynamic_cast<Monster*>(pTargetCreature);
				pMonster->addEnemy(pSlayer);
			}

			// 공격자와 상대의 아이템 내구성 떨어트림.
			decreaseDurability(pSlayer, pTargetCreature, NULL, &_GCAttackArmsOK1, &_GCAttackArmsOK2);

			ZoneCoord_t targetX = pTargetCreature->getX();
			ZoneCoord_t targetY = pTargetCreature->getY();
			ZoneCoord_t myX     = pSlayer->getX();
			ZoneCoord_t myY     = pSlayer->getY();
			
			_GCAttackArmsOK1.setSkillType(getSkillType());
			_GCAttackArmsOK1.setObjectID(TargetObjectID);
			_GCAttackArmsOK1.setBulletNum(RemainBullet);

			_GCAttackArmsOK3.setSkillType(getSkillType());
			_GCAttackArmsOK3.setObjectID(pSlayer->getObjectID());
			_GCAttackArmsOK3.setTargetXY (targetX, targetY);
			
			_GCAttackArmsOK4.setSkillType(getSkillType());
			_GCAttackArmsOK4.setTargetObjectID (TargetObjectID);
			
			_GCAttackArmsOK5.setSkillType(getSkillType());
			_GCAttackArmsOK5.setObjectID(pSlayer->getObjectID());
			_GCAttackArmsOK5.setTargetObjectID (TargetObjectID);

			pPlayer->sendPacket(&_GCAttackArmsOK1);

			list<Creature *> cList;
			cList.push_back(pTargetCreature);
			cList.push_back(pSlayer);
			cList = pZone->broadcastSkillPacket(myX, myY, targetX, targetY, &_GCAttackArmsOK5, cList);
			pZone->broadcastPacket(myX, myY, &_GCAttackArmsOK3, cList);
			pZone->broadcastPacket(targetX, targetY, &_GCAttackArmsOK4, cList);

			pSkillSlot->setRunTime(output.Delay);
		} 
		else 
		{
			executeSkillFailNormalWithGun(pSlayer, getSkillType(), pTargetCreature, RemainBullet);
			//cout << pSlayer->getName().c_str() << " Fail : " 
			//	<< (int)bManaCheck << (int)bTimeCheck << (int)bRangeCheck 
			//	<< (int)bBulletCheck << (int)bHitRoll << (int)bPK << endl;
		}
	} 
	catch (Throwable &t) 
	{
		executeSkillFailException(pSlayer, getSkillType());
	}

	//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << " End" << endl;

	__END_DEBUG __END_CATCH
}

void JabbingVein::execute(Monster* pMonster, Creature* pEnemy) 
	throw(Error)
{
	__BEGIN_TRY __BEGIN_DEBUG

	//cout << "JabbingVein::executeMonster" << endl;

	try 
	{
		BYTE RemainBullet = 0;

		Zone* pZone = pMonster->getZone();
		Assert(pZone != NULL);

		// NoSuch제거. by sigi. 2002.5.2
		if (pEnemy==NULL
			|| pEnemy->isNPC()) 
		{
			//cout << "WrongEnemy" << endl;
			executeSkillFailNormalWithGun(pMonster, getSkillType(), pEnemy, RemainBullet);
			return;
		}

		GCAttackArmsOK2 _GCAttackArmsOK2;
		GCAttackArmsOK3 _GCAttackArmsOK3;
		GCAttackArmsOK4 _GCAttackArmsOK4;
		GCAttackArmsOK5 _GCAttackArmsOK5;

		SkillType_t       SkillType  = getSkillType();
		SkillInfo*        pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);

		SkillInput input(pMonster);
		SkillOutput output;
		computeOutput(input, output);

		// 페널티 값을 계산한다.
		int ToHitPenalty = getPercentValue(pMonster->getToHit(), output.ToHit); 

		bool bRangeCheck  = verifyDistance(pMonster, pEnemy, pSkillInfo->getRange());
		bool bHitRoll     = HitRoll::isSuccess(pMonster, pEnemy, ToHitPenalty);

		if (bRangeCheck && bHitRoll)
		{
			_GCAttackArmsOK5.setSkillSuccess(true);

			ObjectID_t TargetObjectID = pEnemy->getObjectID();

			bool bCriticalHit = false;

			// 데미지를 계산하고, quickfire 페널티를 가한다.
			// output.Damage가 음수이기 때문에, %값을 구해 더하면 결국 빼는 것이 된다.
			int Damage = computeDamage(pMonster, pEnemy, 0, bCriticalHit);
			Damage += getPercentValue(Damage, output.Damage);
			Damage = max(0, Damage);

			//cout << "JabbingVeinDamage:" << Damage << endl;

			// 데미지를 세팅한다.
			setDamage(pEnemy, Damage, pMonster, SkillType, &_GCAttackArmsOK2);
			//computeAlignmentChange(pEnemy, Damage, pMonster, &_GCAttackArmsOK2, &_GCAttackArmsOK1);

			if (!pEnemy->isFlag(Effect::EFFECT_CLASS_JABBING_VEIN) && rand()%100 <= output.Range )
			{
				EffectJabbingVein* pEffect = new EffectJabbingVein(pEnemy);
				pEffect->setDeadline(output.Duration);
				pEnemy->addEffect(pEffect);
				pEnemy->setFlag(pEffect->getEffectClass());

				GCAddEffect gcAddEffect;
				gcAddEffect.setObjectID(pEnemy->getObjectID());
				gcAddEffect.setEffectID(pEffect->getSendEffectClass());
				gcAddEffect.setDuration(output.Duration);

				pZone->broadcastPacket(pEnemy->getX(), pEnemy->getY(), &gcAddEffect);
			}

			// 크리티컬 히트라면 상대방을 뒤로 물러나게 한다.
			if (bCriticalHit)
			{
				knockbackCreature(pZone, pEnemy, pMonster->getX(), pMonster->getY());
			}

			if (pEnemy->isPC()) 
			{
				Player* pTargetPlayer = pEnemy->getPlayer();
				if (pTargetPlayer != NULL) 
				{ 
					_GCAttackArmsOK2.setObjectID(pMonster->getObjectID());
					pTargetPlayer->sendPacket(&_GCAttackArmsOK2);
				}
			} 
			else if (pEnemy->isMonster())
			{
				Monster* pOtherMonster = dynamic_cast<Monster*>(pEnemy);
				pOtherMonster->addEnemy(pMonster);
			}

			// 공격자와 상대의 아이템 내구성 떨어트림.
			decreaseDurability(pMonster, pEnemy, NULL, NULL, &_GCAttackArmsOK2);

			ZoneCoord_t targetX = pEnemy->getX();
			ZoneCoord_t targetY = pEnemy->getY();
			ZoneCoord_t myX     = pMonster->getX();
			ZoneCoord_t myY     = pMonster->getY();
			
			_GCAttackArmsOK3.setObjectID(pMonster->getObjectID());
			_GCAttackArmsOK3.setTargetXY (targetX, targetY);
			
			_GCAttackArmsOK4.setTargetObjectID (TargetObjectID);
			
			_GCAttackArmsOK5.setObjectID(pMonster->getObjectID());
			_GCAttackArmsOK5.setTargetObjectID (TargetObjectID);

			list<Creature *> cList;
			cList.push_back(pEnemy);
			cList.push_back(pMonster);
			cList = pZone->broadcastSkillPacket(myX, myY, targetX, targetY, &_GCAttackArmsOK5, cList);
			pZone->broadcastPacket(myX, myY, &_GCAttackArmsOK3, cList);
			pZone->broadcastPacket(targetX, targetY, &_GCAttackArmsOK4, cList);

			//pSkillSlot->setRunTime(output.Delay);
		} 
		else 
		{
			//cout << "Failed: " << (int)bRangeCheck << ", " << (int)bHitRoll << endl;
			executeSkillFailNormalWithGun(pMonster, getSkillType(), pEnemy, RemainBullet);
		}
	} 
	catch (Throwable &t) 
	{
		//cout << t.toString().c_str() << endl;
		executeSkillFailException(pMonster, getSkillType());
	}


	//cout << "JabbingVein::executeMonster OK" << endl;

	__END_DEBUG __END_CATCH
}

JabbingVein g_JabbingVein;
