//////////////////////////////////////////////////////////////////////////////
// Filename    : ChainThrowingAxe.cpp
// Written by  : excel96
// Description : 
//////////////////////////////////////////////////////////////////////////////

#include "ChainThrowingAxe.h"
#include "RankBonus.h"
#include "EffectMeteorStrike.h"

#include "Gpackets/GCSkillToTileOK1.h"
#include "Gpackets/GCSkillToTileOK2.h"
#include "Gpackets/GCSkillToTileOK3.h"
#include "Gpackets/GCSkillToTileOK4.h"
#include "Gpackets/GCSkillToTileOK5.h"
#include "Gpackets/GCSkillToTileOK6.h"
#include "Gpackets/GCAddEffect.h"

//////////////////////////////////////////////////////////////////////////////
// 몬스터 타일 핸들러
//////////////////////////////////////////////////////////////////////////////
void ChainThrowingAxe::execute(Monster* pMonster, ZoneCoord_t X, ZoneCoord_t Y)
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

		SkillType_t SkillType  = SKILL_CHAIN_THROWING_AXE;
		SkillInfo*  pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);

		bool bRangeCheck = verifyDistance(pMonster, X, Y, pSkillInfo->getRange());
		bool bHitRoll    = HitRoll::isSuccessMagic(pMonster, pSkillInfo);

		Dir_t dir = getDirection(pMonster->getX(), pMonster->getY(), X, Y);
		X = pMonster->getX() + dirMoveMask[dir].x * 7;
		Y = pMonster->getY() + dirMoveMask[dir].y * 7;

		bool bTileCheck = false;
		VSRect rect(0, 0, pZone->getWidth()-1, pZone->getHeight()-1);
		if (rect.ptInRect(X, Y))
		{
			Tile& tile = pZone->getTile(X, Y);
			if (tile.canAddEffect()) bTileCheck = true;
		}

		if (bRangeCheck && bHitRoll && bTileCheck)
		{
			if(rand() % 100 < 50 ) {
				checkMine(pZone, X, Y);
			}

			Tile&   tile  = pZone->getTile(X, Y);
			Range_t Range = 1;	// 항상 1이다.
		
			// 데미지와 지속 시간을 계산한다.
			SkillInput input(pMonster);
			input.SkillLevel = pMonster->getLevel();
			SkillOutput output;
			computeOutput(input, output);
			
			for(int i=0; i < 3; i++ )
			{
				// 이펙트 오브젝트를 생성한다.
				EffectMeteorStrike* pEffect = new EffectMeteorStrike(pZone, X, Y);
				pEffect->setNextTime(output.Duration + (int)(i * 2.5 ));
				pEffect->setUserObjectID(pMonster->getObjectID());
				pEffect->setBroadcastingEffect(false);
				//pEffect->setNextTime(0);
				//pEffect->setTick(output.Tick);
				pEffect->setDamage(output.Damage);
				pEffect->setSplashRatio(1, 75);
				pEffect->setSplashRatio(2, 50);
				//pEffect->setLevel(pSkillInfo->getLevel()/2);

				// 타일에 붙은 이펙트는 OID를 받아야 한다.
				ObjectRegistry & objectregister = pZone->getObjectRegistry();
				objectregister.registerObject(pEffect);
			
				// 존 및 타일에다가 이펙트를 추가한다.
				pZone->addEffect(pEffect);	
				tile.addEffect(pEffect);
			}
			
			ZoneCoord_t myX = pMonster->getX();
			ZoneCoord_t myY = pMonster->getY();

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

ChainThrowingAxe g_ChainThrowingAxe;
