//////////////////////////////////////////////////////////////////////////////
// Filename    : DivineSpirits.cpp
// Written by  : 
// Description : 
//////////////////////////////////////////////////////////////////////////////

#include "DivineSpirits.h"
#include "EffectDivineSpirits.h"

#include "Gpackets/GCSkillToSelfOK1.h"
#include "Gpackets/GCSkillToSelfOK2.h"
#include "Gpackets/GCAddEffect.h"

//////////////////////////////////////////////////////////////////////////////
// 슬레이어 셀프 핸들러
//////////////////////////////////////////////////////////////////////////////
void DivineSpirits::execute(Ousters* pOusters, OustersSkillSlot* pOustersSkillSlot, CEffectID_t CEffectID)
	throw(Error)
{
	__BEGIN_TRY

	//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << " Begin(slayer)" << endl;

	Assert(pOusters != NULL);
	Assert(pOustersSkillSlot != NULL);

	try 
	{
		Player* pPlayer = pOusters->getPlayer();
		Zone* pZone = pOusters->getZone();

		Assert(pPlayer != NULL);
		Assert(pZone != NULL);

		GCSkillToSelfOK1 _GCSkillToSelfOK1;
		GCSkillToSelfOK2 _GCSkillToSelfOK2;

		SkillType_t       SkillType  = pOustersSkillSlot->getSkillType();
		SkillInfo*        pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);
		//SkillDomainType_t DomainType = pSkillInfo->getDomainType();
		//SkillLevel_t      SkillLevel = pOustersSkillSlot->getExpLevel();

		int  RequiredMP  = pSkillInfo->getConsumeMP();
		bool bManaCheck  = hasEnoughMana(pOusters, RequiredMP);
		bool bTimeCheck  = verifyRunTime(pOustersSkillSlot);
		bool bRangeCheck = checkZoneLevelToUseSkill(pOusters);
		bool bHitRoll    = HitRoll::isSuccessMagic(pOusters, pSkillInfo, pOustersSkillSlot);
		bool bEffected   = pOusters->isFlag(Effect::EFFECT_CLASS_DIVINE_SPIRITS);
		bool bSatisfyRequire = pOusters->satisfySkillRequire(pSkillInfo);

		if (bManaCheck && bTimeCheck && bRangeCheck && bHitRoll && !bEffected && bSatisfyRequire)
		{
			decreaseMana(pOusters, RequiredMP, _GCSkillToSelfOK1);

			// 지속 시간을 계산한다.
			SkillInput input(pOusters, pOustersSkillSlot);
			SkillOutput output;
			computeOutput(input, output);

			// 이팩트 클래스를 만들어 붙인다.
			EffectDivineSpirits* pEffect = new EffectDivineSpirits(pOusters);
			pEffect->setDeadline(output.Duration);
			pEffect->setBonus(output.Damage);
			pOusters->addEffect(pEffect);
			pOusters->setFlag(Effect::EFFECT_CLASS_DIVINE_SPIRITS);

//			OUSTERS_RECORD prev;
//			pOusters->getOustersRecord(prev);
		//	pOusters->initAllStat();
//			pOusters->addModifyInfo(prev, _GCSkillToSelfOK1);

			_GCSkillToSelfOK1.setSkillType(SkillType);
			_GCSkillToSelfOK1.setCEffectID(CEffectID);
			_GCSkillToSelfOK1.setDuration(output.Duration);
		
			_GCSkillToSelfOK2.setObjectID(pOusters->getObjectID());
			_GCSkillToSelfOK2.setSkillType(SkillType);
			_GCSkillToSelfOK2.setDuration(output.Duration);
		
			pPlayer->sendPacket(&_GCSkillToSelfOK1);
			pZone->broadcastPacket(pOusters->getX(), pOusters->getY(),  &_GCSkillToSelfOK2, pOusters);

			GCAddEffect gcAddEffect;
			gcAddEffect.setObjectID(pOusters->getObjectID());
			gcAddEffect.setEffectID(pEffect->getEffectClass());
			gcAddEffect.setDuration(output.Duration);
			pZone->broadcastPacket(pOusters->getX(), pOusters->getY(), &gcAddEffect, pOusters);

			pOustersSkillSlot->setRunTime(output.Delay);
		} 
		else 
		{
			executeSkillFailNormal(pOusters, getSkillType(), NULL);
		}
	} 
	catch (Throwable & t) 
	{
		executeSkillFailException(pOusters, getSkillType());
	}

	//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << " End(slayer)" << endl;

	__END_CATCH
}

DivineSpirits g_DivineSpirits;
