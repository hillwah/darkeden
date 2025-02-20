//////////////////////////////////////////////////////////////////////////////
// Filename    : IceHail.cpp
// Written by  : excel96
// Description : 
//////////////////////////////////////////////////////////////////////////////

#include "IceHail.h"
#include "EffectIceHail.h"
#include "ZoneUtil.h"

#include "Gpackets/GCSkillToTileOK1.h"
#include "Gpackets/GCSkillToTileOK2.h"
#include "Gpackets/GCSkillToTileOK3.h"
#include "Gpackets/GCSkillToTileOK4.h"
#include "Gpackets/GCSkillToTileOK5.h"
#include "Gpackets/GCSkillToTileOK6.h"
#include "Gpackets/GCAddEffect.h"
#include "Gpackets/GCAddEffectToTile.h"

//////////////////////////////////////////////////////////////////////////////
// 몬스터 타일 핸들러
//////////////////////////////////////////////////////////////////////////////
void IceHail::execute(Monster* pMonster, ZoneCoord_t X, ZoneCoord_t Y)
	throw(Error)
{
	__BEGIN_TRY

	//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << "begin(monster) " << endl;

	try
	{
		Zone* pZone = pMonster->getZone();
		Assert(pZone != NULL);

		GCSkillToTileOK2 _GCSkillToTileOK2;
		GCSkillToTileOK3 _GCSkillToTileOK3;
		GCSkillToTileOK4 _GCSkillToTileOK4;
		GCSkillToTileOK5 _GCSkillToTileOK5;
		GCSkillToTileOK6 _GCSkillToTileOK6;

		SkillType_t SkillType  = getSkillType();
		SkillInfo*  pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);

		bool bHitRoll    = HitRoll::isSuccessMagic(pMonster, pSkillInfo);

		bool bTileCheck = false;
		if (isValidZoneCoord(pZone, X, Y ) )
		{
			Tile& tile = pZone->getTile(X, Y);
			if (tile.canAddEffect()) bTileCheck = true;
			// 머시 그라운드 있음 추가 못한당.
//			if (tile.getEffect(Effect::EFFECT_CLASS_MERCY_GROUND) != NULL ) bTileCheck=false;
		}

		Range_t Range = 5;

		ZoneCoord_t myX = pMonster->getX();
		ZoneCoord_t myY = pMonster->getY();

		if (bHitRoll && bTileCheck)
		{
			// 데미지와 지속 시간을 계산한다.
			SkillInput input(pMonster);
			SkillOutput output;
			computeOutput(input, output);

			// 이펙트 오브젝트를 생성한다.
			EffectIceHail* pEffect = new EffectIceHail(pZone, X, Y);
			pEffect->setDeadline(output.Duration);
			pEffect->setDamage(output.Damage);
			pEffect->setNextTime(15);
			pEffect->setTick(output.Tick);
			pEffect->setRange(2);
			pEffect->setCasterID(pMonster->getObjectID());

			// 타일에 붙은 이펙트는 OID를 받아야 한다.
			ObjectRegistry & objectregister = pZone->getObjectRegistry();
			objectregister.registerObject(pEffect);
		
			// 존 및 타일에다가 이펙트를 추가한다.
			pZone->addEffect(pEffect);	
			pZone->getTile(X, Y ).addEffect(pEffect);

			GCAddEffectToTile gcAE;
			gcAE.setXY(X, Y);
			gcAE.setEffectID(Effect::EFFECT_CLASS_ICICLE_DROP_SIGN);
			gcAE.setDuration(10);
			pZone->broadcastPacket(X, Y, &gcAE);

			_GCSkillToTileOK3.setObjectID(pMonster->getObjectID());
			_GCSkillToTileOK3.setSkillType(SkillType);
			_GCSkillToTileOK3.setX(X);
			_GCSkillToTileOK3.setY(Y);

			_GCSkillToTileOK4.setSkillType(SkillType);
			_GCSkillToTileOK4.setX(X);
			_GCSkillToTileOK4.setY(Y);
			_GCSkillToTileOK4.setDuration(output.Duration);
			_GCSkillToTileOK4.setRange(Range);

			_GCSkillToTileOK5.setObjectID(pMonster->getObjectID());
			_GCSkillToTileOK5.setSkillType(SkillType);
			_GCSkillToTileOK5.setX(X);
			_GCSkillToTileOK5.setY(Y);
			_GCSkillToTileOK5.setDuration(output.Duration);
			_GCSkillToTileOK5.setRange(Range);

			list<Creature*> cList;
			cList.push_back(pMonster);

			cList = pZone->broadcastSkillPacket(myX, myY, X, Y, &_GCSkillToTileOK5, cList);

			pZone->broadcastPacket(myX, myY,  &_GCSkillToTileOK3 , cList);
			pZone->broadcastPacket(X, Y,  &_GCSkillToTileOK4 , cList);
		}
		else 
		{
			executeSkillFailNormal(pMonster, getSkillType(), NULL);
		}
	}
	catch (Throwable & t) 
	{
		executeSkillFailException(pMonster, getSkillType());
	}

	//cout << "TID[" << Thread::self() << "]" << getSkillHandlerName() << " end(monster) " << endl;

	__END_CATCH
}

IceHail g_IceHail;
