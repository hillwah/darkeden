//////////////////////////////////////////////////////////////////////////////
// Filename    : SkillUtil.cpp
// Written by  : excel96
// Description :
//////////////////////////////////////////////////////////////////////////////

#include "SkillUtil.h"
#include "SkillInfo.h"
#include "Player.h"
#include "Monster.h"
//#include "AttrBalanceInfo.h"
#include "VampEXPInfo.h"
#include "OustersEXPInfo.h"
#include "SkillDomainInfoManager.h"
#include "Party.h"
#include "AlignmentManager.h"
#include "ItemUtil.h"
#include "ZoneUtil.h"
#include "CreatureUtil.h"
#include "Zone.h"
#include "PrecedenceTable.h"
//#include "LogClient.h"
#include "Thread.h"
#include "HitRoll.h"
#include "GamePlayer.h"
#include "MasterLairInfoManager.h"
#include "PacketUtil.h"
#include "DynamicZone.h"
#include "billing/BillingInfo.h"
#include <stdio.h>
#include <math.h>

#include "ItemFactoryManager.h"
#include "EventItemUtil.h"
#include "BalloonHeadbandUtil.h"

#include "EffectAuraShield.h"
#include "EffectAlignmentRecovery.h"
#include "EffectEnemyErase.h"
#include "EffectPrecedence.h"
#include "EffectRevealer.h"
#include "EffectMephisto.h"
#include "EffectHymn.h"
#include "EffectGrandMasterSlayer.h"
#include "EffectGrandMasterVampire.h"
#include "EffectGrandMasterOusters.h"
#include "EffectSharpShield.h"
#include "EffectAirShield.h"
#include "EffectArmageddon.h"
#include "EffectSleep.h"
#include "EffectRequital.h"
#include "EffectHandsOfFire.h"
#include "EffectWaterBarrier.h"
#include "EffectIceOfSoulStone.h"
#include "EffectBlockHead.h"
#include "EffectDivineSpirits.h"
#include "EffectRediance.h"
#include "EffectExpansion.h"
#include "EffectStoneSkin.h"
#include "EffectFrozenArmor.h"
#include "EffectIceFieldToCreature.h"
#include "EffectExplosionWater.h"
#include "EffectStriking.h"
#include "EffectBlindness.h"
#include "EffectShareHP.h"

#include "EffectCanEnterGDRLair.h"
#include "EffectSwordOfThor.h"
#include "EffectInstallTurret.h"
#include "EffectReactiveArmor.h"

#include "VariableManager.h"
#include "SkillPropertyManager.h"
#include "PKZoneInfoManager.h"
#include "GQuestManager.h"
#include "GDRLairManager.h"

#include "SummonGroundElemental.h"

#include "EventHeadCount.h"

#include "Properties.h"
#include "GameServerInfoManager.h"

#include "DB.h"

#include "mission/QuestManager.h"
#include "mission/MonsterKillQuestStatus.h"
#include "mission/EventQuestLootingManager.h"

#include "Gpackets/GCLearnSkillReady.h"
#include "Gpackets/GCRemoveFromGear.h"
#include "Gpackets/GCStatusCurrentHP.h"
#include "Gpackets/GCRemoveEffect.h"
#include "Gpackets/GCAddEffect.h"
#include "Gpackets/GCAddEffectToTile.h"
#include "Gpackets/GCAddInjuriousCreature.h"
#include "Gpackets/GCSkillFailed1.h"
#include "Gpackets/GCSkillFailed2.h"
#include "Gpackets/GCSystemMessage.h"
#include "Gpackets/GCOtherModifyInfo.h"
#include "Gpackets/GCKickMessage.h"
#include "Gpackets/GCSkillToObjectOK4.h"
#include "Gpackets/GCSkillToObjectOK6.h"

//////////////////////////////////////////////////////////////////////////////
// data structure & helper functions
//////////////////////////////////////////////////////////////////////////////
typedef struct DomainStruct {
    int DomainType;
    int DomainLevel;
} SDomain;

class isBig {
public:
	isBig() {};

	bool operator () (const DomainStruct &t, const DomainStruct &b) {
		if (t.DomainLevel > b.DomainLevel)
			return true;
		return false;
	}
};

void checkFreeLevelLimit(PlayerCreature* pPC) throw(Error) {
	__BEGIN_TRY
//	static const char MsgLevelLimitOver[] = "무료로 사용할 수 있는 레벨 제한에 도달해서";

	// by sigi. 2002.11.19
	// 유료 사용자가 아니거나
	// 무료 사용기간이 남아있지 않으면(혹은 능력치 over) 짜른다.
	// 애드빌 빌링 시스템은 사용하지 않고 사용자 제한은 하는 경우. by sigi. 2003.2.21
#if defined(__PAY_SYSTEM_FREE_LIMIT__)

	if (!pPC->isPayPlayAvaiable()) {
		GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(pPC->getPlayer());

		pGamePlayer->kickPlayer(30, KICK_MESSAGE_EXPIRE_FREEPLAY);

		//pGamePlayer->setPenaltyFlag(PENALTY_TYPE_KICKED);
/*		EventKick* pEventKick = new EventKick(pGamePlayer);
		pEventKick->setDeadline(30*10);
		pGamePlayer->addEvent(pEventKick);
		// 몇 초후에 짤린다..고 보내준다.
		//pEventKick->setMessage(MsgLevelLimitOver);
		//pEventKick->sendMessage();

		// 몇 초후에 짤린다..고 보내준다.
		GCKickMessage gcKickMessage;
		gcKickMessage.setType(KICK_MESSAGE_EXPIRE_FREEPLAY);
		gcKickMessage.setSeconds(30);
		pGamePlayer->sendPacket(&gcKickMessage);
*/
	}
#endif

	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
// 유료화존 경험치 뽀나스
//////////////////////////////////////////////////////////////////////////////
//const uint g_pVariableManager->getPremiumExpBonusPercent() = 150;


//////////////////////////////////////////////////////////////////////////////
// 공격자와 피공격자 사이의 파라미터를 계산해 최종 데미지를 산출한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computeDamage(Creature* pCreature, Creature* pTargetCreature) {
	Assert(pCreature != NULL);
	Assert(pTargetCreature != NULL);

	Damage_t Damage       = 0;
	bool     bCriticalHit = false;

	try {
		if (pCreature->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);
			Assert(pSlayer != NULL);
			Damage = computeSlayerDamage(pSlayer, pTargetCreature, bCriticalHit);
		} else if (pCreature->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire *>(pCreature);
			Assert(pVampire != NULL);
			Damage = computeVampireDamage(pVampire, pTargetCreature, bCriticalHit);
		} else if (pCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);
			Assert(pOusters != NULL);
			Damage = computeOustersDamage(pOusters, pTargetCreature, bCriticalHit);
		} else if (pCreature->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster *>(pCreature);
			Assert(pMonster != NULL);
			Damage = computeMonsterDamage(pMonster, pTargetCreature, bCriticalHit);
		}
		else
			// NPC라는 말인가...
			return 0;
	} catch (Throwable & t) { 
		cerr << t.toString() << endl; 
	}

	return Damage;
}

//////////////////////////////////////////////////////////////////////////////
// 공격자의 순수 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computePureDamage(Creature* pCreature) {
	Damage_t Damage = 0;
	
	if (pCreature == NULL)
		return Damage;

	if (pCreature->isSlayer()) {
		Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);
		Assert(pSlayer != NULL);
		Damage = computePureSlayerDamage(pSlayer);
	} else if (pCreature->isVampire()) {
		Vampire* pVampire = dynamic_cast<Vampire *>(pCreature);
		Assert(pVampire != NULL);
		Damage = computePureVampireDamage(pVampire);
	} else if (pCreature->isOusters()) {
		Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);
		Assert(pOusters != NULL);
		Damage = computePureOustersDamage(pOusters);
	} else if (pCreature->isMonster()) {
		Monster* pMonster = dynamic_cast<Monster *>(pCreature);
		Assert(pMonster != NULL);
		Damage = computePureMonsterDamage(pMonster);
	}
	else
		// NPC라는 말인가...
		return 0;

	return Damage;
}

//////////////////////////////////////////////////////////////////////////////
// 공격자와 피공격자 사이의 파라미터를 계산해 최종 데미지를 산출한다.
// 위의 함수와 같으나, 이 함수를 부를 경우에는 내부적으로 크리티컬 
// 히트와 관련된 부분이 처리된다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computeDamage(Creature* pCreature, Creature* pTargetCreature, int CriticalBonus, bool& bCritical) {
	Assert(pCreature != NULL);
	Assert(pTargetCreature != NULL);

	Damage_t Damage       = 0;
	bool     bCriticalHit = HitRoll::isCriticalHit(pCreature, CriticalBonus);

	try {
		if (pCreature->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);
			Assert(pSlayer != NULL);

			Damage = computeSlayerDamage(pSlayer, pTargetCreature, bCriticalHit);
		} else if (pCreature->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire *>(pCreature);
			Assert(pVampire != NULL);
			Damage = computeVampireDamage(pVampire, pTargetCreature, bCriticalHit);
		} else if (pCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters *>(pCreature);
			Assert(pOusters != NULL);
			Damage = computeOustersDamage(pOusters, pTargetCreature, bCriticalHit);
		} else if (pCreature->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster *>(pCreature);
			Assert(pMonster != NULL);
			Damage = computeMonsterDamage(pMonster, pTargetCreature, bCriticalHit);
		}
		else
			// NPC라는 말인가...
			return 0;
	} catch (Throwable & t) { 
		cerr << t.toString() << endl; 
	}

	bCritical = bCriticalHit;

	// 크리티컬 히트이고, 맞는 놈이 몬스터라면 150%의 데미지를 주게 된다. 
	if (bCritical && pTargetCreature->isMonster())
		Damage = getPercentValue(Damage, 150);

	return Damage;
}


//////////////////////////////////////////////////////////////////////////////
// 원래 데미지에서 프로텍션을 제외한 최종 데미지를 리턴한다.
//////////////////////////////////////////////////////////////////////////////
double computeFinalDamage(Damage_t minDamage, Damage_t maxDamage, Damage_t realDamage, Protection_t Protection, bool bCritical) {
	// 크리티컬 히트라면 프로텍션을 고려하지 않고, 데미지 그대로 들어간다.
	if (bCritical)
		return realDamage;

	if (Protection > 640)
		Protection = 640;

	Damage_t FinalDamage;
	FinalDamage = realDamage - (realDamage * (Protection / 8)) / 100;
	//FinalDamage = realDamage - (realDamage * (Protection / 10)) / 100;

	return max(1, (int)FinalDamage);
/*
	Damage_t avgDamage   = (minDamage + maxDamage) >> 1;
	int      DamageRatio = 100;

	if (Protection < avgDamage)
	{
		DamageRatio = 100;
	}
	else if (Protection < getPercentValue(avgDamage, 150))
	{
		DamageRatio = 90;
	}
	else if (Protection < getPercentValue(avgDamage, 200))
	{
		DamageRatio = 80;
	}
	else if (Protection < getPercentValue(avgDamage, 250))
	{
		DamageRatio = 70;
	}
	else if (Protection < getPercentValue(avgDamage, 300))
	{
		DamageRatio = 60;
	}
	else
	{
		DamageRatio = 50;
	}

	return max(1, getPercentValue(realDamage, DamageRatio));
*/
}


//////////////////////////////////////////////////////////////////////////////
// 슬레이어 공격자와 피공격자 사이의 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computeSlayerDamage(Slayer* pSlayer, Creature* pTargetCreature, bool bCritical) {
	Assert(pSlayer != NULL);
	Assert(pTargetCreature != NULL);

	Item*    pItem       = pSlayer->getWearItem(Slayer::WEAR_RIGHTHAND);
	uint     timeband    = getZoneTimeband(pSlayer->getZone());
	double   FinalDamage = 0;

	// 일단 맨손의 데미지를 받아온다.
	Damage_t MinDamage  = pSlayer->getDamage(ATTR_CURRENT);
	Damage_t MaxDamage  = pSlayer->getDamage(ATTR_MAX);

	// 무기를 들고 있다면, min, max에 무기의 min, max를 계산해 준다.
	if (pItem != NULL && pSlayer->isRealWearingEx(Slayer::WEAR_RIGHTHAND)) {
		// 스트라이킹 데미지를 계산하는 부분을 Slayer::initAllStat() 부분으로
		// 옮기면서 그곳에서 m_Damage[]를 세팅해 버리기 때문에, 여기서 더할 
		// 필요가 없어졌다. -- 2002.01.17 김성민
		//MinDamage += (pItem->getMinDamage() + pItem->getBonusDamage());
		//MaxDamage += (pItem->getMaxDamage() + pItem->getBonusDamage());
		MinDamage += pItem->getMinDamage();
		MaxDamage += pItem->getMaxDamage();
	}

	// 실제 랜덤 데미지를 계산한다.
	Damage_t RealDamage = max(1, Random(MinDamage, MaxDamage));

	if (pTargetCreature->isSlayer()) {
		Slayer* pTargetSlayer = dynamic_cast<Slayer*>(pTargetCreature);
		Assert(pTargetSlayer != NULL);

		Protection_t Protection = pTargetSlayer->getProtection();

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isVampire()) {
		Vampire* pTargetVampire = dynamic_cast<Vampire*>(pTargetCreature);
		Assert(pTargetVampire != NULL);

		Protection_t Protection = pTargetVampire->getProtection();
		Protection = (Protection_t)getPercentValue(Protection, VampireTimebandFactor[timeband]);

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isOusters()) {
		Ousters* pTargetOusters = dynamic_cast<Ousters*>(pTargetCreature);
		Assert(pTargetOusters != NULL);

		Protection_t Protection = pTargetOusters->getProtection();

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isMonster()) {
		Monster* pTargetMonster = dynamic_cast<Monster*>(pTargetCreature);
		Assert(pTargetMonster != NULL);

		Protection_t Protection = pTargetMonster->getProtection();
		Protection = (Protection_t)getPercentValue(Protection, MonsterTimebandFactor[timeband]);

#ifdef __UNDERWORLD__
		if (pTargetMonster->isUnderworld() || pTargetMonster->getMonsterType() == 599)
			Protection = pTargetMonster->getProtection();
#endif
		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	}
	else
		// NPC라는 말인가...
		return 0;

	// AbilityBalance.cpp에서 한다.
	//FinalDamage += g_pVariableManager->getCombatSlayerDamageBonus();

	return (Damage_t)FinalDamage;
}


//////////////////////////////////////////////////////////////////////////////
// 뱀파이어 공격자와 피공격자 사이의 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computeVampireDamage(Vampire* pVampire, Creature* pTargetCreature, bool bCritical) {
	Assert(pVampire != NULL);
	Assert(pTargetCreature != NULL);

	double 	 FinalDamage = 0;
	Damage_t MinDamage   = pVampire->getDamage(ATTR_CURRENT);
	Damage_t MaxDamage   = pVampire->getDamage(ATTR_MAX);
	uint     timeband    = getZoneTimeband(pVampire->getZone());

	// vampire 무기에 의한 데미지
	Item*    pItem       = pVampire->getWearItem(Vampire::WEAR_RIGHTHAND);

	// 무기를 들고 있다면, min, max에 무기의 min, max를 계산해 준다.
	if (pItem != NULL && pVampire->isRealWearingEx(Vampire::WEAR_RIGHTHAND)) {
		MinDamage += pItem->getMinDamage();
		MaxDamage += pItem->getMaxDamage();
	}

	// 실제 랜덤 데미지를 계산한다.
	Damage_t RealDamage = max(1, Random(MinDamage, MaxDamage));

	RealDamage = (Damage_t)getPercentValue(RealDamage, VampireTimebandFactor[timeband]);

	if (pTargetCreature->isSlayer()) {
		Slayer* pTargetSlayer = dynamic_cast<Slayer*>(pTargetCreature);
		Assert(pTargetSlayer != NULL);

		Protection_t Protection = pTargetSlayer->getProtection();

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isVampire()) {
		Vampire* pTargetVampire = dynamic_cast<Vampire*>(pTargetCreature);
		Assert(pTargetVampire != NULL);

		Protection_t Protection = pTargetVampire->getProtection();
		Protection = (Protection_t)getPercentValue(Protection, VampireTimebandFactor[timeband]);

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isOusters()) {
		Ousters* pTargetOusters = dynamic_cast<Ousters*>(pTargetCreature);
		Assert(pTargetOusters != NULL);

		Protection_t Protection = pTargetOusters->getProtection();

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isMonster()) {
		Monster* pTargetMonster = dynamic_cast<Monster*>(pTargetCreature);
		Assert(pTargetMonster != NULL);

		Protection_t Protection = pTargetMonster->getProtection();
		Protection = (Protection_t)getPercentValue(Protection, MonsterTimebandFactor[timeband]);

#ifdef __UNDERWORLD__
		if (pTargetMonster->isUnderworld() || pTargetMonster->getMonsterType() == 599)
			Protection = pTargetMonster->getProtection();
#endif
		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	}
	else
		// NPC라는 말인가...
		return 0;

	// AbilityBalance.cpp에서 한다.
	//FinalDamage += g_pVariableManager->getCombatVampireDamageBonus();

	return (Damage_t)FinalDamage;
}


//////////////////////////////////////////////////////////////////////////////
// 아우스터스 공격자와 피공격자 사이의 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computeOustersDamage(Ousters* pOusters, Creature* pTargetCreature, bool bCritical) {
	Assert(pOusters != NULL);
	Assert(pTargetCreature != NULL);

	double 	 FinalDamage = 0;
	Damage_t MinDamage   = pOusters->getDamage(ATTR_CURRENT);
	Damage_t MaxDamage   = pOusters->getDamage(ATTR_MAX);
	uint     timeband    = getZoneTimeband(pOusters->getZone());

	// Ousters 무기에 의한 데미지
	Item*    pItem       = pOusters->getWearItem(Ousters::WEAR_RIGHTHAND);

	// 무기를 들고 있다면, min, max에 무기의 min, max를 계산해 준다.
	if (pItem != NULL && pOusters->isRealWearingEx(Ousters::WEAR_RIGHTHAND)) {
		MinDamage += pItem->getMinDamage();
		MaxDamage += pItem->getMaxDamage();
	}

	// 실제 랜덤 데미지를 계산한다.
	Damage_t RealDamage = max(1, Random(MinDamage, MaxDamage));

	if (pTargetCreature->isSlayer()) {
		Slayer* pTargetSlayer = dynamic_cast<Slayer*>(pTargetCreature);
		Assert(pTargetSlayer != NULL);

		Protection_t Protection = pTargetSlayer->getProtection();

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isVampire()) {
		Vampire* pTargetVampire = dynamic_cast<Vampire*>(pTargetCreature);
		Assert(pTargetVampire != NULL);

		Protection_t Protection = pTargetVampire->getProtection();
		Protection = (Protection_t)getPercentValue(Protection, VampireTimebandFactor[timeband]);

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isOusters()) {
		Ousters* pTargetOusters = dynamic_cast<Ousters*>(pTargetCreature);
		Assert(pTargetOusters != NULL);

		Protection_t Protection = pTargetOusters->getProtection();

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isMonster()) {
		Monster* pTargetMonster = dynamic_cast<Monster*>(pTargetCreature);
		Assert(pTargetMonster != NULL);

		Protection_t Protection = pTargetMonster->getProtection();
		Protection = (Protection_t)getPercentValue(Protection, MonsterTimebandFactor[timeband]);

#ifdef __UNDERWORLD__
		if (pTargetMonster->isUnderworld() || pTargetMonster->getMonsterType() == 599)
			Protection = pTargetMonster->getProtection();
#endif

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	}
	else
		// NPC라는 말인가...
		return 0;

	// AbilityBalance.cpp에서 한다.
	//FinalDamage += g_pVariableManager->getCombatOustersDamageBonus();

	return (Damage_t)FinalDamage;
}


//////////////////////////////////////////////////////////////////////////////
// 몬스터 공격자와 피공격자 사이의 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computeMonsterDamage(Monster* pMonster, Creature* pTargetCreature, bool bCritical) {
	Assert(pMonster != NULL);
	Assert(pTargetCreature != NULL);

	double   FinalDamage = 0;
	Damage_t MinDamage   = pMonster->getDamage(ATTR_CURRENT);
	Damage_t MaxDamage   = pMonster->getDamage(ATTR_MAX);
	Damage_t RealDamage  = max(1, Random(MinDamage, MaxDamage));
	uint     timeband    = getZoneTimeband(pMonster->getZone());

	RealDamage = (Damage_t)getPercentValue(RealDamage, MonsterTimebandFactor[timeband]);

	if (pTargetCreature->isSlayer()) {
		Slayer* pTargetSlayer = dynamic_cast<Slayer*>(pTargetCreature);
		Assert(pTargetSlayer != NULL);

		Protection_t Protection = pTargetSlayer->getProtection();

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isVampire()) {
		Vampire* pTargetVampire = dynamic_cast<Vampire*>(pTargetCreature);
		Assert(pTargetVampire != NULL);

		Protection_t Protection = pTargetVampire->getProtection();
		Protection = (Protection_t)getPercentValue(Protection, VampireTimebandFactor[timeband]);

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isOusters()) {
		Ousters* pTargetOusters = dynamic_cast<Ousters*>(pTargetCreature);
		Assert(pTargetOusters != NULL);

		Protection_t Protection = pTargetOusters->getProtection();

		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	} else if (pTargetCreature->isMonster()) {
		Monster* pTargetMonster = dynamic_cast<Monster*>(pTargetCreature);
		Assert(pTargetMonster != NULL);

		Protection_t Protection = pTargetMonster->getProtection();
		Protection = (Protection_t)getPercentValue(Protection, MonsterTimebandFactor[timeband]);

#ifdef __UNDERWORLD__
		if (pTargetMonster->isUnderworld() || pTargetMonster->getMonsterType() == 599)
			Protection = pTargetMonster->getProtection();
#endif
		FinalDamage = computeFinalDamage(MinDamage, MaxDamage, RealDamage, Protection, bCritical);
	}
	else
		// NPC라는 말인가?
		return 0;

	return (Damage_t)FinalDamage;
}

//////////////////////////////////////////////////////////////////////////////
// resistance를 고려한 마법 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computeMagicDamage(Creature* pTargetCreature, int Damage, SkillType_t SkillType, bool bVampire, Creature* pAttacker) {
	Assert(pTargetCreature != NULL);

	SkillInfo* pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);
	Assert(pSkillInfo != NULL);

	int MagicDomain = pSkillInfo->getMagicDomain();
	int MagicLevel  = pSkillInfo->getLevel();

	int Resist = pTargetCreature->getResist(MagicDomain);

	if (pAttacker != NULL && pAttacker->isVampire()) {
//		cout << "before mastery " << Resist << endl;
		Vampire* pVampire = dynamic_cast<Vampire*>(pAttacker);
		if (pVampire != NULL) {
			if (pSkillInfo->getMagicDomain() >= MAGIC_DOMAIN_POISON && pSkillInfo->getMagicDomain() <= MAGIC_DOMAIN_BLOOD) {
				int mastery = pVampire->getMastery(pSkillInfo->getMagicDomain());
				Resist = getPercentValue(Resist, 100-mastery);
			}
		}
//		cout << "after mastery " << Resist << endl;
	}

#ifdef __CHINA_SERVER__
	if (bVampire)
		Resist = (int)(Resist / 1.2);

	// 저항력에 따라 데미지를 가감한다.
	int penalty = (int)(MagicLevel/5.0 - Resist);
	penalty = min(penalty, 100);
	penalty = max(penalty, -100);

	Damage = getPercentValue(Damage, 100 + penalty);
	return (Damage_t)max(0, (int)Damage);
#else
	float penalty = 1.5 * (Resist - (MagicLevel/5.0)) / (Resist - (MagicLevel/5.0) + 100.0);
	penalty = max(penalty, -0.2f);
	Damage = (int)(Damage * (1.0 - penalty));

	return (Damage_t)max(0, (int)Damage);
#endif
}

//////////////////////////////////////////////////////////////////////////////
// 리스틀릿을 고려한 아우스터즈 마법 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computeOustersMagicDamage(Ousters* pOusters, Creature* pTargetCreature, int Damage, SkillType_t SkillType) {
	Assert(pOusters != NULL);

	Item* pWeapon = pOusters->getWearItem(Ousters::WEAR_RIGHTHAND);
	if (pWeapon == NULL || pWeapon->getItemClass() != Item::ITEM_CLASS_OUSTERS_WRISTLET)
		return 0;

	Damage_t MinDamage = pWeapon->getMinDamage();
	Damage_t MaxDamage = pWeapon->getMaxDamage();

	Damage_t RealDamage = max(1, Random(MinDamage, MaxDamage));
	return RealDamage + Damage;
}

//////////////////////////////////////////////////////////////////////////////
// 타겟에게 미치는 은 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computeSlayerSilverDamage(Creature* pCreature, int Damage, ModifyInfo* pMI) {
	Assert(pCreature != NULL);

	// 슬레이어가 아니라면 은 데미지가 나올 이유가 없다.
	if (pCreature->isSlayer() == false)
		return 0;

	Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);
	Assert(pSlayer != NULL);

	// 무기가 있는지 검사하고, 없다면 0을 리턴한다.
	Item* pWeapon = pSlayer->getWearItem(Slayer::WEAR_RIGHTHAND);
	if (pWeapon == NULL)
		return 0;

	Damage_t silverDamage = 0;

	if (isMeleeWeapon(pWeapon)) {
		// 성직자 무기일 경우에는, 기본적으로 10%의 은 데미지를 준다.
		if (isClericWeapon(pWeapon)) {
			silverDamage = max(1, (int)(Damage*0.1));
			silverDamage = min((int)silverDamage, (int)pWeapon->getSilver());

			if (silverDamage > 0) {
				pWeapon->setSilver(pWeapon->getSilver() - silverDamage);
				//pWeapon->save(pSlayer->getName(), STORAGE_GEAR, 0, Slayer::WEAR_RIGHTHAND, 0);

				if (pMI != NULL) pMI->addShortData(MODIFY_SILVER_DURABILITY, pWeapon->getSilver());
			}

			// 기본으로 들어가는 10%의 은 데미지
			silverDamage += max(1, (int)(Damage*0.1));
		}
		// 성직자 무기가 아닐 경우에는, 은 도금을 했을 때만 은 데미지를 준다.
		else {
			silverDamage = max(1, (int)(Damage*0.1));
			silverDamage = min((int)silverDamage, (int)pWeapon->getSilver());

			if (silverDamage > 0) {
				pWeapon->setSilver(pWeapon->getSilver() - silverDamage);
				//pWeapon->save(pSlayer->getName(), STORAGE_GEAR, 0, Slayer::WEAR_RIGHTHAND, 0);

				if (pMI != NULL) pMI->addShortData(MODIFY_SILVER_DURABILITY, pWeapon->getSilver());
			}
		}
	} else if (isArmsWeapon(pWeapon) && pWeapon->getSilver() > 0) {
		// 총 계열의 무기라면, 은 탄알이 나가는 것이므로,
		// 무기 자체의 은을 줄이면 안 된다. 이것은 외부에서,
		// 즉 총알을 줄이는 부분에서 처리하기로 한다.
		silverDamage = max(1, (int)(Damage*0.1));
	}

	return silverDamage;
}

void computeCriticalBonus(Ousters* pOusters, SkillType_t skillType, Damage_t& Damage, bool& bCriticalHit) {
	switch (skillType) {
		case SKILL_KASAS_ARROW:
		case SKILL_BLAZE_BOLT:
		case SKILL_EARTHS_TEETH:
		case SKILL_HANDS_OF_NIZIE:
		case SKILL_STONE_AUGER:
		case SKILL_EMISSION_WATER:
		case SKILL_BEAT_HEAD:
		case SKILL_MAGNUM_SPEAR:
		case SKILL_FIRE_PIERCING:
		case SKILL_SUMMON_FIRE_ELEMENTAL:
		case SKILL_ICE_LANCE:
		case SKILL_EXPLOSION_WATER:
		case SKILL_METEOR_STORM: {
			OustersSkillSlot* pSkillSlot = pOusters->hasSkill(SKILL_CRITICAL_MAGIC);
			if (pSkillSlot == NULL) return;

			SkillLevel_t level = pSkillSlot->getExpLevel();

			int Ratio = 5 + (level / 3);
			if ((rand()%100) < Ratio) {
				bCriticalHit = true;
				if (level <= 15)
					Damage += (Damage_t)(Damage * level / 19.5);
				else {
					int bonus = (Damage_t)(Damage * (0.1 + (level / 75.0)));
					if (level == 30) bonus = (Damage_t)(bonus * 1.1);

					Damage += bonus;
				}
			}
		}

		default:
			return;
	}
}

RankExp_t computeRankExp(int myLevel, int otherLevel) { // by sigi. 2002.12.31
	RankExp_t rankExp = 0;

	if (myLevel!=0 && otherLevel!=0) {
		int checkValue = otherLevel * 100 / myLevel;

		if (checkValue > 120)
			rankExp = (otherLevel-myLevel)*(otherLevel-myLevel)/10 + otherLevel*100/myLevel;
		else
			rankExp = otherLevel*100/myLevel - 10;

		rankExp = max(0, (int)rankExp);

		// by sigi. 2002.12.31
		rankExp = getPercentValue(rankExp, g_pVariableManager->getVariable(RANK_EXP_GAIN_PERCENT));
	}

	rankExp = getPercentValue(rankExp, g_pVariableManager->getPremiumExpBonusPercent());

	return rankExp;
}

//////////////////////////////////////////////////////////////////////////////
// Creature를 죽였을때의 효과
//
// 죽은 사람에게 KillCount를 증가시켜준다. --> 계급 경험치
// by sigi. 2002.8.31
//////////////////////////////////////////////////////////////////////////////
void affectKillCount(Creature* pAttacker, Creature* pDeadCreature) {
	// [처리할 필요 없는 경우]
	// 공격한 사람이 없거나
	// 죽은애가 없거나
	// 공격한 사람이 사람이 아니거나 -_-;
	// 죽은애가 죽은게 아니면 -_-; 
	if (pAttacker==NULL ||
		pDeadCreature==NULL ||
		!pAttacker->isPC() ||
		pDeadCreature->isAlive())
		return;

#ifdef __UNDERWORLD__
	if (pDeadCreature->isMonster()) {
		Monster* pMonster = dynamic_cast<Monster*>(pDeadCreature);
		Assert(pMonster != NULL);

		if (pMonster->isUnderworld()) {
			pMonster->setUnderworld(false);
			giveUnderworldGift(pAttacker);
		}
	}
#endif

	if (pDeadCreature->isMonster()) {
		Monster* pMonster = dynamic_cast<Monster*>(pDeadCreature);
		
		if (pMonster->getLastKiller() == 0)
			pMonster->setLastKiller(pAttacker->getObjectID());

		if (g_pVariableManager->getVariable(COUNT_KILL_MONSTER) != 0) {
			Statement* pStmt = NULL;
			
			BEGIN_DB {
				static uint Dimension =	g_pConfig->getPropertyInt("Dimension");
				static uint WorldID = 	g_pConfig->getPropertyInt("WorldID");
				uint DWorldID = Dimension * 3 + WorldID;
				pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
				pStmt->executeQuery("INSERT INTO MonsterKillLog VALUES (%u, %u, %u, now())", DWorldID, pMonster->getZoneID(), pMonster->getMonsterType());

				SAFE_DELETE(pStmt);
			} END_DB(pStmt)
		}
	}

	int myLevel = 0;
	int otherLevel = 0;
	int bonusPercent = 100;

	if (pAttacker->isSlayer()) {
		// 슬레이어가 슬레이어를 죽인 경우
		if (pDeadCreature->isSlayer())
			return;

		Slayer* pSlayer = dynamic_cast<Slayer*>(pAttacker);
		myLevel = pSlayer->getHighestSkillDomainLevel();

		// 슬레이어일 경우 무기를 들고 있지 않다면 무시한다.
		if (!pSlayer->isRealWearingEx(Slayer::WEAR_RIGHTHAND))
			return;

		// 슬레이어가 뱀파이어를 죽인 경우
		if (pDeadCreature->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pDeadCreature);
			otherLevel = pVampire->getLevel();
			bonusPercent = 150;
		}
		// 슬레이어가 아우스터스를 죽인 경우
		else if (pDeadCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pDeadCreature);
			otherLevel = pOusters->getLevel();
			bonusPercent = 150;
		}
		// 슬레이어가 몬스터를 죽인 경우
		else if (pDeadCreature->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pDeadCreature);

			// 마스터는 MasterLairManager에서 처리한다.
			if (pMonster->isMaster()) {
				// last kill한 사람은 경험치 한번 더 먹는다.
				pSlayer->increaseRankExp(MASTER_KILL_RANK_EXP);
				return;
			}

			otherLevel = pMonster->getLevel();
		}
		else
			return;
	} else if (pAttacker->isVampire()) {
		// 뱀파이어가 뱀파이어를 죽인 경우
		if (pDeadCreature->isVampire())
			return;

		Vampire* pVampire = dynamic_cast<Vampire*>(pAttacker);
		myLevel = pVampire->getLevel();

		// 뱀파이어가 슬레이어를 죽인 경우
		if (pDeadCreature->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pDeadCreature);
			otherLevel = pSlayer->getHighestSkillDomainLevel();
			bonusPercent = 150;
		}
		// 뱀파이어가 아우스터스를 죽인 경우
		else if (pDeadCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pDeadCreature);
			otherLevel = pOusters->getLevel();
			bonusPercent = 150;
		}
		// 뱀파이어가 몬스터를 죽인 경우
		else if (pDeadCreature->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pDeadCreature);

			// 마스터는 MasterLairManager에서 처리한다.
			if (pMonster->isMaster()) {
				// last kill한 사람은 경험치 한번 더 먹는다.
				pVampire->increaseRankExp(MASTER_KILL_RANK_EXP);
				return;
			}

			otherLevel = pMonster->getLevel();
		}
		else
			return;
	} else if (pAttacker->isOusters()) {
		// 아우스터스가 아우스터스를 죽인 경우
		if (pDeadCreature->isOusters())
			return;

		Ousters* pOusters = dynamic_cast<Ousters*>(pAttacker);
		myLevel = pOusters->getLevel();

		// 아우스터스가가 슬레이어를 죽인 경우
		if (pDeadCreature->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pDeadCreature);
			otherLevel = pSlayer->getHighestSkillDomainLevel();
			bonusPercent = 150;
		}
		// 아우스터즈가 뱀파이어를 죽인 경우
		if (pDeadCreature->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pDeadCreature);
			otherLevel = pVampire->getLevel();
			bonusPercent = 150;
		}
		// 뱀파이어가 몬스터를 죽인 경우
		else if (pDeadCreature->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pDeadCreature);

			// 마스터는 MasterLairManager에서 처리한다.
			if (pMonster->isMaster()) {
				// last kill한 사람은 경험치 한번 더 먹는다.
				pOusters->increaseRankExp(MASTER_KILL_RANK_EXP);
				return;
			}
			otherLevel = pMonster->getLevel();
		}
		else
			return;
	}
	else
		return;

	PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pAttacker);

	if (pDeadCreature->isMonster()) {
		Monster* pMonster = dynamic_cast<Monster*>(pDeadCreature);
		PrecedenceTable* pTable = pMonster->getPrecedenceTable();
		
//		if (pMonster->getLastKiller() == 0)
//			pMonster->setLastKiller(pAttacker->getObjectID());

		if (pMonster->getMonsterType() == 722) {
			if (pPC->getPartyID() == 0) {
				if (!pPC->isFlag(Effect::EFFECT_CLASS_CAN_ENTER_GDR_LAIR)) {
					EffectCanEnterGDRLair* pEffect = new EffectCanEnterGDRLair(pPC);
					pEffect->setDeadline(216000);

					pPC->setFlag(pEffect->getEffectClass());
					pPC->addEffect(pEffect);

					pEffect->create(pPC->getName());

					GCAddEffect gcAddEffect;
					gcAddEffect.setObjectID(pPC->getObjectID());
					gcAddEffect.setEffectID(pEffect->getSendEffectClass());
					gcAddEffect.setDuration(21600);

					pPC->getZone()->broadcastPacket(pPC->getX(), pPC->getY(), &gcAddEffect);
				}
//					addSimpleCreatureEffect(pPC, Effect::EFFECT_CLASS_CAN_ENTER_GDR_LAIR, 216000);
			} else {
				LocalPartyManager* pLPM = pPC->getLocalPartyManager();
				Assert(pLPM != NULL);
				pLPM->shareGDRLairEnter(pPC->getPartyID(), pPC);
			}

			Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_EVENT_ITEM, 29, list<OptionType_t>());
			pMonster->setQuestItem(pItem);
		}

		if (pTable != NULL && pTable->canGainRankExp(pPC)) {
			QuestManager* pQM = pPC->getQuestManager();

			if (pQM != NULL && pQM->killedMonster(pMonster))
				pPC->sendCurrentQuestInfo();

/*			if (pMonster->getQuestItem() == NULL)
			{
				int value = rand()%5000;
				Item* pItem = NULL;
				if (value < 2)
				{
					// 옐로 스톤
					pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_EVENT_STAR, 22, list<OptionType_t>());
				}
				else if (value < 4)
				{
					// 오라 스톤
					pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_DYE_POTION, 58 + (rand()%4), list<OptionType_t>());
				}
				else if (value < 9)
				{
					// 멀티팩
					pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_SUB_INVENTORY, 0, list<OptionType_t>());
				}

				pMonster->setQuestItem(pItem);
				logEventItemCount(pItem);
			}*/

			if (g_pEventQuestLootingManager->killed(pPC, pMonster))
				pTable->setQuestHostName(pPC->getName());

			if (pMonster->getQuestItem() == NULL && g_pVariableManager->getVariable(EVENT_NEW_YEAR_2005) != 0) {
				Item* pItem = getNewYear2005Item(getNewYear2005ItemKind(pPC, pMonster));
				pMonster->setQuestItem(pItem);
				if (pItem != NULL) logEventItemCount(pItem);
			}

			if (pMonster->getQuestItem() == NULL && g_pVariableManager->isEventMoonCard()) {
				Item* pItem = getCardItem(getCardKind(pPC, pMonster));
//				setItemGender(pItem, (pPC->getSex()==FEMALE)?GENDER_FEMALE:GENDER_MALE);
				pMonster->setQuestItem(pItem);
			}

			if (pMonster->getQuestItem() == NULL && g_pVariableManager->isEventLuckyBag()) {
				Item* pItem = getLuckyBagItem(getLuckyBagKind(pPC, pMonster));
				pMonster->setQuestItem(pItem);
			}

			if (pMonster->getQuestItem() == NULL && g_pVariableManager->getVariable(NICKNAME_PEN_EVENT)!=0 && canGiveEventItem(pPC, pMonster)) {
				int value = rand() % 100000;
				if (value < g_pVariableManager->getVariable(NICKNAME_PEN_RATIO)) {
					Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_EVENT_GIFT_BOX, 22, list<OptionType_t>());
					pMonster->setQuestItem(pItem);
				}
			}

			if (pMonster->getQuestItem() == NULL && g_pVariableManager->getVariable(CLOVER_EVENT)!=0 && canGiveEventItem(pPC, pMonster)) {
				int value = rand() % 100000;
				if (value < g_pVariableManager->getVariable(CLOVER_RATIO)) {
					Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_MOON_CARD, 3, list<OptionType_t>());
					pMonster->setQuestItem(pItem);
				}
			}

			if (pMonster->getQuestItem() == NULL && g_pVariableManager->getVariable(PINE_CAKE_EVENT)!=0 && canGiveEventItem(pPC, pMonster)) {
				int value = rand() % 100000;
				if (value < g_pVariableManager->getVariable(PINE_CAKE_RATIO)) {
					int value = rand() % 10;
					int add = 0;
					if (value < 6)
						add = 0;
					else if (value < 9)
						add = 1;
					else
						add = 2;
					Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_EVENT_ETC, 15+add, list<OptionType_t>());
					pMonster->setQuestItem(pItem);
				}
			}

			if (g_pVariableManager->getVariable(NETMARBLE_CARD_EVENT) != 0 && pMonster->getQuestItem() == NULL &&
				g_pVariableManager->getVariable(NETMARBLE_CARD_RATIO) > (rand()%100000) && canGiveEventItem(pPC, pMonster)) {
				Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_MOON_CARD, 2, list<OptionType_t>());
				pMonster->setQuestItem(pItem);
			}

			if (g_pVariableManager->isHeadCount()) {
				GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(pAttacker->getPlayer());
				if (pGamePlayer != NULL) {
					EventHeadCount* pEvent = dynamic_cast<EventHeadCount*>(pGamePlayer->getEvent(Event::EVENT_CLASS_HEAD_COUNT));
					if (pEvent != NULL) pEvent->cutHead();
				}
			}

			if (canGiveEventItem(pPC, pMonster) && rand()%100000 < g_pVariableManager->getVariable(GOLD_MEDAL_RATIO))
				giveGoldMedal(pPC);

			if (pPC->getLevel()-20 <= pMonster->getLevel() && pPC->getLevel() >= 30 && !GDRLairManager::Instance().isGDRLairZone(pPC->getZoneID()) &&
				!pMonster->isChief() && !pMonster->isMaster())
				addOlympicStat(pPC, 1);

			if (pMonster->isChief()) {
				if (canGiveEventItem(pPC, pMonster) && !GDRLairManager::Instance().isGDRLairZone(pPC->getZoneID()))
					addOlympicStat(pPC, 3);
			}

			if (pMonster->isMaster()) {
				if (canGiveEventItem(pPC, pMonster))
					addOlympicStat(pPC, 3);
			}

			if (pMonster->getQuestItem() == NULL && canGiveEventItem(pPC, pMonster) && rand()%100000 < g_pVariableManager->getVariable(OLYMPIC_ITEM_RATIO)) {
				Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_MOON_CARD, 4, list<OptionType_t>());
				pMonster->setQuestItem(pItem);
			}

			if (pMonster->getQuestItem() == NULL && canGiveEventItem(pPC, pMonster) && rand()%100000 < g_pVariableManager->getVariable(LUCK_CHARM_RATIO)) {
				Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_EVENT_ITEM, 30, list<OptionType_t>());
				pMonster->setQuestItem(pItem);
			}

			if (pMonster->getQuestItem() == NULL && pMonster->getMonsterType() >= 769 && canGiveEventItem(pPC, pMonster) &&
				rand()%100000 < g_pVariableManager->getVariable(HOURGLASS_RATIO_S)) {
				Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_EFFECT_ITEM, 6, list<OptionType_t>());
				pMonster->setQuestItem(pItem);
			}

			if (pMonster->getQuestItem() == NULL && pMonster->getMonsterType() >= 769 && canGiveEventItem(pPC, pMonster) &&
				rand()%100000 < g_pVariableManager->getVariable(HOURGLASS_RATIO_M)) {
				Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_EFFECT_ITEM, 5, list<OptionType_t>());
				pMonster->setQuestItem(pItem);
			}

			if (pMonster->getQuestItem() == NULL && pMonster->getMonsterType() >= 769 && canGiveEventItem(pPC, pMonster) &&
				rand()%100000 < g_pVariableManager->getVariable(HOURGLASS_RATIO_L)) {
				Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_EFFECT_ITEM, 4, list<OptionType_t>());
				pMonster->setQuestItem(pItem);
			}

			if (pMonster->getZone()->isDynamicZone() && pMonster->getZone()->getDynamicZone()->getTemplateZoneID() == 4002 &&
				pMonster->getQuestItem() == NULL && (rand()%20) == 0) {
				Item* pItem = g_pItemFactoryManager->createItem(Item::ITEM_CLASS_EVENT_ITEM, 31, list<OptionType_t>());
				pMonster->setQuestItem(pItem);
			}

			if (g_pVariableManager->getVariable(PET_FOOD_EVENT) != 0 && pMonster->getQuestItem() == NULL &&
				g_pVariableManager->getVariable(PET_FOOD_RATIO) > rand()%100000 && canGiveEventItem(pPC, pMonster)) {
				bool isHigher = g_pVariableManager->getVariable(HIGHER_PET_FOOD_RATIO) > rand()%100;

				int itemClassSeed = rand()%100;
				int RacePetFoodRatio = g_pVariableManager->getVariable(RACE_PET_FOOD_RATIO);
				int RevivalSetRatio = g_pVariableManager->getVariable(REVIVAL_SET_RATIO);

				Item::ItemClass iClass = Item::ITEM_CLASS_PET_FOOD;
				ItemType_t itemType = 1;

				if (itemClassSeed < RevivalSetRatio) {
					iClass = Item::ITEM_CLASS_PET_ENCHANT_ITEM;
					itemType = 13;
				}
//				bool isRace = g_pVariableManager->getVariable(RACE_PET_FOOD_RATIO) > rand()%100;
				else if (itemClassSeed - RevivalSetRatio < RacePetFoodRatio) {
					switch (pPC->getCreatureClass()) {
						case Creature::CREATURE_CLASS_SLAYER:
							itemType = 6;
							break;

						case Creature::CREATURE_CLASS_VAMPIRE:
							itemType = 10;
							break;

						case Creature::CREATURE_CLASS_OUSTERS:
							itemType = 14;
							break;

						default:
							break;
					}
				}

				if (iClass == Item::ITEM_CLASS_PET_FOOD && isHigher)
					++itemType;

				Item* pItem = g_pItemFactoryManager->createItem(iClass, itemType, list<OptionType_t>());
				pMonster->setQuestItem(pItem);
			}

			// 패밀리 코인 아이템
			// 로컬 파티 크기가 6이상. 풀파티
			if (pMonster->getQuestItem() == NULL && canGiveEventItem(pPC, pMonster)
				&& g_pVariableManager->getVariable(FAMILY_COIN_EVENT) != 0
				&& pPC->getPartyID() != 0 && pPC->getLocalPartyManager() != NULL
				&& pPC->getLocalPartyManager()->getParty(pPC->getPartyID()) != NULL
				&& pPC->getLocalPartyManager()->getParty(pPC->getPartyID())->getSize() >= 6
				&& g_pVariableManager->getVariable(FAMILY_COIN_RATIO) != 0) {
				int value = rand() % g_pVariableManager->getVariable(FAMILY_COIN_RATIO);
				if (value == 0) {
					Item::ItemClass iClass = Item::ITEM_CLASS_EVENT_ETC;
					ItemType_t iType = 18;

					Item* pItem = g_pItemFactoryManager->createItem(iClass, iType, list<OptionType_t>());
					pMonster->setQuestItem(pItem);

					// EventItemCount 증가. DB 에 갯수를 남긴다.
					increaseEventItemCount(pItem);
				}
			}

			// 풍선 머리띠 아이템
			if (pMonster->getQuestItem() == NULL && canGiveEventItem(pPC, pMonster)
				&& g_pVariableManager->getVariable(BALLOON_HEADBAND_EVENT) != 0
				&& g_pVariableManager->getVariable(BALLOON_HEADBAND_RATIO) != 0) {
				Item* pItem = getBalloonHeadbandItem(getBalloonHeadbandKind(pPC, pMonster));
				pMonster->setQuestItem(pItem);
			}
		}
	}

	int PartyID = pPC->getPartyID();
	if (PartyID != 0) {
		// 파티에 가입되어 있다면 로컬 파티 매니저를 통해 
		// 주위의 파티원들과 경험치를 공유한다.
		LocalPartyManager* pLPM = pPC->getLocalPartyManager();
		Assert(pLPM != NULL);
		pLPM->shareRankExp(PartyID, pAttacker, otherLevel);
		if (pPC->isAdvanced())
			pLPM->shareAdvancementExp(PartyID, pAttacker, computeCreatureExp(pDeadCreature,1));
	} else {
		PlayerCreature* pAttackPC = dynamic_cast<PlayerCreature*>(pAttacker);
		if (pAttackPC->isAdvanced())
			pAttackPC->increaseAdvancementClassExp(computeCreatureExp(pDeadCreature,1));

		// 파티에 가입되어있지 않다면 혼자 올라간다.
		RankExp_t rankExp = computeRankExp(myLevel, otherLevel);

		if (pDeadCreature->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pDeadCreature);
			PrecedenceTable* pTable = pMonster->getPrecedenceTable();

			if (pTable != NULL && pTable->canGainRankExp(pPC))
				pPC->increaseRankExp(rankExp);
		}
		else
			pPC->increaseRankExp(rankExp);
	}
}

HP_t setCounterDamage(Creature* pAttacker, Creature* pTarget, Damage_t counterDamage, bool& bBroadcastAttackerHP, bool& bSendAttackerHP) {
	HP_t Result2 = 0;
	// 안전지대 체크
	// 2003.1.10 by bezz, Sequoia
	if (pAttacker != NULL && checkZoneLevelToHitTarget(pAttacker)) {
		if (pAttacker->isSlayer()) {
			Slayer* pSlayerAttacker = dynamic_cast<Slayer*>(pAttacker);
			Result2 = max(0, (int)pSlayerAttacker->getHP()-(int)counterDamage); 
			pSlayerAttacker->setHP(Result2, ATTR_CURRENT);

			bBroadcastAttackerHP = true;
			bSendAttackerHP      = true;
		} else if (pAttacker->isVampire()) {
			Vampire* pVampireAttacker = dynamic_cast<Vampire*>(pAttacker);
			Result2 = max(0, (int)pVampireAttacker->getHP()-(int)counterDamage); 
			pVampireAttacker->setHP(Result2, ATTR_CURRENT);

			bBroadcastAttackerHP = true;
			bSendAttackerHP      = true;

			// Mephisto 이펙트 걸려있으면 HP 30% 이하일때 풀린다.
			if (pVampireAttacker->isFlag(Effect::EFFECT_CLASS_MEPHISTO)) {
				HP_t maxHP = pVampireAttacker->getHP(ATTR_MAX);

				// 33% ... 케케..
				if (Result2*3 < maxHP) {
					Effect* pEffect = pVampireAttacker->findEffect(Effect::EFFECT_CLASS_MEPHISTO);
					if (pEffect != NULL)
						pEffect->setDeadline(0);
					else
						pVampireAttacker->removeFlag(Effect::EFFECT_CLASS_MEPHISTO);
//					pVampireAttacker->getEffectManager()->deleteEffect(Effect::EFFECT_CLASS_MEPHISTO);
				}
			}
		} else if (pAttacker->isOusters()) {
			Ousters* pOustersAttacker = dynamic_cast<Ousters*>(pAttacker);
			Result2 = max(0, (int)pOustersAttacker->getHP()-(int)counterDamage); 
			pOustersAttacker->setHP(Result2, ATTR_CURRENT);

			bBroadcastAttackerHP = true;
			bSendAttackerHP      = true;
		} else if (pAttacker->isMonster()) {
			Monster* pMonsterAttacker = dynamic_cast<Monster*>(pAttacker);
			Result2 = max(0, (int)pMonsterAttacker->getHP()-(int)counterDamage); 
			pMonsterAttacker->setHP(Result2, ATTR_CURRENT);
			pMonsterAttacker->setDamaged(true);

			// 몬스터가 역 데미지를 받을 경우에도 샤프실드 쓰고 공격받은 슬레이어에게 우선권이 주어진다.
			pMonsterAttacker->addPrecedence(pTarget->getName(), pTarget->getPartyID(), counterDamage);
			pMonsterAttacker->setLastHitCreatureClass(pTarget->getCreatureClass());

			bBroadcastAttackerHP = true;
			if (pMonsterAttacker->getHP(ATTR_CURRENT)*3 < pMonsterAttacker->getHP(ATTR_MAX)) {
				PrecedenceTable* pTable = pMonsterAttacker->getPrecedenceTable();

				// HP가 3분의 1 이하인 상태라고 무조건 계산을 하면, 
				// 매턴마다 의미가 없는 계산을 계속 하게 되므로, 
				// 한번 계산을 하고 나면, 죽기 전까지는 다시 계산하지 않도록
				// 플래그를 세팅해 준다. 이 플래그를 이용하여 필요없는 계산을 줄인다.
				if (pTable->getComputeFlag() == false) {
					// 계산을 해준다.
					pTable->compute();

					// 호스트의 이름과 파티 ID를 이용하여, 이펙트를 걸어준다.
					EffectPrecedence* pEffectPrecedence = new EffectPrecedence(pMonsterAttacker);
					pEffectPrecedence->setDeadline(100);
					pEffectPrecedence->setHostName(pTable->getHostName());
					pEffectPrecedence->setHostPartyID(pTable->getHostPartyID());
					pMonsterAttacker->setFlag(Effect::EFFECT_CLASS_PRECEDENCE);
					pMonsterAttacker->addEffect(pEffectPrecedence);
				}
			}
		}
	}
	return Result2;
}

bool canBlockByWaterBarrier(SkillType_t SkillType) {
	switch (SkillType) {
		case SKILL_METEOR_STRIKE:
		case SKILL_POISON_STORM:
		case SKILL_GREEN_POISON:
		case SKILL_GREEN_STALKER:
		case SKILL_ACID_STORM:
		case SKILL_ACID_BOLT:
		case SKILL_ACID_STRIKE:
		case SKILL_ACID_SWAMP:
		case SKILL_BLOODY_KNIFE:
		case SKILL_BLOODY_WAVE:
		case SKILL_BLOODY_STRIKE:
		case SKILL_BLOODY_BALL:
		case SKILL_BLOODY_WALL:
		case SKILL_BLOODY_SPEAR:
		case SKILL_WIND_DIVIDER:
		case SKILL_SWORD_RAY:
		case SKILL_WIDE_LIGHTNING:
		case SKILL_EARTHQUAKE:
		case SKILL_POWER_OF_LAND:
		case SKILL_MULTI_AMPUTATE:
		case SKILL_WILD_TYPHOON:
		case SKILL_ATTACK_ARMS:
		case SKILL_DOUBLE_SHOT:
		case SKILL_TRIPLE_SHOT:
		case SKILL_MULTI_SHOT:
		case SKILL_HEAD_SHOT:
		case SKILL_QUICK_FIRE:
		case SKILL_ULTIMATE_BLOW:
		case SKILL_PIERCING:
		case SKILL_THROW_BOMB:
		case SKILL_BULLET_OF_LIGHT:
		case SKILL_GUN_SHOT_GUIDANCE:
		case SKILL_HOLY_ARROW:
		case SKILL_CAUSE_LIGHT_WOUNDS:
		case SKILL_CAUSE_SERIOUS_WOUNDS:
		case SKILL_CAUSE_CRITICAL_WOUNDS:
		case SKILL_VIGOR_DROP:
		case SKILL_TURN_UNDEAD:
		case SKILL_ILLENDUE:
		case SKILL_THROW_HOLY_WATER:
		case SKILL_LIGHT_BALL:
		case SKILL_AURA_BALL:
		case SKILL_AURA_RING:
		case SKILL_ENERGY_DROP:
		case SKILL_REBUKE:
		case SKILL_SPIRIT_GUARD:
		case SKILL_KASAS_ARROW:
		case SKILL_PROMINENCE:
		case SKILL_RING_OF_FLARE:
		case SKILL_BLAZE_BOLT:
		case SKILL_HANDS_OF_NIZIE:
		case SKILL_STONE_AUGER:
		case SKILL_EARTHS_TEETH:
			return true;
		default:
			return false;
	}
}
bool canBlockByGrayDarkness(SkillType_t skillType) {
	switch(skillType) {
		case SKILL_SWORD_WAVE:
		case SKILL_SWORD_RAY:
		case SKILL_WIND_DIVIDER:
		case SKILL_THUNDER_BOLT:
		case SKILL_THUNDER_STORM:
		case SKILL_WIDE_LIGHTNING:
		case SKILL_TORNADO_SEVER:
		case SKILL_EARTHQUAKE:
		case SKILL_POWER_OF_LAND:
		case SKILL_WILD_TYPHOON:
		case SKILL_MULTI_SHOT:
		case SKILL_MOLE_SHOT:
		case SKILL_GUN_SHOT_GUIDANCE:
		case SKILL_VIGOR_DROP:
		case SKILL_TURN_UNDEAD:
		case SKILL_ENERGY_DROP:
			return true;
		default:
			return false;
	}
}

bool canGiveSkillExp(Slayer* pSlayer, SkillDomainType_t SkillDomainType, SkillType_t UseSkillType) {
	if (pSlayer == NULL)
		return false;
	if (!pSlayer->isRealWearing(Slayer::WEAR_RIGHTHAND))
		return false;

	Item* pWeapon = pSlayer->getWearItem(Slayer::WEAR_RIGHTHAND);
	if (pWeapon == NULL)
		return false;

	switch (SkillDomainType) {
		case SKILL_DOMAIN_BLADE:
			if (pWeapon->getItemClass() != Item::ITEM_CLASS_BLADE)
				return false;
			break;
		case SKILL_DOMAIN_SWORD:
			if (pWeapon->getItemClass() != Item::ITEM_CLASS_SWORD)
				return false;
			break;
		case SKILL_DOMAIN_GUN:
			if (!isArmsWeapon(pWeapon)) return false;
			break;
		case SKILL_DOMAIN_HEAL:
			if (pWeapon->getItemClass() != Item::ITEM_CLASS_CROSS)
				return false;
			break;
		case SKILL_DOMAIN_ENCHANT:
			if (pWeapon->getItemClass() != Item::ITEM_CLASS_MACE)
				return false;
			break;
		default:
			return false;
	}

	if (UseSkillType < SKILL_DOUBLE_IMPACT)
		return true;
	SkillInfo* pUseSkillInfo = g_pSkillInfoManager->getSkillInfo(UseSkillType);
	if (SkillDomainType != pUseSkillInfo->getDomainType())
		return false;

	return true;
}

void giveSkillExp(Slayer* pSlayer, SkillType_t SkillType, ModifyInfo& AttackerMI) {
	if (pSlayer == NULL)
		return;

	SkillSlot* pSkillSlot = pSlayer->getSkill(SkillType);
	SkillInfo* pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);
	if (pSkillSlot != NULL && pSkillInfo != NULL)
		increaseSkillExp(pSlayer, pSkillInfo->getDomainType(), pSkillSlot, pSkillInfo, AttackerMI);
}

//////////////////////////////////////////////////////////////////////////////
// 직접적으로 데미지를 세팅한다.
//////////////////////////////////////////////////////////////////////////////
HP_t setDamage(Creature* pTargetCreature, Damage_t Damage, Creature* pAttacker, SkillType_t SkillType, ModifyInfo* pMI, ModifyInfo* pAttackerMI,
				bool canKillTarget, bool canSteal) {
	Assert(pTargetCreature != NULL);

	if (pTargetCreature->isFlag(Effect::EFFECT_CLASS_NO_DAMAGE) ||
		pTargetCreature->isFlag(Effect::EFFECT_CLASS_TENDRIL) ||
		pTargetCreature->isDead()) {
		//if (pTargetCreature->isVampire())
		// return값으로 현재 HP를 넘겨줘야 정상이겠지만
		// return값을 사용하는 부분이 없어서 일단 무시한다.
		// by sigi. 2002.9.5
		return 0;
	}

//	bool canKillTarget = (SkillType==SKILL_ACID_SWAMP)?false:true;

	Zone* pZone = pTargetCreature->getZone();
	Assert(pZone != NULL);

	// 질드레 레어에서
	if (pZone->getZoneID() == 1412 || pZone->getZoneID() == 1413) {
		if (pTargetCreature->isPC() && pAttacker != NULL && pAttacker->isPC())
			return 0;
	}

	HP_t        Result  = 0;
	HP_t        Result2 = 0;
	HP_t        hp      = 0;
	MP_t        mp      = 0;
	ZoneCoord_t TX      = 0;
	ZoneCoord_t TY      = 0;
	ObjectID_t  TOID    = 0;
	ZoneCoord_t AX      = 0;
	ZoneCoord_t AY      = 0;
	ObjectID_t  AOID    = 0;

	Damage_t	OriginalDamage = Damage;

	bool bBroadcastTargetHP   = false; // 피공격자의 HP를 브로드캐스팅하나?
	bool bSendTargetHP        = false; // 피공격자의 HP를 보내주나?
	bool bSendTargetMP        = false; // 피공격자의 MP를 보내주나?
	bool bBroadcastAttackerHP = false; // 공격자의 HP를 브로드캐스팅하나?
	bool bSendAttackerHP      = false; // 공격자의 HP를 보내주나?
	bool bSendAttackerMP      = false; // 공격자의 MP를 보내주나?

	GCStatusCurrentHP gcTargetHP;
	GCStatusCurrentHP gcAttackerHP;

	SkillProperty* pSkillProperty = g_pSkillPropertyManager->getSkillProperty(SkillType);
	bool bPhysicDamage = pSkillProperty->isPhysic();
	bool bMagicDamage = pSkillProperty->isMagic();

	SkillInfo* pSkillInfo = NULL;
	
	if (SkillType >= SKILL_DOUBLE_IMPACT) {
		pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);
		Assert(pSkillInfo != NULL);

		BYTE domain = pSkillInfo->getDomainType();
		switch (domain) {
			case SKILL_DOMAIN_BLADE:
			case SKILL_DOMAIN_SWORD:
			case SKILL_DOMAIN_GUN:
				bPhysicDamage = true;
				bMagicDamage = false;
				break;
			case SKILL_DOMAIN_HEAL:
			case SKILL_DOMAIN_ENCHANT:
				bPhysicDamage = false;
				bMagicDamage = true;
				break;
			default:
				break;
		}
	}

	// 아마게돈 이펙트가 걸려있을 경우 수정구슬(?)의 HP를 깎아주고 타겟은 공격받지 않는다.
	// SKILL_ARMAGEDDON일 경우 아마게돈 이펙트 자체의 데미지이므로 그냥 타겟을 공격하는 쪽으로 넘어간다.
	/*
	if(pTargetCreature != NULL && pTargetCreature->isFlag(Effect::EFFECT_CLASS_ARMAGEDDON) && SkillType != SKILL_ARMAGEDDON)
	{
		EffectArmageddon* pEffect = dynamic_cast<EffectArmageddon*>(pTargetCreature->findEffect(Effect::EFFECT_CLASS_ARMAGEDDON));
		Assert(pEffect != NULL);

		Damage = computePureDamage(pAttacker);

		pEffect->decreaseHP(Damage);

		if (pTargetCreature->isSlayer())
		{
			Slayer* pSlayer = dynamic_cast<Slayer*>(pTargetCreature);
			Result = pSlayer->getHP(ATTR_CURRENT);
		}
		else if (pTargetCreature->isVampire())
		{
			Vampire* pVampire = dynamic_cast<Vampire*>(pTargetCreature);
			Result = pVampire->getHP(ATTR_CURRENT);
		}
		else if (pTargetCreature->isMonster())
		{
			Monster* pMonster = dynamic_cast<Monster*>(pTargetCreature);
			Result = pMonster->getHP(ATTR_CURRENT);
		}

		return Result;
	}
	*/

	if (pTargetCreature != NULL && pTargetCreature->isMonster()) {
		Monster* pMonster = dynamic_cast<Monster*>(pTargetCreature);
		if (pMonster != NULL && pMonster->getMonsterType() == GROUND_ELEMENTAL_TYPE)
			if (pAttacker != NULL && pAttacker->isOusters())
				return 0;
	}

	// 스트라이킹이 걸려있으면 마법 데미지 뻥튀기
	if (pAttacker != NULL && pSkillProperty->isMagic() && pAttacker->isFlag(Effect::EFFECT_CLASS_STRIKING)) {
		EffectStriking* pEffect = dynamic_cast<EffectStriking*>(pAttacker->findEffect(Effect::EFFECT_CLASS_STRIKING));
		if (pEffect != NULL)
			Damage += pEffect->getDamageBonus();
	}

	if (pTargetCreature != NULL && pTargetCreature->isFlag(Effect::EFFECT_CLASS_INSTALL_TURRET)) {
		EffectInstallTurret* pEffect = dynamic_cast<EffectInstallTurret*>(pTargetCreature->findEffect(Effect::EFFECT_CLASS_INSTALL_TURRET));
		if (pEffect != NULL)
			Damage -= getPercentValue(Damage, pEffect->getDefense());
	}

	// Denial Magic 이 걸려있을 경우 마법데미지인지 체크해서 배짼다.
	if (pTargetCreature != NULL &&
		pTargetCreature->isFlag(Effect::EFFECT_CLASS_DENIAL_MAGIC) &&
		pSkillProperty->isMagic()) {
		// 기본스킬은 SkillInfo 가 없다.
		if (SkillType >= SKILL_DOUBLE_IMPACT) {
			// 뱀파이어의 마법데미지만 막아준다.
			if (pSkillInfo->getDomainType() == SKILL_DOMAIN_VAMPIRE || pSkillInfo->getDomainType() == SKILL_DOMAIN_OUSTERS) {
				// 성공적으로 막았을 경우 이펙트를 멋지게 날려준다.
				GCAddEffect gcAddEffect;
				gcAddEffect.setObjectID(pTargetCreature->getObjectID());
				gcAddEffect.setEffectID(Effect::EFFECT_CLASS_DENIAL_MAGIC_DAMAGED);
				gcAddEffect.setDuration(0);

				pTargetCreature->getZone()->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);

//				return 0;
				// 배째지 말고 데미지 60%...... ㅜ.ㅠ
				Damage = max(1, (int)(Damage * 0.4));
			}
		}
	}

	// Water Barrier 가 걸려있을 경우 총슬 공격에 대해서만 데미지를 줄여준다.
	if (pTargetCreature != NULL &&
		pTargetCreature->isFlag(Effect::EFFECT_CLASS_WATER_BARRIER) &&
		!pSkillProperty->isMelee()) {
		// 기본스킬은 SkillInfo 가 없다.
		if (canBlockByWaterBarrier(SkillType)) {
			EffectWaterBarrier* pEWB = dynamic_cast<EffectWaterBarrier*>(pTargetCreature->findEffect(Effect::EFFECT_CLASS_WATER_BARRIER));
			if (pEWB != NULL) {
				int bonus = pEWB->getBonus();
				if (bonus > 100)
					bonus = 100;

				Damage = (Damage_t)(Damage * ((float)(100-bonus) / 100.0));
				Damage = max(1, (int)Damage);
			}
		}
	}

	// Water Shield 가 걸릴 수 있는 경우 물리 공격에 대해서는 데미지를 주지 않는다
	if (pTargetCreature != NULL && pSkillProperty->isPhysic()) {
		if (pTargetCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pTargetCreature);

			if (pOusters->isPassiveAvailable(SKILL_WATER_SHIELD)) {
				if ((rand()%100) < min(20, (int)(pOusters->getPassiveRatio() * 3/2))) {
					GCAddEffect gcAddEffect;
					gcAddEffect.setObjectID(pOusters->getObjectID());
					gcAddEffect.setEffectID(Effect::EFFECT_CLASS_WATER_SHIELD);
					gcAddEffect.setDuration(0);

					pZone->broadcastPacket(pOusters->getX(), pOusters->getY(), &gcAddEffect);
					//cout << "워터실드 발동했습니다." << endl;
					return 0;
				}
			}
		}
	}

	if (pTargetCreature != NULL &&
		pTargetCreature->isFlag(Effect::EFFECT_CLASS_STONE_SKIN) &&
		(SkillType >= SKILL_DOUBLE_IMPACT || SkillType == SKILL_ATTACK_ARMS)) {
		if (SkillType == SKILL_ATTACK_ARMS || g_pSkillInfoManager->getSkillInfo(SkillType)->getDomainType() == SKILL_DOMAIN_GUN) {
			EffectStoneSkin* pStoneSkin = dynamic_cast<EffectStoneSkin*>(pTargetCreature->findEffect(Effect::EFFECT_CLASS_STONE_SKIN));
			if (pStoneSkin != NULL) {
				if ((rand()%100) < pStoneSkin->getBonus()) {
					Damage = 0;
					GCAddEffect gcAddEffect;
					gcAddEffect.setObjectID(pTargetCreature->getObjectID());
					gcAddEffect.setEffectID(Effect::EFFECT_CLASS_STONE_SKIN_DAMAGED);
					gcAddEffect.setDuration(0);

					pTargetCreature->getZone()->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);
				}
			}
		}
	}


	if (pAttacker != NULL && pAttacker->isOusters() && SkillType >= SKILL_DOUBLE_IMPACT) {
		Ousters* pOusters = dynamic_cast<Ousters*>(pAttacker);
		Assert(pOusters != NULL);
		SkillInfo* pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);
		Assert(pSkillInfo != NULL);

		if (pSkillProperty->isMagic()) {
			switch (pSkillInfo->getElementalDomain()) {
				case ELEMENTAL_DOMAIN_FIRE:
					Damage += pOusters->getFireDamage();
					//cout << "FireDamage적용 : " << pOusters->getFireDamage() <<"," <<Damage << endl;;

					if (pOusters->isFlag(Effect::EFFECT_CLASS_HANDS_OF_FIRE)) {
						EffectHandsOfFire* pEffect = dynamic_cast<EffectHandsOfFire*>(pOusters->findEffect(Effect::EFFECT_CLASS_HANDS_OF_FIRE));
						if (pEffect != NULL)
							Damage = (Damage_t)(Damage * (1.0 + (pEffect->getBonus() / 100.0)));
					}
					break;
				case ELEMENTAL_DOMAIN_WATER:
					Damage += pOusters->getWaterDamage();
					break;
				case ELEMENTAL_DOMAIN_EARTH:
					Damage += pOusters->getEarthDamage();
					break;
				default:
					break;
			}
		}
	}

	if (pTargetCreature != NULL) {
		TX   = pTargetCreature->getX();
		TY   = pTargetCreature->getY();
		TOID = pTargetCreature->getObjectID();

		gcTargetHP.setObjectID(TOID);
	}

	////////////////////////////////////////////////////////////////////
	// Target Creature 에 이펙트를 처리한다.
	////////////////////////////////////////////////////////////////////
	if (pTargetCreature != NULL) {
		// SLEEP 이펙트가 걸려 있다면 이펙트를 삭제한다.
		if (pTargetCreature->isFlag(Effect::EFFECT_CLASS_SLEEP) && SkillType != SKILL_REBUKE) {
			EffectSleep* pEffect = dynamic_cast<EffectSleep*>(pTargetCreature->findEffect(Effect::EFFECT_CLASS_SLEEP));
			Assert(pEffect != NULL);

			pEffect->setDeadline(0);
		}
	}

	if (pAttacker != NULL) {
		AX   = pAttacker->getX();
		AY   = pAttacker->getY();
		AOID = pAttacker->getObjectID();

		gcAttackerHP.setObjectID(AOID);

		// 언젠가 최적화를 하게 된다면.. -_-;
		// Creature에다가 Penalty관련 member들을 넣는게 나을 것이다.
		// Hymn걸려있다면 damage penalty% 받는다.
		if (pAttacker->isFlag(Effect::EFFECT_CLASS_HYMN)) {
			EffectHymn* pHymn = dynamic_cast<EffectHymn*>(pAttacker->getEffectManager()->findEffect(Effect::EFFECT_CLASS_HYMN));

			Damage = Damage*(100-pHymn->getDamagePenalty())/100;
		}

		if (pAttacker->isSlayer() && pAttackerMI != NULL && !pTargetCreature->isSlayer()) {
			Slayer* pAttackSlayer = dynamic_cast<Slayer*>(pAttacker);
			Assert(pAttackSlayer != NULL);

			// 셀프스킬들 경험치 주기 --;;
			if (canGiveSkillExp(pAttackSlayer, SKILL_DOMAIN_SWORD, SkillType)) {
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_DANCING_SWORD) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_DANCING_SWORD, *pAttackerMI);
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_REDIANCE) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_REDIANCE, *pAttackerMI);
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_EXPANSION) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_EXPANSION, *pAttackerMI);
			} else if (canGiveSkillExp(pAttackSlayer, SKILL_DOMAIN_BLADE, SkillType)) {
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_GHOST_BLADE) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_GHOST_BLADE, *pAttackerMI);
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_POTENTIAL_EXPLOSION) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_POTENTIAL_EXPLOSION, *pAttackerMI);
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_CHARGING_POWER) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_CHARGING_POWER, *pAttackerMI);
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_BERSERKER) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_BERSERKER, *pAttackerMI);
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_AIR_SHIELD_1) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_AIR_SHIELD, *pAttackerMI);
			} else if (canGiveSkillExp(pAttackSlayer, SKILL_DOMAIN_GUN, SkillType)) {
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_CONCEALMENT) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_CONCEALMENT, *pAttackerMI);
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_REVEALER) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_REVEALER, *pAttackerMI);
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_OBSERVING_EYE) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_OBSERVING_EYE, *pAttackerMI);
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_HEART_CATALYST) && (rand()%2)!=0) giveSkillExp(pAttackSlayer, SKILL_HEART_CATALYST, *pAttackerMI);
			} else if (canGiveSkillExp(pAttackSlayer, SKILL_DOMAIN_ENCHANT, SkillType)) {
				if (pAttackSlayer->isFlag(Effect::EFFECT_CLASS_AURA_SHIELD) && (rand()%2)!=0)
					giveSkillExp(pAttackSlayer, SKILL_AURA_SHIELD, *pAttackerMI);
			}

/*			if (pAttacker->isFlag(Effect::EFFECT_CLASS_REDIANCE) && pAttacker->isSlayer() && pAttackerMI != NULL && !pTargetCreature->isSlayer())
			{
				EffectRediance* pEffect = dynamic_cast<EffectRediance*>(pAttacker->findEffect(Effect::EFFECT_CLASS_REDIANCE));
				if (pEffect != NULL && pEffect->canGiveExp())
				{
					Slayer* pSlayer = dynamic_cast<Slayer*>(pAttacker);
					Assert(pSlayer != NULL);

					SkillSlot* pSkillSlot = pSlayer->getSkill(SKILL_REDIANCE);
					SkillInfo* pSkillInfo = g_pSkillInfoManager->getSkillInfo(SKILL_REDIANCE);
					if (pSkillSlot != NULL && pSkillInfo != NULL)
					{
						increaseSkillExp(pSlayer, SKILL_DOMAIN_SWORD,  pSkillSlot, pSkillInfo, *pAttackerMI);
					}
				}
			}
			if (pAttacker->isFlag(Effect::EFFECT_CLASS_EXPANSION) && pAttacker->isSlayer() && pAttackerMI != NULL && !pTargetCreature->isSlayer())
			{
				Effect* pEffect = pAttacker->findEffect(Effect::EFFECT_CLASS_EXPANSION);
				if (pEffect != NULL && (rand()%2)==1)
				{
					Slayer* pSlayer = dynamic_cast<Slayer*>(pAttacker);
					Assert(pSlayer != NULL);

					SkillSlot* pSkillSlot = pSlayer->getSkill(SKILL_EXPANSION);
					SkillInfo* pSkillInfo = g_pSkillInfoManager->getSkillInfo(SKILL_EXPANSION);
					if (pSkillSlot != NULL && pSkillInfo != NULL)
					{
						increaseSkillExp(pSlayer, SKILL_DOMAIN_SWORD,  pSkillSlot, pSkillInfo, *pAttackerMI);
					}
				}
			}
			if (pAttacker->isFlag(Effect::EFFECT_CLASS_GHOST_BLADE) && pAttacker->isSlayer() && pAttackerMI != NULL && !pTargetCreature->isSlayer())
			{
				Effect* pEffect = pAttacker->findEffect(Effect::EFFECT_CLASS_GHOST_BLADE);
				if (pEffect != NULL && (rand()%2)==1)
				{
					Slayer* pSlayer = dynamic_cast<Slayer*>(pAttacker);
					Assert(pSlayer != NULL);

					SkillSlot* pSkillSlot = pSlayer->getSkill(SKILL_GHOST_BLADE);
					SkillInfo* pSkillInfo = g_pSkillInfoManager->getSkillInfo(SKILL_GHOST_BLADE);
					if (pSkillSlot != NULL && pSkillInfo != NULL)
					{
						increaseSkillExp(pSlayer, SKILL_DOMAIN_BLADE,  pSkillSlot, pSkillInfo, *pAttackerMI);
					}
				}
			}*/
		}

		// Blood Bible 보너스를 적용한다.
		if (pAttacker->isPC()) {
			PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pAttacker);
			Damage_t MagicBonusDamage = pPC->getMagicBonusDamage();
			Damage_t PhysicBonusDamage = pPC->getPhysicBonusDamage();

//			if (MagicBonusDamage != 0 && pSkillProperty!= NULL && pSkillProperty->isMagic())
			if (MagicBonusDamage != 0 && bMagicDamage)
				Damage += MagicBonusDamage;
//			if (PhysicBonusDamage != 0 && pSkillProperty!= NULL && pSkillProperty->isPhysic())
			if (PhysicBonusDamage != 0 && bPhysicDamage)
				Damage += PhysicBonusDamage;
		}
	}

	////////////////////////////////////////////////////////////
	// 먼저 hp, mp steal을 처리한다.
	////////////////////////////////////////////////////////////
	if (pAttacker != NULL && canSteal)//(SkillType != SKILL_PROMINENCE && SkillType != SKILL_HELLFIRE)
	{
		Steal_t HPStealAmount = pAttacker->getHPStealAmount();
		Steal_t MPStealAmount = pAttacker->getMPStealAmount();

		// HP 스틸을 체크한다.
		if (HPStealAmount != 0 && (rand()%100) < pAttacker->getHPStealRatio()) {
			if (pAttacker->isSlayer()) {
				if (pAttacker->isAlive()) {
					Slayer* pSlayer = dynamic_cast<Slayer*>(pAttacker);

					// 현재의 HP에다 스틸한 양을 더하고, 
					// 맥스를 넘지는 않는지 체크를 한다.
					hp = pSlayer->getHP(ATTR_CURRENT) + (int)HPStealAmount;
					hp = min(hp, pSlayer->getHP(ATTR_MAX));

					// HP를 세팅하고, 플래그를 켠다.
					pSlayer->setHP(hp, ATTR_CURRENT);
					bBroadcastAttackerHP = true;
					bSendAttackerHP      = true;
				}
			} else if (pAttacker->isVampire()) {
				Vampire* pVampire = dynamic_cast<Vampire*>(pAttacker);

				// 현재의 HP에다 스틸한 양을 더하고, 
				// 맥스를 넘지는 않는지 체크를 한다.
				hp = pVampire->getHP(ATTR_CURRENT) + (int)HPStealAmount;
				hp = min(hp, pVampire->getHP(ATTR_MAX));

				// HP를 세팅하고, 플래그를 켠다.
				pVampire->setHP(hp, ATTR_CURRENT);
				bBroadcastAttackerHP = true;
				bSendAttackerHP      = true;
			} else if (pAttacker->isOusters()) {
				// 죽은넘 HP올려주지말자
				if (pAttacker->isAlive()) {
					Ousters* pOusters = dynamic_cast<Ousters*>(pAttacker);

					// 현재의 HP에다 스틸한 양을 더하고, 
					// 맥스를 넘지는 않는지 체크를 한다.
					//cout << "HP Steal!" << (int)HPStealAmount << endl;
					hp = pOusters->getHP(ATTR_CURRENT) + (int)HPStealAmount;
					hp = min(hp, pOusters->getHP(ATTR_MAX));

					// HP를 세팅하고, 플래그를 켠다.
					pOusters->setHP(hp, ATTR_CURRENT);
					bBroadcastAttackerHP = true;
					bSendAttackerHP      = true;
				}
			}
			else
				Assert(false);
		}

		// MP 스틸을 체크한다.
		if (MPStealAmount != 0 && (rand()%100) < pAttacker->getMPStealRatio()) {
			// 슬레이어와 아우스터스일 경우 MP 스틸을 처리한다.
			if (pAttacker->isSlayer()) {
				Slayer* pSlayer = dynamic_cast<Slayer*>(pAttacker);

				// 현재의 MP에다 스틸한 양을 더하고, 
				// 맥스를 넘지는 않는지 체크를 한다.
				mp = pSlayer->getMP(ATTR_CURRENT) + (int)MPStealAmount;
				mp = min(mp, pSlayer->getMP(ATTR_MAX));

				pSlayer->setMP(mp, ATTR_CURRENT);

				bSendAttackerMP = true;
			} else if (pAttacker->isOusters()) {
				Ousters* pOusters = dynamic_cast<Ousters*>(pAttacker);

				if (pOusters->getMP(ATTR_CURRENT) < pOusters->getMP(ATTR_MAX)) {
					// 현재의 MP에다 스틸한 양을 더하고, 
					// 맥스를 넘지는 않는지 체크를 한다.
					mp = pOusters->getMP(ATTR_CURRENT) + (int)MPStealAmount;
					mp = min(mp, pOusters->getMP(ATTR_MAX));

					pOusters->setMP(mp, ATTR_CURRENT);

					bSendAttackerMP = true;
				}
			}
		}
	}

	if (pTargetCreature != NULL && pTargetCreature->isFlag(Effect::EFFECT_CLASS_EXPLOSION_WATER)) {
		EffectExplosionWater* pEffect = dynamic_cast<EffectExplosionWater*>(pTargetCreature->findEffect(Effect::EFFECT_CLASS_EXPLOSION_WATER));

		if (pEffect != NULL) {
			int ratio = 100;
			ratio -= pEffect->getDamageReduce();
			if (ratio < 0)
				ratio = 0;

			Damage = max(1,getPercentValue(Damage, ratio));
		}
	}

	if (pAttacker != NULL && pAttacker->isFlag(Effect::EFFECT_CLASS_REPUTO_FACTUM)) {
		Damage_t counterDamage = getPercentValue(Damage, 30);
		
		GCAddEffect gcAddEffect;
		gcAddEffect.setObjectID(pAttacker->getObjectID());
		gcAddEffect.setEffectID(Effect::EFFECT_CLASS_REPUTO_FACTUM);
		gcAddEffect.setDuration(0);
		pAttacker->getZone()->broadcastPacket(pAttacker->getX(), pAttacker->getY(), &gcAddEffect);
		setCounterDamage(pAttacker, pTargetCreature, counterDamage, bBroadcastAttackerHP, bSendAttackerHP);
	}

	if (pTargetCreature != NULL && !pTargetCreature->isSlayer()) {
		Effect* pEffect = pTargetCreature->getZone()->getTile(pTargetCreature->getX(), pTargetCreature->getY()).getEffect(Effect::EFFECT_CLASS_SWORD_OF_THOR);
		if (pEffect != NULL) {
			EffectSwordOfThor* pThor = dynamic_cast<EffectSwordOfThor*>(pEffect);
			if (pThor != NULL)
				Damage = getPercentValue(Damage, 140 + (pThor->getLevel()/10));
		}
	}

	////////////////////////////////////////////////////////////
	// 맞는 놈이 슬레이어일 경우
	////////////////////////////////////////////////////////////
	if (pTargetCreature->isSlayer()) {
		// 아우스터즈가 뱀파이어를 죽인 경우
		Slayer* pSlayer = dynamic_cast<Slayer *>(pTargetCreature);
		bool bSetDamage = false;

		if (pSlayer->isFlag(Effect::EFFECT_CLASS_REQUITAL) && pAttacker != NULL && pSkillProperty != NULL && pSkillProperty->isMelee()) {
			EffectRequital* pEffectRequital = (EffectRequital*)(pSlayer->findEffect(Effect::EFFECT_CLASS_REQUITAL));
			int refl = pEffectRequital->getReflection();
			Damage_t counterDamage = max(1, getPercentValue(Damage, refl));

			Result2 = setCounterDamage(pAttacker, pSlayer, counterDamage, bBroadcastAttackerHP, bSendAttackerHP);
		}

		// AuraShield 효과로 HP대신 MP가 소모되는 경우가 있다.
		// 마스터 레어에서 뜨는 그라운드 어택(땅에서 튀어나오는 불기둥) 맞으면 오라실드 무시하고 HP 닳게 한다. 2003. 1.16. Sequoia
		if (pSlayer->isFlag(Effect::EFFECT_CLASS_AURA_SHIELD) && SkillType != SKILL_GROUND_ATTACK) {
			// 공격자에게 데미지를 돌려줘야 한다.
			if (pAttacker != NULL) {
				Damage_t counterDamage = 0;

				EffectAuraShield* pEffectAuraShield = (EffectAuraShield*)(pSlayer->findEffect(Effect::EFFECT_CLASS_AURA_SHIELD));
				Assert(pEffectAuraShield != NULL);

				// 카운터 데미지는 원래 데미지의 10분의 1이다.
				counterDamage = max(1, getPercentValue(Damage, 10));

				if (pAttacker->isVampire()) {
					Vampire* pVampireAttacker = dynamic_cast<Vampire*>(pAttacker);
					Result2 = max(0, (int)pVampireAttacker->getHP()-(int)counterDamage); 
					pVampireAttacker->setHP(Result2, ATTR_CURRENT);

					bBroadcastAttackerHP = true;
					bSendAttackerHP      = true;
				} else if (pAttacker->isOusters()) {
					Ousters* pOustersAttacker = dynamic_cast<Ousters*>(pAttacker);
					Result2 = max(0, (int)pOustersAttacker->getHP()-(int)counterDamage); 
					pOustersAttacker->setHP(Result2, ATTR_CURRENT);

					bBroadcastAttackerHP = true;
					bSendAttackerHP      = true;
				} else if (pAttacker->isMonster()) {
					Monster* pMonsterAttacker = dynamic_cast<Monster*>(pAttacker);
					Result2 = max(0, (int)pMonsterAttacker->getHP()-(int)counterDamage); 
					pMonsterAttacker->setHP(Result2, ATTR_CURRENT);
					pMonsterAttacker->setDamaged(true);

					bBroadcastAttackerHP = true;
				}
			}

#ifdef __CHINA_SERVER__
			Result = max(0, (int)pSlayer->getMP(ATTR_CURRENT) - (int)(Damage*2.5));
#else
			Result = max(0, (int)pSlayer->getMP(ATTR_CURRENT) - (int)(Damage*2));
#endif

			pSlayer->setMP(Result, ATTR_CURRENT);
			bSendTargetMP = true;

			// Result가 0인 경우, 마나가 다 닳았단 말이다.
			// 그러므로 effect를 삭제해 준다.
			if (Result == 0) {
				Effect* pEffect = pSlayer->findEffect(Effect::EFFECT_CLASS_AURA_SHIELD);
				if (pEffect != NULL) pEffect->setDeadline(0);
/*				pSlayer->deleteEffect(Effect::EFFECT_CLASS_AURA_SHIELD);
				pSlayer->removeFlag(Effect::EFFECT_CLASS_AURA_SHIELD);

				GCRemoveEffect removeEffect;
				removeEffect.setObjectID(TOID);
				removeEffect.addEffectList(Effect::EFFECT_CLASS_AURA_SHIELD);
				pZone->broadcastPacket(TX, TY, &removeEffect);*/

				// 기술을 다시 쓸 수 있도록 기술 딜레이를 날려준다.
				SkillSlot* pSkillSlot = pSlayer->hasSkill(SKILL_AURA_SHIELD);
				if (pSkillSlot != NULL)
					pSkillSlot->setRunTime(0, false);
			}	
			bSetDamage = true;
		}

		// by Sequoia 2002.12.26
		// Melee 어택의 경우 데미지가 줄어들고 때린 넘에게 데미지를 준다.
		// switch 로 된 걸 isMeleeSkill 을 사용하는 코드로 바꾼다. 2003. 1. 1.
		if(pSlayer->isFlag(Effect::EFFECT_CLASS_SHARP_SHIELD_1) && pSkillProperty != NULL && pSkillProperty->isMelee()  && pAttacker != NULL) {
			// Sharp Shield 가 있으면 밀리 어택의 데미지는 반이다.
			Damage = max (0, (int)Damage - ((int)OriginalDamage >> 1));

			EffectSharpShield* pEffect = dynamic_cast<EffectSharpShield*>(pSlayer->findEffect(Effect::EFFECT_CLASS_SHARP_SHIELD_1));
			Assert(pEffect!=NULL);

			Damage_t counterDamage = pEffect->getDamage();

			Result2 = setCounterDamage(pAttacker, pSlayer, counterDamage, bBroadcastAttackerHP, bSendAttackerHP);

			if (pAttacker != NULL && !pAttacker->isSlayer())
				if (pMI != NULL && (rand()%2)!=0)
					giveSkillExp(pSlayer, SKILL_SHARP_SHIELD, *pMI);
//			SkillSlot* pSkillSlot = pSlayer->getSkill(SKILL_SHARP_SHIELD);
//			SkillInfo* pSkillInfo = g_pSkillInfoManager->getSkillInfo(SKILL_SHARP_SHIELD);
//
//			if (rand()%2) increaseSkillExp(pSlayer, SKILL_DOMAIN_SWORD, pSkillSlot, pSkillInfo, *pAttackerMI);

/*			// 안전지대 체크
			// 2003.1.10 by bezz, Sequoia
			if (checkZoneLevelToHitTarget(pAttacker))
			{
				if (pAttacker->isSlayer())
				{
					Slayer* pSlayerAttacker = dynamic_cast<Slayer*>(pAttacker);
					Result2 = max(0, (int)pSlayerAttacker->getHP()-(int)counterDamage); 
					pSlayerAttacker->setHP(Result2, ATTR_CURRENT);

					bBroadcastAttackerHP = true;
					bSendAttackerHP      = true;
				}
				else if (pAttacker->isVampire())
				{
					Vampire* pVampireAttacker = dynamic_cast<Vampire*>(pAttacker);
					Result2 = max(0, (int)pVampireAttacker->getHP()-(int)counterDamage); 
					pVampireAttacker->setHP(Result2, ATTR_CURRENT);

					bBroadcastAttackerHP = true;
					bSendAttackerHP      = true;

					// Mephisto 이펙트 걸려있으면 HP 30% 이하일때 풀린다.
					if (pVampireAttacker->isFlag(Effect::EFFECT_CLASS_MEPHISTO))
					{
						HP_t maxHP = pVampireAttacker->getHP(ATTR_MAX);

						// 33% ... 케케..
						if (Result2*3 < maxHP)
						{
							pVampireAttacker->getEffectManager()->deleteEffect(Effect::EFFECT_CLASS_MEPHISTO);
						}
					}
				}
				else if (pAttacker->isMonster())
				{
					Monster* pMonsterAttacker = dynamic_cast<Monster*>(pAttacker);
					Result2 = max(0, (int)pMonsterAttacker->getHP()-(int)counterDamage); 
					pMonsterAttacker->setHP(Result2, ATTR_CURRENT);
					pMonsterAttacker->setDamaged(true);

					// 몬스터가 역 데미지를 받을 경우에도 샤프실드 쓰고 공격받은 슬레이어에게 우선권이 주어진다.
					pMonsterAttacker->addPrecedence(pSlayer->getName(), pSlayer->getPartyID(), counterDamage);
					pMonsterAttacker->setLastHitCreatureClass(pSlayer->getCreatureClass());

					bBroadcastAttackerHP = true;
					if (pMonsterAttacker->getHP(ATTR_CURRENT)*3 < pMonsterAttacker->getHP(ATTR_MAX))
					{
						PrecedenceTable* pTable = pMonsterAttacker->getPrecedenceTable();

						// HP가 3분의 1 이하인 상태라고 무조건 계산을 하면, 
						// 매턴마다 의미가 없는 계산을 계속 하게 되므로, 
						// 한번 계산을 하고 나면, 죽기 전까지는 다시 계산하지 않도록
						// 플래그를 세팅해 준다. 이 플래그를 이용하여 필요없는 계산을 줄인다.
						if (pTable->getComputeFlag() == false)
						{
							// 계산을 해준다.
							pTable->compute();

							// 호스트의 이름과 파티 ID를 이용하여, 이펙트를 걸어준다.
							EffectPrecedence* pEffectPrecedence = new EffectPrecedence(pMonsterAttacker);
							pEffectPrecedence->setDeadline(100);
							pEffectPrecedence->setHostName(pTable->getHostName());
							pEffectPrecedence->setHostPartyID(pTable->getHostPartyID());
							pMonsterAttacker->setFlag(Effect::EFFECT_CLASS_PRECEDENCE);
							pMonsterAttacker->addEffect(pEffectPrecedence);
						}
					}
				}
			}*/
		}

		if(pSlayer->isFlag(Effect::EFFECT_CLASS_AIR_SHIELD_1) && pSkillProperty != NULL && pSkillProperty->isMelee()) {
			bool isUnderworld = false;

#ifdef __UNDERWORLD__
			if (pAttacker != NULL && pAttacker->isMonster()) {
				Monster* pMonster = dynamic_cast<Monster*>(pAttacker);
				if (pMonster != NULL && (pMonster->isUnderworld() || pMonster->getMonsterType() == 599)) isUnderworld = true;
			}
#endif

			if (!isUnderworld) {
				EffectAirShield* pEffect = dynamic_cast<EffectAirShield*>(pSlayer->findEffect(Effect::EFFECT_CLASS_AIR_SHIELD_1));
				Assert(pEffect!=NULL);

				Damage = max (1, (int)Damage - getPercentValue(OriginalDamage, pEffect->getDamage()));
			}
		}

		Tile& rTile = pZone->getTile(TX, TY);

		if (rTile.getEffect(Effect::EFFECT_CLASS_MAGIC_ELUSION) != NULL &&
			// Magic Elusion 이 걸려있을 때, 뱀파이어가 사용한 마법 레인지 공격에 대해 데미지를 50% 줄여준다.
			(!pSkillProperty->isMelee() && pSkillProperty->isMagic()) &&
			pAttacker != NULL && pAttacker->isVampire())
			Damage /= 2;

		if(!bSetDamage) {
			if (pSlayer->isFlag(Effect::EFFECT_CLASS_HAS_BLOOD_BIBLE))
				Damage = (Damage_t)(Damage * 1.5);

//			if (pSkillProperty->isMagic())
			if (bMagicDamage)
				Damage -= min(Damage-1, (int)pSlayer->getMagicDamageReduce());
//			else if (pSkillProperty->isPhysic())
			else if (bPhysicDamage)
				Damage -= min(Damage-1, (int)pSlayer->getPhysicDamageReduce());

			// AuraShield가 없으니, 그냥 몸으로 맞아야 한다.
			if (canKillTarget)
				Result = max(0, (int)pSlayer->getHP(ATTR_CURRENT) - (int)Damage);
			else
				Result = max(1, (int)pSlayer->getHP(ATTR_CURRENT) - (int)Damage);

			pSlayer->setHP(Result, ATTR_CURRENT);

			bBroadcastTargetHP = true;
			bSendTargetHP      = true;
		}
	} 
	////////////////////////////////////////////////////////////
	// 맞는 놈이 뱀파이어일 경우
	////////////////////////////////////////////////////////////
	else if (pTargetCreature->isVampire()) {
		Vampire* pVampire     = dynamic_cast<Vampire *>(pTargetCreature);
		Silver_t silverDamage = 0;

		Tile& rTile = pZone->getTile(TX, TY);

		if (rTile.getEffect(Effect::EFFECT_CLASS_GRAY_DARKNESS) != NULL &&
			canBlockByGrayDarkness(SkillType))
			// 그레이 다크니스 안에서 데미지 30%감소
			Damage = (Damage_t)(Damage*0.7);

		if (pAttacker != NULL && pAttacker->isSlayer())
			// 공격자가 슬레이어라면 데미지에 은 데미지가 추가될 수가 있다.
			silverDamage = computeSlayerSilverDamage(pAttacker, Damage, pAttackerMI);

		// 은 데미지는 추가 데미지이다.
		Damage += silverDamage;

		if (pVampire->isFlag(Effect::EFFECT_CLASS_HAS_BLOOD_BIBLE))
			Damage = (Damage_t)(Damage * 1.5);

//		if (pSkillProperty->isMagic())
		if (bMagicDamage)
			Damage -= min(Damage-1, (int)pVampire->getMagicDamageReduce());
//		else if (pSkillProperty->isPhysic())
		else if (bPhysicDamage)
			Damage -= min(Damage-1, (int)pVampire->getPhysicDamageReduce());

		HP_t currentHP = pVampire->getHP(ATTR_CURRENT);

		if (canKillTarget)
			Result = max(0, (int)currentHP - (int)Damage);
		else
			Result = max(1, (int)currentHP - (int)Damage);

		pVampire->setHP(Result, ATTR_CURRENT);

		// Mephisto 이펙트 걸려있으면 HP 30% 이하일때 풀린다.
		if (pVampire->isFlag(Effect::EFFECT_CLASS_MEPHISTO)) {
			HP_t maxHP = pVampire->getHP(ATTR_MAX);

			// 33% ... 케케..
			if (currentHP*3 < maxHP) {
				Effect* pEffect = pVampire->findEffect(Effect::EFFECT_CLASS_MEPHISTO);
				if (pEffect != NULL)
					pEffect->setDeadline(0);
				else
					pVampire->removeFlag(Effect::EFFECT_CLASS_MEPHISTO);
//				pVampire->getEffectManager()->deleteEffect(Effect::EFFECT_CLASS_MEPHISTO);
			}
		}


		bBroadcastTargetHP = true;
		bSendTargetHP      = true;

		if (silverDamage != 0 && pVampire->getSilverDamage() < (pVampire->getHP(ATTR_MAX)/2)) {
			Silver_t newSilverDamage = pVampire->getSilverDamage() + silverDamage;
			pVampire->saveSilverDamage(newSilverDamage);
			if (pMI)
				pMI->addShortData(MODIFY_SILVER_DAMAGE, newSilverDamage);
		}
	} 
	////////////////////////////////////////////////////////////
	// 맞는 놈이 아우스터스일 경우
	////////////////////////////////////////////////////////////
	else if (pTargetCreature->isOusters()) {
		Ousters* pOusters     = dynamic_cast<Ousters *>(pTargetCreature);
		Silver_t silverDamage = 0;

		if (pOusters->isFlag(Effect::EFFECT_CLASS_REACTIVE_ARMOR)) {
			EffectReactiveArmor* pEffect = dynamic_cast<EffectReactiveArmor*>(pOusters->findEffect(Effect::EFFECT_CLASS_REACTIVE_ARMOR));
			if (pEffect != NULL && pEffect->getDamageReduce() != 0)
				Damage -= getPercentValue(Damage, pEffect->getDamageReduce());
		}

		// Divine Shield 로 인해 마법 데미지가 일부 MP로 흡수된다.
		if (pOusters->isFlag(Effect::EFFECT_CLASS_DIVINE_SPIRITS) && pSkillProperty->isMagic()) {
			EffectDivineSpirits* pEffect = dynamic_cast<EffectDivineSpirits*>(pOusters->findEffect(Effect::EFFECT_CLASS_DIVINE_SPIRITS));
			if (pEffect != NULL) {
				int Ratio = pEffect->getBonus();
				Damage_t mpDamage = (Damage_t)(((DWORD)Damage) * Ratio / 100);
				mpDamage = min(Damage, mpDamage);
				mpDamage = min(pOusters->getMP(), mpDamage);

				Damage -= mpDamage;

				MP_t resultMP = pOusters->getMP() - mpDamage;
				pOusters->setMP(resultMP, ATTR_CURRENT);
				bSendTargetMP = true;
			}
		}

		if (pOusters->isFlag(Effect::EFFECT_CLASS_FROZEN_ARMOR) && pSkillProperty->isMelee()) {
			EffectFrozenArmor* pFrozenArmor = dynamic_cast<EffectFrozenArmor*>(pOusters->findEffect(Effect::EFFECT_CLASS_FROZEN_ARMOR));
			if (pFrozenArmor != NULL) {
				Damage -= getPercentValue(Damage, pFrozenArmor->getBonus());
				if (pAttacker != NULL) {
					// 이팩트 클래스를 만들어 붙인다.
					EffectIceFieldToCreature* pEffect = new EffectIceFieldToCreature(pAttacker, true);
					pEffect->setDeadline(pFrozenArmor->getTargetDuration());
					pAttacker->addEffect(pEffect);
					pAttacker->setFlag(Effect::EFFECT_CLASS_ICE_FIELD_TO_CREATURE);

					GCAddEffect gcAddEffect;
					gcAddEffect.setObjectID(pAttacker->getObjectID());
					gcAddEffect.setEffectID(pEffect->getSendEffectClass());
					gcAddEffect.setDuration(pFrozenArmor->getTargetDuration());

					pAttacker->getZone()->broadcastPacket(pAttacker->getX(), pAttacker->getY(), &gcAddEffect);
				}
			}
		}

		if (pAttacker != NULL && pAttacker->isSlayer()) {
			// 공격자가 슬레이어라면 데미지에 은 데미지가 추가될 수가 있다.
			// 아우스터스는 은 데미지를 1.5배 받는다.
			silverDamage = (Silver_t)(computeSlayerSilverDamage(pAttacker, Damage, pAttackerMI) * 1.5);
			silverDamage = max(0, getPercentValue(silverDamage, 100 - pOusters->getSilverResist()));
		}

		// 은 데미지는 추가 데미지이다.
		Damage += silverDamage;

		HP_t currentHP = pOusters->getHP(ATTR_CURRENT);

//		if (pSkillProperty->isMagic())
		if (bMagicDamage)
			Damage -= min(Damage-1, (int)pOusters->getMagicDamageReduce());
//		else if (pSkillProperty->isPhysic())
		else if (bPhysicDamage)
			Damage -= min(Damage-1, (int)pOusters->getPhysicDamageReduce());

		if (canKillTarget)
			Result = max(0, (int)currentHP - (int)Damage);
		else
			Result = max(1, (int)currentHP - (int)Damage);

		pOusters->setHP(Result, ATTR_CURRENT);

		bBroadcastTargetHP = true;
		bSendTargetHP      = true;

		if (silverDamage != 0 && pOusters->getSilverDamage() < (pOusters->getHP(ATTR_MAX)/2)) {
			Silver_t newSilverDamage = pOusters->getSilverDamage() + silverDamage;
			pOusters->saveSilverDamage(newSilverDamage);
			if (pMI)
				pMI->addShortData(MODIFY_SILVER_DAMAGE, newSilverDamage);
		}
	} 
	////////////////////////////////////////////////////////////
	// 맞는 놈이 몬스터일 경우
	////////////////////////////////////////////////////////////
	else if (pTargetCreature->isMonster()) {
		Monster* pMonster     = dynamic_cast<Monster *>(pTargetCreature);
		Silver_t silverDamage = 0;

		if (pAttacker != NULL && pAttacker->isSlayer())
			// 공격자가 슬레이어라면 데미지에 은 데미지가 추가될 수가 있다.
			silverDamage = computeSlayerSilverDamage(pAttacker, Damage, pAttackerMI);

		// 은 데미지는 추가 데미지이다.
		Damage += silverDamage;

		if (canKillTarget)
			Result = max(0, (int)pMonster->getHP(ATTR_CURRENT) - (int)Damage);
		else
			Result = max(1, (int)pMonster->getHP(ATTR_CURRENT) - (int)Damage);

		pMonster->setHP(Result,  ATTR_CURRENT);

		if (pMonster->isFlag(Effect::EFFECT_CLASS_SHARE_HP)) {
			EffectShareHP* pEffect = dynamic_cast<EffectShareHP*>(pMonster->findEffect(Effect::EFFECT_CLASS_SHARE_HP));
			if (pEffect != NULL) {
				list<ObjectID_t>::iterator itr = pEffect->getSharingCreatures().begin();

				for (; itr != pEffect->getSharingCreatures().end() ; ++itr) {
					Monster* pMonster = dynamic_cast<Monster*>(pZone->getCreature(*itr));
					if (pMonster != NULL) {
						GCStatusCurrentHP gcHP;
						pMonster->setHP(Result, ATTR_CURRENT);
						gcHP.setObjectID(pMonster->getObjectID());
						gcHP.setCurrentHP(Result);
						pZone->broadcastPacket(pMonster->getX(), pMonster->getY(), &gcHP);
					}
				}
			}
		}

		if (Result == 0 && pAttacker!=NULL && pAttacker->isPC()) {
			increaseFame(pAttacker, Damage);

			PlayerCreature* pAttackerPC = dynamic_cast<PlayerCreature*>(pAttacker);
			Assert(pAttackerPC != NULL);

			GCModifyInformation gcFameMI;
			gcFameMI.addLongData(MODIFY_FAME, pAttackerPC->getFame());

			pAttackerPC->getPlayer()->sendPacket(&gcFameMI);

//			if (pAttackerMI != NULL) pAttackerMI->addLongData(MODIFY_FAME, pAttackerPC->getFame());
		}

		bBroadcastTargetHP = true;

		if (silverDamage != 0 && pMonster->getSilverDamage() < (pMonster->getHP(ATTR_MAX)/2))
			pMonster->setSilverDamage(pMonster->getSilverDamage() + silverDamage);

		pMonster->setDamaged(true);

		if (pAttacker != NULL && pAttacker->isPC()) {
			// 맞는 놈이 몬스터이고, 공격자가 사람이라면,
			// 데미지에 따라서 변하는 우선권 테이블을 갱신해 주어야 한다.
			pMonster->addPrecedence(pAttacker->getName(), pAttacker->getPartyID(), Damage);
			pMonster->setLastHitCreatureClass(pAttacker->getCreatureClass());
		}

		/*
		// 몬스터가 만약 죽었다면 우선권 계산을 해줘야 한다.
		if (pMonster->isDead())
		{
			PrecedenceTable* pTable = pMonster->getPrecedenceTable();
			
			pTable->compute();

			pMonster->setHostName(pTable->getHostName());
			pMonster->setHostPartyID(pTable->getHostPartyID());
		}
		*/
		// 몬스터가 아직 죽지는 않았지만, 흡혈이 가능한 상태라면,
		// 만약 우선권 계산을 하지 않았다면 계산을 해준다.
		if (pMonster->getHP(ATTR_CURRENT)*3 < pMonster->getHP(ATTR_MAX)) {
			PrecedenceTable* pTable = pMonster->getPrecedenceTable();

			// HP가 3분의 1 이하인 상태라고 무조건 계산을 하면, 
			// 매턴마다 의미가 없는 계산을 계속 하게 되므로, 
			// 한번 계산을 하고 나면, 죽기 전까지는 다시 계산하지 않도록
			// 플래그를 세팅해 준다. 이 플래그를 이용하여 필요없는 계산을 줄인다.
			if (pTable->getComputeFlag() == false) {
				// 계산을 해준다.
				pTable->compute();

				// 호스트의 이름과 파티 ID를 이용하여, 이펙트를 걸어준다.
				EffectPrecedence* pEffectPrecedence = new EffectPrecedence(pMonster);
				pEffectPrecedence->setDeadline(100);
				pEffectPrecedence->setHostName(pTable->getHostName());
				pEffectPrecedence->setHostPartyID(pTable->getHostPartyID());
				pMonster->setFlag(Effect::EFFECT_CLASS_PRECEDENCE);
				pMonster->addEffect(pEffectPrecedence);
			}
		}

		if (pMonster->getMonsterType() == 722 && pAttacker != NULL && !pAttacker->isFlag(Effect::EFFECT_CLASS_BLINDNESS)) {
			if ((rand()%100)<30) {
				// 질드레 석상이지롱
				EffectBlindness* pEffect = new EffectBlindness(pAttacker);
				pEffect->setDamage(50);
				pEffect->setNextTime(0);
				pEffect->setDeadline(100);
				pAttacker->setFlag(Effect::EFFECT_CLASS_BLINDNESS);
				pAttacker->addEffect(pEffect);

				GCAddEffect gcAddEffect;
				gcAddEffect.setObjectID(pAttacker->getObjectID());
				gcAddEffect.setEffectID(pEffect->getSendEffectClass());
				gcAddEffect.setDuration(100);

				pZone->broadcastPacket(pAttacker->getX(), pAttacker->getY(), &gcAddEffect);
			}
		}
	}

//	if (pAttacker != NULL) pAttacker->setLastTarget(pTargetCreature->getObjectID());

	////////////////////////////////////////////////////////////
	// 변경된 사항을 플래그에 따라서 보내준다.
	////////////////////////////////////////////////////////////
	if (bBroadcastTargetHP && pTargetCreature != NULL) { // 맞는 놈의 hp가 줄었으니, 브로드 캐스팅해준다.
		if (pTargetCreature->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pTargetCreature);
			gcTargetHP.setCurrentHP(pSlayer->getHP(ATTR_CURRENT));
		} else if (pTargetCreature->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pTargetCreature);
			gcTargetHP.setCurrentHP(pVampire->getHP(ATTR_CURRENT));
		} else if (pTargetCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pTargetCreature);
			gcTargetHP.setCurrentHP(pOusters->getHP(ATTR_CURRENT));
		} else if (pTargetCreature->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pTargetCreature);
			gcTargetHP.setCurrentHP(pMonster->getHP(ATTR_CURRENT));
		}
		else
			Assert(false);

		pZone->broadcastPacket(TX, TY, &gcTargetHP, pTargetCreature);
	}

	if (bSendTargetHP && pTargetCreature != NULL) { // 맞는 당사자에게 HP가 줄었다고 알려준다. 
		if (pTargetCreature->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pTargetCreature);
			if (pMI != NULL)
				pMI->addShortData(MODIFY_CURRENT_HP, pSlayer->getHP(ATTR_CURRENT));
			//if (pSlayer->getHP(ATTR_CURRENT) == 0) increaseFame(pAttacker, Damage);
		} else if (pTargetCreature->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pTargetCreature);
			if (pMI != NULL)
				pMI->addShortData(MODIFY_CURRENT_HP, pVampire->getHP(ATTR_CURRENT));
			//if (pVampire->getHP(ATTR_CURRENT) == 0) increaseFame(pAttacker, Damage);
		} else if (pTargetCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pTargetCreature);
			if (pMI != NULL) pMI->addShortData(MODIFY_CURRENT_HP, pOusters->getHP(ATTR_CURRENT));
		}
		else
			Assert(false);
	}

	if (bSendTargetMP && pTargetCreature != NULL) { // 맞는 당사자에게 MP가 줄었다고 알려준다.
		if (pTargetCreature->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pTargetCreature);
			if (pMI != NULL)
				pMI->addShortData(MODIFY_CURRENT_MP, pSlayer->getMP(ATTR_CURRENT));
			else {
				GCModifyInformation gcMI;
				gcMI.addShortData(MODIFY_CURRENT_MP, pSlayer->getMP(ATTR_CURRENT));

				pSlayer->getPlayer()->sendPacket(&gcMI);
			}
		} else if (pTargetCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pTargetCreature);
			if (pMI != NULL)
				pMI->addShortData(MODIFY_CURRENT_MP, pOusters->getMP(ATTR_CURRENT));
			else {
				GCModifyInformation gcMI;
				gcMI.addShortData(MODIFY_CURRENT_MP, pOusters->getMP(ATTR_CURRENT));

				pOusters->getPlayer()->sendPacket(&gcMI);
			}
		}
		else
			Assert(false);
	}

	if (bBroadcastAttackerHP && pAttacker != NULL) { // 때리는 놈의 HP가 줄었으니, 브로드캐스팅해준다.
		if (pAttacker->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pAttacker);
			gcAttackerHP.setCurrentHP(pSlayer->getHP(ATTR_CURRENT));
		} else if (pAttacker->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pAttacker);
			gcAttackerHP.setCurrentHP(pVampire->getHP(ATTR_CURRENT));
		} else if (pAttacker->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pAttacker);
			gcAttackerHP.setCurrentHP(pOusters->getHP(ATTR_CURRENT));
		} else if (pAttacker->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pAttacker);
			gcAttackerHP.setCurrentHP(pMonster->getHP(ATTR_CURRENT));
		}
		else
			Assert(false);

		pZone->broadcastPacket(AX, AY, &gcAttackerHP, pAttacker);
	}

	if (bSendAttackerHP && pAttacker != NULL) { // 때리는 놈에게 HP가 줄었다고 알려준다.
		if (pAttacker->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pAttacker);
			if (pAttackerMI != NULL) pAttackerMI->addShortData(MODIFY_CURRENT_HP, pSlayer->getHP(ATTR_CURRENT));
			//if (pSlayer->getHP(ATTR_CURRENT) == 0) increaseFame(pTargetCreature, Damage);
		} else if (pAttacker->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pAttacker);
			if (pAttackerMI != NULL) pAttackerMI->addShortData(MODIFY_CURRENT_HP, pVampire->getHP(ATTR_CURRENT));
			//if (pVampire->getHP(ATTR_CURRENT) == 0) increaseFame(pTargetCreature, Damage);
		} else if (pAttacker->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pAttacker);
			if (pAttackerMI != NULL) pAttackerMI->addShortData(MODIFY_CURRENT_HP, pOusters->getHP(ATTR_CURRENT));
		}
		else
			Assert(false);
	}

	if (bSendAttackerMP && pAttacker != NULL) { // 때리는 놈에게 MP가 줄었다고 알려준다.
		if (pAttacker->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pAttacker);
			if (pAttackerMI != NULL) pAttackerMI->addShortData(MODIFY_CURRENT_MP, pSlayer->getMP(ATTR_CURRENT));
		} else if (pAttacker->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pAttacker);
			if (pAttackerMI != NULL) pAttackerMI->addShortData(MODIFY_CURRENT_MP, pOusters->getMP(ATTR_CURRENT));
		}
		else
			Assert(false);
	}

	// 죽인 경우의 KillCount 증가. by sigi. 2002.8.31
	if (pTargetCreature->isDead())
		affectKillCount(pAttacker, pTargetCreature);
		
	return Result;
}

//////////////////////////////////////////////////////////////////////////////
// 아이템 내구도를 떨어뜨린다.
//////////////////////////////////////////////////////////////////////////////
void decreaseDurability(Creature* pCreature, Creature* pTargetCreature, SkillInfo* pSkillInfo, ModifyInfo* pMI1, ModifyInfo* pMI2) {
//#ifdef __TEST_SERVER__
	WORD Point = (pSkillInfo)?(pSkillInfo->getConsumeMP()/3):1;
//#else
//	WORD Point = (pSkillInfo)?pSkillInfo->getPoint():1;
//#endif

	// 떨어뜨릴 내구도가 0이라면 걍 리턴해야쥐...
	if (Point == 0)
		return;
	
	Item*        pWeapon = NULL;
	Item*        pGear   = NULL;
	int          slot    = 0;
	int          durDiff = 0;
	int          CurDur  = 0;
	int          Result  = 0;
	ulong        value   = 0;

	////////////////////////////////////////////////////////////////
	// 공격하는 자의 무기 내구도 떨어트림.
	// 무기 들고 있는 자는 무조건 슬레이어 아닌가...
	////////////////////////////////////////////////////////////////
	if (pCreature != NULL) {
		if (pCreature->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);

			slot    = Slayer::WEAR_RIGHTHAND;
			pWeapon = pSlayer->getWearItem((Slayer::WearPart)slot);

			// 무기를 들고 있다면 떨어뜨린다.
			if (pWeapon != NULL && canDecreaseDurability(pWeapon))
//				&& !pWeapon->isUnique())
			{
				CurDur  = pWeapon->getDurability();
				durDiff = Point;
				Result  = max(0, CurDur - durDiff);

				if (Result == 0) { // 무기가 내구도가 0이라면 파괴한다.
									// 내구도가 0이 되면 파괴하는 대신에 걍 못쓰게 한다.
					// 무기의 내구도 저장
					pWeapon->setDurability(Result);

					// 바뀐 정보를 보내준다.
					SLAYER_RECORD prev;
					pSlayer->getSlayerRecord(prev);
					pSlayer->initAllStat();
					pSlayer->sendRealWearingInfo();
					pSlayer->addModifyInfo(prev, *pMI1);

					value = (DWORD)(slot) << 24 | (DWORD)(Result);
					pMI1->addLongData(MODIFY_DURABILITY, value);

					/*GCRemoveFromGear gcRemoveFromGear;
					gcRemoveFromGear.setSlotID(slot);
					pSlayer->takeOffItem((Slayer::WearPart)slot, false, true);

					Player* pPlayer = pSlayer->getPlayer();
					pPlayer->sendPacket(&gcRemoveFromGear);	

					// 로그
					log(LOG_DESTROY_ITEM, pCreature->getName(), "", pWeapon->toString());

					// 떨어진 내구성을 저장한다.
					pWeapon->setDurability(Result);
					pWeapon->save(pCreature->getName(), STORAGE_GEAR, 0, slot, 0);

					// DB에서 삭제한다.
					pWeapon->destroy();
					SAFE_DELETE(pWeapon);
					*/
				} else { 
					if (pMI1 == NULL)
						return;

					pWeapon->setDurability(Result);

//					value = MAKEDWORD(slot, Result);
					value = (DWORD)(slot) << 24 | (DWORD)(Result);
					pMI1->addLongData(MODIFY_DURABILITY, value);

					// 떨어진 내구성을 저장한다.
					//pWeapon->save(pCreature->getName(), STORAGE_GEAR, 0, slot, 0);
				} 
			} //if (pWeapon != NULL)
		} //if (pCreature->isSlayer()) 
		else if (pCreature->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);

			slot    = Vampire::WEAR_RIGHTHAND;
			pWeapon = pVampire->getWearItem((Vampire::WearPart)slot);

			// 무기를 들고 있다면 떨어뜨린다.
			if (pWeapon != NULL && canDecreaseDurability(pWeapon)) {
				CurDur  = pWeapon->getDurability();
				durDiff = Point;
				Result  = max(0, CurDur - durDiff);

				if (Result == 0) { // 무기가 내구도가 0이라면 파괴한다.
									// 내구도가 0이 되면 파괴하는 대신에 걍 못쓰게 한다.
					// 무기의 내구도 저장
					pWeapon->setDurability(Result);

					// 바뀐 정보를 보내준다.
					VAMPIRE_RECORD prev;
					pVampire->getVampireRecord(prev);
					pVampire->initAllStat();
					pVampire->sendRealWearingInfo();
					pVampire->addModifyInfo(prev, *pMI1);

					value = (DWORD)(slot) << 24 | (DWORD)(Result);
					pMI1->addLongData(MODIFY_DURABILITY, value);

					/*GCRemoveFromGear gcRemoveFromGear;
					gcRemoveFromGear.setSlotID(slot);
					pVampire->takeOffItem((Vampire::WearPart)slot, false, true);

					Player* pPlayer = pVampire->getPlayer();
					pPlayer->sendPacket(&gcRemoveFromGear);	

					// 로그
					log(LOG_DESTROY_ITEM, pCreature->getName(), "", pWeapon->toString());

					// 떨어진 내구성을 저장한다.
					pWeapon->setDurability(Result);
					pWeapon->save(pCreature->getName(), STORAGE_GEAR, 0, slot, 0);

					// DB에서 삭제한다.
					pWeapon->destroy();
					SAFE_DELETE(pWeapon);
					*/
				} else {
					if (pMI1 == NULL)
						return;

					pWeapon->setDurability(Result);

//					value = MAKEDWORD(slot, Result);
					value = (DWORD)(slot) << 24 | (DWORD)(Result);
					pMI1->addLongData(MODIFY_DURABILITY, value);
				} 
			} //if (pWeapon != NULL)
		} //if (pCreature->isVampire()) 
		else if (pCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);

			slot    = Ousters::WEAR_RIGHTHAND;
			pWeapon = pOusters->getWearItem((Ousters::WearPart)slot);

			// 무기를 들고 있다면 떨어뜨린다.
			if (pWeapon != NULL && canDecreaseDurability(pWeapon)) {
				CurDur  = pWeapon->getDurability();
				durDiff = Point;
				Result  = max(0, CurDur - durDiff);

				if (Result == 0) { // 무기가 내구도가 0이라면 파괴한다.
									// 내구도가 0이 되면 파괴하는 대신에 걍 못쓰게 한다.
					// 무기의 내구도 저장
					pWeapon->setDurability(Result);

					// 바뀐 정보를 보내준다.
					OUSTERS_RECORD prev;
					pOusters->getOustersRecord(prev);
					pOusters->initAllStat();
					pOusters->sendRealWearingInfo();
					pOusters->addModifyInfo(prev, *pMI1);

					value = (DWORD)(slot) << 24 | (DWORD)(Result);
					pMI1->addLongData(MODIFY_DURABILITY, value);

					/*GCRemoveFromGear gcRemoveFromGear;
					gcRemoveFromGear.setSlotID(slot);
					pOusters->takeOffItem((Ousters::WearPart)slot, false, true);

					Player* pPlayer = pOusters->getPlayer();
					pPlayer->sendPacket(&gcRemoveFromGear);	

					// 로그
					log(LOG_DESTROY_ITEM, pCreature->getName(), "", pWeapon->toString());

					// 떨어진 내구성을 저장한다.
					pWeapon->setDurability(Result);
					pWeapon->save(pCreature->getName(), STORAGE_GEAR, 0, slot, 0);

					// DB에서 삭제한다.
					pWeapon->destroy();
					SAFE_DELETE(pWeapon);
					*/
				} else { 
					if (pMI1 == NULL)
						return;

					pWeapon->setDurability(Result);

//					value = MAKEDWORD(slot, Result);
					value = (DWORD)(slot) << 24 | (DWORD)(Result);
					pMI1->addLongData(MODIFY_DURABILITY, value);
				} 
			} //if (pWeapon != NULL)
		} //if (pCreature->isOusters()) 
	} //if (pCreature != NULL)
	
	////////////////////////////////////////////////////////////////
	// 공격당하는 자의 방어구 Durability을 떨어트림 랜덤하게 
	////////////////////////////////////////////////////////////////
	if (pTargetCreature != NULL) {
		// 어느 슬랏에 있는 기어의 내구도를 떨어뜨릴지 결정한다.
		if (pTargetCreature->isSlayer()) {
			slot = Random(0, Slayer::WEAR_MAX-1);
			pGear = dynamic_cast<Slayer*>(pTargetCreature)->getWearItem((Slayer::WearPart)slot);
		} else if (pTargetCreature->isVampire()) {
			slot  = Random(0, Vampire::VAMPIRE_WEAR_MAX-1);
			pGear = dynamic_cast<Vampire*>(pTargetCreature)->getWearItem((Vampire::WearPart)slot);
		} else if (pTargetCreature->isOusters()) {
			slot  = Random(0, Ousters::OUSTERS_WEAR_MAX-1);
			pGear = dynamic_cast<Ousters*>(pTargetCreature)->getWearItem((Ousters::WearPart)slot);
		} 

		// 선택된 슬랏에 아이템을 장착하고 있다면
		// vampire amulet은 안 닳는다.
		if (pGear != NULL && canDecreaseDurability(pGear))
//			&& !pGear->isUnique()
//			&& pGear->getItemClass()!=Item::ITEM_CLASS_VAMPIRE_AMULET) 
		{
			// 선택된 슬랏에 존재하는 아이템이 양손 무기라면, 
			// 슬랏을 무조건 오른쪽으로 바꾸어준다.
			if (isTwohandWeapon(pGear)) {
				if (pTargetCreature->isSlayer())
					slot = Slayer::WEAR_RIGHTHAND;
				else if (pTargetCreature->isVampire()) 
					slot = Vampire::WEAR_RIGHTHAND;
				else if (pTargetCreature->isOusters()) 
					slot = Ousters::WEAR_RIGHTHAND;
			}

			CurDur  = pGear->getDurability();
			durDiff = Point * Random(1,3);
			Result  = max(0, CurDur - durDiff);

			if (Result == 0) {
				// 내구도를 저장
				pGear->setDurability(Result);

				// 바뀐 정보를 보내준다.

				if (pTargetCreature->isSlayer()) {
					Slayer* pTargetSlayer = dynamic_cast<Slayer*>(pTargetCreature);
					Assert(pTargetSlayer != NULL);

					Player* pPlayer = pTargetSlayer->getPlayer();
					Assert(pPlayer != NULL);

					SLAYER_RECORD prev;
					pTargetSlayer->getSlayerRecord(prev);
					pTargetSlayer->initAllStat();
					pTargetSlayer->sendRealWearingInfo();

					if (pMI2 != NULL) {
						pTargetSlayer->addModifyInfo(prev, *pMI2);
						value = (DWORD)(slot) << 24 | (DWORD)(Result);
						pMI2->addLongData(MODIFY_DURABILITY, value);
					} else {
						GCModifyInformation gcModifyInformation;
						pTargetSlayer->addModifyInfo(prev, gcModifyInformation);
						value = (DWORD)(slot) << 24 | (DWORD)(Result);
						gcModifyInformation.addLongData(MODIFY_DURABILITY, value);
						
						pPlayer->sendPacket(&gcModifyInformation);
					}
				} else if (pTargetCreature->isVampire()) {
					Vampire* pTargetVampire = dynamic_cast<Vampire*>(pTargetCreature);
					Assert(pTargetVampire != NULL);

					Player* pPlayer = pTargetVampire->getPlayer();
					Assert(pPlayer != NULL);

					VAMPIRE_RECORD prev;
					pTargetVampire->getVampireRecord(prev);
					pTargetVampire->initAllStat();
					pTargetVampire->sendRealWearingInfo();

					if (pMI2 != NULL) {
						pTargetVampire->addModifyInfo(prev, *pMI2);
						value = (DWORD)(slot) << 24 | (DWORD)(Result);
						pMI2->addLongData(MODIFY_DURABILITY, value);
					} else {
						GCModifyInformation gcModifyInformation;
						pTargetVampire->addModifyInfo(prev, gcModifyInformation);
						value = (DWORD)(slot) << 24 | (DWORD)(Result);
						gcModifyInformation.addLongData(MODIFY_DURABILITY, value);

						pPlayer->sendPacket(&gcModifyInformation);
					}
				} else if (pTargetCreature->isOusters()) {
					Ousters* pTargetOusters = dynamic_cast<Ousters*>(pTargetCreature);
					Assert(pTargetOusters != NULL);

					Player* pPlayer = pTargetOusters->getPlayer();
					Assert(pPlayer != NULL);

					OUSTERS_RECORD prev;
					pTargetOusters->getOustersRecord(prev);
					pTargetOusters->initAllStat();
					pTargetOusters->sendRealWearingInfo();

					if (pMI2 != NULL) {
						pTargetOusters->addModifyInfo(prev, *pMI2);
						value = (DWORD)(slot) << 24 | (DWORD)(Result);
						pMI2->addLongData(MODIFY_DURABILITY, value);
					} else {
						GCModifyInformation gcModifyInformation;
						pTargetOusters->addModifyInfo(prev, gcModifyInformation);
						value = (DWORD)(slot) << 24 | (DWORD)(Result);
						gcModifyInformation.addLongData(MODIFY_DURABILITY, value);

						pPlayer->sendPacket(&gcModifyInformation);
					}
				}

				/*GCRemoveFromGear gcRemoveFromGear;
				gcRemoveFromGear.setSlotID(slot);
				if (pTargetCreature->isSlayer())
				{
					Slayer* pTargetSlayer = dynamic_cast<Slayer*>(pTargetCreature);
					Player* pPlayer = pTargetSlayer->getPlayer();

					Assert(pTargetSlayer != NULL);
					Assert(pPlayer != NULL);

					pTargetSlayer->takeOffItem((Slayer::WearPart)slot, false, true);
					pPlayer->sendPacket(&gcRemoveFromGear);
				}
				else if (pTargetCreature->isVampire())
				{
					Vampire* pTargetVampire = dynamic_cast<Vampire*>(pTargetCreature);
					Player* pPlayer = pTargetVampire->getPlayer();

					Assert(pTargetVampire != NULL);
					Assert(pPlayer != NULL);

					pTargetVampire->takeOffItem((Vampire::WearPart)slot, false, true);
					pPlayer->sendPacket(&gcRemoveFromGear);
				}
				else if (pTargetCreature->isOusters())
				{
					Ousters* pTargetOusters = dynamic_cast<Ousters*>(pTargetCreature);
					Player* pPlayer = pTargetOusters->getPlayer();

					Assert(pTargetOusters != NULL);
					Assert(pPlayer != NULL);

					pTargetOusters->takeOffItem((Ousters::WearPart)slot, false, true);
					pPlayer->sendPacket(&gcRemoveFromGear);
				}

				// 로그
				log(LOG_DESTROY_ITEM, pTargetCreature->getName(), "", pGear->toString());

				// 파괴
				pGear->save(pTargetCreature->getName(), STORAGE_GEAR, 0, slot, 0);
				pGear->destroy();
				SAFE_DELETE(pGear);
				*/
			} else { 
				pGear->setDurability(Result);

//				value = MAKEDWORD(slot, Result);
				value = (DWORD)(slot) << 24 | (DWORD)(Result);

				if (pMI2 == NULL)
					return;
				
				pMI2->addLongData(MODIFY_DURABILITY, value);

				// 떨어진 내구성을 저장한다.
				//pGear->save(pTargetCreature->getName(), STORAGE_GEAR, 0, slot, 0);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// 타겟을 맞출 가능성이 있는가?
//////////////////////////////////////////////////////////////////////////////
bool canHit(Creature* pAttacker, Creature* pDefender, SkillType_t SkillType, SkillLevel_t SkillLevel) {
	// 무적 상태
//	if (pDefender->isFlag(Effect::EFFECT_CLASS_NO_DAMAGE))
//	{
//		return false;
//	}

	// 스킬의 종류에 무관하게, 맞출 수 없는 상태를 체크한다.
	if (pAttacker->isSlayer()) {
		// 종족 검사도 할 수 있지만,
		// 속도 문제로 될 수 있는 한 체크를 적게 하기 위해서 생략했다.

		// Attacker 의 Revealer 이펙트를 가져온다.
		EffectRevealer* pEffectRevealer = NULL;
		if (pAttacker->isFlag(Effect::EFFECT_CLASS_REVEALER)) {
			pEffectRevealer = dynamic_cast<EffectRevealer*>(pAttacker->findEffect(Effect::EFFECT_CLASS_REVEALER));
			Assert(pEffectRevealer);
		}
		
		// 하이드하고 있으면, Detect hidden 마법이 걸려있어야 볼 수 있다.
		if (pDefender->isFlag(Effect::EFFECT_CLASS_HIDE)) {
			if (!pAttacker->isFlag(Effect::EFFECT_CLASS_DETECT_HIDDEN) &&
				!(pEffectRevealer != NULL && pEffectRevealer->canSeeHide(pDefender)))
				return false;
		}
		// 투명화 상태라면, Detect invisibility 마법이 걸려있어야 볼 수 있다.
		if (pDefender->isFlag(Effect::EFFECT_CLASS_INVISIBILITY))
		{
			if (!pAttacker->isFlag(Effect::EFFECT_CLASS_DETECT_INVISIBILITY) &&
				!(pEffectRevealer != NULL && pEffectRevealer->canSeeInvisibility(pDefender)))
				return false;
		}
	}

	// 스킬의 타입에 따라 맞출 수 있는지 검사한다.
	// 기본 공격은 스킬 인포가 없기 때문에 여기서 체크한다.
	switch (SkillType) {
		// 일반 밀리 공격이나, 흡혈은 날아다니는 상대에게는 불가능하다.
		case SKILL_ATTACK_MELEE:
		case SKILL_BLOOD_DRAIN:
			if (pDefender != NULL) {
				if (pDefender->isFlag(Effect::EFFECT_CLASS_TRANSFORM_TO_BAT))
					return false;
			}
			return true;

		// 총으로 하는 공격은 날아다니는 상대에게도 가능하다.
		case SKILL_ATTACK_ARMS:
			return true;

		default:
			break;
	}

	// 스킬 타입과 상대의 현재 무브모드에 따라, 공격의 가능 여부를 리턴한다.
	SkillInfo* pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);
	Assert(pSkillInfo != NULL);

	uint TType = pSkillInfo->getTarget();

	if ((TType & TARGET_GROUND) && (pDefender->getMoveMode() == Creature::MOVE_MODE_WALKING))
		return true;
	else if ((TType & TARGET_UNDERGROUND) && (pDefender->getMoveMode() == Creature::MOVE_MODE_BURROWING))
		return true;
	else if ((TType & TARGET_AIR) && (pDefender->getMoveMode() == Creature::MOVE_MODE_FLYING))
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////
// 인트에 따라 마나 소모량이 변하는 뱀파이어 마법의 마나 소모량을 계산한다.
//////////////////////////////////////////////////////////////////////////////
MP_t decreaseConsumeMP(Vampire* pVampire, SkillInfo* pSkillInfo) {
	Assert(pVampire != NULL);
	Assert(pSkillInfo != NULL);

	int OriginalMP     = pSkillInfo->getConsumeMP();
	int MagicLevel     = pSkillInfo->getLevel();
	int INTE           = max(0, pVampire->getINT()-20);
	int DecreaseAmount = 0;

	if (INTE <= MagicLevel) {}
	else if (MagicLevel < INTE && INTE <= (MagicLevel*1.5))
		DecreaseAmount = getPercentValue(OriginalMP, 10);
	else if ((MagicLevel*1.5) < INTE && INTE <= (MagicLevel*2.0))
		DecreaseAmount = getPercentValue(OriginalMP, 25);
	else if ((MagicLevel*2.0) < INTE && INTE <= (MagicLevel*3.0))
		DecreaseAmount = getPercentValue(OriginalMP, 50);
	else if ((MagicLevel*3.0) < INTE && INTE <= (MagicLevel*4.0))
		DecreaseAmount = getPercentValue(OriginalMP, 60);
	else if ((MagicLevel*4.0) < INTE && INTE <= (MagicLevel*5.0))
		DecreaseAmount = getPercentValue(OriginalMP, 75);
	else if ((MagicLevel*5.0) < INTE && INTE <= (MagicLevel*6.0))
		DecreaseAmount = getPercentValue(OriginalMP, 85);
	else if ((MagicLevel*6.0) < INTE && INTE <= (MagicLevel*7.0))
		DecreaseAmount = getPercentValue(OriginalMP, 90);
	else if ((MagicLevel*7.0) < INTE)
		DecreaseAmount = getPercentValue(OriginalMP, 95);

	return (OriginalMP - DecreaseAmount);
}


//////////////////////////////////////////////////////////////////////////////
// 기술을 사용하기 위한 충분한 마나를 가지고 있는가?
//////////////////////////////////////////////////////////////////////////////
bool hasEnoughMana(Creature* pCaster, int RequiredMP) {
	if (pCaster->isSlayer()) {
		Slayer* pSlayer = dynamic_cast<Slayer*>(pCaster);

		int decreaseRatio = pSlayer->getConsumeMPRatio(); 
		if (decreaseRatio != 0)
			RequiredMP += getPercentValue(RequiredMP, decreaseRatio);

		// Sacrifice를 쓴 상태라면 마나가 모자라도 HP로 대신할 수 있다.
		if (pSlayer->isFlag(Effect::EFFECT_CLASS_SACRIFICE)) {
			//cout << "RequiredMP : " << (int)RequiredMP << endl;
			int margin = RequiredMP - pSlayer->getMP(ATTR_CURRENT);
			//cout << "margin: " << (int)margin<< endl;

			// 요구치에서 현재 수치를 뺀 값이 0이상이라면 , 
			// 요구치가 더 크다는 말이다. 이 수치는 HP에서 제거한다.
			if (margin > 0) {
				margin = (int)pSlayer->getHP(ATTR_CURRENT)*2 - (int)margin ;
				//cout << "margin: " << (int)margin<< endl;
				if (margin > 0) return true;
			}
			else
				return true;
		}
		else
			if (pSlayer->getMP(ATTR_CURRENT) >= (MP_t)RequiredMP)
				return true;
	} else if (pCaster->isVampire()) {
		Vampire* pVampire = dynamic_cast<Vampire*>(pCaster);

		// 뱀파이어는 HP가 곧 MP이기 때문에 마나를 사용하고,
		// 죽어버리면 곤란하다. 그러므로 기술을 사용하고 나서 
		// HP는 1 이상이어야 한다. 그래서 >= 대신 >를 사용한다.

		int decreaseRatio = pVampire->getConsumeMPRatio();
		if (decreaseRatio != 0)
			RequiredMP += getPercentValue(RequiredMP, decreaseRatio);

		if (pVampire->getHP(ATTR_CURRENT) > (HP_t)RequiredMP)
			return true;
	} else if (pCaster->isOusters()) {
		Ousters* pOusters = dynamic_cast<Ousters*>(pCaster);

		int decreaseRatio = pOusters->getConsumeMPRatio();
		if (decreaseRatio != 0)
			RequiredMP += getPercentValue(RequiredMP, decreaseRatio);

		if (pOusters->getMP(ATTR_CURRENT) >= (MP_t)RequiredMP)
			return true;
	} else if (pCaster->isMonster()) {
		// 몬스터는 무한 마나 되겠다. 음홧홧 
		// 나중에라도 몬스터에 마법 카운트나 뭐 그럴 것이 생길지도 모르지.
		// comment by 김성민
		return true;
	} else {
		cerr << "hasEnoughMana() : Invalid Creature Class" << endl;
		Assert(false);
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////////
// 주어진 포인트만큼의 마나를 줄인다.
// 단 슬레이어 같은 경우에는 Sacrifice 같은 이펙트가 붙어있으면,
// 마나가 모자랄 경우, HP가 닳을 수도 있다.
//////////////////////////////////////////////////////////////////////////////
int decreaseMana(Creature* pCaster, int MP, ModifyInfo& info) {
	Assert(pCaster != NULL);

	int RemainHP = 0;
	int RemainMP = 0;

	if (pCaster->isFlag(Effect::EFFECT_CLASS_NO_DAMAGE))
		return 0;

	if (pCaster->isSlayer()) {
		Slayer* pSlayer = dynamic_cast<Slayer*>(pCaster);

		// Magic Brain 이 있다면 MP 소모량 25% 감소
		if (pSlayer->hasRankBonus(RankBonus::RANK_BONUS_MAGIC_BRAIN)) {
			RankBonus* pRankBonus = pSlayer->getRankBonus(RankBonus::RANK_BONUS_MAGIC_BRAIN);
			Assert(pRankBonus != NULL);

			MP -= getPercentValue(MP, pRankBonus->getPoint());
		}

		// Blood Bible 보너스 적용
		int decreaseRatio = pSlayer->getConsumeMPRatio();
		if (decreaseRatio != 0) {
			// decreaseRatio 값 자체가 마이너스 값이다.
			MP += getPercentValue(MP, decreaseRatio);
		}

		// sacrifice를 쓴 상태라면 먼저 MP에서 깍고, 모자라면 HP도 깍는다.
		if (pSlayer->isFlag(Effect::EFFECT_CLASS_SACRIFICE)) {
			int margin = (int)MP - (int)pSlayer->getMP(ATTR_CURRENT);

			// 마진이 0보다 크다는 말은 요구치보다 현재 MP가 적다는 말이다.
			if (margin > 0) {
				// MP를 깍고...
				pSlayer->setMP(0, ATTR_CURRENT);
				// HP도 깍는다.
				RemainHP = max(0, (int)(pSlayer->getHP(ATTR_CURRENT) - margin/2));
				pSlayer->setHP(RemainHP, ATTR_CURRENT);

				info.addShortData(MODIFY_CURRENT_MP, pSlayer->getMP(ATTR_CURRENT));
				info.addShortData(MODIFY_CURRENT_HP, pSlayer->getHP(ATTR_CURRENT));
				return CONSUME_BOTH;
			}

			// sacrifice를 쓰지 않은 상태라면 걍 MP에서 깍는다.
			RemainMP = max(0, ((int)pSlayer->getMP(ATTR_CURRENT) - (int)MP));
			pSlayer->setMP(RemainMP, ATTR_CURRENT);

			info.addShortData(MODIFY_CURRENT_MP, pSlayer->getMP(ATTR_CURRENT));
			return CONSUME_MP;
		} else { // sacrifice를 쓰지 않은 상태라면 걍 MP에서 깍는다.
			RemainMP = max(0, ((int)pSlayer->getMP(ATTR_CURRENT) - (int)MP));
			pSlayer->setMP(RemainMP, ATTR_CURRENT);

			info.addShortData(MODIFY_CURRENT_MP, pSlayer->getMP(ATTR_CURRENT));
			return CONSUME_MP;
		}
	} else if (pCaster->isVampire()) {
		Vampire* pVampire = dynamic_cast<Vampire*>(pCaster);

		// Wisdom of Blood 가 있다면 HP 소모량 10% 감소
		if (pVampire->hasRankBonus(RankBonus::RANK_BONUS_WISDOM_OF_BLOOD)) {
			RankBonus* pRankBonus = pVampire->getRankBonus(RankBonus::RANK_BONUS_WISDOM_OF_BLOOD);
			Assert(pRankBonus != NULL);

			MP -= getPercentValue(MP, pRankBonus->getPoint());
		}

		// Blood Bible 보너스 적용
		int decreaseRatio = pVampire->getConsumeMPRatio();
		if (decreaseRatio != 0) {
			// decreaseRatio 값 자체가 마이너스 값이다.
			MP += getPercentValue(MP, decreaseRatio);
		}

		HP_t currentHP = pVampire->getHP(ATTR_CURRENT);
		RemainHP = max(0, ((int)currentHP - (int)MP));
		pVampire->setHP(RemainHP, ATTR_CURRENT);

		// Mephisto 이펙트 걸려있으면 HP 30% 이하일때 풀린다.
		if (pVampire->isFlag(Effect::EFFECT_CLASS_MEPHISTO)) {
			HP_t maxHP = pVampire->getHP(ATTR_MAX);

			// 33% ... 케케..
			if (currentHP*3 < maxHP) {
				Effect* pEffect = pVampire->findEffect(Effect::EFFECT_CLASS_MEPHISTO);
				if (pEffect != NULL)
					pEffect->setDeadline(0);
				else
					pVampire->removeFlag(Effect::EFFECT_CLASS_MEPHISTO);
//				pVampire->getEffectManager()->deleteEffect(Effect::EFFECT_CLASS_MEPHISTO);
			}
		}

		info.addShortData(MODIFY_CURRENT_HP, pVampire->getHP(ATTR_CURRENT));
		return CONSUME_HP;
	} else if (pCaster->isOusters()) {
		Ousters* pOusters = dynamic_cast<Ousters*>(pCaster);

		// Blood Bible 보너스 적용
		int decreaseRatio = pOusters->getConsumeMPRatio();
		if (decreaseRatio != 0) {
			// decreaseRatio 값 자체가 마이너스 값이다.
			MP += getPercentValue(MP, decreaseRatio);
		}

		RemainMP = max(0, ((int)pOusters->getMP(ATTR_CURRENT) - (int)MP));
		pOusters->setMP(RemainMP, ATTR_CURRENT);

		info.addShortData(MODIFY_CURRENT_MP, pOusters->getMP(ATTR_CURRENT));
		return CONSUME_MP;
	} else if (pCaster->isMonster()) {
		// 몬스터는 무한 마나 되겠다. 음홧홧 
		// 나중에라도 몬스터에 마법 카운트나 뭐 그럴 것이 생길지도 모르지.
		// comment by 김성민
		cerr << "decreaseMana() : Monster don't have Mana" << endl;
		Assert(false);
	} else {
		cerr << "hasEnoughMana() : Invalid Creature Class" << endl;
		Assert(false);
	}

	return CONSUME_MP;
}

//////////////////////////////////////////////////////////////////////////////
// 슬레이어용 스킬의 사정거리를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Range_t computeSkillRange(SkillSlot* pSkillSlot, SkillInfo* pSkillInfo) {
	Assert(pSkillSlot != NULL);
	Assert(pSkillInfo != NULL);

	// Skill의 Min/Max Range 를 받아온다.
	Range_t SkillMinPoint = pSkillInfo->getMinRange();
	Range_t SkillMaxPoint = pSkillInfo->getMaxRange();

	// Skill Level을 받아온다.
	SkillLevel_t SkillLevel = pSkillSlot->getExpLevel();

	// Skill의 Range를 계산한다.
	Range_t Range = (int)(SkillMinPoint + (SkillMaxPoint - SkillMinPoint)* (double)(SkillLevel* 0.01));

	return Range;
}


//////////////////////////////////////////////////////////////////////////////
// (OX,OY)와 (TX,TY) 사이의 거리를 구한다.
//////////////////////////////////////////////////////////////////////////////
Range_t getDistance(ZoneCoord_t Ox, ZoneCoord_t Oy, ZoneCoord_t Tx, ZoneCoord_t Ty) {
	double OriginX = Ox;
	double OriginY = Oy;
	double TargetX = Tx;
	double TargetY = Ty;

	double  XOffset = pow(OriginX - TargetX, 2.0);
	double  YOffset = pow(OriginY - TargetY, 2.0);
	Range_t range   = (Range_t)(sqrt(XOffset + YOffset));

	return range;
}

//////////////////////////////////////////////////////////////////////////////
// 스킬을 쓸 수 있는 적당한 거리인가를 검증한다.
//////////////////////////////////////////////////////////////////////////////
bool verifyDistance(Creature* pCreature, ZoneCoord_t X, ZoneCoord_t Y, Range_t Dist) {
	Assert(pCreature != NULL);

	Zone* pZone = pCreature->getZone();
	Assert(pZone != NULL);

	ZoneCoord_t cx = pCreature->getX();
	ZoneCoord_t cy = pCreature->getY();

	ZoneLevel_t AttackerZoneLevel = pZone->getZoneLevel(cx, cy);
	ZoneLevel_t DefenderZoneLevel = pZone->getZoneLevel(X, Y);

	// 아담의 성지나 PK존 내의 안전지대에서는 기술을 사용할 수 없다.
	if ((AttackerZoneLevel & SAFE_ZONE) && (g_pPKZoneInfoManager->isPKZone(pZone->getZoneID()) || pZone->isHolyLand()))
		return false;

	// 공격자가 서 있는 위치가 슬레이어 안전지대라면, 
	// 슬레이어가 아닌 자는 기술을 사용할 수 없다.
	if ((AttackerZoneLevel & SLAYER_SAFE_ZONE) && !pCreature->isSlayer())
		return false;
	// 공격자가 서 있는 위치가 뱀파이어 안전지대라면,
	// 뱀파이어가 아닌 자는 기술을 사용할 수 없다.
	else if ((AttackerZoneLevel & VAMPIRE_SAFE_ZONE) && !pCreature->isVampire())
		return false;
	// 공격자가 서 있는 위치가 아우스터스 안전지대라면,
	// 아우스터스가 아닌 자는 기술을 사용할 수 없다.
	else if ((AttackerZoneLevel & OUSTERS_SAFE_ZONE) && !pCreature->isOusters())
		return false;
	// 완전 안전지대라면 슬레이어든 뱀파이어든 기술을 사용할 수 없다.
	else if (AttackerZoneLevel & COMPLETE_SAFE_ZONE)
		return false;

	// 방어자가 서 있는 곳이 완전지대라면 기술을 사용할 수 없다.
//	if (DefenderZoneLevel & COMPLETE_SAFE_ZONE)
//		return false;

	if ((abs(cx - X) <= Dist) && (abs(cy - Y) <= Dist))
		return true;

	return false;
}


//////////////////////////////////////////////////////////////////////////////
// 스킬을 쓸 수 있는 적당한 거리인가를 검증한다.
//////////////////////////////////////////////////////////////////////////////
bool verifyDistance(Creature* pCreature, Creature* pTargetCreature, Range_t Dist) {
	Assert(pCreature != NULL);
	Assert(pTargetCreature != NULL);

	Zone* pZone = pCreature->getZone();
	Assert(pZone != NULL);

	int ox = pCreature->getX();
	int oy = pCreature->getY();
	int tx = pTargetCreature->getX();
	int ty = pTargetCreature->getY();

	ZoneLevel_t AttackerZoneLevel = pZone->getZoneLevel(ox, oy);
	ZoneLevel_t DefenderZoneLevel = pZone->getZoneLevel(tx, ty);

	// 아담의 성지나 PK존 내의 안전지대에서는 기술을 사용할 수 없다.
	if ((AttackerZoneLevel & SAFE_ZONE) && (g_pPKZoneInfoManager->isPKZone(pZone->getZoneID()) || pZone->isHolyLand()))
		return false;

	// 공격자가 서 있는 위치가 슬레이어 안전지대라면, 
	// 슬레이어가 아닌 자는 기술을 사용할 수 없다.
	if ((AttackerZoneLevel & SLAYER_SAFE_ZONE) && !pCreature->isSlayer())
		return false;
	// 공격자가 서 있는 위치가 뱀파이어 안전지대라면,
	// 뱀파이어가 아닌 자는 기술을 사용할 수 없다.
	else if ((AttackerZoneLevel & VAMPIRE_SAFE_ZONE) && !pCreature->isVampire())
		return false;
	// 공격자가 서 있는 위치가 아우스터스 안전지대라면,
	// 아우스터스가 아닌 자는 기술을 사용할 수 없다.
	else if ((AttackerZoneLevel & OUSTERS_SAFE_ZONE) && !pCreature->isOusters())
		return false;
	// 완전 안전지대라면 슬레이어든 뱀파이어든 기술을 사용할 수 없다.
	else if (AttackerZoneLevel & COMPLETE_SAFE_ZONE)
		return false;

	// 방어자가 서 있는 위치가 슬레이어 안전지대이고,
	// 방어자가 슬레이어라면 기술은 맞지 않는다.
	if ((DefenderZoneLevel & SLAYER_SAFE_ZONE) && pTargetCreature->isSlayer())
		return false;
	// 방어자가 서 있는 위치가 뱀파이어 안전지대이고,
	// 방어자가 뱀파이어라면 기술은 맞지 않는다.
	else if ((DefenderZoneLevel & VAMPIRE_SAFE_ZONE) && pTargetCreature->isVampire())
		return false;
	// 방어자가 서 있는 위치가 아우스터스 안전지대이고,
	// 방어자가 아우스터스라면 기술은 맞지 않는다.
	else if ((DefenderZoneLevel & OUSTERS_SAFE_ZONE) && pTargetCreature->isOusters())
		return false;
	// 완전 안전지대라면 기술을 사용할 수 없다.
	else if (DefenderZoneLevel & COMPLETE_SAFE_ZONE)
		return false;

	if ((abs(tx - ox) <= Dist) && (abs(ty - oy) <= Dist))
		return true;

	return false;
}


//////////////////////////////////////////////////////////////////////////////
// 슬레이어용 스킬의 실행시간을 검증한다.
//////////////////////////////////////////////////////////////////////////////
bool verifyRunTime(SkillSlot* pSkillSlot) {
	Assert(pSkillSlot != NULL);

	Timeval CurrentTime;
	Timeval LastTime = pSkillSlot->getRunTime();
		
	getCurrentTime(CurrentTime);

	if (CurrentTime > LastTime)
		return true;

	return false;
}


//////////////////////////////////////////////////////////////////////////////
// 뱀파이어용 스킬의 실행시간을 검증한다.
//////////////////////////////////////////////////////////////////////////////
bool verifyRunTime(VampireSkillSlot* pSkillSlot) {
	Assert(pSkillSlot != NULL);

	Timeval CurrentTime;
	Timeval LastTime = pSkillSlot->getRunTime();
		
	getCurrentTime(CurrentTime);

	if (CurrentTime > LastTime)
		return true;

	return false;
}


//////////////////////////////////////////////////////////////////////////////
// 아우스터스용 스킬의 실행시간을 검증한다.
//////////////////////////////////////////////////////////////////////////////
bool verifyRunTime(OustersSkillSlot* pSkillSlot) {
	Assert(pSkillSlot != NULL);

	Timeval CurrentTime;
	Timeval LastTime = pSkillSlot->getRunTime();
		
	getCurrentTime(CurrentTime);

	if (CurrentTime > LastTime)
		return true;

	return false;
}


//////////////////////////////////////////////////////////////////////////////
// 각 존의 PK 정책에 따라, PK가 되느냐 안 되느냐를 정한다.
//////////////////////////////////////////////////////////////////////////////
bool verifyPK(Creature* pAttacker, Creature* pDefender) {
	Zone* pZone = pDefender->getZone();
	Assert(pZone != NULL);

	if (pDefender != NULL && pAttacker != NULL) {
		if (pZone->getZoneID() == 1412 || pZone->getZoneID() == 1413) {
			if (pDefender->isPC() && pAttacker->isPC())
				return false;
		}

		if (pDefender->getCreatureClass() == pAttacker->getCreatureClass() && pAttacker->isPC()) {
			// 존 레벨이 PK가 안 되는 곳이라면 공격할 수 없다.
			if (pZone->getZoneLevel() == NO_PK_ZONE)
				return false;

			// 같은 파티원끼리는 공격할 수 없다.
			int PartyID1 = pAttacker->getPartyID();
			int PartyID2 = pDefender->getPartyID();
			if (PartyID1 != 0 && PartyID1 == PartyID2)
				return false;

			if (pDefender->isFlag(Effect::EFFECT_CLASS_HAS_FLAG) || pDefender->isFlag(Effect::EFFECT_CLASS_HAS_SWEEPER)) {
				if (pAttacker->isPC() && !dynamic_cast<PlayerCreature*>(pAttacker)->hasEnemy(pDefender->getName()))
					return false;
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////
// 기술을 사용할 수 있는 존인가?
// (셀프 기술일 경우, 존 레벨을 체크하는 함수다...)
//////////////////////////////////////////////////////////////////////////////
bool checkZoneLevelToUseSkill(Creature* pCaster) {
	Assert(pCaster != NULL);

	if (pCaster->isFlag(Effect::EFFECT_CLASS_PLEASURE_EXPLOSION))
		return false;

	Zone* pZone = pCaster->getZone();
	Assert(pZone != NULL);

	ZoneCoord_t cx = pCaster->getX();
	ZoneCoord_t cy = pCaster->getY();
	ZoneLevel_t ZoneLevel = pZone->getZoneLevel(cx, cy);

	// 안전지대에서는 셀프 기술을 사용할 수 없다.
	if ((ZoneLevel & SAFE_ZONE))// && pZone->isHolyLand())
		return false;

	if (pCaster->isFlag(Effect::EFFECT_CLASS_REFINIUM_TICKET))
		return false;

/*	// 슬레이어 안전지대에서는 슬레이어만이 기술을 사용할 수 있다.
	if ((ZoneLevel & SLAYER_SAFE_ZONE) && !pCaster->isSlayer())
		return false;
	// 마찬가지로 뱀파이어 안전지대에서는 뱀파이어만이 기술을 사용할 수 있다.
	else if ((ZoneLevel & VAMPIRE_SAFE_ZONE) && !pCaster->isVampire())
		return false;
	// 통합 안전지대에서는 누구도 기술을 사용할 수 없다.
	else if (ZoneLevel & COMPLETE_SAFE_ZONE) 
		return false;
*/
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// X, Y에 서 있는 크리쳐가 임의의 기술에 영향을 받는지 체크하는 함수다.
//////////////////////////////////////////////////////////////////////////////
bool checkZoneLevelToHitTarget(Creature* pTargetCreature) {
	Assert(pTargetCreature != NULL);

	Zone* pZone = pTargetCreature->getZone();
	Assert(pZone != NULL);

	ZoneCoord_t tx = pTargetCreature->getX();
	ZoneCoord_t ty = pTargetCreature->getY();
	ZoneLevel_t ZoneLevel = pZone->getZoneLevel(tx, ty);

	// 슬레이어 안전지대에서 슬레이어는 기술에 맞지 않는다.
	if ((ZoneLevel & SLAYER_SAFE_ZONE) && pTargetCreature->isSlayer())
		return false;
	// 뱀파이어 안전지대에서 뱀파이어는 기술에 맞지 않는다.
	else if ((ZoneLevel & VAMPIRE_SAFE_ZONE) && pTargetCreature->isVampire())
		return false;
	// 아우스터즈 안전지대에서 아우스터즈는 기술에 맞지 않는다.
	else if ((ZoneLevel & OUSTERS_SAFE_ZONE) && pTargetCreature->isOusters())
		return false;
	// 통합 안전지대에서는 누구도 맞지 않는다.
	else if (ZoneLevel & COMPLETE_SAFE_ZONE) 
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////
// 성향을 변경한다.
// 기술을 사용하거나, PK를 할 때 생기는 성향 변화를 계산하는 함수다.
//////////////////////////////////////////////////////////////////////////////
void computeAlignmentChange(Creature* pTargetCreature, Damage_t Damage, Creature* pAttacker, ModifyInfo* pMI, ModifyInfo* pAttackerMI) {
	Assert(pTargetCreature != NULL);

	// PK존에서는 성향이 변하지 않는다.
	if (g_pPKZoneInfoManager->isPKZone(pAttacker->getZoneID()))
		return;

	Zone* pZone = pTargetCreature->getZone();
	Assert(pZone != NULL);

	bool bSameRace = false;

	// 공격자가 있다면, 같은 종족은 아닌지 체크한다.
	if (pAttacker != NULL) {
		// 이벤트 경기장에서는 성향이 안바뀌게 되는 코드이다.
		// ZoneInfo에 넣고, Zone에서 읽을 수 있게 하면 좋겠지만,
		// 갑자기 떨어진 일이라 귀찮다는 이유로 하드 코딩이다. - -; 
		// 2002.8.21. by sigi
		//int zoneID = pAttacker->getZone()->getZoneID();

		//zoneID==1005 || zoneID==1006)

		bSameRace = isSameRace(pTargetCreature, pAttacker);

		bool bPKOlympic = false;

		switch (pAttacker->getZoneID()) {
			case 1122:
			case 1131:
			case 1132:
			case 1133:
			case 1134:
				bPKOlympic = true;
				break;
			default:
				break;
		}

		// 같은 종족이 아니면 올림픽 금메달~
		if (!bSameRace && bPKOlympic && pTargetCreature->isPC() && pTargetCreature->isDead() &&
			!GDRLairManager::Instance().isGDRLairZone(pTargetCreature->getZoneID())) {
			PlayerCreature* pAttackPC = dynamic_cast<PlayerCreature*>(pAttacker);
			if (pAttackPC->getLevel()-10 <= pTargetCreature->getLevel() && !pTargetCreature->isFlag(Effect::EFFECT_CLASS_PK_COUNTED)) {
				addSimpleCreatureEffect(pTargetCreature, Effect::EFFECT_CLASS_PK_COUNTED, 6000, false);
				addOlympicStat(pAttackPC, 2);
			}
		}

		if (pZone->isPKZone())
			return;
	}

	// 같은 종족이라면 성향에 변화가 생길 수 있다.
	if (bSameRace) {
		PlayerCreature* pAttackPC = dynamic_cast<PlayerCreature*>(pAttacker);
		PlayerCreature* pTargetPC = dynamic_cast<PlayerCreature*>(pTargetCreature);

		string	AttackName = pAttackPC->getName();
		string	TargetName = pTargetPC->getName();
		
		Alignment_t AttackAlignment = pAttackPC->getAlignment();
		Alignment_t TargetAlignment = pTargetPC->getAlignment();

		Alignment_t ModifyAlignment = 0;

		// 감소하는 것인지 증가하는 것인지 알아둔다.
		bool bdecrease = false;
		if (pTargetPC->isDead()) {
			ModifyAlignment = g_pAlignmentManager->getMultiplier(AttackAlignment, TargetAlignment); // Damage* 2

			if (ModifyAlignment < 0) {
				ModifyAlignment = ModifyAlignment * 10;
				bdecrease = true;
			} else if (ModifyAlignment > 0) {
				// (피살자 레벨) / (살해자 레벨) * (기존 성향 획득량) :// max = (기존 성향 획득량)

				if (pAttackPC->getLevel() - 10 <= pTargetPC->getLevel() && !pTargetPC->isFlag(Effect::EFFECT_CLASS_PUNISH_COUNTED)) {
					addOlympicStat(pAttackPC, 7);
					addSimpleCreatureEffect(pTargetPC, Effect::EFFECT_CLASS_PUNISH_COUNTED, 6000, false);
				}

				if (pAttackPC->isSlayer()) {
					Slayer* pAttackSlayer = dynamic_cast<Slayer*>(pAttackPC);
					Slayer* pTargetSlayer = dynamic_cast<Slayer*>(pTargetPC);

					SkillLevel_t AttackLevel = pAttackSlayer->getHighestSkillDomainLevel();
					SkillLevel_t TargetLevel = pTargetSlayer->getHighestSkillDomainLevel();

					if (AttackLevel > TargetLevel  && AttackLevel != 0)
						ModifyAlignment = (Alignment_t)(ModifyAlignment * ((float)TargetLevel / (float)AttackLevel));
				} else if (pAttackPC->isVampire()) {
					Vampire* pAttackVampire = dynamic_cast<Vampire*>(pAttackPC);
					Vampire* pTargetVampire = dynamic_cast<Vampire*>(pTargetPC);

					Level_t AttackLevel = pAttackVampire->getLevel();
					Level_t TargetLevel = pTargetVampire->getLevel();

					if (AttackLevel > TargetLevel  && AttackLevel != 0)
						ModifyAlignment = (Alignment_t)(ModifyAlignment * ((float)TargetLevel / (float)AttackLevel));
				} else if (pAttackPC->isOusters()) {
					Ousters* pAttackOusters = dynamic_cast<Ousters*>(pAttackPC);
					Ousters* pTargetOusters = dynamic_cast<Ousters*>(pTargetPC);

					Level_t AttackLevel = pAttackOusters->getLevel();
					Level_t TargetLevel = pTargetOusters->getLevel();

					if (AttackLevel > TargetLevel  && AttackLevel != 0)
						ModifyAlignment = (Alignment_t)(ModifyAlignment * ((float)TargetLevel / (float)AttackLevel));
				}
			}
			pTargetPC->setPK(true);
		} 

		Alignment_t ResultAlignment = AttackAlignment + ModifyAlignment ;

		ResultAlignment = max(-10000, ResultAlignment);
		ResultAlignment = min(10000, ResultAlignment);

		EffectManager* pAttackEffectManager = pAttackPC->getEffectManager();
		EffectManager* pTargetEffectManager = pTargetPC->getEffectManager();

		// 성향에 관계 없이 정당방위에 해당되지 않는 사람을 때리면 무조건 상대방에게 정당방위 권한을 준다.
		if (!pAttackPC->hasEnemy(TargetName) && g_pAlignmentManager->getAlignmentType(TargetAlignment) >= NEUTRAL) {
			GCAddInjuriousCreature gcAddInjuriousCreature;
			gcAddInjuriousCreature.setName(AttackName);
			pTargetPC->getPlayer()->sendPacket(&gcAddInjuriousCreature);

			// 공격당하는 사람에게 선공자 리스트에 추가하고
			// 5분 뒤에 사라진다는 이펙트를 붙인다.
			pTargetPC->addEnemy(AttackName);

			EffectEnemyErase* pEffectEnemyErase = new EffectEnemyErase(pTargetPC);
			pEffectEnemyErase->setDeadline(3000);
			pEffectEnemyErase->setEnemyName(AttackName);
			pEffectEnemyErase->create(TargetName);
			pTargetEffectManager->addEffect(pEffectEnemyErase);
		}

		// 상대가 나에게 정당방위의 대상이고 상대를 죽였을 경우 이펙트를 지워준다.
		if (pAttackPC->hasEnemy(TargetName) && pTargetPC->isDead()) {
			EffectEnemyErase* pAttackerEffect = (EffectEnemyErase*)pAttackEffectManager->findEffect(Effect::EFFECT_CLASS_ENEMY_ERASE, TargetName);

			if (pAttackerEffect!= NULL) {
				// 선공자 리스트에 있다는 말은 선공자를 지워주는 이펙트가 무조건 있다는 얘기이다. 따라서 NULL이 될 수 없다.
				Assert(pAttackerEffect != NULL);
				Assert(pAttackerEffect->getEffectClass() == Effect::EFFECT_CLASS_ENEMY_ERASE);
				// 지워준다.
				pAttackerEffect->setDeadline(0);
			}
		}

			// 선공자의 리스트에 방어자의 이름이 있고, 자신의 성향이 Good 또는 Neutral 이라면 정당방위로 인정하고, 성향이 떨어지지는 않게 한다.
		if (!(bdecrease && pAttackPC->hasEnemy(TargetName) && g_pAlignmentManager->getAlignmentType(AttackAlignment) >= NEUTRAL)) {
			// 올라가든 내려가든 먼저 셋팅을 해 놓아야 한당.
			// 먼저 셋팅을 해 놓는다.
			if (pAttackerMI && ModifyAlignment != 0) {
				if (pAttackPC->isSlayer()) {
					Slayer* pSlayer = dynamic_cast<Slayer*>(pAttackPC);

					pAttackerMI->addShortData(MODIFY_ALIGNMENT, ResultAlignment);
					pSlayer->setAlignment(ResultAlignment);

					WORD AlignmentSaveCount = pSlayer->getAlignmentSaveCount();
					if (AlignmentSaveCount > ALIGNMENT_SAVE_PERIOD) {
						char pField[80];
						sprintf(pField, "ALIGNMENT=%d", ResultAlignment);
						pSlayer->tinysave(pField);

						AlignmentSaveCount = 0;
					}
					else
						AlignmentSaveCount++;

					pSlayer->setAlignmentSaveCount(AlignmentSaveCount);
				} else if (pAttackPC->isVampire()) {
					Vampire* pVampire = dynamic_cast<Vampire*>(pAttackPC);

					pAttackerMI->addShortData(MODIFY_ALIGNMENT, ResultAlignment);
					pVampire->setAlignment(ResultAlignment);

					WORD AlignmentSaveCount = pVampire->getAlignmentSaveCount();
					if (AlignmentSaveCount > ALIGNMENT_SAVE_PERIOD) {
						char pField[80];
						sprintf(pField, "ALIGNMENT=%d", ResultAlignment);
						pVampire->tinysave(pField);

						AlignmentSaveCount = 0;
					}
					else
						AlignmentSaveCount++;

					pVampire->setAlignmentSaveCount(AlignmentSaveCount);
				} else if (pAttackPC->isOusters()) {
					Ousters* pOusters = dynamic_cast<Ousters*>(pAttackPC);

					pAttackerMI->addShortData(MODIFY_ALIGNMENT, ResultAlignment);
					pOusters->setAlignment(ResultAlignment);

					WORD AlignmentSaveCount = pOusters->getAlignmentSaveCount();
					if (AlignmentSaveCount > ALIGNMENT_SAVE_PERIOD) {
						char pField[80];
						sprintf(pField, "ALIGNMENT=%d", ResultAlignment);
						pOusters->tinysave(pField);

						AlignmentSaveCount = 0;
					}
					else
						AlignmentSaveCount++;

					pOusters->setAlignmentSaveCount(AlignmentSaveCount);
				}
			}

			// 성향이 감소될때 이펙트를 통하여 10배를 줄인다음 서서히 회복시키는 방법이다.
			if (bdecrease) {
				// 만약 방어자의 선공자 리스트에 내 이름이 있다면, 공격자는 나쁜넘이다.
				// 선공자의 리스트에 이름이 있다는 것은 아직 이펙트가 붙어있다는 얘기이다.
				// 성향을 회복시키는 이펙트는 한순간에 하나 이하로 존재할 수 있다. 중복되지 않는다.
				EffectAlignmentRecovery* pAttackerEffect = (EffectAlignmentRecovery*)pAttackEffectManager->findEffect(Effect::EFFECT_CLASS_ALIGNMENT_RECOVERY);
				// 이펙트를 받아와서 값을 다시 셋팅한다.
				// 아마도 선공자의 이름에 내가 있으므로 이펙트는 필시 있을 것이다.
				// 하나 동기가 깨질 수 있는 상황이 깨질 수 있으므로, 데드라인을 약간 길게 잡도록 한다.

				if (pAttackerEffect != NULL) {
					// 얼마나 회복시킬 것 것인가?
					Alignment_t Amount = abs(ModifyAlignment / 10 * 9);

					// 얼마씩 회복시킬 것인가? 10씩
					Alignment_t Quantity = 10;

					// 회복 주기는 얼마인가? 30초
					int     DelayProvider = 300;

					// 몇번 회복시킬 것인가?
					double temp     = (double)((double)Amount/(double)Quantity);
					int    Period   = (uint)floor(temp);

					// 다 회복시키는데 걸리는 시간은 얼마인가?
					Turn_t Deadline = Period* DelayProvider;

					pAttackerEffect->setQuantity(Quantity);
					pAttackerEffect->setPeriod(Period);
					pAttackerEffect->setDeadline(Deadline);
					pAttackerEffect->setDelay(DelayProvider);
				} else {
					// 없다면 최초로 선공하는 것이다 새 이펙트를 생성해서 붙이고 5분간 지속 될 것이다.
					// 방어자에게 사라지는 이펙트를 붙여야 함을 잊지 말아야 한다.
					// 사라지는 것은 상대의 이펙트 메니져에 속해있다.
					// 30초마다 10씩 성향을 회복시키는 이펙트를 공격자에게 붙인다.

					// 얼마나 회복시킬 것 것인가?
					Alignment_t Amount = abs(ModifyAlignment / 10 * 9);

					// 얼마씩 회복시킬 것인가? 10씩
					Alignment_t Quantity = 10;

					// 회복 주기는 얼마인가? 30초
					int     DelayProvider = 300;

					// 몇번 회복시킬 것인가?
					double temp     = (double)((double)Amount/(double)Quantity);
					int    Period   = (uint)floor(temp);

					// 다 회복시키는데 걸리는 시간은 얼마인가?
					Turn_t Deadline = Period* DelayProvider;

					// 먼저 회복 이펙트를 붙인다.
					EffectAlignmentRecovery* pEffectAlignmentRecovery = new EffectAlignmentRecovery();

					pEffectAlignmentRecovery->setTarget(pAttackPC);
					pEffectAlignmentRecovery->setDeadline(Deadline);
					pEffectAlignmentRecovery->setDelay(DelayProvider);
					pEffectAlignmentRecovery->setNextTime(DelayProvider);
					pEffectAlignmentRecovery->setQuantity(Quantity);
					pEffectAlignmentRecovery->setPeriod(Period);

					pAttackEffectManager->addEffect(pEffectAlignmentRecovery);
				}

				// 방어자에게 붙어있는 이펙트의 데드라인을 다시 셋팅 해 주어야 한다.
				EffectEnemyErase* pDefenderEffect = (EffectEnemyErase*)pTargetEffectManager->findEffect(Effect::EFFECT_CLASS_ENEMY_ERASE, AttackName);

				if (pDefenderEffect != NULL) {
					// 선공자 리스트에 있다는 말은 선공자를 지워주는 이펙트가 무조건 있다는 얘기이다. 따라서 NULL이 될 수 없다.
					Assert(pDefenderEffect != NULL);
					Assert(pDefenderEffect->getEffectClass() == Effect::EFFECT_CLASS_ENEMY_ERASE);
					// 5분으로 셋팅
					pDefenderEffect->setDeadline(36000);
					pDefenderEffect->save(TargetName);
				}
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// 슬레이어 및 뱀파이어가 몹을 죽일 때 성향을 약간씩 회복시킨다. 
//////////////////////////////////////////////////////////////////////////////
void increaseAlignment(Creature* pCreature, Creature* pEnemy, ModifyInfo& mi) {
	Assert(pCreature != NULL);
	Assert(pEnemy != NULL);

	// PK존에서는 성향을 안 올려준다.
	if (g_pPKZoneInfoManager->isPKZone(pCreature->getZoneID()))
		return;

	// 다이나믹 존 안에서는 성향을 안 올려준다.
	if (pCreature->getZone() != NULL && pCreature->getZone()->isDynamicZone())
		return;

	// 몬스터가 아직 살아있을 경우에는 성향이 변화되지 않는다.
	if (!pEnemy->isDead()) return;

	// 적이 NPC이거나, 동족끼리 공격하는 경우에는 성향을 증가시키지 않는다.
	if (pEnemy->isNPC()) return;
	if (pCreature->isSlayer() && pEnemy->isSlayer()) return;
	if (pCreature->isVampire() && pEnemy->isVampire()) return;
	if (pCreature->isOusters() && pEnemy->isOusters()) return;

	Alignment_t OldAlignValue = 0;
	Alignment_t NewAlignValue = 0;

	if (pCreature->isSlayer()) {
		Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);

		// 현재 성향 값을 읽어온다.
		OldAlignValue = pSlayer->getAlignment();

		// 성향이 0이상인 경우에는 몬스터를 죽여도 성향의 변화가 없다.
		if (OldAlignValue > 0) return;

		// 올라갈 성향의 수치를 계산한다.
		if (pEnemy->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pEnemy);
			Assert(pMonster != NULL);
			NewAlignValue = max(0, (int)(pMonster->getLevel()/10));
		} else if (pEnemy->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pEnemy);
			Assert(pVampire != NULL);
			NewAlignValue = max(0, (int)(pVampire->getLevel()/5));
		} else if (pEnemy->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pEnemy);
			Assert(pOusters != NULL);
			NewAlignValue = max(0, (int)(pOusters->getLevel()/5));
		}

		NewAlignValue = OldAlignValue + NewAlignValue;

		if (OldAlignValue != NewAlignValue) {
			// 패킷에다 성향이 바뀌었다고 알려준다.
			mi.addShortData(MODIFY_ALIGNMENT, NewAlignValue);

			WORD AlignmentSaveCount = pSlayer->getAlignmentSaveCount();
			if (AlignmentSaveCount > ALIGNMENT_SAVE_PERIOD) {
				pSlayer->saveAlignment(NewAlignValue);
				AlignmentSaveCount = 0;
			} else {
				pSlayer->setAlignment(NewAlignValue);
				AlignmentSaveCount++;
			}

			pSlayer->setAlignmentSaveCount(AlignmentSaveCount);
		}
	} else if (pCreature->isVampire()) {
		Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);

		// 현재 성향 값을 읽어온다.
		OldAlignValue = pVampire->getAlignment();

		// 성향이 0이상인 경우에는 몬스터를 죽여도 성향의 변화가 없다.
		if (OldAlignValue > 0) return;

		// 올라갈 성향의 수치를 계산한다.
		NewAlignValue = 0;
		if (pEnemy->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pEnemy);
			Assert(pMonster != NULL);
			NewAlignValue = max(0, (int)(pMonster->getLevel()/10));
		} else if (pEnemy->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pEnemy);
			Assert(pSlayer != NULL);
			NewAlignValue = max(0, (int)(pSlayer->getSTR(ATTR_BASIC) + pSlayer->getDEX(ATTR_BASIC) + pSlayer->getINT(ATTR_BASIC) + pSlayer->getSkillDomainLevelSum()) / 5);
		} else if (pEnemy->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pEnemy);
			Assert(pOusters != NULL);
			NewAlignValue = max(0, (int)(pOusters->getLevel()/5));
		}

		NewAlignValue = OldAlignValue + NewAlignValue;

		if (OldAlignValue != NewAlignValue) {
			// 패킷에다 성향이 바뀌었다고 알려준다.
			mi.addShortData(MODIFY_ALIGNMENT, NewAlignValue);

			WORD AlignmentSaveCount = pVampire->getAlignmentSaveCount();
			if (AlignmentSaveCount > ALIGNMENT_SAVE_PERIOD) {
				pVampire->saveAlignment(NewAlignValue);
				AlignmentSaveCount = 0;
			} else {
				pVampire->setAlignment(NewAlignValue);
				AlignmentSaveCount++;
			}
		}
	} else if (pCreature->isOusters()) {
		Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);

		// 현재 성향 값을 읽어온다.
		OldAlignValue = pOusters->getAlignment();

		// 성향이 0이상인 경우에는 몬스터를 죽여도 성향의 변화가 없다.
		if (OldAlignValue > 0) return;

		// 올라갈 성향의 수치를 계산한다.
		NewAlignValue = 0;
		if (pEnemy->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pEnemy);
			Assert(pMonster != NULL);
			NewAlignValue = max(0, (int)(pMonster->getLevel()/10));
		} else if (pEnemy->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pEnemy);
			Assert(pSlayer != NULL);
			NewAlignValue = max(0, (int)(pSlayer->getSTR(ATTR_BASIC) + pSlayer->getDEX(ATTR_BASIC) + pSlayer->getINT(ATTR_BASIC) + pSlayer->getSkillDomainLevelSum()) / 5);
		} else if (pEnemy->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pEnemy);
			Assert(pVampire != NULL);
			NewAlignValue = max(0, (int)(pVampire->getLevel()/5));
		}

		NewAlignValue = OldAlignValue + NewAlignValue;

		if (OldAlignValue != NewAlignValue) {
			// 패킷에다 성향이 바뀌었다고 알려준다.
			mi.addShortData(MODIFY_ALIGNMENT, NewAlignValue);

			WORD AlignmentSaveCount = pOusters->getAlignmentSaveCount();
			if (AlignmentSaveCount > ALIGNMENT_SAVE_PERIOD) {
				pOusters->saveAlignment(NewAlignValue);
				AlignmentSaveCount = 0;
			} else {
				pOusters->setAlignment(NewAlignValue);
				AlignmentSaveCount++;
			}
		}
	}

	// 성향 단계가 바뀌면 다른 사람들에게도 알려줘야 한다.  by sigi. 2002.1.6
	Alignment beforeAlignment = g_pAlignmentManager->getAlignmentType(OldAlignValue);
	Alignment afterAlignment = g_pAlignmentManager->getAlignmentType(NewAlignValue);

	if (beforeAlignment!=afterAlignment) {
		GCOtherModifyInfo gcOtherModifyInfo;
		gcOtherModifyInfo.setObjectID(pCreature->getObjectID());
		gcOtherModifyInfo.addShortData(MODIFY_ALIGNMENT, NewAlignValue);

		Zone* pZone = pCreature->getZone();
		Assert(pZone!=NULL);
		pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcOtherModifyInfo, pCreature);
	}
}


//////////////////////////////////////////////////////////////////////////////
// 파티 관련 슬레이어 경험치 계산 함수
//////////////////////////////////////////////////////////////////////////////
void shareAttrExp(Slayer* pSlayer, Damage_t Damage, BYTE STRMultiplier, BYTE DEXMultiplier, BYTE INTMultiplier, ModifyInfo& _ModifyInfo) {
	Assert(pSlayer != NULL);

	// PK존 안에서는 경험치를 주지 않는다.
	if (g_pPKZoneInfoManager->isPKZone(pSlayer->getZoneID()))
		return;

	// 다이나믹 존 안에서는 경험치를 주지 않는다.
	if (pSlayer->getZone() != NULL && pSlayer->getZone()->isDynamicZone())
		return;

	// 유료화 존에서는 경험치를 더 받는다.
	GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(pSlayer->getPlayer());
	Assert(pGamePlayer!=NULL);

	if (pGamePlayer->isPremiumPlay() || pGamePlayer->isFamilyFreePass())//pZone->isPayPlay() || pZone->isPremiumZone())
	{
		Damage = getPercentValue(Damage, g_pVariableManager->getPremiumExpBonusPercent());
		STRMultiplier = getPercentValue(STRMultiplier, g_pVariableManager->getPremiumExpBonusPercent());
		DEXMultiplier = getPercentValue(DEXMultiplier, g_pVariableManager->getPremiumExpBonusPercent());
		INTMultiplier = getPercentValue(INTMultiplier, g_pVariableManager->getPremiumExpBonusPercent());
	}

	if (pGamePlayer->isPCRoomPlay()) {
		Damage = getPercentValue(Damage, g_pVariableManager->getPCRoomExpBonusPercent());
		STRMultiplier = getPercentValue(STRMultiplier, g_pVariableManager->getPCRoomExpBonusPercent());
		DEXMultiplier = getPercentValue(DEXMultiplier, g_pVariableManager->getPCRoomExpBonusPercent());
		INTMultiplier = getPercentValue(INTMultiplier, g_pVariableManager->getPCRoomExpBonusPercent());
	}

	int PartyID = pSlayer->getPartyID();
	if (PartyID != 0) {
		// 파티에 가입되어 있다면 로컬 파티 매니저를 통해 
		// 주위의 파티원들과 경험치를 공유한다.
		LocalPartyManager* pLPM = pSlayer->getLocalPartyManager();
		Assert(pLPM != NULL);
		pLPM->shareAttrExp(PartyID, pSlayer, Damage, STRMultiplier, DEXMultiplier, INTMultiplier, _ModifyInfo);
	}
	else
		// 파티에 가입되어있지 않다면 혼자 올라간다.
		divideAttrExp(pSlayer, Damage, STRMultiplier, DEXMultiplier, INTMultiplier, _ModifyInfo);
}

//////////////////////////////////////////////////////////////////////////////
// 파티 관련 뱀파이어 경험치 계산 함수
//////////////////////////////////////////////////////////////////////////////
void shareVampExp(Vampire* pVampire, Exp_t Point, ModifyInfo& _ModifyInfo) {
	Assert(pVampire != NULL);
	if (Point <= 0) return;

	// PK존 안에서는 경험치를 받지 않는다.
	if (g_pPKZoneInfoManager->isPKZone(pVampire->getZoneID()))
		return;

	// 다이나믹 존 안에서는 경험치를 안 올려준다.
	if (pVampire->getZone() != NULL && pVampire->getZone()->isDynamicZone())
		return;

	// 유료화 존에서는 경험치를 더 받는다.
	GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(pVampire->getPlayer());
	Assert(pGamePlayer!=NULL);

	if (pGamePlayer->isPremiumPlay() || pGamePlayer->isFamilyFreePass())//pZone->isPayPlay() || pZone->isPremiumZone())
		Point = getPercentValue(Point, g_pVariableManager->getPremiumExpBonusPercent());

	if (pGamePlayer->isPCRoomPlay())//pZone->isPayPlay() || pZone->isPremiumZone())
		Point = getPercentValue(Point, g_pVariableManager->getPCRoomExpBonusPercent());
	
	int PartyID = pVampire->getPartyID();
	if (PartyID != 0) {
		// 파티에 가입되어 있다면 로컬 파티 매니저를 통해 
		// 주위의 파티원들과 경험치를 공유한다.
		LocalPartyManager* pLPM = pVampire->getLocalPartyManager();
		Assert(pLPM != NULL);
		pLPM->shareVampireExp(PartyID, pVampire, Point, _ModifyInfo);
	}
	else
		// 파티에 가입되어있지 않다면 혼자 올라간다.
		increaseVampExp(pVampire, Point, _ModifyInfo);
}

//////////////////////////////////////////////////////////////////////////////
// 파티 관련 아우스터스 경험치 계산 함수
//////////////////////////////////////////////////////////////////////////////
void shareOustersExp(Ousters* pOusters, Exp_t Point, ModifyInfo& _ModifyInfo) {
	Assert(pOusters != NULL);
	if (Point <= 0) return;

	// PK존 안에서는 경험치를 받지 않는다.
	if (g_pPKZoneInfoManager->isPKZone(pOusters->getZoneID()))
		return;

	// 다이나믹 존 안에서는 경험치를 안 올려준다.
	if (pOusters->getZone() != NULL && pOusters->getZone()->isDynamicZone())
		return;

	// 유료화 존에서는 경험치를 더 받는다.
	GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(pOusters->getPlayer());
	Assert(pGamePlayer!=NULL);

	if (pGamePlayer->isPremiumPlay() || pGamePlayer->isFamilyFreePass())
		Point = getPercentValue(Point, g_pVariableManager->getPremiumExpBonusPercent());

	if (pGamePlayer->isPCRoomPlay())
		Point = getPercentValue(Point, g_pVariableManager->getPCRoomExpBonusPercent());
	
	int PartyID = pOusters->getPartyID();
	if (PartyID != 0) {
		// 파티에 가입되어 있다면 로컬 파티 매니저를 통해 
		// 주위의 파티원들과 경험치를 공유한다.
		LocalPartyManager* pLPM = pOusters->getLocalPartyManager();
		Assert(pLPM != NULL);
		pLPM->shareOustersExp(PartyID, pOusters, Point, _ModifyInfo);
	}
	else
		// 파티에 가입되어있지 않다면 혼자 올라간다.
		increaseOustersExp(pOusters, Point, _ModifyInfo);
}

/*void decreaseSTR(Slayer* pSlayer)
{
	StringStream  msg1;

	Attr_t CurSTR = pSlayer->getSTR(ATTR_BASIC);

	// exp level과 능력치를 올려준다.
	CurSTR--;
	pSlayer->setSTR(CurSTR, ATTR_BASIC);
	//_ModifyInfo.addLongData(MODIFY_BASIC_STR, CurSTR);

	// 다음 레벨의 STRInfo를 받아온다.
	STRBalanceInfo* pAfterSTRInfo = g_pSTRBalanceInfoManager->getSTRBalanceInfo(CurSTR);
	// 이전 레벨의 STRInfo를 받아온다.
//	STRBalanceInfo* pBeforeSTRInfo = g_pSTRBalanceInfoManager->getSTRBalanceInfo(CurSTR-1);

	// 새로운 목표 경험치를 셋팅해 줘야 한다.
	Exp_t NewGoalExp = pAfterSTRInfo->getGoalExp();
//	Exp_t NewExp = pBeforeSTRInfo->getAccumExp();
	pSlayer->setSTRGoalExp(NewGoalExp);
//	pSlayer->setSTRExp(NewExp);

	// DB에 올라간 능력치를 저장한다.
	msg1 << "STR = " << (int)CurSTR << ", STRGoalExp = " << NewGoalExp;

	pSlayer->tinysave(msg1.toString());

//	cout << "힘을 낮춥니다." << endl;
}

void decreaseINT(Slayer* pSlayer)
{
	StringStream  msg1;

	Attr_t CurINT = pSlayer->getINT(ATTR_BASIC);

	// exp level과 능력치를 올려준다.
	CurINT--;
	pSlayer->setINT(CurINT, ATTR_BASIC);
	//_ModifyInfo.addLongData(MODIFY_BASIC_INT, CurINT);

	// 다음 레벨의 INTInfo를 받아온다.
	INTBalanceInfo* pAfterINTInfo = g_pINTBalanceInfoManager->getINTBalanceInfo(CurINT);
	// 이전 레벨의 INTInfo를 받아온다.
//	INTBalanceInfo* pBeforeINTInfo = g_pINTBalanceInfoManager->getINTBalanceInfo(CurINT-1);

	// 새로운 목표 경험치를 셋팅해 줘야 한다.
	Exp_t NewGoalExp = pAfterINTInfo->getGoalExp();
//	Exp_t NewExp = pBeforeINTInfo->getAccumExp();
	pSlayer->setINTGoalExp(NewGoalExp);
//	pSlayer->setINTExp(NewExp);

	// DB에 올라간 능력치를 저장한다.
	msg1 << "INTE = " << (int)CurINT << ", INTGoalExp = " << NewGoalExp;

	pSlayer->tinysave(msg1.toString());

//	cout << "인트를 낮춥니다." << endl;
}

void decreaseDEX(Slayer* pSlayer)
{
	StringStream  msg1;

	Attr_t CurDEX = pSlayer->getDEX(ATTR_BASIC);

	// exp level과 능력치를 올려준다.
	CurDEX--;
	pSlayer->setDEX(CurDEX, ATTR_BASIC);
	//_ModifyInfo.addLongData(MODIFY_BASIC_DEX, CurDEX);

	// 다음 레벨의 DEXInfo를 받아온다.
	DEXBalanceInfo* pAfterDEXInfo = g_pDEXBalanceInfoManager->getDEXBalanceInfo(CurDEX);
	// 이전 레벨의 DEXInfo를 받아온다.
//	DEXBalanceInfo* pBeforeDEXInfo = g_pDEXBalanceInfoManager->getDEXBalanceInfo(CurDEX-1);

	// 새로운 목표 경험치를 셋팅해 줘야 한다.
	Exp_t NewGoalExp = pAfterDEXInfo->getGoalExp();
//	Exp_t NewExp = pBeforeDEXInfo->getAccumExp();
	pSlayer->setDEXGoalExp(NewGoalExp);
//	pSlayer->setDEXExp(NewExp);

	// DB에 올라간 능력치를 저장한다.
	msg1 << "DEX = " << (int)CurDEX << ", DEXGoalExp = " << NewGoalExp;

	pSlayer->tinysave(msg1.toString());

//	cout << "덱스를 낮춥니다." << endl;
}*/

//////////////////////////////////////////////////////////////////////////////
// 슬레이어 능력치 (STR, DEX, INT) 경험치를 계산한다.
//////////////////////////////////////////////////////////////////////////////
void divideAttrExp(Slayer* pSlayer, Damage_t Damage, BYTE STRMultiplier, BYTE DEXMultiplier, BYTE INTMultiplier, ModifyInfo& _ModifyInfo, int numPartyMember) {
	Assert(pSlayer != NULL);

	// STR 포인트가 제일 크다.
	if(STRMultiplier > DEXMultiplier && STRMultiplier > INTMultiplier)
		pSlayer->divideAttrExp(ATTR_KIND_STR, Damage, _ModifyInfo);
	// DEX 포인트가 제일 크다.
	else if (DEXMultiplier > STRMultiplier && DEXMultiplier > INTMultiplier)
		pSlayer->divideAttrExp(ATTR_KIND_DEX, Damage, _ModifyInfo);
	// INT 포인트가 제일 크다.
	else if (INTMultiplier > STRMultiplier && INTMultiplier > DEXMultiplier)
		pSlayer->divideAttrExp(ATTR_KIND_INT, Damage, _ModifyInfo);

	return;

/*	SkillLevel_t	MaxDomainLevel	= pSlayer->getHighestSkillDomainLevel();
	Attr_t			TotalAttr		= pSlayer->getTotalAttr(ATTR_BASIC);
	Attr_t			TotalAttrBound		= 0;		// 능력치 총합 제한
	Attr_t			AttrBound			= 0;		// 단일 능력치 제한
	Attr_t			OneAttrExpBound		= 0;		// 한 개의 능력치에만 경험치 주는 능력치 총합 경계값

	// 슬레이어 능력치는 도메인 레벨 100이전에는 총합 300으로 제한 된다.(기존처럼 50, 200, 50 으로..)또한 그 이후의 경험치는 누적되지 않는다. 
	// 그리고 도메인 레벨 이 100을 넘어서면 다시 능력치 경험치가 누적되어 능력치가 올라가기 시작한다.
	// 도메인 레벨이 100 아래로 도로 떨어졌어도 능력치 총합이 300을 넘었을 경우 300의 제한을 받지 않는다.

	if (MaxDomainLevel <= SLAYER_BOUND_LEVEL && TotalAttr <= SLAYER_BOUND_ATTR_SUM)
	{
		TotalAttrBound	= SLAYER_BOUND_ATTR_SUM;		// 300
		AttrBound		= SLAYER_BOUND_ATTR;			// 200
		OneAttrExpBound	= SLAYER_BOUND_ONE_EXP_ATTR;	// 200
	}
	else
	{
		TotalAttrBound	= SLAYER_MAX_ATTR_SUM;			// 435
		AttrBound		= SLAYER_MAX_ATTR;				// 295
		OneAttrExpBound	= SLAYER_ONE_EXP_ATTR;			// 400
	}

	// 현재의 슬레이어 능력치를 저장한다.
	SLAYER_RECORD prev;
	pSlayer->getSlayerRecord(prev);

	// 시간대에 따라 올라가는 경험치가 달라진다.
	Damage = (Damage_t)getPercentValue(Damage, AttrExpTimebandFactor[getZoneTimeband(pSlayer->getZone())]);

	// VariableManager에 의한 Point증가치를 계산한다.
	if(g_pVariableManager->getExpRatio()>100 && g_pVariableManager->getEventActivate() == 1)
		Damage = getPercentValue(Damage, g_pVariableManager->getExpRatio());

	Exp_t STRPoint = max(1, Damage * STRMultiplier / 10);
	Exp_t DEXPoint = max(1, Damage * DEXMultiplier / 10);
	Exp_t INTPoint = max(1, Damage * INTMultiplier / 10);

	// 현재 순수 능력치를 받는다.
	Attr_t CurSTR = pSlayer->getSTR(ATTR_BASIC);
	Attr_t CurDEX = pSlayer->getDEX(ATTR_BASIC);
	Attr_t CurINT = pSlayer->getINT(ATTR_BASIC);
	Attr_t CurSUM = CurSTR + CurDEX + CurINT;

	// 능력 합이 200 이상인 사람들은 쓰는 계열에 따라 능력에 바로 적용 된다.
	// 나머지 배분은 무시 하게 된다.
	// 이렇게 되었을때, 계열렙에만 프리 하다면 능력치를 어느정도 자유롭게 올릴 수 있게 된다.
	if(CurSUM >= OneAttrExpBound) {
		// 어느 멀티플라이어가 가장 큰지 조사 한다.
		// STR 포인트가 제일 크다.
		if(STRMultiplier > DEXMultiplier && STRMultiplier > INTMultiplier) {
			DEXPoint = 0;
			DEXMultiplier = 0;
			INTPoint = 0;
			INTMultiplier = 0;
		// DEX 포인트가 제일 크다.
		} else if (DEXMultiplier > STRMultiplier && DEXMultiplier > INTMultiplier) {
			STRPoint = 0;
			STRMultiplier = 0;
			INTPoint = 0;
			INTMultiplier = 0;

		// INT 포인트가 제일 크다.
		} else if (INTMultiplier > STRMultiplier && INTMultiplier > DEXMultiplier) {
			STRPoint = 0;
			STRMultiplier = 0;
			DEXPoint = 0;
			DEXMultiplier = 0;
		}
	}

	// 힘 경험치
	Exp_t CurSTRGoalExp = max(0, (int)(pSlayer->getSTRGoalExp() - STRPoint    ));
	// 덱스 경험치
	Exp_t CurDEXGoalExp = max(0, (int)(pSlayer->getDEXGoalExp() - DEXPoint    ));
	// 인트 경험치
	Exp_t CurINTGoalExp = max(0, (int)(pSlayer->getINTGoalExp() - INTPoint));

	// STR, DEX, INT 경험치를 올린다.
	pSlayer->setSTRGoalExp(CurSTRGoalExp);
	pSlayer->setDEXGoalExp(CurDEXGoalExp);
	pSlayer->setINTGoalExp(CurINTGoalExp);

	bool bInitAll = false;

	// 경험치가 누적되어 기본 능력치가 상승할 때다...
	if (STRMultiplier != 0 && CurSTRGoalExp == 0 && CurSTR < AttrBound)
	{
		bool isUp = true;

		// 능력치 총합이 200을 넘어갈려고 하는 경우.
		if (CurSTR + CurDEX + CurINT >= TotalAttrBound) 
		{
			isUp= true;

			// 힘이 오를 경우 DEX나 INT중 높은것을 떨어트리고, 같을 경우 DEX를 떨어트린다.
			if (CurDEX >= CurINT) 
			{
				decreaseDEX(pSlayer);
			} 
			else 
			{
				decreaseINT(pSlayer);
			}
		}

		if (isUp) 
		{
			StringStream  msg1;

			// exp level과 능력치를 올려준다.
			CurSTR         += 1;
			pSlayer->setSTR(CurSTR, ATTR_BASIC);

			// 새로운 레벨의 STRInfo를 받아온다.
			STRBalanceInfo* pNewSTRInfo = g_pSTRBalanceInfoManager->getSTRBalanceInfo(CurSTR);

			// 새로운 목표 경험치를 셋팅해 줘야 한다.
			Exp_t NewGoalExp = pNewSTRInfo->getGoalExp();
			pSlayer->setSTRGoalExp(NewGoalExp);

			// DB에 올라간 능력치를 저장한다.
			msg1 << "STR = " << (int)CurSTR << ", STRGoalExp = " << NewGoalExp;

			pSlayer->tinysave(msg1.toString());

			bInitAll = true;
		}
	}

	// 경험치가 누적되어 기본 능력치가 상승할 때다...
	if (DEXMultiplier != 0 && CurDEXGoalExp == 0 && CurDEX < AttrBound)
	{
		bool isUp = true;

		// 능력치 총합이 200을 넘어갈려고 하는 경우.
		if (CurSTR + CurDEX + CurINT >= TotalAttrBound) 
		{
			isUp= true;

			// 민첩이 오를 경우 STR나 INT중 높은것을 떨어트리고, 같을 경우 STR를 떨어트린다.
			if (CurSTR >= CurINT) 
			{
				decreaseSTR(pSlayer);
			} 
			else 
			{
				decreaseINT(pSlayer);
			}
		}

		if (isUp) 
		{
			StringStream  msg1;

			// exp level과 능력치를 올려준다.
			CurDEX         += 1;
			pSlayer->setDEX(CurDEX, ATTR_BASIC);

			// 새로운 레벨의 DEXInfo를 받아온다.
			DEXBalanceInfo* pNewDEXInfo = g_pDEXBalanceInfoManager->getDEXBalanceInfo(CurDEX);

			// 새로운 목표 경험치를 셋팅해 줘야 한다.
			Exp_t NewGoalExp = pNewDEXInfo->getGoalExp();
			pSlayer->setDEXGoalExp(NewGoalExp);

			// DB에 올라간 능력치를 저장한다.
			msg1 << "DEX = " << (int)CurDEX << ", DEXGoalExp = " << NewGoalExp;
			pSlayer->tinysave(msg1.toString());

			bInitAll = true;
		}
	}

	// 경험치가 누적되어 기본 능력치가 상승할 때다...
	if (INTMultiplier != 0 && CurINTGoalExp == 0 && CurINT < AttrBound)
	{
		bool isUp = true;
	
		// 능력치 총합이 200을 넘어갈려고 하는 경우.
		if (CurSTR + CurDEX + CurINT >= TotalAttrBound) 
		{
			isUp= true;

			// 지식이 오를 경우 STR나 DEX중 높은것을 떨어트리고, 같을 경우 STR를 떨어트린다.
			if (CurSTR >= CurDEX) 
			{
				decreaseSTR(pSlayer);
			} 
			else 
			{
				decreaseDEX(pSlayer);
			}
		}

		if (isUp) 
		{
			StringStream  msg1;

			// exp level과 능력치를 올려준다.
			CurINT         += 1;
			pSlayer->setINT(CurINT, ATTR_BASIC);
			// 새로운 레벨의 INTInfo를 받아온다.
			INTBalanceInfo* pNewINTInfo = g_pINTBalanceInfoManager->getINTBalanceInfo(CurINT);

			// 새로운 목표 경험치를 셋팅해 줘야 한다.
			Exp_t NewGoalExp = pNewINTInfo->getGoalExp();
			pSlayer->setINTGoalExp(NewGoalExp);

			// DB에 올라간 능력치를 저장한다.
			msg1 << "INTE = " << (int)CurINT << ", INTGoalExp = " << NewGoalExp;

			pSlayer->tinysave(msg1.toString());

			bInitAll = true;
		}
	}

	// 패킷에다 바뀐 데이터를 입력한다. 
	// 능력치가 합계 제한에 의해 내려갈 수도 있으므로 모든 처리를 한 뒤 변경정보를 넣는다 - by Bezz
	_ModifyInfo.addLongData(MODIFY_STR_EXP, pSlayer->getSTRGoalExp());//CurSTRExp);
	_ModifyInfo.addLongData(MODIFY_DEX_EXP, pSlayer->getDEXGoalExp());//CurDEXExp);
	_ModifyInfo.addLongData(MODIFY_INT_EXP, pSlayer->getINTGoalExp());//CurINTExp);

	// 올라간 경험치를 DB에 저장한다.
	WORD AttrExpSaveCount = pSlayer->getAttrExpSaveCount();
	if (AttrExpSaveCount > ATTR_EXP_SAVE_PERIOD)
	{
		char pField[256];
		sprintf(pField, "STRGoalExp=%ld, DEXGoalExp=%ld, INTGoalExp=%ld",
							pSlayer->getSTRGoalExp(), pSlayer->getDEXGoalExp(), pSlayer->getINTGoalExp());

		pSlayer->tinysave(pField);

		AttrExpSaveCount = 0;
	}
	else AttrExpSaveCount++;

	pSlayer->setAttrExpSaveCount(AttrExpSaveCount);

	// 기존의 능력치와 비교해서 변경된 능력치를 보내준다.
	if (bInitAll)
	{
		healCreatureForLevelUp(pSlayer, _ModifyInfo, &prev);

		// 레벨업 이펙트도 보여준다. by sigi. 2002.11.9
        sendEffectLevelUp(pSlayer);

		// 능력치 합이 40이고, 야전사령부이면 딴데로 보낸다.  by sigi. 2002.11.7
		if (g_pVariableManager->isNewbieTransportToGuild())
		{
			checkNewbieTransportToGuild(pSlayer);
		}
	}*/
}


//////////////////////////////////////////////////////////////////////////////
// 슬레이어 기술 경험치를 계산한다.
//////////////////////////////////////////////////////////////////////////////
void increaseSkillExp(Slayer* pSlayer, SkillDomainType_t DomainType, SkillSlot*  pSkillSlot, SkillInfo*  pSkillInfo, ModifyInfo& _ModifyInfo) {
	Assert(pSlayer != NULL);
	Assert(pSkillSlot != NULL);
	Assert(pSkillInfo != NULL);

	if (pSkillInfo->getLevel() >= 150)
        return;

	// PK존 안에서는 경험치를 주지 않는다.
	if (g_pPKZoneInfoManager->isPKZone(pSlayer->getZoneID()))
		return;

	// 다이나믹 존 안에서는 경험치를 주지 않는다.
	if (pSlayer->getZone() != NULL && pSlayer->getZone()->isDynamicZone())
		return;

	// 만약 NewLevel이 현재의 도메인 레벨에서 넘을 수 없는 경우에는 경험치를 올려주지 않는다.
	Level_t CurrentLevel = pSkillSlot->getExpLevel();

	// 현재 슬레이어의 도메인을 받아온다.
	Level_t DomainLevel = pSlayer->getSkillDomainLevel(DomainType);

	// 도메인의 단계를 받아온다.
	SkillGrade Grade = g_pSkillInfoManager->getGradeByDomainLevel(DomainLevel);

	// 현재 단계에서 + 1 한 단계의 제한 레벨을 받아온다.
	Level_t LimitLevel = g_pSkillInfoManager->getLimitLevelByDomainGrade(SkillGrade(Grade + 1));

	if (CurrentLevel < LimitLevel) {
		// 경험치를 계산한다.
		Exp_t MaxExp = pSkillInfo->getSubSkill();
		Exp_t OldExp = pSkillSlot->getExp();
		Exp_t NewExp;

		Exp_t plusExp = 1;

		if(g_pVariableManager->getEventActivate() == 1) {
			plusExp = plusExp * g_pVariableManager->getExpRatio() / 100;
			plusExp = getPercentValue(plusExp, g_pVariableManager->getPremiumExpBonusPercent());
			if (pSlayer->isFlag(Effect::EFFECT_CLASS_BONUS_EXP))
                plusExp *= 2;
		}

		// 경험치 두배
		if (isAffectExp2X())
            plusExp *= 2;

		NewExp = min(MaxExp, OldExp + plusExp);

		pSkillSlot->setExp(NewExp);

		SkillLevel_t NewLevel = (NewExp * 100 / MaxExp) + 1;
		NewLevel = min((int)NewLevel, 100);
	
		ulong longData = (((ulong)pSkillSlot->getSkillType()) << 16) | (ulong)(NewExp / 10);
		_ModifyInfo.addLongData(MODIFY_SKILL_EXP, longData);

		// 퀵파이어는 나중에 DB를 수정해야 할 것이다.
		if (CurrentLevel != NewLevel) {
			pSkillSlot->setExpLevel(NewLevel);
			pSkillSlot->save();

			longData = (((ulong)pSkillSlot->getSkillType()) << 16) | (ulong)NewLevel;
			_ModifyInfo.addLongData(MODIFY_SKILL_LEVEL, longData);

			pSlayer->getGQuestManager()->skillLevelUp(pSkillSlot);

            pSlayer->sendSlayerSkillInfo();
		} else {
			WORD SkillExpSaveCount = pSlayer->getSkillExpSaveCount();
			if (SkillExpSaveCount > SKILL_EXP_SAVE_PERIOD) {
				pSkillSlot->save();
				SkillExpSaveCount = 0;
			}
			else SkillExpSaveCount++;
			pSlayer->setSkillExpSaveCount(SkillExpSaveCount);
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// 슬레이어 계열 경험치를 계산한다.
//////////////////////////////////////////////////////////////////////////////
bool increaseDomainExp(Slayer* pSlayer, SkillDomainType_t Domain, Exp_t Point, ModifyInfo& _ModifyInfo, Level_t EnemyLevel, int TargetNum) {
	if (pSlayer == NULL || Point == 0 || TargetNum == 0 || pSlayer->isAdvanced() ||
		g_pPKZoneInfoManager->isPKZone(pSlayer->getZoneID()) ||
		pSlayer->getZone() != NULL && pSlayer->getZone()->isDynamicZone())
		return false;

	int PartyID = pSlayer->getPartyID();

	if (EnemyLevel != 0) {
		int levelDiff = (int)pSlayer->getLevel() - (int)EnemyLevel;

//		cout << "enemyLevel : " << (int)EnemyLevel << " , TargetNum : " << TargetNum << endl;

//		cout << "Point : " << Point << endl;

		if (levelDiff > 50) Point = getPercentValue(Point, 30);
		else if (levelDiff > 40) Point = getPercentValue(Point, 50);
		else if (levelDiff < -40) Point = getPercentValue(Point, 140);
		else if (levelDiff < -30) Point = getPercentValue(Point, 130);
		else if (levelDiff < -20) Point = getPercentValue(Point, 120);

//		cout << "after level Point : " << Point << endl;
	}

	if (TargetNum != -1)
		Point = Point * (TargetNum+1) / 3;

//	cout << "after target Point : " << Point << endl;

	// 이미 지정된 domain에 맞는 무기를 들고 있다고 가정하고..
	// 무기 type에 따라서 SkillPoint를 다르게 준다.
	// by sigi. 2002.10.30
	Item* pWeapon = pSlayer->getWearItem(Slayer::WEAR_RIGHTHAND);
	if (pWeapon!=NULL) {
		SkillLevel_t DomainLevel = pSlayer->getSkillDomainLevel(Domain);

		Point = computeSkillPointBonus(Domain, DomainLevel, pWeapon, Point);
	}



	// 유료화 존에서는 경험치를 더 받는다.
	GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(pSlayer->getPlayer());
	Assert(pGamePlayer!=NULL);

	if (pGamePlayer->isPremiumPlay() || pGamePlayer->isFamilyFreePass())//pZone->isPayPlay() || pZone->isPremiumZone())
		Point = getPercentValueEx(Point, g_pVariableManager->getPremiumExpBonusPercent());

	if (pGamePlayer->isPCRoomPlay())//pZone->isPayPlay() || pZone->isPremiumZone())
		Point = getPercentValueEx(Point, g_pVariableManager->getPCRoomExpBonusPercent());

	if (PartyID != 0) {
		LocalPartyManager* pLPM = pSlayer->getLocalPartyManager();
		Assert(pLPM != NULL);

		int nMemberSize = pLPM->getAdjacentMemberSize(PartyID, pSlayer);
		switch (nMemberSize) {
			case 2: Point = getPercentValue(Point, 110); break;
			case 3: Point = getPercentValue(Point, 120); break;
			case 4: Point = getPercentValue(Point, 130); break;
			case 5: Point = getPercentValue(Point, 140); break;
			case 6:
				// 풀파티 경험치 이벤트
				if (g_pVariableManager->getVariable(FULL_PARTY_EXP_EVENT) != 0)
					Point = getPercentValue(Point, 165);
				else
					Point = getPercentValue(Point, 150);
				break;
			default: break;
		}
	}

	// VariableManager에 의한 Point증가치를 계산한다.
	if(g_pVariableManager->getExpRatio()>100 && g_pVariableManager->getEventActivate() == 1)
		Point = getPercentValue(Point, g_pVariableManager->getExpRatio());

	if (pSlayer->isFlag(Effect::EFFECT_CLASS_BONUS_EXP))
		Point *= 2;

	// 경험치 두배
	if (isAffectExp2X())
		Point *= 2;

	Level_t     CurDomainLevel = pSlayer->getSkillDomainLevel(Domain);
	Level_t     NewDomainLevel = CurDomainLevel;
	SkillType_t LearnSkillType = g_pSkillInfoManager->getSkillTypeByLevel(Domain, CurDomainLevel);
	Exp_t       NewGoalExp     = 0;
	bool        availiable     = false;

	// 현재 레벨에서 배울 수 있는 기술이 있는지 본다.
	if (LearnSkillType != 0) {
		// 배울 수 있는 기술이 있고 이미 배운 상태라면 Domain 경험치를 올려준다.
		if (pSlayer->hasSkill(LearnSkillType)) 
			availiable = true;
	} 
	else 
		availiable = true;

	if (availiable) {
		bool isLevelUp = false;

		// 시간대에 따라 올라가는 경험치가 달라진다.
		Point = (Exp_t)getPercentValue(Point, DomainExpTimebandFactor[getZoneTimeband(pSlayer->getZone())]);

		// 보상용 코드
		//Point = max(2, (int)getPercentValue(Point, 150));

		// 도메인 목표 경험치
		// 도메인 누적 경험치
		Exp_t GoalExp    = pSlayer->getGoalExp(Domain);
//		Exp_t CurrentExp = pSlayer->getSkillDomainExp(Domain);

		// 새로운 목표 경험치
		NewGoalExp = max(0, (int)(GoalExp - Point));

		// 누적 경험치에는 목표경험치가 줄어든 만큼 올라가야 정상이다.
		// 새로운 누적 경험치
//		Exp_t DiffExp = max(0, (int)(GoalExp - NewGoalExp));

//		Exp_t NewExp = 0;

		// 레벨이 최고에 달한 사람이라도 경험치는 쌓인다.
//		if(DiffExp == 0 && CurDomainLevel >= SLAYER_MAX_DOMAIN_LEVEL) {
//			NewExp  = CurrentExp + Point;
//		} else {
//			NewExp  = CurrentExp + DiffExp;
//		}

		// 새로운 목표 경험치 셋팅
		// 새로운 누적 경험치 셋팅
		pSlayer->setGoalExp(Domain, NewGoalExp);
//		pSlayer->setSkillDomainExp(Domain, NewExp);

		// 목표 경험치가 0 이라면, 레벨업을 할 수 있는 상태인가를 검사한다.
		if (NewGoalExp == 0 && CurDomainLevel != SLAYER_MAX_DOMAIN_LEVEL) {
			// 도메인 레벨을 올려주고, 그에 따른 기술을 배울 수 있다면 기술을 배울 수 있다는 것을 알려준다.
			NewDomainLevel = CurDomainLevel + 1;

			// 도메인 인포 메니져를 만들어서 목표 경험치를 셋팅하고 레벨을 재 설정 한다.
			NewGoalExp = g_pSkillDomainInfoManager->getDomainInfo((SkillDomain)Domain, NewDomainLevel)->getGoalExp();

			pSlayer->setGoalExp (Domain, NewGoalExp);
			pSlayer->setSkillDomainLevel (Domain, NewDomainLevel);

			//cout << "레벨업해서 남은 경험치는 " << NewGoalExp << endl;

			SkillType_t NewLearnSkillType = g_pSkillInfoManager->getSkillTypeByLevel(Domain, NewDomainLevel);

			// 현재 레벨에서 배울 수 있는 기술이 있는지 본다.
			if (NewLearnSkillType != 0) {
				// 배울 수 있는 기술이 있고 이미 배우지 않은 상태라면 기술을 배울 수 있다는 패킷을 날린다.
				if (pSlayer->hasSkill(NewLearnSkillType) == NULL) {
					// GCLearnSkillReady의 m_SkillType에 level up된 도메인의 가장 최근
					// 기술을 대입한다. 즉, 클라이언트 그 다음 스킬을 배울수 있다...
					GCLearnSkillReady readyPacket;
					readyPacket.setSkillDomainType((SkillDomainType_t)Domain);
					// send packet
					pSlayer->getPlayer()->sendPacket(&readyPacket);
				}
			}

			isLevelUp = true;
			//cout << "레벨업 할 수 있습니다." << endl;
		}

/*		if (DiffExp != 0)
		{
			switch (Domain)
			{
				case SKILL_DOMAIN_BLADE:   _ModifyInfo.addLongData(MODIFY_BLADE_DOMAIN_EXP, NewGoalExp);   break;
				case SKILL_DOMAIN_SWORD:   _ModifyInfo.addLongData(MODIFY_SWORD_DOMAIN_EXP, NewGoalExp);   break;
				case SKILL_DOMAIN_GUN:     _ModifyInfo.addLongData(MODIFY_GUN_DOMAIN_EXP, NewGoalExp);     break;
				case SKILL_DOMAIN_HEAL:    _ModifyInfo.addLongData(MODIFY_HEAL_DOMAIN_EXP, NewGoalExp);    break;
				case SKILL_DOMAIN_ENCHANT: _ModifyInfo.addLongData(MODIFY_ENCHANT_DOMAIN_EXP, NewGoalExp); break;
				default: break;
			}
		}*/

		Level_t DomainLevelSum = pSlayer->getSkillDomainLevelSum();

		// 레벨업이 되었을 경우, 도메인 총합이 100을 넘는다면 현재 도메인을 제외한 
		// 도메인 중에서 가장 높은 도메인 레벨을 떨어뜨려야 한다.
		if (isLevelUp && DomainLevelSum > SLAYER_MAX_DOMAIN_LEVEL) {
			SDomain ds[SKILL_DOMAIN_VAMPIRE];

			for(int i = 0; i < SKILL_DOMAIN_VAMPIRE; i++) {
				ds[i].DomainType = i;
				ds[i].DomainLevel = pSlayer->getSkillDomainLevel((SkillDomain)i);
			}

			// 현제 도메인을 제외한 가장 큰 숫자를 찾는다.
			stable_sort(ds, ds+SKILL_DOMAIN_VAMPIRE, isBig());

			// 소팅을 하고 난 다음 제일 위에 있는 스트럭쳐가 가장 높은 레벨을 가지고 있는 도메인이다.
			int j = 0;
			while(ds[j].DomainType == Domain) {
				j++;
				if (j > SKILL_DOMAIN_VAMPIRE) {
					cerr << "Out of Skill Domain Range!!!" << endl;
					Assert(false);
				}
			}

			// 결국 ds[j]의 값은 현재 도메인과 같지 않은 가장 높은 레벨의 도메인이다.
			SkillDomainType_t DownDomainType  = ds[j].DomainType;
			Level_t           DownDomainLevel = ds[j].DomainLevel;

			//cout << (int)DownDomainType << "도메인의 도메인 레벨을 낮춥니다." << endl;

			// 현재 도메인에서 배울 수 있는 기술이 있다면 Disable 시킨다.
			SkillType_t eraseSkillType = g_pSkillInfoManager->getSkillTypeByLevel(DownDomainType, DownDomainLevel);
			SkillSlot* pESkillSlot = pSlayer->hasSkill(eraseSkillType);
			if (pESkillSlot != NULL) 
				pESkillSlot->setDisable();

			// 도메인의 레벨을 떨어트린다.
			DownDomainLevel--;

			// 다운 도메인의 레벨을 셋팅한다.
			pSlayer->setSkillDomainLevel(DownDomainType, DownDomainLevel);

			// 다운 도메인의 목표 경험치를 찾아온다.
			// 다운 도메인의 누적 경험치를 찾아온다.
			Exp_t DownDomainGoalExp = g_pSkillDomainInfoManager->getDomainInfo((SkillDomain)DownDomainType, DownDomainLevel)->getGoalExp();
//			Exp_t DownDomainSumExp  = g_pSkillDomainInfoManager->getDomainInfo((SkillDomain)DownDomainType, DownDomainLevel)->getAccumExp();

			// 다운 그레이드된 목표 경험치로 재 셋팅한다.
			// 다운 그레이드 되었으므로 그 레벨에 맞는 도메인 경험치를 셋팅한다.
			pSlayer->setGoalExp(DownDomainType, DownDomainGoalExp);
//			pSlayer->setSkillDomainExp(DownDomainType, DownDomainSumExp);
			//cout << "레벨 : " << (int)DownDomainLevel << endl;
			//cout << "남은경험치 : " << (int)DownDomainGoalExp << endl;

			StringStream DownSave;
			if (DownDomainType == SKILL_DOMAIN_BLADE) {
				DownSave << "BladeLevel = " << (int)DownDomainLevel << ",BladeGoalExp = " << (int)DownDomainGoalExp;
				_ModifyInfo.addShortData(MODIFY_BLADE_DOMAIN_LEVEL, DownDomainLevel);
				_ModifyInfo.addLongData(MODIFY_BLADE_DOMAIN_EXP, DownDomainGoalExp);
			} else if (DownDomainType == SKILL_DOMAIN_SWORD) {
				DownSave << "SwordLevel = " << (int)DownDomainLevel << ",SwordGoalExp = " << (int)DownDomainGoalExp;
				_ModifyInfo.addLongData(MODIFY_SWORD_DOMAIN_EXP, DownDomainGoalExp);
				_ModifyInfo.addShortData(MODIFY_SWORD_DOMAIN_LEVEL, DownDomainLevel);
			} else if (DownDomainType == SKILL_DOMAIN_GUN) {
				DownSave << "GunLevel = " << (int)DownDomainLevel << ",GunGoalExp = " << (int)DownDomainGoalExp;
				_ModifyInfo.addLongData(MODIFY_GUN_DOMAIN_EXP, DownDomainGoalExp);
				_ModifyInfo.addShortData(MODIFY_GUN_DOMAIN_LEVEL, DownDomainLevel);
			} else if (DownDomainType == SKILL_DOMAIN_ENCHANT) {
				DownSave << "EnchantLevel = " << (int)DownDomainLevel << ",EnchantGoalExp = " << (int)DownDomainGoalExp;
				_ModifyInfo.addShortData(MODIFY_ENCHANT_DOMAIN_LEVEL, DownDomainLevel);
				_ModifyInfo.addLongData(MODIFY_ENCHANT_DOMAIN_EXP, DownDomainGoalExp);
			} else if (DownDomainType == SKILL_DOMAIN_HEAL) {
				DownSave << "HealLevel = " << (int)DownDomainLevel << ",HealGoalExp = " << (int)DownDomainGoalExp;
				_ModifyInfo.addShortData(MODIFY_HEAL_DOMAIN_LEVEL, DownDomainLevel);
				_ModifyInfo.addLongData(MODIFY_HEAL_DOMAIN_EXP, DownDomainGoalExp);
			} else if (DownDomainType == SKILL_DOMAIN_ETC) {
				DownSave << "ETCLevel = " << (int)DownDomainLevel << ",ETCGoalExp = " << (int)DownDomainGoalExp;
				_ModifyInfo.addShortData(MODIFY_ETC_DOMAIN_LEVEL, DownDomainLevel);
			}

			// 떨어뜨린 도메인 레벨을 세이브한다.
			pSlayer->tinysave(DownSave.toString());
		}

		WORD DomainExpSaveCount = pSlayer->getDomainExpSaveCount();

		if (Domain == SKILL_DOMAIN_BLADE) {
			StringStream attrsave;
			if (isLevelUp) {
				attrsave << "BladeLevel = " << (int)NewDomainLevel << ",BladeGoalExp = " << (int)NewGoalExp;
				_ModifyInfo.addShortData(MODIFY_BLADE_DOMAIN_LEVEL, NewDomainLevel);
				pSlayer->tinysave(attrsave.toString());
			} else {
				attrsave << "BladeGoalExp = " << (int)NewGoalExp;

				if (DomainExpSaveCount > DOMAIN_EXP_SAVE_PERIOD) {
					pSlayer->tinysave(attrsave.toString());
					DomainExpSaveCount = 0;
				}
				else
					DomainExpSaveCount++;
			}

			_ModifyInfo.addLongData(MODIFY_BLADE_DOMAIN_EXP , NewGoalExp);
		} else if (Domain == SKILL_DOMAIN_SWORD) {
			StringStream attrsave;
			if (isLevelUp) {
				attrsave << "SwordLevel = " << (int)NewDomainLevel << ",SwordGoalExp = " << (int)NewGoalExp;
				_ModifyInfo.addShortData(MODIFY_SWORD_DOMAIN_LEVEL, NewDomainLevel);
				pSlayer->tinysave(attrsave.toString());
			} else {
				attrsave << "SwordGoalExp = " << (int)NewGoalExp;

				if (DomainExpSaveCount > DOMAIN_EXP_SAVE_PERIOD) {
					pSlayer->tinysave(attrsave.toString());
					DomainExpSaveCount = 0;
				}
				else
					DomainExpSaveCount++;
			}

			_ModifyInfo.addLongData(MODIFY_SWORD_DOMAIN_EXP , NewGoalExp);
		} else if (Domain == SKILL_DOMAIN_GUN) {
			StringStream attrsave;
			if (isLevelUp) {
				attrsave << "GunLevel = " << (int)NewDomainLevel << ",GunGoalExp = " << (int)NewGoalExp;
				_ModifyInfo.addShortData(MODIFY_GUN_DOMAIN_LEVEL, NewDomainLevel);
				pSlayer->tinysave(attrsave.toString());
			} else {
				attrsave << "GunGoalExp = " << (int)NewGoalExp;

				if (DomainExpSaveCount > DOMAIN_EXP_SAVE_PERIOD) {
					pSlayer->tinysave(attrsave.toString());
					DomainExpSaveCount = 0;
				}
				else
					DomainExpSaveCount++;
			}

			_ModifyInfo.addLongData(MODIFY_GUN_DOMAIN_EXP , NewGoalExp);
		} else if (Domain == SKILL_DOMAIN_ENCHANT) {
			StringStream attrsave;
			if (isLevelUp) {
				attrsave << "EnchantLevel = " << (int)NewDomainLevel << ",EnchantGoalExp = " << (int)NewGoalExp;
				_ModifyInfo.addShortData(MODIFY_ENCHANT_DOMAIN_LEVEL, NewDomainLevel);
				pSlayer->tinysave(attrsave.toString());
			} else {
				attrsave << "EnchantGoalExp = " << (int)NewGoalExp;

				if (DomainExpSaveCount > DOMAIN_EXP_SAVE_PERIOD) {
					pSlayer->tinysave(attrsave.toString());
					DomainExpSaveCount = 0;
				}
				else
					DomainExpSaveCount++;
			}

			_ModifyInfo.addLongData(MODIFY_ENCHANT_DOMAIN_EXP , NewGoalExp);
		} else if (Domain == SKILL_DOMAIN_HEAL) {
			StringStream attrsave;
			if (isLevelUp) {
				attrsave << "HealLevel = " << (int)NewDomainLevel << ",HealGoalExp = " << (int)NewGoalExp;
				_ModifyInfo.addShortData(MODIFY_HEAL_DOMAIN_LEVEL, NewDomainLevel);
				pSlayer->tinysave(attrsave.toString());
			} else {
				attrsave << "HealGoalExp = " << (int)NewGoalExp;

				if (DomainExpSaveCount > DOMAIN_EXP_SAVE_PERIOD) {
					pSlayer->tinysave(attrsave.toString());
					DomainExpSaveCount = 0;
				}
				else
					DomainExpSaveCount++;
			}

			_ModifyInfo.addLongData(MODIFY_HEAL_DOMAIN_EXP , NewGoalExp);
		} else if (Domain == SKILL_DOMAIN_ETC) {
			StringStream attrsave;
			if (isLevelUp) {
				attrsave << "ETCLevel = " << (int)NewDomainLevel << ",ETCGoalExp = " << (int)NewGoalExp;
				_ModifyInfo.addShortData(MODIFY_ETC_DOMAIN_LEVEL, NewDomainLevel);
				pSlayer->tinysave(attrsave.toString());
			} else {
				attrsave << "ETCGoalExp = " << (int)NewGoalExp;

				if (DomainExpSaveCount > DOMAIN_EXP_SAVE_PERIOD) {
					pSlayer->tinysave(attrsave.toString());
					DomainExpSaveCount = 0;
				}
				else
					DomainExpSaveCount++;
			}

			_ModifyInfo.addLongData(MODIFY_ETC_DOMAIN_EXP , NewGoalExp);
		}

		// GrandMaster인 경우는 Effect를 붙여준다.
	    // by sigi. 2002.11.8
		if (isLevelUp && DomainLevelSum >= GRADE_GRAND_MASTER_LIMIT_LEVEL) {
			// 하나가 100렙 넘고 아직 Effect가 안 붙어있다면..
			if (pSlayer->getHighestSkillDomainLevel()>=GRADE_GRAND_MASTER_LIMIT_LEVEL &&
				!pSlayer->isFlag(Effect::EFFECT_CLASS_GRAND_MASTER_SLAYER)) {
				EffectGrandMasterSlayer* pEffect = new EffectGrandMasterSlayer(pSlayer);
				pEffect->setDeadline(999999);

				pSlayer->getEffectManager()->addEffect(pEffect);

				// affect()안에서.. Flag걸어주고, 주위에 broadcast도 해준다.
				pEffect->affect();
			} else if (pSlayer->getHighestSkillDomainLevel() == 130 || pSlayer->getHighestSkillDomainLevel() == 150) {
				Effect* pEffect = pSlayer->findEffect(Effect::EFFECT_CLASS_GRAND_MASTER_SLAYER);
				if (pEffect != NULL) pEffect->affect();
			}
		}

		pSlayer->setDomainExpSaveCount(DomainExpSaveCount);

		// 뭔가 레벨업했다면 체력을 체워준다.
		if (isLevelUp) {
			SLAYER_RECORD prev;
			pSlayer->getSlayerRecord(prev);
			pSlayer->initAllStat();
			healCreatureForLevelUp(pSlayer, _ModifyInfo, &prev);

			// 레벨업 이펙트도 보여준다. by sigi. 2002.11.9
			sendEffectLevelUp(pSlayer);

			checkFreeLevelLimit(pSlayer);
			pSlayer->whenQuestLevelUpgrade();
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////
// 뱀파이어 경험치를 계산한다.
//////////////////////////////////////////////////////////////////////////////
void increaseVampExp(Vampire* pVampire, Exp_t Point, ModifyInfo& _ModifyInfo) {
	Assert(pVampire != NULL);
	if (Point <= 0 ||
		pVampire->isAdvanced() ||
		pVampire->isFlag(Effect::EFFECT_CLASS_TRANSFORM_TO_BAT) ||
		(pVampire->getZone() != NULL && pVampire->getZone()->isDynamicZone()))
		return;

	Level_t curLevel = pVampire->getLevel();

	// VariableManager에 의한 증가
	if(g_pVariableManager->getExpRatio() > 100 && g_pVariableManager->getEventActivate() == 1)
		Point = getPercentValue(Point, g_pVariableManager->getExpRatio());

	if (pVampire->isFlag(Effect::EFFECT_CLASS_BONUS_EXP))
		Point *= 2;

	// 경험치 두배
	if (isAffectExp2X())
		Point *= 2;

/*	if (curLevel >= VAMPIRE_MAX_LEVEL) 
	{
		// 레벨 한계에 도달해도 경험치는 쌓게 해준다.
		// by sigi. 2002.8.31
		Exp_t NewExp = pVampire->getExp() + Point;

		WORD ExpSaveCount = pVampire->getExpSaveCount();
		if (ExpSaveCount > VAMPIRE_EXP_SAVE_PERIOD)
		{
			char pField[80];
			sprintf(pField, "Exp=%lu", NewExp);
			pVampire->tinysave(pField);

			ExpSaveCount = 0;
		}
		else ExpSaveCount++;
		pVampire->setExpSaveCount(ExpSaveCount);

		pVampire->setExp(NewExp);

		return;
	}*/

//	Exp_t OldExp = pVampire->getExp();

	Exp_t OldGoalExp = pVampire->getGoalExp();
	Exp_t NewGoalExp = max(0, (int)(OldGoalExp - Point));

	// 누적 경험치에는 목표 경험치가 줄어든 만큼 플러스 하여야 한다.
//	Exp_t DiffGoalExp = max(0, (int)(OldGoalExp - NewGoalExp));
//	Exp_t NewExp      = OldExp + DiffGoalExp;

//	pVampire->setExp(NewExp);
	pVampire->setGoalExp(NewGoalExp);

//	_ModifyInfo.addLongData(MODIFY_VAMP_GOAL_EXP, NewGoalExp);

	// 목표 경험치가 0이 아니거나, 현재 레벨이 115 이상이라면 경험치만 저장하고,
	// 레벨은 올라가지 않는다.
/*	if (NewGoalExp > 0 || curLevel >= 115)
	{
		WORD ExpSaveCount = pVampire->getExpSaveCount();

		// 경험치 세이브 카운트가 일정 수치에 다다르면 세이브하고,
		// 카운트를 초기화시켜 준다. 
		if (ExpSaveCount > VAMPIRE_EXP_SAVE_PERIOD)
		{
			StringStream attrsave;
			attrsave << "Exp = " << NewExp << ", GoalExp = " << NewGoalExp;
			pVampire->tinysave(attrsave.toString());

			ExpSaveCount = 0;
		}
		else ExpSaveCount++;

		pVampire->setExpSaveCount(ExpSaveCount);
	}
	// 목표 경험치가 0 이라면 레벨 업이다.
	else*/
	if (NewGoalExp > 0 || curLevel == VAMPIRE_MAX_LEVEL) {
		_ModifyInfo.addLongData(MODIFY_VAMP_GOAL_EXP, NewGoalExp);
		WORD ExpSaveCount = pVampire->getExpSaveCount();

		// 경험치 세이브 카운트가 일정 수치에 다다르면 세이브하고,
		// 카운트를 초기화시켜 준다. 
		if (ExpSaveCount > VAMPIRE_EXP_SAVE_PERIOD) {
			StringStream attrsave;
//			attrsave << "Exp = " << NewExp << ", GoalExp = " << NewGoalExp;
			attrsave << "GoalExp = " << NewGoalExp;
			pVampire->tinysave(attrsave.toString());

			ExpSaveCount = 0;
		}
		else
			ExpSaveCount++;

		pVampire->setExpSaveCount(ExpSaveCount);
	} else {
		// 레벨 업!!
		VAMPIRE_RECORD prev;
		pVampire->getVampireRecord(prev);

		curLevel++;

		pVampire->setLevel(curLevel);
		_ModifyInfo.addShortData(MODIFY_LEVEL, curLevel);

		// add bonus point
		Bonus_t bonus = pVampire->getBonus();

//		if ((pVampire->getSTR(ATTR_BASIC) + pVampire->getDEX(ATTR_BASIC) + pVampire->getINT(ATTR_BASIC) + pVampire->getBonus() - 60) < ((pVampire->getLevel() - 1) * 3)) 
		{
			// 레벨에 상관치 않고, 무조건 3으로 변경되었다.
			// 2001.12.12 김성민
			bonus += 3;
		}

		pVampire->setBonus(bonus);
		_ModifyInfo.addShortData(MODIFY_BONUS_POINT, bonus);

//		VampEXPInfo* pBeforeExpInfo = g_pVampEXPInfoManager->getVampEXPInfo(curLevel-1);
		VampEXPInfo* pNextExpInfo   = g_pVampEXPInfoManager->getVampEXPInfo(curLevel);
		Exp_t        NextGoalExp    = pNextExpInfo->getGoalExp();

		pVampire->setGoalExp(NextGoalExp);
		_ModifyInfo.addLongData(MODIFY_VAMP_GOAL_EXP, NextGoalExp);
		//cout << "남은 경험치는 " << NextGoalExp << " 입니다." << endl;

		StringStream sav;
		sav << "Level = " << (int)curLevel 
//			<< ",Exp = " << (int)pBeforeExpInfo->getAccumExp() 
			<< ",GoalExp = " << (int)NextGoalExp << ",Bonus = " << (int)bonus;
		pVampire->tinysave(sav.toString());

		// 레벨이 올라서 새로 배울 수 있는 기술이 생겼다면 기술을 배울 수 있다고 알린다.
		SkillType_t NewLearnSkillType = g_pSkillInfoManager->getSkillTypeByLevel(SKILL_DOMAIN_VAMPIRE, curLevel);
		if (NewLearnSkillType != 0) {
			// 배울 수 있는 기술이 있고 이미 배우지 않은 상태라면 기술을 배울 수 있다는 패킷을 날린다.
			if (pVampire->hasSkill(NewLearnSkillType) == NULL) {
				// GCLearnSkillReady의 m_SkillType에 level up된 도메인의 가장 최근
				// 기술을 대입한다. 즉, 클라이언트 그 다음 스킬을 배울수 있다...
				GCLearnSkillReady readyPacket;
				readyPacket.setSkillDomainType(SKILL_DOMAIN_VAMPIRE);
				pVampire->getPlayer()->sendPacket(&readyPacket);
			}
		}

		healCreatureForLevelUp(pVampire, _ModifyInfo, &prev);

		// 레벨업 이펙트도 보여준다. by sigi. 2002.11.9
		sendEffectLevelUp(pVampire);

		// by sigi. 2002.11.19
		// 유료 사용자가 아니거나
		// 무료 사용기간이 남아있지 않으면(혹은 능력치 over) 짜른다.
		checkFreeLevelLimit(pVampire);
		pVampire->whenQuestLevelUpgrade();

		// GrandMaster인 경우는 Effect를 붙여준다.
		// 100렙 넘고 아직 Effect가 안 붙어있다면..
	    // by sigi. 2002.11.9
		if (curLevel >= GRADE_GRAND_MASTER_LIMIT_LEVEL && !pVampire->isFlag(Effect::EFFECT_CLASS_GRAND_MASTER_VAMPIRE)) {
			EffectGrandMasterVampire* pEffect = new EffectGrandMasterVampire(pVampire);
			pEffect->setDeadline(999999);

			pVampire->getEffectManager()->addEffect(pEffect);

			// affect()안에서.. Flag걸어주고, 주위에 broadcast도 해준다.
			pEffect->affect();
		} else if (curLevel == 130 || curLevel == 150) {
			Effect* pEffect = pVampire->findEffect(Effect::EFFECT_CLASS_GRAND_MASTER_VAMPIRE);
			if (pEffect != NULL) pEffect->affect();
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// 아우스터스 경험치를 계산한다.
//////////////////////////////////////////////////////////////////////////////
void increaseOustersExp(Ousters* pOusters, Exp_t Point, ModifyInfo& _ModifyInfo) {
	Assert(pOusters != NULL);
	if (Point <= 0 ||
		pOusters->isAdvanced() ||
		pOusters->getZone() != NULL && pOusters->getZone()->isDynamicZone())
		return;

	Level_t curLevel = pOusters->getLevel();

	// VariableManager에 의한 증가
	if(g_pVariableManager->getExpRatio() > 100 && g_pVariableManager->getEventActivate() == 1)
		Point = getPercentValue(Point, g_pVariableManager->getExpRatio());

	if (pOusters->isFlag(Effect::EFFECT_CLASS_BONUS_EXP))
		Point *= 2;

	// 경험치 두배
	if (isAffectExp2X())
		Point *= 2;

	// 시간대에 따라 올라가는 경험치가 달라진다.
	Point = (Exp_t)getPercentValue(Point, DomainExpTimebandFactor[getZoneTimeband(pOusters->getZone())]);

	//cout << pOusters->getName() << " 에게 " << Point << "만큼 경험치를 줍니다." << endl;

/*	if (curLevel >= OUSTERS_MAX_LEVEL) 
	{
		// 레벨 한계에 도달해도 경험치는 쌓게 해준다.
		// by sigi. 2002.8.31
		Exp_t NewExp = pOusters->getExp() + Point;

		WORD ExpSaveCount = pOusters->getExpSaveCount();
		if (ExpSaveCount > OUSTERS_EXP_SAVE_PERIOD)
		{
			char pField[80];
			sprintf(pField, "Exp=%lu", NewExp);
			pOusters->tinysave(pField);

			ExpSaveCount = 0;
		}
		else ExpSaveCount++;
		pOusters->setExpSaveCount(ExpSaveCount);

		pOusters->setExp(NewExp);

		return;
	}

	Exp_t OldExp = pOusters->getExp();
*/
	Exp_t OldGoalExp = pOusters->getGoalExp();
	Exp_t NewGoalExp = max(0, (int)(OldGoalExp - Point));

	// 누적 경험치에는 목표 경험치가 줄어든 만큼 플러스 하여야 한다.
//	Exp_t DiffGoalExp = max(0, (int)(OldGoalExp - NewGoalExp));
//	Exp_t NewExp      = OldExp + DiffGoalExp;

//	pOusters->setExp(NewExp);
	pOusters->setGoalExp(NewGoalExp);

//	_ModifyInfo.addLongData(MODIFY_OUSTERS_EXP, NewExp);

//	if (NewGoalExp > 0)
	if (NewGoalExp > 0 || curLevel == OUSTERS_MAX_LEVEL) {
		WORD ExpSaveCount = pOusters->getExpSaveCount();
		_ModifyInfo.addLongData(MODIFY_OUSTERS_GOAL_EXP, NewGoalExp);

		// 경험치 세이브 카운트가 일정 수치에 다다르면 세이브하고,
		// 카운트를 초기화시켜 준다. 
		if (ExpSaveCount > OUSTERS_EXP_SAVE_PERIOD) {
			StringStream attrsave;
			attrsave << "GoalExp = " << NewGoalExp;
			pOusters->tinysave(attrsave.toString());

			ExpSaveCount = 0;
		}
		else
			ExpSaveCount++;

		pOusters->setExpSaveCount(ExpSaveCount);
	} else {
		// 레벨 업!!
		OUSTERS_RECORD prev;
		pOusters->getOustersRecord(prev);

		curLevel++;
		pOusters->setLevel(curLevel);

//		OustersEXPInfo* pBeforeExpInfo = g_pOustersEXPInfoManager->getOustersEXPInfo(curLevel-1);
		OustersEXPInfo* pNextExpInfo   = g_pOustersEXPInfoManager->getOustersEXPInfo(curLevel);
		Exp_t        	NextGoalExp    = pNextExpInfo->getGoalExp();

		// add bonus point
		Bonus_t bonus = pOusters->getBonus();
		SkillBonus_t skillBonus = pOusters->getSkillBonus();

		bonus += 3;
		skillBonus += pNextExpInfo->getSkillPointBonus();
	
		pOusters->setBonus(bonus);
		pOusters->setSkillBonus(skillBonus);

		_ModifyInfo.addShortData(MODIFY_LEVEL, curLevel);
		_ModifyInfo.addShortData(MODIFY_BONUS_POINT, bonus);
		_ModifyInfo.addShortData(MODIFY_SKILL_BONUS_POINT, skillBonus);

		pOusters->setGoalExp(NextGoalExp);
		_ModifyInfo.addLongData(MODIFY_OUSTERS_GOAL_EXP, NextGoalExp);

		StringStream sav;
		sav << "Level = " << (int)curLevel 
//			<< ",Exp = " << (int)pBeforeExpInfo->getAccumExp() 
			<< ",GoalExp = " << (int)NextGoalExp << ",Bonus = " << (int)bonus << ",SkillBonus = " << (int)skillBonus;
		pOusters->tinysave(sav.toString());

		// 레벨이 올라서 새로 배울 수 있는 기술이 생겼다면 기술을 배울 수 있다고 알린다.
		SkillType_t NewLearnSkillType = g_pSkillInfoManager->getSkillTypeByLevel(SKILL_DOMAIN_OUSTERS, curLevel);
		if (NewLearnSkillType != 0) {
			// 배울 수 있는 기술이 있고 이미 배우지 않은 상태라면 기술을 배울 수 있다는 패킷을 날린다.
			if (pOusters->hasSkill(NewLearnSkillType) == NULL) {
				// GCLearnSkillReady의 m_SkillType에 level up된 도메인의 가장 최근
				// 기술을 대입한다. 즉, 클라이언트 그 다음 스킬을 배울수 있다...
				GCLearnSkillReady readyPacket;
				readyPacket.setSkillDomainType(SKILL_DOMAIN_OUSTERS);
				pOusters->getPlayer()->sendPacket(&readyPacket);
			}
		}

		healCreatureForLevelUp(pOusters, _ModifyInfo, &prev);

		// 레벨업 이펙트도 보여준다. by sigi. 2002.11.9
		sendEffectLevelUp(pOusters);

		// by sigi. 2002.11.19
		// 유료 사용자가 아니거나
		// 무료 사용기간이 남아있지 않으면(혹은 능력치 over) 짜른다.
		checkFreeLevelLimit(pOusters);
		pOusters->whenQuestLevelUpgrade();

		// GrandMaster인 경우는 Effect를 붙여준다.
		// 100렙 넘고 아직 Effect가 안 붙어있다면..
	    // by sigi. 2002.11.9
		if (curLevel >= GRADE_GRAND_MASTER_LIMIT_LEVEL && !pOusters->isFlag(Effect::EFFECT_CLASS_GRAND_MASTER_OUSTERS)) {
			EffectGrandMasterOusters* pEffect = new EffectGrandMasterOusters(pOusters);
			pEffect->setDeadline(999999);

			pOusters->getEffectManager()->addEffect(pEffect);

			// affect()안에서.. Flag걸어주고, 주위에 broadcast도 해준다.
			pEffect->affect();
		} else if (curLevel == 130 || curLevel == 150) {
			Effect* pEffect = pOusters->findEffect(Effect::EFFECT_CLASS_GRAND_MASTER_OUSTERS);
			if (pEffect != NULL)
				pEffect->affect();
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
// 슬레이어 및 뱀파이어 명성을 계산한다.
//////////////////////////////////////////////////////////////////////////////
void increaseFame(Creature* pCreature, uint amount)
{
	if (pCreature == NULL ||
		g_pPKZoneInfoManager->isPKZone(pCreature->getZoneID()) ||
		(pCreature->getZone() != NULL && pCreature->getZone()->isDynamicZone()))
		return;

	// 로컬 파티가 존재한다면, 파티원의 숫자에 따라서 올라가는 수치가 변한다.
	int PartyID = pCreature->getPartyID();
	if (PartyID != 0) {
		LocalPartyManager* pLPM = pCreature->getLocalPartyManager();
		Assert(pLPM != NULL);

		int nMemberSize = pLPM->getAdjacentMemberSize(PartyID, pCreature);
		switch (nMemberSize) {
			case 2: amount = getPercentValue(amount, 120); break;
			case 3: amount = getPercentValue(amount, 140); break;
			case 4: amount = getPercentValue(amount, 160); break;
			case 5: amount = getPercentValue(amount, 180); break;
			case 6: amount = getPercentValue(amount, 200); break;
			default: break;
		}
	}

	StringStream attrsave;

    if (pCreature->isSlayer()) {
        Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);
        Fame_t  OldFame = pSlayer->getFame();
		Fame_t  NewFame = OldFame + amount;

		NewFame = min(2000000000, (int)NewFame);

		if (NewFame != OldFame) {
			WORD FameSaveCount = pSlayer->getFameSaveCount();
			if (FameSaveCount > FAME_SAVE_PERIOD) {
				attrsave << "Fame = " << (int)NewFame;
				pSlayer->tinysave(attrsave.toString());

				FameSaveCount = 0;
			}
			else
				FameSaveCount++;

			pSlayer->setFameSaveCount(FameSaveCount);
			pSlayer->setFame(NewFame);
		}
    } else if (pCreature->isVampire()) {
        Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);
        Fame_t   OldFame  = pVampire->getFame();
		Fame_t   NewFame  = OldFame + amount;

		NewFame = min(2000000000, (int)NewFame);

		if (NewFame != OldFame) {
			WORD FameSaveCount = pVampire->getFameSaveCount();
			if (FameSaveCount > FAME_SAVE_PERIOD) {
				attrsave << "Fame = " << (int)NewFame;
				pVampire->tinysave(attrsave.toString());

				FameSaveCount = 0;
			}
			else
				FameSaveCount++;

			pVampire->setFameSaveCount(FameSaveCount);
			pVampire->setFame(NewFame);
		}
    } else if (pCreature->isOusters()) {
        Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);
        Fame_t   OldFame  = pOusters->getFame();
		Fame_t   NewFame  = OldFame + amount;

		NewFame = min(2000000000, (int)NewFame);

		if (NewFame != OldFame) {
			WORD FameSaveCount = pOusters->getFameSaveCount();
			if (FameSaveCount > FAME_SAVE_PERIOD) {
				attrsave << "Fame = " << (int)NewFame;
				pOusters->tinysave(attrsave.toString());

				FameSaveCount = 0;
			}
			else
				FameSaveCount++;

			pOusters->setFameSaveCount(FameSaveCount);
			pOusters->setFame(NewFame);
		}
    }
}

//////////////////////////////////////////////////////////////////////////////
// 거리에 따른 SG, SR의 보너스를 계산한다.
//////////////////////////////////////////////////////////////////////////////
int computeArmsWeaponSplashSize(Item* pWeapon, int ox, int oy, int tx, int ty) {
	Assert(pWeapon != NULL);
	Item::ItemClass IClass = pWeapon->getItemClass();
	int Splash = 0;

	// SG일 경우에만 스플래시 효과가 존재한다.
	if (IClass == Item::ITEM_CLASS_SG) {
		switch (getDistance(ox, oy, tx, ty)) {
			case 1: Splash = 6; break;
			case 2: Splash = 5; break;
			case 3: Splash = 4; break;
			case 4: Splash = 3; break;
			case 5: Splash = 2; break;
			default: break;
		}
	}

	return Splash;
}

int computeArmsWeaponDamageBonus(Item* pWeapon, int ox, int oy, int tx, int ty) {
	Assert(pWeapon != NULL);

	Item::ItemClass IClass = pWeapon->getItemClass();
	int DamageBonus = 0;

	int range   = getDistance(ox, oy, tx, ty);

	if (IClass == Item::ITEM_CLASS_SR) {
		//DamageBonus = max(0, range - 3);
		// by sigi. 2002.12.3
		DamageBonus = range + 3;
	}
	// by sigi. 2002.12.3
	else if (IClass == Item::ITEM_CLASS_AR || IClass == Item::ITEM_CLASS_SMG) {
		switch (range) {
			case 6:  DamageBonus = 5; break;
			case 5:  DamageBonus = 3; break;
			case 4:  DamageBonus = 1; break;
			case 3:  DamageBonus = 1; break;
			case 2:  DamageBonus = 3; break;
			case 1:  DamageBonus = 5; break;

			default: break;
		}
	}
	else if (IClass == Item::ITEM_CLASS_SG)
		// by sigi. 2002.12.3
		DamageBonus = max(0, 6 - range);

	return DamageBonus;
}

int computeArmsWeaponToHitBonus(Item* pWeapon, int ox, int oy, int tx, int ty) {
	Assert(pWeapon != NULL);

	Item::ItemClass IClass = pWeapon->getItemClass();
	int ToHitBonus = 0;

	if (IClass == Item::ITEM_CLASS_SR) {
		int range  = getDistance(ox, oy, tx, ty);
		ToHitBonus = max(0, range - 1);
	} else if (IClass == Item::ITEM_CLASS_SG) {
		switch (getDistance(ox, oy, tx, ty)) {
			case 1:  ToHitBonus = 20; break;
			case 2:  ToHitBonus = 15; break;
			case 3:  ToHitBonus = 10; break;
			case 4:  ToHitBonus =  0; break;
			case 5:  ToHitBonus =  0; break;
			default: break;
		}
	}

	return ToHitBonus;
}

//////////////////////////////////////////////////////////////////////////////
// 지정된 좌표 주위의 스플래시 데미지를 맞을 크리쳐를 뽑아온다.
//////////////////////////////////////////////////////////////////////////////
int getSplashVictims(Zone* pZone, int cx, int cy, Creature::CreatureClass CClass, list<Creature*>& creatureList, int splash) {
	VSRect rect(0, 0, pZone->getWidth()-1, pZone->getHeight()-1);

	// 해당 크리쳐가 슬레이어라면, 그 슬레이어만 맞고,
	// 주위의 다른 슬레이어들은 맞지 않는다.
	if (CClass == Creature::CREATURE_CLASS_SLAYER) {
		if (rect.ptInRect(cx, cy)) {
			Tile& rTile = pZone->getTile(cx, cy);

			if (rTile.hasCreature(Creature::MOVE_MODE_WALKING)) {
				Creature* pCreature = rTile.getCreature(Creature::MOVE_MODE_WALKING);
				if (pCreature->getCreatureClass() == CClass)
					creatureList.push_back(pCreature);
			}
			// 현재로서는 날아다니는 슬레이어는 없지만...
			if (rTile.hasCreature(Creature::MOVE_MODE_FLYING)) {
				Creature* pCreature = rTile.getCreature(Creature::MOVE_MODE_FLYING);
				if (pCreature->getCreatureClass() == CClass)
					creatureList.push_back(pCreature);
			}
		}

		return (int)creatureList.size();
	}

	vector<Creature*> creatureVector;
	vector<int> pickedVector;

	for (int i=0; i<9; i++) {
		int tilex = cx + dirMoveMask[i].x;
		int tiley = cy + dirMoveMask[i].y;

		if (rect.ptInRect(tilex, tiley)) {
			Tile& rTile = pZone->getTile(tilex, tiley);

			if (rTile.hasCreature(Creature::MOVE_MODE_WALKING)) {
				Creature* pCreature = rTile.getCreature(Creature::MOVE_MODE_WALKING);

				if (CClass == Creature::CREATURE_CLASS_MAX)
					// CREATURE_CLASS_MAX가 파라미터로 넘어오는 경우에는 무조건 더하자.
					creatureVector.push_back(pCreature);
				else if (pCreature->getCreatureClass() == CClass)
					// 아닌 경우에는 CreatureClass가 같은 경우에만 더한다.
					creatureVector.push_back(pCreature);
			}

			if (rTile.hasCreature(Creature::MOVE_MODE_FLYING)) {
				Creature* pCreature = rTile.getCreature(Creature::MOVE_MODE_FLYING);
				if (CClass == Creature::CREATURE_CLASS_MAX)
					// CREATURE_CLASS_MAX가 파라미터로 넘어오는 경우에는 무조건 더하자.
					creatureVector.push_back(pCreature);
				else if (pCreature->getCreatureClass() == CClass)
					// 아닌 경우에는 CreatureClass가 같은 경우에만 더한다.
					creatureVector.push_back(pCreature);
			}
		}
	}

	// 스플래시 데미지를 입힐 놈들의 숫자보다 현재 있는 크리쳐가 적다면,
	// 모두 스플래시 데미지를 입히면 된다.
	if ((int)creatureVector.size() <= splash) {
		for (int i=0; i<(int)creatureVector.size(); i++)
			creatureList.push_back(creatureVector[i]);
	}
	// 스플래시 데미지를 입힐 놈보다 현재 존재하는 크리쳐들이 많다면,
	// 이 중에 splash 숫자만큼의 크리쳐를 임의로 뽑아야 한다.
	else {
		// 제일 처음에 6놈이 있고, 이 중에 4놈을 뽑아야 한다고
		// 가정하면, size = 6이 된다.
		// Indexes 배열에는 (0, 1, 2, 3, 4, 5, -1...)이 들어간다.
		// 이 중에 2를 뽑았다고 가정하자.
		// 그러면 이 배열에서 2를 제거해 줘야 한다.
		// 뒤에서부터 앞으로 한칸씩 옮겨줘야 한다.
		// (0, 1, 3, 4, 5, 5...)
		// 그 다음 사이즈를 줄이고, 다시 그 중에서 하나를 랜덤으로
		// 뽑아가면 겹치지 않는 크리쳐의 리스트를 얻을 수 있다.
		int Indexes[50] = { -1, }, i;
		int size = creatureVector.size();
		for (i=0; i<size; i++)
			Indexes[i] = i;

		for (i=0; i<splash; i++) {
			int index = rand()%size;
			int realIndex = Indexes[index];
			creatureList.push_back(creatureVector[realIndex]);

			for (int m=index+1; m<size; m++)
				Indexes[m-1] = Indexes[m];

			size--;
		}
	}

	return (int)creatureList.size();
}


//////////////////////////////////////////////////////////////////////////////
// 지정된 좌표 주위의 크리처를 찾아서 넘겨준다.
//////////////////////////////////////////////////////////////////////////////
int getSplashVictims(Zone* pZone, int cx, int cy, Creature::CreatureClass CClass, list<Creature*>& creatureList, int splash, int range) {
	VSRect rect(0, 0, pZone->getWidth()-1, pZone->getHeight()-1);

	vector<Creature*> creatureVector;
	vector<int> pickedVector;

	for (int y=-range; y<=range; y++) {
		for (int x=-range; x<=range; x++) {
			int tilex = cx + x;
			int tiley = cy + y;;

			if (rect.ptInRect(tilex, tiley)) {
				Tile& rTile = pZone->getTile(tilex, tiley);

				if (rTile.hasCreature(Creature::MOVE_MODE_WALKING)) {
					Creature* pCreature = rTile.getCreature(Creature::MOVE_MODE_WALKING);

					if (CClass == Creature::CREATURE_CLASS_MAX)
						// CREATURE_CLASS_MAX가 파라미터로 넘어오는 경우에는 무조건 더하자.
						creatureVector.push_back(pCreature);
					else if (pCreature->getCreatureClass() == CClass)
						// 아닌 경우에는 CreatureClass가 같은 경우에만 더한다.
						creatureVector.push_back(pCreature);
				}

				if (rTile.hasCreature(Creature::MOVE_MODE_FLYING)) {
					Creature* pCreature = rTile.getCreature(Creature::MOVE_MODE_FLYING);
					if (CClass == Creature::CREATURE_CLASS_MAX)
						// CREATURE_CLASS_MAX가 파라미터로 넘어오는 경우에는 무조건 더하자.
						creatureVector.push_back(pCreature);
					else if (pCreature->getCreatureClass() == CClass)
						// 아닌 경우에는 CreatureClass가 같은 경우에만 더한다.
						creatureVector.push_back(pCreature);
				}
			}
		}
	}

	// 스플래시 데미지를 입힐 놈들의 숫자보다 현재 있는 크리쳐가 적다면,
	// 모두 스플래시 데미지를 입히면 된다.
	if ((int)creatureVector.size() <= splash) {
		for (int i=0; i<(int)creatureVector.size(); i++)
			creatureList.push_back(creatureVector[i]);
	}
	// 스플래시 데미지를 입힐 놈보다 현재 존재하는 크리쳐들이 많다면,
	// 이 중에 splash 숫자만큼의 크리쳐를 임의로 뽑아야 한다.
	else {
		// 제일 처음에 6놈이 있고, 이 중에 4놈을 뽑아야 한다고
		// 가정하면, size = 6이 된다.
		// Indexes 배열에는 (0, 1, 2, 3, 4, 5, -1...)이 들어간다.
		// 이 중에 2를 뽑았다고 가정하자.
		// 그러면 이 배열에서 2를 제거해 줘야 한다.
		// 뒤에서부터 앞으로 한칸씩 옮겨줘야 한다.
		// (0, 1, 3, 4, 5, 5...)
		// 그 다음 사이즈를 줄이고, 다시 그 중에서 하나를 랜덤으로
		// 뽑아가면 겹치지 않는 크리쳐의 리스트를 얻을 수 있다.
		int Indexes[50] = { -1, }, i;
		int size = creatureVector.size();
		for (i=0; i<size; i++)
			Indexes[i] = i;

		for (i=0; i<splash; i++) {
			int index = rand()%size;
			int realIndex = Indexes[index];
			creatureList.push_back(creatureVector[realIndex]);

			for (int m=index+1; m<size; m++)
				Indexes[m-1] = Indexes[m];

			size--;
		}
	}

	return (int)creatureList.size();
}

//////////////////////////////////////////////////////////////////////////////
// 능력치가 하나라도 상승했을 때, HP와 MP를 만땅으로 채워주는 함수다. 
// 슬레이어용 -- 2002.01.14 김성민
//////////////////////////////////////////////////////////////////////////////
void healCreatureForLevelUp(Slayer* pSlayer, ModifyInfo& _ModifyInfo, SLAYER_RECORD* prev) {
	// 능력치를 재계산한다.
	pSlayer->initAllStat();

	// 능력치가 상승했으니 무언가 부가적인 능력치가 변했으므로 보내준다.
	pSlayer->sendRealWearingInfo();
	pSlayer->addModifyInfo(*prev, _ModifyInfo);

	if (pSlayer->isDead())
		return;

	// 능력치가 하나라도 상승했다면 HP와 MP를 만땅으로 채워준다.
	HP_t OldHP = pSlayer->getHP(ATTR_CURRENT);
	HP_t OldMP = pSlayer->getMP(ATTR_CURRENT);

	// 만땅 채우기...
	pSlayer->setHP(pSlayer->getHP(ATTR_MAX), ATTR_CURRENT);
	pSlayer->setMP(pSlayer->getMP(ATTR_MAX), ATTR_CURRENT);

	HP_t NewHP = pSlayer->getHP(ATTR_CURRENT);
	HP_t NewMP = pSlayer->getMP(ATTR_CURRENT);

	// HP가 바뀌었다면...
	if (OldHP != NewHP) {
		_ModifyInfo.addShortData(MODIFY_CURRENT_HP, NewHP);

		// 바뀐 체력을 주위에 브로드캐스팅해준다.
		GCStatusCurrentHP gcStatusCurrentHP;
		gcStatusCurrentHP.setObjectID(pSlayer->getObjectID());
		gcStatusCurrentHP.setCurrentHP(NewHP);
		Zone* pZone = pSlayer->getZone();
		Assert(pZone != NULL);
		pZone->broadcastPacket(pSlayer->getX(), pSlayer->getY(), &gcStatusCurrentHP, pSlayer);
	}

	// MP가 바뀌었다면...
	if (OldMP != NewMP)
		_ModifyInfo.addShortData(MODIFY_CURRENT_MP, NewMP);

//	pSlayer->sendModifyInfo(*prev);
}

//////////////////////////////////////////////////////////////////////////////
// 능력치가 상승했을 때, HP를 만땅으로 채워주는 함수다. 뱀파이어용
// -- 2002.01.14 김성민
//////////////////////////////////////////////////////////////////////////////
void healCreatureForLevelUp(Vampire* pVampire, ModifyInfo& _ModifyInfo, VAMPIRE_RECORD* prev) {
	// 능력치를 재계산한다.
	pVampire->initAllStat();

	// 능력치가 상승했으니 무언가 부가적인 능력치가 변했으므로 보내준다.
	pVampire->sendRealWearingInfo();
	pVampire->addModifyInfo(*prev, _ModifyInfo);

	if (pVampire->isDead())
		return;

	HP_t OldHP = pVampire->getHP(ATTR_CURRENT);

	// 만땅 채우기...
	pVampire->setHP(pVampire->getHP(ATTR_MAX), ATTR_CURRENT);

	HP_t NewHP = pVampire->getHP(ATTR_CURRENT);

	// HP가 바뀌었다면...
	if (OldHP != NewHP) {
		_ModifyInfo.addShortData(MODIFY_CURRENT_HP, NewHP);

		// 바뀐 체력을 주위에 브로드캐스팅해준다.
		GCStatusCurrentHP gcStatusCurrentHP;
		gcStatusCurrentHP.setObjectID(pVampire->getObjectID());
		gcStatusCurrentHP.setCurrentHP(NewHP);
		Zone* pZone = pVampire->getZone();
		Assert(pZone != NULL);
		pZone->broadcastPacket(pVampire->getX(), pVampire->getY(), &gcStatusCurrentHP, pVampire);
	}

//	pVampire->sendModifyInfo(*prev);
}

//////////////////////////////////////////////////////////////////////////////
// 기술 실패시 패킷을 날린다.
// 일반적인 실패 (히트롤 실패했다던가, 마나가 없다던가...)일 경우,
// 본인과 그것을 보는 이들에게 패킷을 날린다.
//////////////////////////////////////////////////////////////////////////////
void executeSkillFailNormal(Creature* pCreature, SkillType_t SkillType, Creature* pTargetCreature, BYTE Grade) {
	Assert(pCreature != NULL);

	if (pCreature->isPC()) {
		GCSkillFailed1 gcSkillFailed1;
		gcSkillFailed1.setSkillType(SkillType);
		gcSkillFailed1.setGrade(Grade);
		(pCreature->getPlayer())->sendPacket(&gcSkillFailed1);
	}

	GCSkillFailed2 gcSkillFailed2;
	gcSkillFailed2.setSkillType(SkillType);
	gcSkillFailed2.setObjectID(pCreature->getObjectID());
	gcSkillFailed2.setGrade(Grade);

	// ObjectSkill일 경우, 상대방의 OID가 존재한다면 패킷에다 실어서 보내준다.
	// 셀프 스킬이나 타일 스킬인 경우에는 NULL로 parameter가 넘어오는 것이 정상이다.
	// (클라이언트에서는 셀프나 타일 스킬이 실패해서 날아오는 GCSkillFailed2일 경우에는,
	// TargetObjectID를 읽지도 않는다.)
	if (pTargetCreature != NULL)
		gcSkillFailed2.setTargetObjectID(pTargetCreature->getObjectID());
	else
		gcSkillFailed2.setTargetObjectID(0);

	Zone* pZone = pCreature->getZone();
	Assert(pZone != NULL);

	pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcSkillFailed2, pCreature);
}

//////////////////////////////////////////////////////////////////////////////
// 기술 실패시 패킷을 날린다.
// 스킬의 결과를 2번 날려줘야 된다.
// 라바 만들기에 대한 것 하나 하고
// 흡영에 관한 것 하나.
// 그래서 처음에 조건 체크하다가 실패할 경우에
// SkillFail 패킷을 2번 보내준다.
//////////////////////////////////////////////////////////////////////////////
void executeAbsorbSoulSkillFail(Creature* pCreature, SkillType_t SkillType, ObjectID_t TargetObjectID, bool bBroadcast, bool bSendTwice) {
	Assert(pCreature != NULL);

	// 클라이언트에 락이 걸려있으면 스킬 사용한 본인에게는 검증 패킷을 2번 보내줘야 된다.
	if (pCreature->isPC()) {
		GCSkillFailed1 gcSkillFailed1;
		gcSkillFailed1.setSkillType(SkillType);
		(pCreature->getPlayer())->sendPacket(&gcSkillFailed1);
		if (bSendTwice)
			(pCreature->getPlayer())->sendPacket(&gcSkillFailed1);
	}

	if (bBroadcast) {
		GCSkillFailed2 gcSkillFailed2;
		gcSkillFailed2.setSkillType(SkillType);
		gcSkillFailed2.setObjectID(pCreature->getObjectID());
		gcSkillFailed2.setTargetObjectID(TargetObjectID);

		Zone* pZone = pCreature->getZone();
		Assert(pZone != NULL);

		pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcSkillFailed2, pCreature);
	}
}

//////////////////////////////////////////////////////////////////////////////
// 기술 실패시 패킷을 날린다.
// 일반적인 실패 (히트롤 실패했다던가, 마나가 없다던가...)일 경우,
// 본인과 그것을 보는 이들에게 패킷을 날린다.
//////////////////////////////////////////////////////////////////////////////
void executeSkillFailNormalWithGun(Creature* pCreature, SkillType_t SkillType, Creature* pTargetCreature, BYTE RemainBullet) {
	Assert(pCreature != NULL);

	if (pCreature->isPC()) {
		GCSkillFailed1 gcSkillFailed1;
		gcSkillFailed1.setSkillType(SkillType);
		gcSkillFailed1.addShortData(MODIFY_BULLET, RemainBullet);
		(pCreature->getPlayer())->sendPacket(&gcSkillFailed1);
	}

	GCSkillFailed2 gcSkillFailed2;
	gcSkillFailed2.setSkillType(SkillType);
	gcSkillFailed2.setObjectID(pCreature->getObjectID());

	// ObjectSkill일 경우, 상대방의 OID가 존재한다면 패킷에다 실어서 보내준다.
	// 셀프 스킬이나 타일 스킬인 경우에는 NULL로 parameter가 넘어오는 것이 정상이다.
	// (클라이언트에서는 셀프나 타일 스킬이 실패해서 날아오는 GCSkillFailed2일 경우에는,
	// TargetObjectID를 읽지도 않는다.)
	if (pTargetCreature != NULL)
		gcSkillFailed2.setTargetObjectID(pTargetCreature->getObjectID());
	else
		gcSkillFailed2.setTargetObjectID(0);

	Zone* pZone = pCreature->getZone();
	Assert(pZone != NULL);

	pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcSkillFailed2, pCreature);
}

//////////////////////////////////////////////////////////////////////////////
// 기술 실패시 패킷을 날린다.
// 예외적인 실패 (NPC를 공격했다던가...)
// 본인에게만 패킷을 날린다.
//////////////////////////////////////////////////////////////////////////////
void executeSkillFailException(Creature* pCreature, SkillType_t SkillType, BYTE Grade) {
	// by sigi. 2002.5.8
	//	Assert(pCreature != NULL);

	if (pCreature!=NULL && pCreature->isPC()) {
		GCSkillFailed1 gcSkillFailed1;
		gcSkillFailed1.setSkillType(SkillType);
		gcSkillFailed1.setGrade(Grade);

		Player* pPlayer = pCreature->getPlayer();
		pPlayer->sendPacket(&gcSkillFailed1);
	}
}

//////////////////////////////////////////////////////////////////////////////
// 능력치가 상승했을 때, HP, MP를 만땅으로 채워주는 함수다. 아우스터스용
// -- 2003.04.19 by bezz
//////////////////////////////////////////////////////////////////////////////
void healCreatureForLevelUp(Ousters* pOusters, ModifyInfo& _ModifyInfo, OUSTERS_RECORD* prev) {
	// 능력치를 재계산한다.
	pOusters->initAllStat();

	// 능력치가 상승했으니 무언가 부가적인 능력치가 변했으므로 보내준다.
	pOusters->sendRealWearingInfo();
	pOusters->addModifyInfo(*prev, _ModifyInfo);

	if (pOusters->isDead())
		return;

	HP_t OldHP = pOusters->getHP(ATTR_CURRENT);
	MP_t OldMP = pOusters->getMP(ATTR_CURRENT);

	// 만땅 채우기...
	pOusters->setHP(pOusters->getHP(ATTR_MAX), ATTR_CURRENT);
    if (OldMP < pOusters->getMP(ATTR_MAX))
        pOusters->setMP(pOusters->getMP(ATTR_MAX), ATTR_CURRENT);

	HP_t NewHP = pOusters->getHP(ATTR_CURRENT);
	MP_t NewMP = pOusters->getMP(ATTR_CURRENT);

	// HP가 바뀌었다면...
	if (OldHP != NewHP) {
		_ModifyInfo.addShortData(MODIFY_CURRENT_HP, NewHP);

		// 바뀐 체력을 주위에 브로드캐스팅해준다.
		GCStatusCurrentHP gcStatusCurrentHP;
		gcStatusCurrentHP.setObjectID(pOusters->getObjectID());
		gcStatusCurrentHP.setCurrentHP(NewHP);
		Zone* pZone = pOusters->getZone();
		Assert(pZone != NULL);
		pZone->broadcastPacket(pOusters->getX(), pOusters->getY(), &gcStatusCurrentHP, pOusters);
	}

	if (OldMP != NewMP)
		_ModifyInfo.addShortData(MODIFY_CURRENT_MP, NewMP);

//	pOusters->sendModifyInfo(*prev);
}


// HP를 줄이는 함수
// by sigi. 2002.9.10
void decreaseHP(Zone* pZone, Creature* pCreature, int Damage, ObjectID_t attackerObjectID) {
	if (!(pZone->getZoneLevel() & COMPLETE_SAFE_ZONE) && !pCreature->isFlag(Effect::EFFECT_CLASS_NO_DAMAGE)) {
		if (pCreature->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);

			HP_t CurrentHP = pSlayer->getHP(ATTR_CURRENT);

			if (CurrentHP > 0) {
				HP_t RemainHP  = max(0, CurrentHP -(int)Damage);

				pSlayer->setHP(RemainHP, ATTR_CURRENT);

				GCModifyInformation gcMI;
				gcMI.addShortData(MODIFY_CURRENT_HP, RemainHP);
				pSlayer->getPlayer()->sendPacket(&gcMI);

				// 변한 HP를 브로드캐스팅해준다.
				GCStatusCurrentHP pkt;
				pkt.setObjectID(pSlayer->getObjectID());
				pkt.setCurrentHP(RemainHP);
				pZone->broadcastPacket(pSlayer->getX(), pSlayer->getY(), &pkt);
			}
		} else if (pCreature->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);

			HP_t CurrentHP = pVampire->getHP(ATTR_CURRENT);

			if (CurrentHP > 0) {
				HP_t RemainHP  = max(0, CurrentHP -(int)Damage);

				pVampire->setHP(RemainHP, ATTR_CURRENT);

				GCModifyInformation gcMI;
				gcMI.addShortData(MODIFY_CURRENT_HP, RemainHP);
				pVampire->getPlayer()->sendPacket(&gcMI);

				// 변한 HP를 브로드캐스팅해준다.
				GCStatusCurrentHP pkt;
				pkt.setObjectID(pVampire->getObjectID());
				pkt.setCurrentHP(RemainHP);
				pZone->broadcastPacket(pVampire->getX(), pVampire->getY(), &pkt);
			}
		} else if (pCreature->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);

			HP_t CurrentHP = pOusters->getHP(ATTR_CURRENT);

			if (CurrentHP > 0) {
				HP_t RemainHP  = max(0, CurrentHP -(int)Damage);

				pOusters->setHP(RemainHP, ATTR_CURRENT);

				GCModifyInformation gcMI;
				gcMI.addShortData(MODIFY_CURRENT_HP, RemainHP);
				pOusters->getPlayer()->sendPacket(&gcMI);

				// 변한 HP를 브로드캐스팅해준다.
				GCStatusCurrentHP pkt;
				pkt.setObjectID(pOusters->getObjectID());
				pkt.setCurrentHP(RemainHP);
				pZone->broadcastPacket(pOusters->getX(), pOusters->getY(), &pkt);
			}
		} else if (pCreature->isMonster()) {
			Monster* pMonster = dynamic_cast<Monster*>(pCreature);

			HP_t CurrentHP = pMonster->getHP(ATTR_CURRENT);

			if (CurrentHP > 0) {
				HP_t RemainHP  = max(0, CurrentHP -(int)Damage);

				pMonster->setHP(RemainHP, ATTR_CURRENT);

				// 변한 HP를 브로드캐스팅해준다.
				GCStatusCurrentHP pkt;
				pkt.setObjectID(pMonster->getObjectID());
				pkt.setCurrentHP(RemainHP);
				pZone->broadcastPacket(pMonster->getX(), pMonster->getY(), &pkt);
			}
		}

		// attackerObjectID가 pCreature를 죽인 경우의 KillCount 처리
		// by sigi. 2002.9.9
		if (attackerObjectID!=0 && pCreature->isDead()) {
			Creature* pAttacker = pZone->getCreature(attackerObjectID);

			if (pAttacker!=NULL)
				affectKillCount(pAttacker, pCreature);
		}
	}
}

//----------------------------------------------------------------------
// Set Direction To Creature
//----------------------------------------------------------------------
// 다른 Creature를 향해서 바라본다.
//----------------------------------------------------------------------
// 8방향에 따른 기준이 되는 기울기 : 가로/세로 비율과 관련
//----------------------------------------------------------------------
const float	BASIS_DIRECTION_LOW = 0.35f;
const float BASIS_DIRECTION_HIGH = 3.0f;

Dir_t getDirectionToPosition(int originX, int originY, int destX, int destY) {
	int	stepX = destX - originX,
	stepY = destY - originY;

	// 0일 때 check
	float	k	= (stepX==0)? 0 : (float)(stepY) / stepX;	// 기울기

	//--------------------------------------------------
	// 방향을 정해야 한다.	
	//--------------------------------------------------
	if (stepY == 0) {
		// X축
		// - -;;
		if (stepX == 0)
			return DOWN;
		else if (stepX > 0)
			return RIGHT;
		else 
			return LEFT;
	}
	else
	if (stepY < 0) {	// UP쪽으로
		// y축 위
		if (stepX == 0)
			return UP;
		// 1사분면
		else if (stepX > 0) {
			if (k < -BASIS_DIRECTION_HIGH)
				return UP;
			else if (k <= -BASIS_DIRECTION_LOW)
				return RIGHTUP;
			else
				return RIGHT;
		}
		// 2사분면
		else {
			if (k > BASIS_DIRECTION_HIGH)
				return UP;
			else if (k >= BASIS_DIRECTION_LOW)
				return LEFTUP;
			else
				return LEFT;
		}
	}
	// 아래쪽
	else {		
		// y축 아래
		if (stepX == 0)
			return DOWN;
		// 4사분면
		else if (stepX > 0) {
			if (k > BASIS_DIRECTION_HIGH)
				return DOWN;
			else if (k >= BASIS_DIRECTION_LOW)
				return RIGHTDOWN;
			else
				return RIGHT;
		}
		// 3사분면
		else {
			if (k < -BASIS_DIRECTION_HIGH)
				return DOWN;
			else if (k <= -BASIS_DIRECTION_LOW)
				return LEFTDOWN;
			else
				return LEFT;
		}
	}
}


Exp_t computeSkillPointBonus(SkillDomainType_t Domain, SkillLevel_t DomainLevel, Item* pWeapon, Exp_t Point) {
	Assert(pWeapon!=NULL);

	ItemType_t itemType = pWeapon->getItemType();
	ItemType_t bestItemType = g_pSkillDomainInfoManager->getDomainInfo((SkillDomain)Domain, DomainLevel)->getBestItemType();

	Exp_t newPoint;

	if (pWeapon->isUnique())
		newPoint = getPercentValue(Point, 120);
	else if (itemType <= bestItemType)
		newPoint = Point * (itemType+1) / (bestItemType+1);
	else {
		Exp_t Point120 = getPercentValue(Point, 120);
		newPoint = Point * itemType / (bestItemType+1);
	
		newPoint = min(Point120, newPoint);
	}

	//cout << "skillPoint: " << (int)itemType << " / " << (int)bestItemType
	//	 << ", " << (int)Point << " --> " << (int)newPoint << endl;

	// by sigi. 2002.11.5
	newPoint = max(1, (int)newPoint);

	return newPoint;

}

//////////////////////////////////////////////////////////////////////////////
// 슬레이어 공격자의 순수 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computePureSlayerDamage(Slayer* pSlayer) {
	Assert(pSlayer != NULL);

	Item*    pItem       = pSlayer->getWearItem(Slayer::WEAR_RIGHTHAND);

	// 일단 맨손의 데미지를 받아온다.
	Damage_t MinDamage  = pSlayer->getDamage(ATTR_CURRENT);
	Damage_t MaxDamage  = pSlayer->getDamage(ATTR_MAX);

	// 무기를 들고 있다면, min, max에 무기의 min, max를 계산해 준다.
	if (pItem != NULL && pSlayer->isRealWearingEx(Slayer::WEAR_RIGHTHAND)) {
		MinDamage += pItem->getMinDamage();
		MaxDamage += pItem->getMaxDamage();
	}

	// 실제 랜덤 데미지를 계산한다.
	Damage_t RealDamage = max(1, Random(MinDamage, MaxDamage));

	return RealDamage;
}

//////////////////////////////////////////////////////////////////////////////
// 뱀파이어 공격자의 순수 데비지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computePureVampireDamage(Vampire* pVampire) {
	Assert(pVampire != NULL);

	Damage_t MinDamage   = pVampire->getDamage(ATTR_CURRENT);
	Damage_t MaxDamage   = pVampire->getDamage(ATTR_MAX);
	uint     timeband    = getZoneTimeband(pVampire->getZone());

	// vampire 무기에 의한 데미지
	Item*    pItem       = pVampire->getWearItem(Vampire::WEAR_RIGHTHAND);

	// 무기를 들고 있다면, min, max에 무기의 min, max를 계산해 준다.
	if (pItem != NULL && pVampire->isRealWearingEx(Vampire::WEAR_RIGHTHAND)) {
		MinDamage += pItem->getMinDamage();
		MaxDamage += pItem->getMaxDamage();
	}

	// 실제 랜덤 데미지를 계산한다.
	Damage_t RealDamage = max(1, Random(MinDamage, MaxDamage));

	RealDamage = (Damage_t)getPercentValue(RealDamage, VampireTimebandFactor[timeband]);
	
	return RealDamage;
}


//////////////////////////////////////////////////////////////////////////////
// 아우스터스 공격자의 순수 데비지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computePureOustersDamage(Ousters* pOusters) {
	Assert(pOusters != NULL);

	Damage_t MinDamage   = pOusters->getDamage(ATTR_CURRENT);
	Damage_t MaxDamage   = pOusters->getDamage(ATTR_MAX);

	// vampire 무기에 의한 데미지
	Item*    pItem       = pOusters->getWearItem(Ousters::WEAR_RIGHTHAND);

	// 무기를 들고 있다면, min, max에 무기의 min, max를 계산해 준다.
	if (pItem != NULL && pOusters->isRealWearingEx(Ousters::WEAR_RIGHTHAND)) {
		MinDamage += pItem->getMinDamage();
		MaxDamage += pItem->getMaxDamage();
	}

	// 실제 랜덤 데미지를 계산한다.
	Damage_t RealDamage = max(1, Random(MinDamage, MaxDamage));

	return RealDamage;
}

//////////////////////////////////////////////////////////////////////////////
// 몬스터 공격자의 순수 데미지를 계산한다.
//////////////////////////////////////////////////////////////////////////////
Damage_t computePureMonsterDamage(Monster* pMonster) {
	Assert(pMonster != NULL);

	Damage_t MinDamage   = pMonster->getDamage(ATTR_CURRENT);
	Damage_t MaxDamage   = pMonster->getDamage(ATTR_MAX);
	Damage_t RealDamage  = max(1, Random(MinDamage, MaxDamage));
	uint     timeband    = getZoneTimeband(pMonster->getZone());

	RealDamage = (Damage_t)getPercentValue(RealDamage, MonsterTimebandFactor[timeband]);

	return RealDamage;
}

// 점과 점사이를 걸어서 갈 수 있는가? (크리쳐로 막힌 경우는 제외)
bool isPassLine(Zone* pZone, ZoneCoord_t sX, ZoneCoord_t sY, ZoneCoord_t eX, ZoneCoord_t eY, bool blockByCreature) {
	list<TPOINT> tpList;

	if (pZone == NULL)
		return false;
	
	// 두 점사이의 진선을 이루는 점들을 구한다.
	getLinePoint(sX, sY, eX, eY, tpList);

	if (tpList.empty())
		return false;

	VSRect rect(0, 0, pZone->getWidth()-1, pZone->getHeight()-1);

	list<TPOINT>::const_iterator itr = tpList.begin();
	TPOINT prev = (*itr);
	for (; itr != tpList.end(); ++itr) {
		TPOINT tp = (*itr);
		if (!rect.ptInRect(tp.x, tp.y))
			return false;

		if (tp.x == sX && tp.y == sY)
			// 시작점은 체크 안한다.
			continue;

		Tile& tile = pZone->getTile(tp.x, tp.y);

		if (blockByCreature)
			if (tile.isGroundBlocked())
				return false;
		else if (tile.isFixedGroundBlocked())
			return false;

		// 대각선으로 바뀐 경우, 한쪽 방향으로만 갈수 있어도 가능하다.
		// (1,1) -> (2,2) 인 경우, (1,2) 나 (2,1) 둘 중에 하나만 지나갈 수 있어도 지나갈 수 있다고 본다.
		if (prev.x != tp.x && prev.y != tp.y) {
			if (!rect.ptInRect(tp.x, prev.y))
				return false;
			if (!rect.ptInRect(prev.x, tp.y))
				return false;

			Tile& tile1 = pZone->getTile(tp.x, prev.y);
			Tile& tile2 = pZone->getTile(prev.x, tp.y);

			if (tile1.isFixedGroundBlocked() && tile2.isFixedGroundBlocked())
				return false;
		}
		prev = tp;
	}
	return true;
}

// 두 점사이의 진선을 이루는 점들을 구한다.
void getLinePoint(ZoneCoord_t sX, ZoneCoord_t sY, ZoneCoord_t eX, ZoneCoord_t eY, list<TPOINT>& tpList) {
	int xLength = abs(sX - eX);
	int yLength = abs(sY - eY);

	if (xLength == 0 && yLength == 0)
		return;

	if (xLength > yLength) {
		if (sX > eX) {
			int tmpX = sX; sX = eX; eX = tmpX;
			int tmpY = sY; sY = eY; eY = tmpY;
		}
		
		float yStep = (float)(eY-sY) / (float)(eX-sX);

		for (int i = sX; i <= eX; i++) {
			TPOINT pt;
			pt.x = i;
			pt.y = sY + (int)(yStep*(float)(i-sX));

			tpList.push_back(pt);
		}
	} else {
		if (sY > eY) {
			int tmpX = sX; sX = eX; eX = tmpX;
			int tmpY = sY; sY = eY; eY = tmpY;
		}
		
		float xStep = (float)(eX-sX) / (float)(eY-sY);

		for (int i = sY; i <= eY; i++) {
			TPOINT pt;
			pt.x = sX + (int)(xStep*(float)(i-sY));
			pt.y = i;
			
			tpList.push_back(pt);
		}
	}
}

ElementalType getElementalTypeFromString(const string& type) {
	if (type == "Fire") return ELEMENTAL_FIRE;
	else if (type == "Water") return ELEMENTAL_WATER;
	else if (type == "Earth") return ELEMENTAL_EARTH;
	else if (type == "Wind") return ELEMENTAL_WIND;

	return ELEMENTAL_MAX;
}

Damage_t computeElementalCombatSkill(Ousters* pOusters, Creature* pTargetCreature, ModifyInfo& AttackerMI) {
	Assert(pOusters != NULL);
	Assert(pTargetCreature != NULL);

	Zone* pZone = pTargetCreature->getZone();
	Assert(pZone != NULL);

	Damage_t ret = 0;
	int ratio = pOusters->getPassiveRatio();
	bool bMaster = false;

	if (pTargetCreature->isMonster()) {
		Monster* pMonster = dynamic_cast<Monster*>(pTargetCreature);
		Assert(pTargetCreature != NULL);

		if (pMonster->isMaster())
			bMaster = true;
	}

	if (pOusters->isPassiveAvailable(SKILL_FIRE_OF_SOUL_STONE)) {
		if ((rand()%100) < min(30, ratio)) {
			ret += pOusters->getPassiveBonus(SKILL_FIRE_OF_SOUL_STONE);

			GCAddEffect gcAddEffect;
			gcAddEffect.setObjectID(pTargetCreature->getObjectID());
			gcAddEffect.setEffectID(Effect::EFFECT_CLASS_FIRE_OF_SOUL_STONE);
			gcAddEffect.setDuration(0);

			pZone->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);
		}
	}

	if (pOusters->isPassiveAvailable(SKILL_ICE_OF_SOUL_STONE)) {
		if (!bMaster && !pTargetCreature->isFlag(Effect::EFFECT_CLASS_ICE_OF_SOUL_STONE) && (rand()%100) < min(23, ratio*2/3)) {
			Turn_t duration = pOusters->getPassiveBonus(SKILL_ICE_OF_SOUL_STONE);
			// 이팩트 클래스를 만들어 붙인다.
			EffectIceOfSoulStone* pEffect = new EffectIceOfSoulStone(pTargetCreature);
			pEffect->setDeadline(duration);
			pTargetCreature->addEffect(pEffect);
			pTargetCreature->setFlag(Effect::EFFECT_CLASS_ICE_OF_SOUL_STONE);
			
			GCAddEffect gcAddEffect;
			gcAddEffect.setObjectID(pTargetCreature->getObjectID());
			gcAddEffect.setEffectID(Effect::EFFECT_CLASS_ICE_OF_SOUL_STONE);
			gcAddEffect.setDuration(duration);

			pZone->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);
		}
	}

	if (pOusters->isPassiveAvailable(SKILL_SAND_OF_SOUL_STONE)) {
		if ((rand()%100) < min(30, ratio)) {
			GCAddEffectToTile gcAddEffect;
			gcAddEffect.setObjectID(pTargetCreature->getObjectID());
			gcAddEffect.setEffectID(Effect::EFFECT_CLASS_SAND_OF_SOUL_STONE);
			gcAddEffect.setDuration(7);
			gcAddEffect.setXY(pTargetCreature->getX(), pTargetCreature->getY());

			pZone->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);

			Damage_t bonusDamage = pOusters->getPassiveBonus(SKILL_SAND_OF_SOUL_STONE);
			VSRect rect(0, 0, pZone->getWidth()-1, pZone->getHeight()-1);

			GCSkillToObjectOK6 gcOK6;
			gcOK6.setXY(pOusters->getX(), pOusters->getY());
			gcOK6.setSkillType(SKILL_ATTACK_MELEE);
			gcOK6.setDuration(0);

			for (int i=pTargetCreature->getX() - 1; i <= pTargetCreature->getX() + 1; ++i)
				for (int j=pTargetCreature->getY() - 1; j <= pTargetCreature->getY() + 1; ++j) {
					if (!rect.ptInRect(i, j))
						continue;

					Tile& rTile = pZone->getTile(i,j);
					if (!rTile.hasCreature(Creature::MOVE_MODE_WALKING))
						continue;
					Creature* pTileCreature = rTile.getCreature(Creature::MOVE_MODE_WALKING);

					if (pTileCreature == NULL || pTileCreature->getObjectID() == pTargetCreature->getObjectID() ||
						pTileCreature->isOusters() || pTileCreature->isNPC())
						continue;

					GCSkillToObjectOK4 gcOK4;
					gcOK4.setTargetObjectID(pTileCreature->getObjectID());
					gcOK4.setSkillType(SKILL_ATTACK_MELEE);
					gcOK4.setDuration(0);

					if (pTileCreature->isPC()) {
						gcOK6.clearList();
						setDamage(pTileCreature, bonusDamage, pOusters, SKILL_SAND_OF_SOUL_STONE, &gcOK6, &AttackerMI);
						pTileCreature->getPlayer()->sendPacket(&gcOK6);
					}
					else if (pTileCreature->isMonster())
						setDamage(pTileCreature, bonusDamage, pOusters, SKILL_SAND_OF_SOUL_STONE, NULL, &AttackerMI);

					pZone->broadcastPacket(pTileCreature->getX(), pTileCreature->getY(), &gcOK4);

					if (pTileCreature->isDead()) {
						int exp = computeCreatureExp(pTileCreature, 100, pOusters);
						shareOustersExp(pOusters, exp, AttackerMI);
						increaseAlignment(pOusters, pTileCreature, AttackerMI);
					}
				}
			ret += bonusDamage;
		}
	}

	if (pOusters->isPassiveAvailable(SKILL_BLOCK_HEAD)) {
		if (!bMaster && !pTargetCreature->isFlag(Effect::EFFECT_CLASS_BLOCK_HEAD) && (rand()%100) < min(15, ratio/2)) {
			Turn_t duration = pOusters->getPassiveBonus(SKILL_BLOCK_HEAD);
			// 이팩트 클래스를 만들어 붙인다.
			EffectBlockHead* pEffect = new EffectBlockHead(pTargetCreature);
			pEffect->setDeadline(duration);
			pTargetCreature->addEffect(pEffect);
			pTargetCreature->setFlag(Effect::EFFECT_CLASS_BLOCK_HEAD);
			
			GCAddEffect gcAddEffect;
			gcAddEffect.setObjectID(pTargetCreature->getObjectID());
			gcAddEffect.setEffectID(Effect::EFFECT_CLASS_BLOCK_HEAD);
			gcAddEffect.setDuration(duration);

			pZone->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);
		}
	}

	if (pOusters->isPassiveAvailable(SKILL_BLESS_FIRE)) {
		if ((rand()%100) < min(30, ratio)) {
			ret += pOusters->getPassiveBonus(SKILL_BLESS_FIRE);

			GCAddEffectToTile gcAddEffect;
			gcAddEffect.setObjectID(pTargetCreature->getObjectID());
			gcAddEffect.setXY(pTargetCreature->getX(), pTargetCreature->getY());
			gcAddEffect.setEffectID(Effect::EFFECT_CLASS_BLESS_FIRE);
			gcAddEffect.setDuration(0);

			pZone->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);
		}
	}

/*	if (pOusters->isPassiveAvailable(SKILL_WATER_SHIELD))
	{
		if ((rand()%100) < min(20, (int)(ratio * 3/2)))
		{
			EffectWaterShield* pEffect = new EffectWaterShield(pOusters);
			pEffect->setDeadline(0);
			pOusters->addEffect(pEffect);
			pOusters->setFlag(Effect::EFFECT_CLASS_WATER_SHIELD);
			
			GCAddEffectToTile gcAddEffect;
			gcAddEffect.setObjectID(pOusters->getObjectID());
			gcAddEffect.setXY(pOusters->getX(), pOusters->getY());
			gcAddEffect.setEffectID(Effect::EFFECT_CLASS_WATER_SHIELD);
			gcAddEffect.setDuration(0);

			pZone->broadcastPacket(pOusters->getX(), pOusters->getY(), &gcAddEffect);
		}
	}
*/	
	if (pOusters->isPassiveAvailable(SKILL_SAND_CROSS)) {
		if ((rand()%100) < min(30, ratio)) {
			GCAddEffectToTile gcAddEffect;
			gcAddEffect.setObjectID(pTargetCreature->getObjectID());
			gcAddEffect.setEffectID(Effect::EFFECT_CLASS_SAND_CROSS);
			gcAddEffect.setDuration(0);
			gcAddEffect.setXY(pTargetCreature->getX(), pTargetCreature->getY());

			pZone->broadcastPacket(pTargetCreature->getX(), pTargetCreature->getY(), &gcAddEffect);

			Damage_t bonusDamage = pOusters->getPassiveBonus(SKILL_SAND_CROSS);
			VSRect rect(0, 0, pZone->getWidth()-1, pZone->getHeight()-1);

			GCSkillToObjectOK6 gcOK6;
			gcOK6.setXY(pOusters->getX(), pOusters->getY());
			gcOK6.setSkillType(SKILL_ATTACK_MELEE);
			gcOK6.setDuration(0);

			for (int i=pTargetCreature->getX() - 3; i <= pTargetCreature->getX() + 3; ++i) {
				int j = pTargetCreature->getY();

				if (!rect.ptInRect(i, j))
					continue;

				Tile& rTile = pZone->getTile(i,j);
				if (!rTile.hasCreature(Creature::MOVE_MODE_WALKING))
					continue;
				Creature* pTileCreature = rTile.getCreature(Creature::MOVE_MODE_WALKING);

				if (pTileCreature == NULL || pTileCreature->getObjectID() == pTargetCreature->getObjectID() ||
					pTileCreature->isOusters() || pTileCreature->isNPC())
					continue;

				GCSkillToObjectOK4 gcOK4;
				gcOK4.setTargetObjectID(pTileCreature->getObjectID());
				gcOK4.setSkillType(SKILL_ATTACK_MELEE);
				gcOK4.setDuration(0);

				if (pTileCreature->isPC()) {
					gcOK6.clearList();
					setDamage(pTileCreature, bonusDamage, pOusters, SKILL_SAND_CROSS, &gcOK6, &AttackerMI);
					pTileCreature->getPlayer()->sendPacket(&gcOK6);
				}
				else if (pTileCreature->isMonster())
					setDamage(pTileCreature, bonusDamage, pOusters, SKILL_SAND_CROSS, NULL, &AttackerMI);

				pZone->broadcastPacket(pTileCreature->getX(), pTileCreature->getY(), &gcOK4);

				if (pTileCreature->isDead()) {
					int exp = computeCreatureExp(pTileCreature, 100, pOusters);
					shareOustersExp(pOusters, exp, AttackerMI);
					increaseAlignment(pOusters, pTileCreature, AttackerMI);
				}
			}

			for (int j=pTargetCreature->getY() - 3; j <= pTargetCreature->getY() + 3; ++j) {
				int i = pTargetCreature->getX();

				if (!rect.ptInRect(i, j))
					continue;

				Tile& rTile = pZone->getTile(i,j);
				if (!rTile.hasCreature(Creature::MOVE_MODE_WALKING))
					continue;
				Creature* pTileCreature = rTile.getCreature(Creature::MOVE_MODE_WALKING);

				if (pTileCreature == NULL || pTileCreature->getObjectID() == pTargetCreature->getObjectID() ||
					pTileCreature->isOusters() || pTileCreature->isNPC())
					continue;

				GCSkillToObjectOK4 gcOK4;
				gcOK4.setTargetObjectID(pTileCreature->getObjectID());
				gcOK4.setSkillType(SKILL_ATTACK_MELEE);
				gcOK4.setDuration(0);

				if (pTileCreature->isPC()) {
					gcOK6.clearList();
					setDamage(pTileCreature, bonusDamage, pOusters, SKILL_SAND_CROSS, &gcOK6, &AttackerMI);
					pTileCreature->getPlayer()->sendPacket(&gcOK6);
				}
				else if (pTileCreature->isMonster())
					setDamage(pTileCreature, bonusDamage, pOusters, SKILL_SAND_CROSS, NULL, &AttackerMI);

				pZone->broadcastPacket(pTileCreature->getX(), pTileCreature->getY(), &gcOK4);

				if (pTileCreature->isDead()) {
					int exp = computeCreatureExp(pTileCreature, 100, pOusters);
					shareOustersExp(pOusters, exp, AttackerMI);
					increaseAlignment(pOusters, pTileCreature, AttackerMI);
				}
			}

			ret += bonusDamage;
		}
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////////////
// 공격할 수 있는가?
// 무적 상태나 non PK 를 위해서 공격할 수 있는지를 체크한다.
//////////////////////////////////////////////////////////////////////////////
bool canAttack(Creature* pAttacker, Creature* pDefender) {
	Assert(pDefender != NULL);

	// 무적 상태 체크
	if (pDefender->isFlag(Effect::EFFECT_CLASS_NO_DAMAGE))
		return false;

	// Attacker 가 NULL 이면 걍 true
	// 젠장 먼가 깔끔하게 고치기 바람 Effect에서 체크할때 Attacker 가 NULL 이 될 수 있다.
	if (pAttacker == NULL)
		return true;

	// 게임서버에 PK 설정이 되었는가?
	static bool bNonPK = g_pGameServerInfoManager->getGameServerInfo(1, g_pConfig->getPropertyInt("ServerID"), g_pConfig->getPropertyInt("WorldID"))->isNonPKServer();
	bool canPK = bNonPK || GDRLairManager::Instance().isGDRLairZone(pAttacker->getZoneID());

	// non PK 체크
	if (bNonPK && pAttacker->isPC() && pDefender->isPC())
		return false;

	return true;
}
