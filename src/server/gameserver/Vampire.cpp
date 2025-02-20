//////////////////////////////////////////////////////////////////////////////
// Filename    : Vampire.cpp
// Written By  : Elca
// Description : 
//////////////////////////////////////////////////////////////////////////////

#include "Vampire.h"
#include "Player.h"
#include "OptionInfo.h"
#include "SkillInfo.h"
#include "ItemLoaderManager.h"
#include "EffectLoaderManager.h"
#include "SkillParentInfo.h"
#include "DB.h"
#include "ItemInfoManager.h"
#include "AbilityBalance.h"
#include "Stash.h"
#include "TradeManager.h"
#include "CreatureUtil.h"
#include "FlagSet.h"
#include "VampEXPInfo.h"
#include "Party.h"
#include "ItemUtil.h"
#include "PacketUtil.h"
#include "SkillUtil.h"
#include "Shape.h"
#include "GamePlayer.h"
//#include "RankEXPInfo.h"
#include "RankExpTable.h"
#include "VariableManager.h"
#include "WarSystem.h"
#include "ResurrectLocationManager.h"
#include "PKZoneInfoManager.h"
#include "TimeLimitItemManager.h"
#include "CastleSkillInfo.h"
#include "PetInfo.h"
#include <stdio.h>

#include "item/AR.h"
#include "item/SR.h"
#include "item/SG.h"
#include "item/SMG.h"
#include "item/PetItem.h"
#include "item/Belt.h"
#include "item/Skull.h"
#include "item/SubInventory.h"

#include "skill/VampireCastleSkillSlot.h"
#include "skill/EffectBless.h"
#include "skill/EffectParalyze.h"
#include "skill/EffectDoom.h"
#include "skill/EffectTransformToWolf.h"
#include "skill/EffectTransformToBat.h"
#include "EffectGrandMasterVampire.h"
#include "RaceWarLimiter.h"

#include "Gpackets/GCModifyInformation.h"
#include "Gpackets/GCChangeShape.h"
#include "Gpackets/GCSkillInfo.h"
#include "Gpackets/GCRealWearingInfo.h"
#include "Gpackets/GCStatusCurrentHP.h"
#include "Gpackets/GCTakeOff.h"
#include "Gpackets/GCOtherModifyInfo.h"
#include "Gpackets/GCPetStashList.h"

#include "MonsterInfo.h"
#include "GuildUnion.h"
#include "Store.h"
#include "AdvancementClassExpTable.h"
#include "DynamicZone.h"

#include "SystemAvailabilitiesManager.h"

#include <map>

const Color_t UNIQUE_COLOR = 0xFFFF;
const Color_t QUEST_COLOR = 0xFFFE;

const Level_t MAX_VAMPIRE_LEVEL = 150;
const Level_t MAX_VAMPIRE_LEVEL_OLD = 100;

//////////////////////////////////////////////////////////////////////////////
// incraseVampExp
//////////////////////////////////////////////////////////////////////////////
/*void Vampire::increaseVampExp(Exp_t Point)
{
	if (Point <= 0) return;

	Level_t curLevel = getLevel();

	if (curLevel >= VAMPIRE_MAX_LEVEL) 
	{
		// 레벨 한계에 도달해도 경험치는 쌓게 해준다.
		// by sigi. 2002.8.31
		Exp_t NewExp = getExp() + Point;

		WORD ExpSaveCount = getExpSaveCount();
		if (ExpSaveCount > VAMPIRE_EXP_SAVE_PERIOD)
		{
			char pField[80];
			sprintf(pField, "Exp=%lu", NewExp);
			tinysave(pField);
			ExpSaveCount = 0;
		}
		else ExpSaveCount++;
		setExpSaveCount(ExpSaveCount);

		setExp(NewExp);

		return;
	}

	Exp_t OldExp = getExp();

	Exp_t OldGoalExp = getGoalExp();
	Exp_t NewGoalExp = max(0, (int)(OldGoalExp - Point));

	// 누적 경험치에는 목표 경험치가 줄어든 만큼 플러스 하여야 한다.
	Exp_t DiffGoalExp = max(0, (int)(OldGoalExp - NewGoalExp));

	Exp_t NewExp = OldExp + DiffGoalExp;

	setExp(NewExp);
	setGoalExp(NewGoalExp);

	// 목표 경험치가 0 이라면 레벨 업이다.
	if (NewGoalExp == 0 && curLevel < VAMPIRE_MAX_LEVEL)
	{
		// 이 함수 자체가 load할때만 적용되는거기 때문에..
		// 필요없다. by sigi. 2002.8.31
	//	VAMPIRE_RECORD prev;
	//	getVampireRecord(prev);

		curLevel++;

		setLevel(curLevel);

		// add bonus point
		Bonus_t bonus = getBonus();

		if ((getSTR(ATTR_BASIC) + getDEX(ATTR_BASIC) + getINT(ATTR_BASIC) + getBonus() - 60) < ((getLevel() - 1) * 3)) 
		{
			// 레벨에 상관치 않고, 무조건 3으로 변경되었다.
			// 2001.12.12 김성민
			bonus += 3;
		}

		setBonus(bonus);

		VampEXPInfo* pBeforeExpInfo = g_pVampEXPInfoManager->getVampEXPInfo(curLevel-1);
		VampEXPInfo* pNextExpInfo = g_pVampEXPInfoManager->getVampEXPInfo(curLevel);
		Exp_t NextGoalExp = pNextExpInfo->getGoalExp();

		setGoalExp(NextGoalExp);

//		StringStream sav;
//		sav << "Level = " << (int)curLevel << "," << "Exp = " << (int)pBeforeExpInfo->getAccumExp() << "," << "GoalExp = " << (int)NextGoalExp << "," << "Bonus = " << (int)bonus;
//		pVampire->tinysave(sav.toString());

		char pField[80];
		sprintf(pField, "Level=%d, Exp=%lu, GoalExp=%lu, Bonus=%d", 
						curLevel, pBeforeExpInfo->getAccumExp(), NextGoalExp, bonus);
		tinysave(pField);
		setExpSaveCount(0);
	}
	else
	{
		// 주기적으로 저장한다.
		WORD ExpSaveCount = getExpSaveCount();
		if (ExpSaveCount > VAMPIRE_EXP_SAVE_PERIOD)
		{
			// by sigi. 2002.5.15
			char pField[80];
			sprintf(pField, "Exp=%lu, GoalExp=%lu", NewExp, NewGoalExp);
			tinysave(pField);

			ExpSaveCount = 0;
		}
		else ExpSaveCount++;
		setExpSaveCount(ExpSaveCount);
	}
}*/

//////////////////////////////////////////////////////////////////////////////
// increaseRankExp
//////////////////////////////////////////////////////////////////////////////
/*void Vampire::increaseRankExp(RankExp_t Point)
{
	if (Point <= 0) return;

	// PK존 안에서는 경험치를 주지 않는다.
	if (g_pPKZoneInfoManager->isPKZone(getZoneID() ) )
		return;

	Rank_t curRank = getRank();

	if (curRank >= VAMPIRE_MAX_RANK)
	{
		// 레벨 한계에 도달해도 경험치는 쌓게 해준다.
		// by sigi. 2002.8.31
		Exp_t NewExp = getRankExp() + Point;

		WORD ExpSaveCount = getRankExpSaveCount();
		if (ExpSaveCount > RANK_EXP_SAVE_PERIOD)
		{
			char pField[80];
			sprintf(pField, "RankExp=%lu", NewExp);
			tinysave(pField);
			ExpSaveCount = 0;
		}
		else ExpSaveCount++;
		setRankExpSaveCount(ExpSaveCount);

		setRankExp(NewExp);

		return;
	}

	Exp_t OldExp = getRankExp();

	Exp_t OldGoalExp = getRankGoalExp();
	Exp_t NewGoalExp = max(0, (int)(OldGoalExp - Point));

	// 누적 경험치에는 목표 경험치가 줄어든 만큼 플러스 하여야 한다.
	Exp_t DiffGoalExp = max(0, (int)(OldGoalExp - NewGoalExp));

	Exp_t NewExp = OldExp + DiffGoalExp;


	// 목표 경험치가 0 이라면 레벨 업이다.
	if (NewGoalExp == 0 && curRank < VAMPIRE_MAX_RANK)
	{
		VAMPIRE_RECORD prev;
		getVampireRecord(prev);

		curRank++;

		setRank(curRank);
		setRankExp(NewExp);
		setRankGoalExp(NewGoalExp);

		RankEXPInfo* pBeforeExpInfo = g_pRankEXPInfoManager[RANK_TYPE_VAMPIRE]->getRankEXPInfo(curRank-1);
		RankEXPInfo* pNextExpInfo = g_pRankEXPInfoManager[RANK_TYPE_VAMPIRE]->getRankEXPInfo(curRank);
		Exp_t NextGoalExp = pNextExpInfo->getGoalExp();

		setRankGoalExp(NextGoalExp);

//		StringStream sav;
//		sav << "Rank = " << (int)curRank << "," << "Exp = " << (int)pBeforeExpInfo->getAccumExp() << "," << "GoalExp = " << (int)NextGoalExp << "," << "Bonus = " << (int)bonus;
//		tinysave(sav.toString());

		char pField[80];
		sprintf(pField, "Rank=%d, RankExp=%lu, RankGoalExp=%lu",
						curRank, pBeforeExpInfo->getAccumExp(), NextGoalExp);
		tinysave(pField);
		setRankExpSaveCount(0);

		sendModifyInfo(prev);

		if (m_pZone != NULL)
		{
			GCOtherModifyInfo gcOtherModifyInfo;
			gcOtherModifyInfo.setObjectID(getObjectID());
			gcOtherModifyInfo.addShortData(MODIFY_RANK, curRank);

			m_pZone->broadcastPacket(m_X, m_Y, &gcOtherModifyInfo, this);
		}
	}
	else
	{
		setRankExp(NewExp);
		setRankGoalExp(NewGoalExp);

		// 주기적으로 저장한다.
		WORD rankExpSaveCount = getRankExpSaveCount();
		if (rankExpSaveCount > RANK_EXP_SAVE_PERIOD)
		{
			char pField[80];
			sprintf(pField, "RankExp=%lu, RankGoalExp=%lu", NewExp, NewGoalExp);
			tinysave(pField);

			rankExpSaveCount = 0;
		}
		else rankExpSaveCount++;
		setRankExpSaveCount(rankExpSaveCount);

		// 계급 경험치를 보내준다. by sigi. 2002.9.13
		GCModifyInformation gcModifyInformation;
		gcModifyInformation.addLongData(MODIFY_RANK_EXP, NewExp);
		m_pPlayer->sendPacket(&gcModifyInformation);
	}
}*/

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////



Vampire::Vampire () 
	throw() 
: PlayerCreature(0, NULL)
{
	__BEGIN_TRY

	m_CClass = CREATURE_CLASS_VAMPIRE;

	m_Mutex.setName("Vampire");

	// AttackMelee 같은 기본 공격을 집어넣어준다.
	for (int i=0; i<SKILL_DOUBLE_IMPACT; i++)
	{
		VampireSkillSlot* pVampireSkillSlot = new VampireSkillSlot;
		//pVampireSkillSlot = new VampireSkillSlot;	// 2002.1.16 by sigi
		pVampireSkillSlot->setName(m_Name);
		pVampireSkillSlot->setSkillType(i);
		pVampireSkillSlot->setInterval(5);
		pVampireSkillSlot->setRunTime();

		addSkill(pVampireSkillSlot);
	}

    for (int i = 0; i < VAMPIRE_WEAR_MAX; i++) 
        m_pWearItem[i] = NULL;

	// 핫 키를 초기화 한다.
//	for (int i = 0; i < 8; i++) 
//	{
//		m_HotKey[i] = 0;
//	}

	m_SilverDamage = 0;
	m_ClanType = 0;

	// HP 리젠 시간 초기화
	getCurrentTime(m_HPRegenTime);

	// 경험치 세이브 카운트 초기화
//	m_RankExpSaveCount       = 0;
	m_ExpSaveCount       = 0;
	m_FameSaveCount      = 0;
	m_AlignmentSaveCount = 0;

	__END_CATCH
}

Vampire::~Vampire() 
    throw()
{
	__BEGIN_TRY

	try
	{
		// 복장 정보를 생성해둔다. by sigi. 2002.6.18
		DWORD   flag;
		Color_t color[PCVampireInfo::VAMPIRE_COLOR_MAX];
		getShapeInfo (flag, color);

		char pField[128];
		sprintf(pField, "Shape=%ld, CoatColor=%d",
							flag,
							color[PCVampireInfo::VAMPIRE_COLOR_COAT]);

		//cout << "SAVE = " << pField << endl;

		tinysave(pField);


		// 떨어진 아이템의 내구성과 경험치, 성향 등을 저장한다.
		saveGears();
		saveExps();
		saveSkills();

		// 입고 있는 아이템을 메모리에서 삭제한다.
		destroyGears();

		// 클래스가 삭제될 경우, 해당하는 교환 정보를 삭제해야 함은 물론,
		// 교환 상대에게도 이 사실을 알려줘야 한다.
		TradeManager* pTradeManager = m_pZone->getTradeManager();
		TradeInfo* pInfo = pTradeManager->getTradeInfo(getName());
		if (pInfo != NULL)
		{
			// 교환 정보를 삭제
			pTradeManager->cancelTrade(this);
		}

		// 글로벌 파티 정보를 삭제한다. 
		// 일반적인 로그아웃의 경우에는
		// CGLogoutHandler에서 Zone::deleteCreature() 함수를 부르게 되고,
		// 비정상적인 경우라고 해도, 
		// GamePlayer::disconnect()에서 Zone::deleteCreature() 함수를 부르게 되므로,
		// 로컬 파티 및 파티 초대, 트레이드 정보를 걱정할 필요는 없다.
		deleteAllPartyInfo(this);

		// 기술들을 삭제
		map<SkillType_t, VampireSkillSlot*>::iterator itr = m_SkillSlot.begin();
		for (; itr != m_SkillSlot.end(); itr++)
		{
			VampireSkillSlot* pVampireSkillSlot = itr->second;
			SAFE_DELETE(pVampireSkillSlot);
		}
	} catch (Throwable& t) {
		filelog("vampireDestructor.txt", "%s", t.toString().c_str());
	} catch (exception& e) {
		filelog("vampireDestructor.txt", "Unknown std::exception");
	} catch (...) {
		filelog("vampireDestructor.txt", "Unknown ... exception");
	}

	m_bDeriveDestructed = true;

	__END_CATCH
}

// registerObject
// Zone에 종속된 ObjectRegistry를 사용해서, Vampire 와 소유아이템들의
// ObjectID를 할당받는다.
void Vampire::registerObject ()
    throw(Error)
{
    __BEGIN_TRY

    Assert(getZone() != NULL);

    // zone 의 object registery 에 접근한다.
    ObjectRegistry & OR = getZone()->getObjectRegistry();

    __ENTER_CRITICAL_SECTION(OR)

	// 모든 아이템에 OID 가 바뀌므로 시간제한 아이템 매니저에서 OID 맵을 지워줘야 한다.
	if (m_pTimeLimitItemManager != NULL)
		m_pTimeLimitItemManager->clear();

	// 우선 뱀파이어의 OID를 등록받는다.
	OR.registerObject_NOLOCKED(this);

	// 인벤토리의 아이템들의 OID를 등록받는다.
	registerInventory(OR);

	// Goods Inventory의 아이템들의 OID를 등록받는다.
	registerGoodsInventory(OR);

	// 장착하고 있는 아이템들의 OID를 등록받는다.
	for (int i = 0; i < VAMPIRE_WEAR_MAX; i++) 
	{
		Item* pItem = m_pWearItem[i];

		if (pItem != NULL) 
		{
			bool bCheck = true;

			// 양손 무기일 경우, WEAR_LEFTHAND 에서 등록했으므로,
			// 또 등록할 필요는 없다.
			if (i == WEAR_RIGHTHAND && isTwohandWeapon(pItem))
				bCheck = false;

			if (bCheck) registerItem(pItem, OR);
		}
	}

	// 마우스에 들고 있는 아이템의 OID를 등록 받는다.
	Item* pSlotItem = m_pExtraInventorySlot->getItem();
	if (pSlotItem != NULL) registerItem(pSlotItem, OR);

	m_Garbage.registerObject(OR);

	for (int i=0; i<MAX_PET_STASH; ++i )
	{
		Item* pItem = getPetStashItem(i);
		if (pItem != NULL ) registerItem(pItem, OR);
	}

    __LEAVE_CRITICAL_SECTION(OR)

	m_VampireInfo.setObjectID(m_ObjectID);
	m_pStore->updateStoreInfo();

    __END_CATCH
}

// Zone에 종속된 ObjectRegistry를 사용해서, Vampire 와 소유아이템들의
// ObjectID를 할당받는다. ItemTrace 를 남길지 여부 결정을 위해 따로 뺐다
void Vampire::registerInitObject ()
    throw(Error)
{
    __BEGIN_TRY

    Assert(getZone() != NULL);

    // zone 의 object registery 에 접근한다.
    ObjectRegistry & OR = getZone()->getObjectRegistry();

    __ENTER_CRITICAL_SECTION(OR)

	// 모든 아이템에 OID 가 바뀌므로 시간제한 아이템 매니저에서 OID 맵을 지워줘야 한다.
	if (m_pTimeLimitItemManager != NULL)
		m_pTimeLimitItemManager->clear();

	// 우선 뱀파이어의 OID를 등록받는다.
	OR.registerObject_NOLOCKED(this);

	// 인벤토리의 아이템들의 OID를 등록받는다.
	registerInitInventory(OR);

	// Goods Inventory의 아이템들의 OID를 등록받는다.
	registerGoodsInventory(OR);

	// 장착하고 있는 아이템들의 OID를 등록받는다.
	for (int i = 0; i < VAMPIRE_WEAR_MAX; i++) 
	{
		Item* pItem = m_pWearItem[i];

		if (pItem != NULL) 
		{
			// ItemTrace 를 남길 것인지 결정
			pItem->setTraceItem(bTraceLog(pItem ));

			bool bCheck = true;

			// 양손 무기일 경우, WEAR_LEFTHAND 에서 등록했으므로,
			// 또 등록할 필요는 없다.
			if (i == WEAR_RIGHTHAND && isTwohandWeapon(pItem))
				bCheck = false;

			if (bCheck) registerItem(pItem, OR);
		}
	}

	// 마우스에 들고 있는 아이템의 OID를 등록 받는다.
	Item* pSlotItem = m_pExtraInventorySlot->getItem();
	if (pSlotItem != NULL)
	{
		// ItemTrace 를 남길 것인지 결정
		pSlotItem->setTraceItem(bTraceLog(pSlotItem ));
		registerItem(pSlotItem, OR);
	}

	m_Garbage.registerObject(OR);

    __LEAVE_CRITICAL_SECTION(OR)

	m_VampireInfo.setObjectID(m_ObjectID);

    __END_CATCH
}

// 시간제한 아이템을 체크한다.
// 모든 아이템이 이미 register 되어있어야 한다.
void Vampire::checkItemTimeLimit() throw(Error)
{
	__BEGIN_TRY

	// 인벤토리에서 찾는다.
	{
		list<Item*> ItemList;
		int height = m_pInventory->getHeight();
		int width  = m_pInventory->getWidth();

		for (int j=0; j<height; j++)
		{
			for (int i=0; i<width; i++)
			{
				Item* pItem = m_pInventory->getItem(i, j);
				if (pItem != NULL)
				{
					// 체크된 아이템의 리스트에서 현재 아이템을 찾는다.
					list<Item*>::iterator itr = find(ItemList.begin(), ItemList.end(), pItem);

					if (itr == ItemList.end())
					{
						i += pItem->getVolumeWidth() - 1;

						if (wasteIfTimeLimitExpired(pItem ) )
						{
							m_pInventory->deleteItem(pItem->getObjectID());
							SAFE_DELETE(pItem);
						}
						else
						{
							// 리스트에 아이템이 없으면
							// 같은 아이템을 두번 체크하지 않기 위해서
							// 리스트에다가 아이템을 집어넣는다.
							ItemList.push_back(pItem);
						}

						// 서브 인벤토리일 경우 안에도 찾는다.
						if (pItem != NULL && pItem->getItemClass() == Item::ITEM_CLASS_SUB_INVENTORY )
						{
							SubInventory* pSubInventoryItem = dynamic_cast<SubInventory*>(pItem);
							Assert(pSubInventoryItem != NULL);
							Inventory* pSubInventory = pSubInventoryItem->getInventory();
							Assert(pSubInventory != NULL);

							list<Item*> SubItemList;

							for (CoordInven_t sy = 0; sy < pSubInventory->getHeight(); ++sy )
							{
								for (CoordInven_t sx = 0; sx < pSubInventory->getWidth(); ++sx )
								{
									Item* pSubItem = pSubInventory->getItem(sx, sy);

									if (pSubItem != NULL )
									{
										// 체크된 아이템의 리스트에서 현재 아이템을 찾는다.
										list<Item*>::iterator itr = find(SubItemList.begin(), SubItemList.end(), pSubItem);

										if (itr == SubItemList.end() )
										{
											if (wasteIfTimeLimitExpired(pSubItem ) )
											{
												pSubInventory->deleteItem(pSubItem->getObjectID());
												SAFE_DELETE(pSubItem);
											}
										}
										else
										{
											// 리스트에 아이템이 없으면
											// 같은 아이템을 두번 체크하지 않기 위해서
											// 리스트에다가 아이템을 집어넣는다.
											SubItemList.push_back(pSubItem);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// 장착하고 있는 것 중에 찾는다.
	{
		for (int i = 0; i < VAMPIRE_WEAR_MAX; i++) 
		{
			Item* pItem = m_pWearItem[i];

			if (pItem != NULL) 
			{
				bool bCheck = true;

				// 양손 무기일 경우, WEAR_LEFTHAND 에서 등록했으므로,
				// 또 등록할 필요는 없다.
				if (i == WEAR_RIGHTHAND && isTwohandWeapon(pItem))
					bCheck = false;

				if (bCheck) 
				{
					if (wasteIfTimeLimitExpired(pItem ) )
					{
						deleteWearItem((WearPart)i);
						if (i == WEAR_LEFTHAND && isTwohandWeapon(pItem) )
							deleteWearItem(WEAR_RIGHTHAND);
						SAFE_DELETE(pItem);
					}
				}
			}
		}
	}

	// 마우스에 들고 있는 아이템을 체크한다.
	{
		Item* pSlotItem = m_pExtraInventorySlot->getItem();
		if (pSlotItem != NULL && wasteIfTimeLimitExpired(pSlotItem ))
		{
			deleteItemFromExtraInventorySlot();
			SAFE_DELETE(pSlotItem);
		}
	}

	__END_CATCH
}

void Vampire::updateEventItemTime(DWORD time ) throw(Error)
{
	__BEGIN_TRY

	// 인벤토리에서 찾는다.
	{
		list<Item*> ItemList;
		int height = m_pInventory->getHeight();
		int width  = m_pInventory->getWidth();

		for (int j=0; j<height; j++)
		{
			for (int i=0; i<width; i++)
			{
				Item* pItem = m_pInventory->getItem(i, j);
				if (pItem != NULL)
				{
					// 체크된 아이템의 리스트에서 현재 아이템을 찾는다.
					list<Item*>::iterator itr = find(ItemList.begin(), ItemList.end(), pItem);

					if (itr == ItemList.end())
					{
						i += pItem->getVolumeWidth() - 1;

						updateItemTimeLimit(pItem, time);

						// 리스트에 아이템이 없으면
						// 같은 아이템을 두번 체크하지 않기 위해서
						// 리스트에다가 아이템을 집어넣는다.
						ItemList.push_back(pItem);
					}
				}
			}
		}
	}

	// 장착하고 있는 것 중에 찾는다.
	{
		for (int i = 0; i < VAMPIRE_WEAR_MAX; i++) 
		{
			Item* pItem = m_pWearItem[i];

			if (pItem != NULL) 
			{
				bool bCheck = true;

				// 양손 무기일 경우, WEAR_LEFTHAND 에서 등록했으므로,
				// 또 등록할 필요는 없다.
				if (i == WEAR_RIGHTHAND && isTwohandWeapon(pItem))
					bCheck = false;

				if (bCheck) 
				{
					updateItemTimeLimit(pItem, time);
				}
			}
		}
	}

	// 마우스에 들고 있는 아이템을 체크한다.
	{
		Item* pSlotItem = m_pExtraInventorySlot->getItem();
		if (pSlotItem != NULL)
		{
			updateItemTimeLimit(pSlotItem, time);
		}
	}

	__END_CATCH
}

///////////////////////////////////////////
//	Vampire와 Slayer사이의 변신을 위해서
//	아템 로딩은 따로 처리한다.
//
void Vampire::loadItem(bool checkTimeLimit )
	throw(InvalidProtocolException, Error)
{
	__BEGIN_TRY

	PlayerCreature::loadItem();

    // 인벤토리를 생성한다.
	SAFE_DELETE(m_pInventory);
	m_pInventory = new Inventory(10, 6);
	m_pInventory->setOwner(getName());

	// 아이템을 로드한다.
	g_pItemLoaderManager->load(this);

	// 구매한 아이템을 로드한다.
	PlayerCreature::loadGoods();

	// 로드한 아이템들을 등록시키고
    registerInitObject();

	if (checkTimeLimit )
	{
		checkItemTimeLimit();
	}

	// 입고 있는 옷에 따라 능력치를 계산해준다.
	initAllStat();

	__END_CATCH
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
bool Vampire::load ()
	throw(InvalidProtocolException, Error)
{
	__BEGIN_TRY

	if (!PlayerCreature::load() ) return false;

	Statement* pStmt   = NULL;
	Result*    pResult = NULL;

	int reward = 0;

	BEGIN_DB
	{
		pStmt   = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
		pResult = pStmt->executeQuery("SELECT Name, AdvancementClass, AdvancementGoalExp, Sex, BatColor, SkinColor, MasterEffectColor, STR, DEX, INTE, HP, CurrentHP, Fame, GoalExp, Level, Bonus, Gold, GuildID, ZoneID, XCoord, YCoord, Sight, Alignment, StashGold, StashNum, Competence, CompetenceShape, ResurrectZone, SilverDamage, Reward, SMSCharge, Rank, RankGoalExp FROM Vampire WHERE Name = '%s' AND Active = 'ACTIVE'", m_Name.c_str());

		if (pResult->getRowCount() == 0) 
		{
			//throw Error("Critical Error : data intergrity broken. (로그인 서버에서 게임 서버로 넘어오는 동안에 캐릭터가 삭제되었습니다.)");
			SAFE_DELETE(pStmt);
			return false;
		}

		pResult->next();

		uint i = 0;

		setName(pResult->getString(++i));

		Level_t advLevel = pResult->getInt(++i);
		Exp_t	advGoalExp = pResult->getInt(++i);

		m_pAdvancementClass = new AdvancementClass(advLevel, advGoalExp, AdvancementClassExpTable::s_AdvancementClassExpTable);
		if (getAdvancementClassLevel() > 0 ) m_bAdvanced = true;

		setSex(pResult->getString(++i));
//		setMasterEffectColor(pResult->getInt(++i));
		setBatColor(pResult->getInt(++i));
		setSkinColor(pResult->getInt(++i));
		setMasterEffectColor(pResult->getInt(++i));

		m_STR[ATTR_BASIC]   = pResult->getInt(++i);
		m_STR[ATTR_CURRENT] = m_STR[ATTR_BASIC];
	   	m_STR[ATTR_MAX]     = m_STR[ATTR_BASIC];

		m_DEX[ATTR_BASIC]   = pResult->getInt(++i);
		m_DEX[ATTR_CURRENT] = m_DEX[ATTR_BASIC];
	   	m_DEX[ATTR_MAX]     = m_DEX[ATTR_BASIC];

		m_INT[ATTR_BASIC]   = pResult->getInt(++i);
		m_INT[ATTR_CURRENT] = m_INT[ATTR_BASIC];
	   	m_INT[ATTR_MAX]     = m_INT[ATTR_BASIC];

		setHP(pResult->getInt(++i) , ATTR_MAX);
		setHP(getHP(ATTR_MAX) , ATTR_BASIC);
		setHP(pResult->getInt(++i) , ATTR_CURRENT);
		
		setFame(pResult->getInt(++i));

//		setExp(pResult->getInt(++i));
		setGoalExp(pResult->getInt(++i));
//		setExpOffset(pResult->getInt(++i));
		setLevel(pResult->getInt(++i));
		setBonus(pResult->getInt(++i));

		//setInMagics(pResult->getString(++i));
		setGold(pResult->getInt(++i));
		setGuildID(pResult->getInt(++i));

//		setZoneID(pResult->getInt(++i));
		ZoneID_t zoneID = pResult->getInt(++i);
		setX(pResult->getInt(++i));
		setY(pResult->getInt(++i));

		setSight (pResult->getInt(++i));

		setAlignment(pResult->getInt(++i));

//		for (int j = 0; j < 8; j++)
//			setHotKey(j, pResult->getInt(++i));

		setStashGold(pResult->getInt(++i));
		setStashNum(pResult->getBYTE(++i));
		
		m_Competence = pResult->getBYTE(++i);

		if (m_Competence >= 4 )
			m_Competence = 3;

		m_CompetenceShape = pResult->getBYTE(++i);

		setResurrectZoneID(pResult->getInt(++i));

		m_SilverDamage = pResult->getInt(++i);

		reward = pResult->getInt(++i);
		setSMSCharge(pResult->getInt(++i));

		Rank_t CurRank               = pResult->getInt(++i);
		RankExp_t RankGoalExp        = pResult->getInt(++i);

		m_pRank = new Rank(CurRank, RankGoalExp, RankExpTable::s_RankExpTables[RANK_TYPE_VAMPIRE]);

//		setRank(pResult->getInt(++i));
//		setRankExp(pResult->getInt(++i));
//		setRankGoalExp(pResult->getInt(++i));

		// maxHP를 다시 계산해서 설정해준다.
		// 2002.7.15 by sigi
		// 공식 바뀌면 AbilityBalance.cpp의 computeHP도 수정해야한다.
		int maxHP = m_STR[ATTR_CURRENT]*2 + m_INT[ATTR_CURRENT] + m_DEX[ATTR_CURRENT] + m_Level;
		maxHP = min((int)maxHP, VAMPIRE_MAX_HP);
		setHP(maxHP, ATTR_MAX);

		try
		{
			setZoneID(zoneID);
		}
		catch (Error& e )
		{
			// 길드 아지트 문제로 본다.
			// 길드 아지트가 한 게임 서버에만 존재하므로 다른 게임서버로 접속할 때 그 아지트로 들어가지 못한다.
			// 길드 아지트 입구로 옮긴다.
			ZONE_COORD ResurrectCoord;
			g_pResurrectLocationManager->getVampirePosition(1003, ResurrectCoord);
			setZoneID(ResurrectCoord.id);
			setX(ResurrectCoord.x);
			setY(ResurrectCoord.y);
		}

		SAFE_DELETE(pStmt);
	} 
	END_DB(pStmt)

/*	if (reward != 0)
	{
		BEGIN_DB
		{
			StringStream SQL;
			SQL << "UPDATE Vampire SET Reward = 0 WHERE Name = '" << m_Name << "'";

			pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
			pStmt->executeQuery(SQL.toString());
			SAFE_DELETE(pStmt);
		}
		END_DB(pStmt)

		int amount = 0;

		if (m_Level>=1 && m_Level<=10)
			amount = m_Level*2000;
		else if (m_Level>10 && m_Level<=30)
			amount = m_Level*3000;
		else if (m_Level>30)
			amount = m_Level*10000;

		int offset = 1000;

		for (int i=0; i<amount; i += offset)
		{
			increaseVampExp(offset);
		}

		reward = 0;
	}*/

	//----------------------------------------------------------------------
	// Vampire Outlook Information 을 구성한다.
	//----------------------------------------------------------------------
	// 뱀파이어는 로딩할때 ObjectID를 세팅 하도록 한다. 근데 접속 할땐? -_-
	m_VampireInfo.setObjectID(m_ObjectID);
	m_VampireInfo.setName(m_Name);
	m_VampireInfo.setSex(m_Sex);
	m_VampireInfo.setBatColor(m_BatColor);
	m_VampireInfo.setSkinColor(m_SkinColor);
	m_VampireInfo.setMasterEffectColor(m_MasterEffectColor);

	m_VampireInfo.setCompetence(m_CompetenceShape);

    //----------------------------------------------------------------------
	// 스킬을 로딩한다.
	//----------------------------------------------------------------------
	BEGIN_DB
	{
		pStmt   = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
		pResult = pStmt->executeQuery("SELECT SkillType, Delay, CastingTime, NextTime FROM VampireSkillSave WHERE OwnerID = '%s'", m_Name.c_str());
	
		while(pResult->next()) 
		{
			int         i          = 0;
			SkillType_t SkillType = pResult->getInt(++i);

			if (hasSkill(SkillType) == NULL) 
			{
				VampireSkillSlot* pVampireSkillSlot = new VampireSkillSlot();
		
				pVampireSkillSlot->setName(m_Name);
				pVampireSkillSlot->setSkillType(SkillType);
				pVampireSkillSlot->setInterval (pResult->getInt(++i));
				pVampireSkillSlot->setCastingTime (pResult->getInt(++i));
				//pVampireSkillSlot->setRunTime (pResult->getInt(++i));
				pVampireSkillSlot->setRunTime();
		
				addSkill(pVampireSkillSlot);
			}
		}
	
		SAFE_DELETE(pStmt);
	} 
	END_DB(pStmt)

    //----------------------------------------------------------------------
	// Rank Bonus 를  로딩한다.
	//----------------------------------------------------------------------
	loadRankBonus();

    //----------------------------------------------------------------------
	// 이펙트를 로딩한다.
	//----------------------------------------------------------------------
	g_pEffectLoaderManager->load(this);

	//----------------------------------------------------------------------
	// GrandMaster인 경우는 Effect를 붙여준다.
	//----------------------------------------------------------------------
	// by sigi. 2002.11.8
	if (m_Level>=100
		&& SystemAvailabilitiesManager::getInstance()->isAvailable(SystemAvailabilitiesManager::SYSTEM_GRAND_MASTER_EFFECT ) )
	{
		if (!isFlag(Effect::EFFECT_CLASS_GRAND_MASTER_VAMPIRE))
		{
			EffectGrandMasterVampire* pEffect = new EffectGrandMasterVampire(this);
			pEffect->setDeadline(999999);
			getEffectManager()->addEffect(pEffect);
			setFlag(Effect::EFFECT_CLASS_GRAND_MASTER_VAMPIRE);
		}
	}

	//----------------------------------------------------------------------
	// 플래그 셋을 로드한다.
	//----------------------------------------------------------------------
	m_pFlagSet->load(getName());

	//----------------------------------------------------------------------
	// Vampire Outlook Information 을 초기화한다.
	//----------------------------------------------------------------------
	/*
	ItemType_t coatType = 0;
	Item* pItem = m_pWearItem[WEAR_BODY];
	if (pItem!=NULL)
	{
		coatType = pItem->getItemType();
	}
	*/

	m_VampireInfo.setCoatType(0);
	m_VampireInfo.setCoatColor(JACKET_BASIC);
	m_VampireInfo.setCoatColor(377);
	m_VampireInfo.setAdvancementLevel(getAdvancementClassLevel());
	//m_VampireInfo.setCoatColor(2 , SUB_COLOR);

	// 잘못된 경험치를 재조정 해준다.
/*	VampEXPInfo* pVampEXPInfo = g_pVampEXPInfoManager->getVampEXPInfo(m_Level);

	if ((pVampEXPInfo->getAccumExp() != m_Exp + m_GoalExp) 
		&& m_Level > 1 && m_Level < VAMPIRE_MAX_LEVEL) 
	{
		//ofstream file("뱀프능력치조정.txt", ios::out | ios::app);
		//file << "NAME:" << m_Name << endl;
		//file << "==VampEXP==" << endl;
		//file << "현재레벨의총경험치 : " << (int)pVampEXPInfo->getAccumExp() << endl;
		//file << "현재 누적 경험치 : " << (int)m_Exp << endl;
		//file << "현재 목표 경험치 : " << (int)m_GoalExp << endl;

		// 이전 레벨의 인포를 받아온다.
		VampEXPInfo* pBeforeVampEXPInfo = g_pVampEXPInfoManager->getVampEXPInfo(m_Level - 1);
		// 이전 레벨의 총 경험치 + 목표 경험치 변화량 = 현재 누적 경험치
		m_Exp = pBeforeVampEXPInfo->getAccumExp() + (pVampEXPInfo->getGoalExp() - m_GoalExp);

		//file << "수정된 누적 경험치 : " << (int)m_Exp << endl;
		//file << "수정된 목표 경험치 : " << (int)m_GoalExp << endl;
		//file << endl;
		//file.close();

	   // by sigi. 2002.5.15
//		StringStream attrsave;
//		attrsave << "Exp = " << (int)m_Exp;
//		tinysave(attrsave.toString());

		char pField[80];
		sprintf(pField, "Exp=%lu", m_Exp);
		tinysave(pField);
	}*/

	// rank가 0이면 초기값이 설정되지 않았다는 의미이다. by sigi. 2002.9.13
	if (getRank()==0)
	{
		saveInitialRank();
	}


	// 잘못된 계급을 재조정 해준다.
/*	RankEXPInfo* pRankEXPInfo = g_pRankEXPInfoManager[RANK_TYPE_VAMPIRE]->getRankEXPInfo(m_Rank);

	if ((pRankEXPInfo->getAccumExp() != m_RankExp + m_RankGoalExp) 
		&& m_Rank > 1 && m_Rank < VAMPIRE_MAX_RANK) 
	{
		//ofstream file("뱀프능력치조정.txt", ios::out | ios::app);
		//file << "NAME:" << m_Name << endl;
		//file << "==VampEXP==" << endl;
		//file << "현재레벨의총경험치 : " << (int)pVampEXPInfo->getAccumExp() << endl;
		//file << "현재 누적 경험치 : " << (int)m_Exp << endl;
		//file << "현재 목표 경험치 : " << (int)m_GoalExp << endl;

		// 이전 레벨의 인포를 받아온다.
		RankEXPInfo* pBeforeRankEXPInfo = g_pRankEXPInfoManager[RANK_TYPE_VAMPIRE]->getRankEXPInfo(m_Rank - 1);
		// 이전 레벨의 총 경험치 + 목표 경험치 변화량 = 현재 누적 경험치
		m_RankExp = pBeforeRankEXPInfo->getAccumExp() + (pRankEXPInfo->getGoalExp() - m_RankGoalExp);

		//file << "수정된 누적 경험치 : " << (int)m_Exp << endl;
		//file << "수정된 목표 경험치 : " << (int)m_GoalExp << endl;
		//file << endl;
		//file.close();

		char pField[80];
		sprintf(pField, "RankExp=%lu", m_RankExp);
		tinysave(pField);
	}
*/

	initAllStat();

	// 전쟁 참가 Flag 체크
	if (RaceWarLimiter::isInPCList(this ) )
	{
		setFlag(Effect::EFFECT_CLASS_RACE_WAR_JOIN_TICKET);
	}

	if (m_pZone->isHolyLand()
        && g_pWarSystem->hasActiveRaceWar()
        && !isFlag(Effect::EFFECT_CLASS_RACE_WAR_JOIN_TICKET ))
    {
        ZONE_COORD ResurrectCoord;
        g_pResurrectLocationManager->getPosition(this, ResurrectCoord);
        setZoneID(ResurrectCoord.id);
        setX(ResurrectCoord.x);
        setY(ResurrectCoord.y);
    }



	return true;

	__END_CATCH
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
void Vampire::save () const
	throw(Error)
{
	__BEGIN_TRY

	__ENTER_CRITICAL_SECTION(m_Mutex)

	Statement* pStmt;

	//--------------------------------------------------------------------------------
	// 뱀파이어 정보를 저장한다.
	//--------------------------------------------------------------------------------
	BEGIN_DB
	{
		StringStream sql;
		sql << "UPDATE Vampire SET"
			//<< " BatColor = " << (int)m_BatColor
			//<< ", SkinColor = " << (int)m_SkinColor
			//<< ", STR = " << (int)m_STR[ATTR_MAX]
			//<< ", DEX = " << (int)m_DEX[ATTR_MAX]
			//<< ", INTE = " << (int)m_INT[ATTR_MAX]
			<< " CurrentHP = " << (int)m_HP[ATTR_CURRENT]
			<< ", HP = " << (int)m_HP[ATTR_MAX]
			<< ", SilverDamage = " << (int)m_SilverDamage
			//<< ", Fame = " << (int)m_Fame
			//<< ", Exp = " << (int)m_Exp
			//<< ", ExpOffset = " << (int)m_ExpOffset
			//<< ", Rank = " << (int)m_Rank
			//<< ", RankExp = " << (int)m_RankExp
			//<< ", Level = " << (int)m_Level
			//<< ", Bonus = " << (int)m_Bonus
			//<< ", Gold = " << (int)m_Gold
			<< ", ZoneID = " << (int)getZoneID()
			<< ", XCoord = " << (int)m_X
			<< ", YCoord = " << (int)m_Y
			//<< ", Sight = " << (int)m_Sight
//			<< ", F5 = " << (int)m_HotKey[0]
//			<< ", F6 = " << (int)m_HotKey[1]
//			<< ", F7 = " << (int)m_HotKey[2]
//			<< ", F8 = " << (int)m_HotKey[3]
//			<< ", F9 = " << (int)m_HotKey[4]
//			<< ", F10 = " << (int)m_HotKey[5]
//			<< ", F11 = " << (int)m_HotKey[6]
//			<< ", F12 = " << (int)m_HotKey[7]
			<< " WHERE Name = '" << m_Name << "'";
			//<< ", InMagics = '" << ??? << "'"
		
		pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();

		pStmt->executeQuery(sql.toString());

		//Assert(pStmt->getAffectedRowCount() != 1);

		SAFE_DELETE(pStmt);
	} 
	END_DB(pStmt)
	
	/*
	//----------------------------------------------------------------------
	// 아이템을 세이브한다.
	//----------------------------------------------------------------------
	//--------------------------------------------------
	// 인벤토리의 아이템들을 세이브 한다.
	//--------------------------------------------------
	m_pInventory->save(m_Name);
	*/


	//--------------------------------------------------
	// 이펙트를 세이브 한다.
	//--------------------------------------------------
	m_pEffectManager->save(m_Name);

	__LEAVE_CRITICAL_SECTION(m_Mutex)

	__END_CATCH
}

//----------------------------------------------------------------------
// tinysave
//----------------------------------------------------------------------
void Vampire::tinysave(const string & field)	// by sigi. 2002.5.15
	    const throw(Error)
{
    __BEGIN_TRY

    Statement* pStmt = NULL;

	BEGIN_DB
	{
		pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
		pStmt->executeQuery("UPDATE Vampire SET %s WHERE Name='%s'", field.c_str(), m_Name.c_str());
		SAFE_DELETE(pStmt);
	} 
	END_DB(pStmt)

    __END_CATCH
}

////////////////////////////////////////////////////////////////////////////////
//
//
// 스킬 관련 함수
//
//
////////////////////////////////////////////////////////////////////////////////

// 특정 Skill을 리턴한다.
VampireSkillSlot* Vampire::getSkill (SkillType_t SkillType) const 
	throw()
{
	__BEGIN_TRY

	map<SkillType_t, VampireSkillSlot*>::const_iterator itr = m_SkillSlot.find(SkillType);
	if (itr != m_SkillSlot.end())
	{
		return itr->second;
	}

	return NULL;

	__END_CATCH
}

// 특정 Skill을 add 한다
void Vampire::addSkill(SkillType_t SkillType)
	throw()
{
	__BEGIN_TRY

	switch (SkillType)
	{
		case SKILL_UN_BURROW:
		case SKILL_UN_TRANSFORM:
		case SKILL_UN_INVISIBILITY:
		case SKILL_THROW_HOLY_WATER:
		case SKILL_EAT_CORPSE:
		//case SKILL_HOWL:
			filelog("VampireError.log", "SkillType[%d], %s", SkillType, toString().c_str());
			Assert(false);
			break;
		default:
			break;
	}

	map<SkillType_t, VampireSkillSlot*>::iterator itr = m_SkillSlot.find(SkillType);

	if (itr == m_SkillSlot.end())
	{
		SkillInfo* pSkillInfo = g_pSkillInfoManager->getSkillInfo(SkillType);
		Turn_t Delay = pSkillInfo->getMaxDelay();

		VampireSkillSlot* pVampireSkillSlot = new VampireSkillSlot;

		pVampireSkillSlot->setName(m_Name);
		pVampireSkillSlot->setSkillType(SkillType);
		pVampireSkillSlot->setInterval(Delay);
		pVampireSkillSlot->setRunTime();
		pVampireSkillSlot->create(m_Name);

		m_SkillSlot[SkillType] = pVampireSkillSlot;
	}

	__END_CATCH
}

// 특정 SkillSlot을 자동으로 빈 슬랏을 찾아 넣는다.
void Vampire::addSkill(VampireSkillSlot* pVampireSkillSlot)
	throw()
{
	__BEGIN_TRY

	SkillType_t SkillType = pVampireSkillSlot->getSkillType();
	switch (SkillType)
	{
		case SKILL_UN_BURROW:
		case SKILL_UN_TRANSFORM:
		case SKILL_UN_INVISIBILITY:
		case SKILL_THROW_HOLY_WATER:
		case SKILL_EAT_CORPSE:
//		case SKILL_HOWL:
			filelog("VampireError.log", "SkillType[%d], %s", SkillType, toString().c_str());
			Assert(false);
			break;
		default:
			break;
	}

	map<SkillType_t, VampireSkillSlot*>::iterator itr = m_SkillSlot.find(pVampireSkillSlot->getSkillType());
	
	if (itr == m_SkillSlot.end())
	{
		m_SkillSlot[pVampireSkillSlot->getSkillType()] = pVampireSkillSlot;
	}
	// 2002.1.16 by sigi
	else
	{
		delete pVampireSkillSlot;
	}

	__END_CATCH
}

// 성지스킬을 지워주는 함수다.
void Vampire::removeCastleSkill(SkillType_t SkillType)
	throw()
{
	__BEGIN_TRY

	// 성지 스킬만 지울 수 있다.
	if (g_pCastleSkillInfoManager->getZoneID(SkillType ) == 0 ) return;

	map<SkillType_t, VampireSkillSlot*>::iterator itr = m_SkillSlot.find(SkillType);
	
	if (itr != m_SkillSlot.end())
	{
		VampireCastleSkillSlot* pCastleSkillSlot = dynamic_cast<VampireCastleSkillSlot*>(itr->second);

		SAFE_DELETE(pCastleSkillSlot);

		m_SkillSlot.erase(itr);
	}

	__END_CATCH
}

// 갖고 있는 모든 성지스킬을 지워주는 함수이다.
void Vampire::removeAllCastleSkill()
	throw()
{
	__BEGIN_TRY

	map<SkillType_t, VampireSkillSlot*>::iterator itr = m_SkillSlot.begin();

	while (itr != m_SkillSlot.end() )
	{
		if (itr->second != NULL )
		{
			VampireSkillSlot* pSkillSlot = itr->second;
			if (g_pCastleSkillInfoManager->getZoneID(pSkillSlot->getSkillType() ) == 0 )
			{
				// 성지스킬이 아니면 다음껄로 넘어간다.
				++itr;
				continue;
			}

			// 성지스킬이면 지워준다. 반복자 사용에 주의
			SAFE_DELETE(pSkillSlot);
			map<SkillType_t, VampireSkillSlot*>::iterator prevItr = itr;
			
			++itr;
			m_SkillSlot.erase(prevItr);
		}
		else
		{
			// 이건 멀까.... Assert 해야 되지 않나 -_-;
			Assert(false);
		}
	}

	__END_CATCH
}


////////////////////////////////////////////////////////////////////////////////
//
//
// 아이템 착/탈 관련 함수
//
//
////////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------
//
// Vampire::WearItem()
//
// Item을 장착창에 장착시키고 능력치를 계산한다.
//
//----------------------------------------------------------------------
void Vampire::wearItem(WearPart Part, Item* pItem)
	throw()
{
	__BEGIN_TRY

	Assert(pItem != NULL);

	Item* pPrevItem = NULL;
	Item* pLeft = NULL;
	Item* pRight = NULL;

	// 현재 기획에서는...능력치가 모자라더라도 아이템을 무조건 사용할 수는
	// 있다. 하지만 아이템에 의한 능력치가 적용이 되지 않는다. 
	// 그러므로 일단 아이템을 해당하는 장착창에다 집어넣는다.

	// vampire 무기 추가. 2002.8.16. by sigi
	// 양손 무기일 경우에는 양손 장착창에다 하나의 아이템 포인터를 할당...
	if (isTwohandWeapon(pItem))
	{
		// 양손에 아이템을 들고 있을 경우
		if (isWear(WEAR_RIGHTHAND) && isWear(WEAR_LEFTHAND))
		{
			pLeft  = getWearItem(WEAR_RIGHTHAND);
			pRight = getWearItem(WEAR_LEFTHAND);
			
			// 양손 무기를 들고 있을 경우
			if (pLeft == pRight)
			{
				// 요구한 아이템을 장착 포인트에 넣고,
				m_pWearItem[WEAR_RIGHTHAND] = pItem;
				m_pWearItem[WEAR_LEFTHAND]  = pItem;

				// by sigi. 2002.5.15
				char pField[80];
				//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
				sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
				pItem->tinysave(pField);

				// 원래 있던 아이템을 마우스 포인터에 달아 준다.
				addItemToExtraInventorySlot(pLeft);
				//pLeft->save(m_Name, STORAGE_EXTRASLOT, 0, 0, 0);
				sprintf(pField, "Storage=%d", STORAGE_EXTRASLOT);
				pLeft->tinysave(pField);
			}
			// 냥냥
			else
			{
				// 양손에 검과 방패를 들고 있었는데...양손 무기를 들려고 하면,
				// 검은 마우스 포인터에 달아줄 수 있지만, 방패는 어떻게 할 수가 없다.
				// 인벤토리에 넣어줘야 할 텐데, 지금 당장은 어떻게 할 지를 모르겠네...
				// 걍 입을 수 없다는 패킷을 보내주자...
				//cerr << "양손에 칼과 방패를 들고 있어서, 양손 무기를 장착할 수 없습니다." << endl;
				return;
			}
		}
		// 양손에 아이템을 들고 있지 않을 경우
		else 
		{
			char pField[80];

			// 오른쪽에 아이템을 들고 있을 경우
			if (isWear(WEAR_RIGHTHAND))
			{
				pRight = getWearItem(WEAR_RIGHTHAND);
				// 요구한 아이템을 장착 포인트에 넣는다.
				m_pWearItem[WEAR_RIGHTHAND] = pItem;
				m_pWearItem[WEAR_LEFTHAND]  = pItem;

				// by sigi. 2002.5.15
				//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
				sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
				pItem->tinysave(pField);

				// 원래 있던 아이템을 마우스 포인터에 달아 준다.
				addItemToExtraInventorySlot(pRight);
				//pRight->save(m_Name, STORAGE_EXTRASLOT, 0, 0, 0);
				sprintf(pField, "Storage=%d", STORAGE_EXTRASLOT);
				pRight->tinysave(pField);
			}
			// 왼쪽에 아이템을 들고 있을 경우
			else if (isWear(WEAR_LEFTHAND))
			{
				pLeft = getWearItem(WEAR_LEFTHAND);
				// 요구한 아이템을 장착 포인트에 넣는다.
				m_pWearItem[WEAR_RIGHTHAND] = pItem;
				m_pWearItem[WEAR_LEFTHAND]  = pItem;

				// by sigi. 2002.5.15
				//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
				sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
				pItem->tinysave(pField);

				// 원래 있던 아이템을 마우스 포인터에 달아 준다.
				addItemToExtraInventorySlot(pLeft);
				//pLeft->save(m_Name, STORAGE_EXTRASLOT, 0, 0, 0);
				sprintf(pField, "Storage=%d", STORAGE_EXTRASLOT);
				pLeft->tinysave(pField);
			}
			// 아무쪽도 아이템을 들고 있지 않을 경우
			else
			{
				// 요구한 아이템을 장착 포인트에 넣는다.
				m_pWearItem[WEAR_RIGHTHAND] = pItem;
				m_pWearItem[WEAR_LEFTHAND]  = pItem;

				// by sigi. 2002.5.15
				//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
				sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
				pItem->tinysave(pField);
			}
		}
	}
	else 
	{
		if (isWear(Part))
		{
			pPrevItem = getWearItem(Part);
			m_pWearItem[Part] = pItem;

			// by sigi. 2002.5.15
			char pField[80];
			//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
			sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
			pItem->tinysave(pField);

			addItemToExtraInventorySlot(pPrevItem);
			//pPrevItem->save(m_Name, STORAGE_EXTRASLOT, 0, 0, 0);
			sprintf(pField, "Storage=%d", STORAGE_EXTRASLOT);
			pPrevItem->tinysave(pField);
		}
		else
		{
			// 요구한 아이템을 장착 포인트에 넣는다.
			m_pWearItem[Part] = pItem;

			// by sigi. 2002.5.15
			char pField[80];
			//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
			sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
			pItem->tinysave(pField);
		}
	}

	// 옷이라면 옷에 따른 색깔을 정해준다.
	// 나중에라도 옷 타입이 여러 가지가 될 수 있으리라 생각하는데,
	// 현재로서는 옷 타입이 하나이므로, 색깔만 세팅해준다.
	if (pItem->getItemClass() == Item::ITEM_CLASS_VAMPIRE_COAT) 
	{
		m_VampireInfo.setCoatColor(getItemShapeColor(pItem ));

		// item type을 설정해준다. 
		m_VampireInfo.setCoatType(pItem->getItemType());
	}

	__END_CATCH
}


//----------------------------------------------------------------------
// Vampire::WearItem()
// Item을 장착창에 장착시키고 능력치를 계산한다.
//----------------------------------------------------------------------
void Vampire::wearItem(WearPart Part)
	throw()
{
	__BEGIN_TRY

	// 장착 준비중인 아이템을 받아온다.
	Item* pItem = getExtraInventorySlotItem();
	Assert(pItem != NULL);

	Item* pPrevItem = NULL;
	Item* pLeft = NULL;
	Item* pRight = NULL;

	// 먼저 옷을 입히거나, 벗기기 전에 현재의 능력치를 버퍼에다 저장해 둔다.
	// 이는 나중에 변한 능력치만을 전송하기 위한 것이다.
	VAMPIRE_RECORD prev;
	getVampireRecord(prev);

	// 현재 기획에서는...능력치가 모자라더라도 아이템을 무조건 사용할 수는
	// 있다. 하지만 아이템에 의한 능력치가 적용이 되지 않는다. 
	// 그러므로 일단 아이템을 해당하는 장착창에다 집어넣는다.
	char pField[80];

	// vampire 무기 추가. 2002.8.16. by sigi
	// 양손 무기일 경우에는 양손 장착창에다 하나의 아이템 포인터를 할당...
	if (isTwohandWeapon(pItem))
	{
		// 양손에 아이템을 들고 있을 경우
		if (isWear(WEAR_RIGHTHAND) && isWear(WEAR_LEFTHAND))
		{
			pLeft  = getWearItem(WEAR_RIGHTHAND);
			pRight = getWearItem(WEAR_LEFTHAND);
			
			// 양손 무기를 들고 있을 경우
			if (pLeft == pRight)
			{
				takeOffItem(WEAR_LEFTHAND, false, false);

				// 요구한 아이템을 장착 포인트에 넣고,
				m_pWearItem[WEAR_RIGHTHAND] = pItem;
				m_pWearItem[WEAR_LEFTHAND]  = pItem;
				// by sigi. 2002.5.15
				//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
				sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
				pItem->tinysave(pField);

				// 요구한 아이템을 마우스 포인터에서 제거한다.
				deleteItemFromExtraInventorySlot();
				// 원래 있던 아이템을 마우스 포인터에 달아 준다.
				addItemToExtraInventorySlot(pLeft);
				//pLeft->save(m_Name, STORAGE_EXTRASLOT, 0, 0, 0);
				sprintf(pField, "Storage=%d", STORAGE_EXTRASLOT);
				pLeft->tinysave(pField);

			}
			// 검과 방패를 들고 있을 경우
			else
			{
				// 양손에 검과 방패를 들고 있었는데...양손 무기를 들려고 하면,
				// 검은 마우스 포인터에 달아줄 수 있지만, 방패는 어떻게 할 수가 없다.
				// 인벤토리에 넣어줘야 할 텐데, 지금 당장은 어떻게 할 지를 모르겠네...
				// 걍 입을 수 없다는 패킷을 보내주자...
				return;
			}
		}
		// 양손에 아이템을 들고 있지 않을 경우
		else 
		{
			// by sigi. 2002.5.15
			// 오른쪽에 아이템을 들고 있을 경우
			if (isWear(WEAR_RIGHTHAND))
			{
				pRight = getWearItem(WEAR_RIGHTHAND);

				takeOffItem(WEAR_RIGHTHAND, false, false);

				// 요구한 아이템을 장착 포인트에 넣는다.
				m_pWearItem[WEAR_RIGHTHAND] = pItem;
				m_pWearItem[WEAR_LEFTHAND]  = pItem;
				//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);

				// by sigi. 2002.5.15
				sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
				pItem->tinysave(pField);

				// 요구한 아이템을 마우스 포인터에서 제거한다.
				deleteItemFromExtraInventorySlot();
				// 원래 있던 아이템을 마우스 포인터에 달아 준다.
				addItemToExtraInventorySlot(pRight);
				//pRight->save(m_Name, STORAGE_EXTRASLOT, 0, 0, 0);
				sprintf(pField, "Storage=%d", STORAGE_EXTRASLOT);
				pRight->tinysave(pField);
				
			}
			// 왼쪽에 아이템을 들고 있을 경우
			else if (isWear(WEAR_LEFTHAND))
			{
				pLeft = getWearItem(WEAR_LEFTHAND);
				
				takeOffItem(WEAR_LEFTHAND, false, false);

				// 요구한 아이템을 장착 포인트에 넣는다.
				m_pWearItem[WEAR_RIGHTHAND] = pItem;
				m_pWearItem[WEAR_LEFTHAND]  = pItem;
				
				// by sigi. 2002.5.15
				//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
				sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
				pItem->tinysave(pField);

				// 요구한 아이템을 마우스 포인터에서 제거한다.
				deleteItemFromExtraInventorySlot();
				// 원래 있던 아이템을 마우스 포인터에 달아 준다.
				addItemToExtraInventorySlot(pLeft);
				//pLeft->save(m_Name, STORAGE_EXTRASLOT, 0, 0, 0);
				sprintf(pField, "Storage=%d", STORAGE_EXTRASLOT);
				pLeft->tinysave(pField);
			}
			// 아무쪽도 아이템을 들고 있지 않을 경우
			else
			{
				// 요구한 아이템을 장착 포인트에 넣는다.
				m_pWearItem[WEAR_RIGHTHAND] = pItem;
				m_pWearItem[WEAR_LEFTHAND]  = pItem;

				pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
				// 요구한 아이템을 마우스 포인터에서 제거한다.
				deleteItemFromExtraInventorySlot();
			}
		}
	}
	else
	{
		if (isWear(Part))
		{
			pPrevItem = getWearItem(Part);
			takeOffItem(Part, false, false);
			m_pWearItem[Part] = pItem;

			// by sigi. 2002.5.15
			//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
			sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
			pItem->tinysave(pField);

			deleteItemFromExtraInventorySlot();
			addItemToExtraInventorySlot(pPrevItem);

			//pPrevItem->save(m_Name, STORAGE_EXTRASLOT, 0, 0, 0);
			sprintf(pField, "Storage=%d", STORAGE_EXTRASLOT);
			pPrevItem->tinysave(pField);
		}
		else
		{
			m_pWearItem[Part] = pItem;
			deleteItemFromExtraInventorySlot();

			// by sigi. 2002.5.15
			//pItem->save(m_Name, STORAGE_GEAR, 0, Part, 0);
			sprintf(pField, "Storage=%d, X=%d", STORAGE_GEAR, Part);
			pItem->tinysave(pField);
		}
	}

	initAllStat();
	sendRealWearingInfo();
	sendModifyInfo(prev);

	// 옷이라면 옷에 따른 색깔을 정해준다.
	// 나중에라도 옷 타입이 여러 가지가 될 수 있으리라 생각하는데,
	// 현재로서는 옷 타입이 하나이므로, 색깔만 세팅해준다.

	// 실제 적용되는 아이템만 복장을 바꾼다. by sigi. 2002.10.30
	if (m_pRealWearingCheck[Part])
	{
		if (pItem->getItemClass() == Item::ITEM_CLASS_VAMPIRE_COAT) 
		{
			Color_t color = getItemShapeColor(pItem);
			m_VampireInfo.setCoatColor(color);
			m_VampireInfo.setCoatType(pItem->getItemType());

			// 옷을 갈아입었으니, 주위에다가 옷 갈아입었다고 정보를 날린다.
			GCChangeShape pkt;
			pkt.setObjectID(getObjectID());
			pkt.setItemClass(Item::ITEM_CLASS_VAMPIRE_COAT);
			pkt.setItemType(pItem->getItemType());
			pkt.setOptionType(pItem->getFirstOptionType());
			pkt.setAttackSpeed(m_AttackSpeed[ATTR_CURRENT]);

			if (color == QUEST_COLOR )
				pkt.setFlag(SHAPE_FLAG_QUEST);

			Zone* pZone = getZone();
			pZone->broadcastPacket(m_X, m_Y , &pkt, this);
		}
	}

	if (m_pZone != NULL)
	{
		GCOtherModifyInfo gcOtherModifyInfo;
		makeGCOtherModifyInfo(&gcOtherModifyInfo, this, &prev);

		if (gcOtherModifyInfo.getShortCount() != 0 || gcOtherModifyInfo.getLongCount() != 0)
		{
			m_pZone->broadcastPacket(m_X, m_Y, &gcOtherModifyInfo, this);
		}
	}
	
	__END_CATCH
}


//----------------------------------------------------------------------
//
// Vampire::takeOffItem()
//
//----------------------------------------------------------------------
void Vampire::takeOffItem(WearPart Part, bool bAddOnMouse, bool bSendModifyInfo)
	throw()
{
	__BEGIN_TRY

	VAMPIRE_RECORD prev;

	// 장착창에 있는 아이템을 받아온다.
	Item* pItem = m_pWearItem[Part];
	Assert(pItem != NULL);

	//m_pWearItem[Part] = NULL;

	// vampire 무기 추가. 2002.8.16. by sigi
	// 장착창에 있는 아이템을 받아온다.
	//Item::ItemClass IClass = pItem->getItemClass();

	if (Part == WEAR_LEFTHAND || Part == WEAR_RIGHTHAND)
	{
		if (m_pWearItem[WEAR_RIGHTHAND] && m_pWearItem[WEAR_LEFTHAND])
		{
			if (m_pWearItem[WEAR_RIGHTHAND] == m_pWearItem[WEAR_LEFTHAND])
			{
				m_pWearItem[WEAR_RIGHTHAND] = NULL;
				m_pWearItem[WEAR_LEFTHAND] = NULL;
			}
		}
	}

	// 아이템을 장착포인트에서 제거한다.
	if (isTwohandWeapon(pItem))
	{
		m_pWearItem[WEAR_RIGHTHAND] = NULL;
		m_pWearItem[WEAR_LEFTHAND] = NULL;
	}
	else m_pWearItem[Part] = NULL;

	// wearItem에서 지정된 슬랏에 옷을 이미 입고 있는 경우에, 그것을 벗기고
	// 다시 옷을 입히는데, 그러면 벗길 때 패킷을 한번, 입었을 때 다시 패킷을
	// 한번, 총 두 번의 패킷을 보내게 된다. 그것을 방지하기 위해서
	// bool 변수를 하나 집어넣었다. -- 2002.01.24 김성민
	if (bSendModifyInfo)
	{
		getVampireRecord(prev);
		initAllStat();
		sendRealWearingInfo();
		sendModifyInfo(prev);
	}
	else
	{
		initAllStat();
	}

	//---------------------------------------------
	// 있어선 안될 체크 -_-; 임시 땜빵
	// 아이템을 마우스 커서에다 달아준당.
	//---------------------------------------------
	if (bAddOnMouse) 
	{
		addItemToExtraInventorySlot(pItem);
		//pItem->save(m_Name, STORAGE_EXTRASLOT, 0, 0, 0);
		// item저장 최적화. by sigi. 2002.5.13
		char pField[80];
        sprintf(pField, "Storage=%d, Durability=%d", STORAGE_EXTRASLOT, pItem->getDurability());
        pItem->tinysave(pField);
	}

	if (pItem->getItemClass() == Item::ITEM_CLASS_VAMPIRE_COAT)
	{
		m_VampireInfo.setCoatColor(377);
		m_VampireInfo.setCoatType(0);

		GCTakeOff pkt;
		pkt.setObjectID(getObjectID());
		pkt.setSlotID((SlotID_t)ADDON_COAT);
		m_pZone->broadcastPacket(getX(), getY(), &pkt, this);
	}

	if (m_pZone != NULL)
	{
		GCOtherModifyInfo gcOtherModifyInfo;
		makeGCOtherModifyInfo(&gcOtherModifyInfo, this, &prev);

		if (gcOtherModifyInfo.getShortCount() != 0 || gcOtherModifyInfo.getLongCount() != 0)
		{
			m_pZone->broadcastPacket(m_X, m_Y, &gcOtherModifyInfo, this);
		}
	}

	__END_CATCH
}




//----------------------------------------------------------------------
// destroyGears
// 장착 아이템을 Delete 한다.
//----------------------------------------------------------------------
void Vampire::destroyGears() 
	throw()
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	for (int j = 0; j < VAMPIRE_WEAR_MAX; j++) 
	{
		Item* pItem = m_pWearItem[j];
		if (pItem != NULL)
		{
			Item::ItemClass IClass = pItem->getItemClass();

			//-------------------------------------------------------------
			// 슬레이어용 아이템을 입고 있는 이상한 자식이 있으면 다 어서트
			//-------------------------------------------------------------
			Assert(IClass != Item::ITEM_CLASS_AR);
			Assert(IClass != Item::ITEM_CLASS_SR);
			Assert(IClass != Item::ITEM_CLASS_SG);
			Assert(IClass != Item::ITEM_CLASS_SMG);
			Assert(IClass != Item::ITEM_CLASS_SWORD);
			Assert(IClass != Item::ITEM_CLASS_BLADE);
			Assert(IClass != Item::ITEM_CLASS_SHIELD);
			Assert(IClass != Item::ITEM_CLASS_CROSS);
			Assert(IClass != Item::ITEM_CLASS_MACE);
			Assert(IClass != Item::ITEM_CLASS_HELM);
			Assert(IClass != Item::ITEM_CLASS_GLOVE);
			Assert(IClass != Item::ITEM_CLASS_TROUSER);
			Assert(IClass != Item::ITEM_CLASS_COAT);

			// 양손 무기인지를 검사해서 아이템 하나를 지우면서
			// 양손을 비워준다.
			if (isTwohandWeapon(pItem))
			{
				m_pWearItem[WEAR_RIGHTHAND] = NULL;
				m_pWearItem[WEAR_LEFTHAND]  = NULL;
			}
			else m_pWearItem[j] = NULL;

			SAFE_DELETE(pItem);
		}
	}

	__END_DEBUG
	__END_CATCH
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
bool Vampire::isRealWearing(WearPart part) const
	throw()
{
	__BEGIN_TRY

	if (part >= VAMPIRE_WEAR_MAX) throw("Vampire::isRealWearing() : invalid wear point!");

	if (m_pWearItem[part] == NULL) return false;
	if (part >= WEAR_ZAP1 && part <= WEAR_ZAP4)
	{
		// 해당 위치에 반지도 있어야 된다.
		if (m_pWearItem[part-WEAR_ZAP1+WEAR_FINGER1]==NULL ) return false;
	}

	return isRealWearing(m_pWearItem[part]);

	__END_CATCH
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
bool Vampire::isRealWearing(Item* pItem) const
	throw()
{
	__BEGIN_TRY

	if (pItem == NULL) return false;
	if (pItem->getDurability() == 0 ) return false;

/*	if (m_pZone != NULL && m_pZone->isDynamicZone() && m_pZone->getDynamicZone()->getTemplateZoneID() == 4004 )
	{
		if (!isVampireWeapon(pItem->getItemClass() ) ) return false;
	}*/

	ItemInfo*       pItemInfo = g_pItemInfoManager->getItemInfo(pItem->getItemClass(), pItem->getItemType());

	Level_t			ReqAdvancedLevel = pItemInfo->getReqAdvancedLevel();
	if (ReqAdvancedLevel > 0 && (!isAdvanced() || getAdvancementClassLevel() < ReqAdvancedLevel ) ) return false;

/*	if (pItem->getItemClass() == Item::ITEM_CLASS_VAMPIRE_COAT || pItem->getItemClass() == Item::ITEM_CLASS_VAMPIRE_WEAPON )
	{
		if (ReqAdvancedLevel <= 0 && isAdvanced() ) return false;
	}
	*/

	if (pItem->isTimeLimitItem() )
	{
		Attr_t    ReqGender = pItemInfo->getReqGender();
		if ((m_Sex == MALE && ReqGender == GENDER_FEMALE ) ||
			 (m_Sex == FEMALE && ReqGender == GENDER_MALE ) ) return false;
		return true;
	}

	// 프리미엄 존에서는 유료사용자만 유니크/레어 아이템이 적용된다.
	// 커플링도 유료사용자만 쓸 수 있다. by Sequoia 2003. 3. 5.
	if (getZone()->isPremiumZone()
		&& (pItem->isUnique() || pItem->getOptionTypeSize()>1 ||
			pItem->getItemClass() == Item::ITEM_CLASS_COUPLE_RING || pItem->getItemClass() == Item::ITEM_CLASS_VAMPIRE_COUPLE_RING))
	{
		GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(getPlayer());
		if (!pGamePlayer->isPayPlaying() 
			&& !pGamePlayer->isPremiumPlay())
		{
			return false;
		}
	}

	if (isCoupleRing(pItem ) ) return true;

	Item::ItemClass IClass    = pItem->getItemClass();
	Level_t         ReqLevel  = pItemInfo->getReqLevel();
	Attr_t          ReqGender = pItemInfo->getReqGender();

	// 베이스 아이템의 요구치가 레벨 100을 넘을 경우 옵션을 포함해서 요구치가 150까지 올라갈 수 있다.
	// 그렇지 않을 경우 요구치가 옵션을 포함해도 100으로 제한된다.
	// 2003.3.21 by Sequoia
	Level_t			ReqLevelMax = ((ReqLevel > MAX_VAMPIRE_LEVEL_OLD ) ? MAX_VAMPIRE_LEVEL : MAX_VAMPIRE_LEVEL_OLD);

	// 아이템이 옵션을 가지고 있다면,
	// 옵션의 종류에 따라서 능력치 제한을 올려준다.
	const list<OptionType_t>& optionTypes = pItem->getOptionTypeList();
	list<OptionType_t>::const_iterator itr;

	for (itr=optionTypes.begin(); itr!=optionTypes.end(); itr++)
	{
		OptionInfo* pOptionInfo = g_pOptionInfoManager->getOptionInfo(*itr);
		ReqLevel += pOptionInfo->getReqLevel();
	}

	// 2003.1.6 by Sequoia, Bezz
	// 2003.3.21 요구치 제한 변경 by Sequoia
	ReqLevel = min(ReqLevel, ReqLevelMax);

	// 능력치 제한이 하나라도 있다면,
	// 그 능력을 만족시키는지 검사해야 한다.
	if (ReqLevel > 0 || ReqGender != GENDER_BOTH)
	{
		if (ReqLevel > 0 && m_Level < ReqLevel) return false;
		if (m_Sex == MALE && ReqGender == GENDER_FEMALE) return false;
		if (m_Sex == FEMALE && ReqGender == GENDER_MALE) return false;
	}

	return true;

	__END_CATCH
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
bool Vampire::isRealWearingEx(WearPart part) const
{
	if (part >= VAMPIRE_WEAR_MAX) return false;
	return m_pRealWearingCheck[part];
}

DWORD Vampire::sendRealWearingInfo(void) const
	throw()
{
	__BEGIN_TRY

	DWORD info = 0;
	DWORD flag = 1;

	for (int i=0; i<VAMPIRE_WEAR_MAX; i++)
	{
		if (isRealWearing((Vampire::WearPart)i)) info |= flag;
		flag <<= 1;
	}

	GCRealWearingInfo pkt;
	pkt.setInfo(info);
	m_pPlayer->sendPacket(&pkt);

	return info;

	__END_CATCH
}

////////////////////////////////////////////////////////////////////////////////
//
//
// 인포 관련 함수
//
//
////////////////////////////////////////////////////////////////////////////////

PCVampireInfo2* Vampire::getVampireInfo2 ()
	throw()
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	PCVampireInfo2* pInfo = new PCVampireInfo2();

	pInfo->setObjectID(m_ObjectID);
	pInfo->setName(m_Name);
	pInfo->setLevel(m_Level);
	pInfo->setSex(m_Sex);
	pInfo->setBatColor(m_BatColor);
	pInfo->setSkinColor(m_SkinColor);
	pInfo->setMasterEffectColor(m_MasterEffectColor);

    // 성향
	pInfo->setAlignment(m_Alignment);

	// 능력치
	pInfo->setSTR(m_STR[ATTR_CURRENT], ATTR_CURRENT);
	pInfo->setSTR(m_STR[ATTR_MAX], ATTR_MAX);
	pInfo->setSTR(m_STR[ATTR_BASIC], ATTR_BASIC);
	pInfo->setDEX(m_DEX[ATTR_CURRENT], ATTR_CURRENT);
	pInfo->setDEX(m_DEX[ATTR_MAX], ATTR_MAX);
	pInfo->setDEX(m_DEX[ATTR_BASIC], ATTR_BASIC);
	pInfo->setINT(m_INT[ATTR_CURRENT], ATTR_CURRENT);
	pInfo->setINT(m_INT[ATTR_MAX], ATTR_MAX);
	pInfo->setINT(m_INT[ATTR_BASIC], ATTR_BASIC);
	
	pInfo->setHP(m_HP[ATTR_CURRENT] , m_HP[ATTR_MAX]);
	pInfo->setFame(m_Fame);
	pInfo->setExp(m_GoalExp);
//	pInfo->setExp(m_Exp);
	pInfo->setGold(m_Gold);
	pInfo->setSight(m_Sight);
	pInfo->setBonus(m_Bonus);
	pInfo->setSilverDamage(m_SilverDamage);

	// by sigi. 2002.8.30
	pInfo->setRank(getRank());
	pInfo->setRankExp(getRankGoalExp());

//	for (int i = 0; i < 8; i++) 
//	{
//		pInfo->setHotKey(i, m_HotKey[i]);
//	}

	pInfo->setCompetence(m_CompetenceShape);
	pInfo->setGuildID(m_GuildID);
	pInfo->setGuildName(getGuildName());
	pInfo->setGuildMemberRank(getGuildMemberRank());

	GuildUnion* pUnion = GuildUnionManager::Instance().getGuildUnion(m_GuildID);
	if (pUnion == NULL ) pInfo->setUnionID(0);
	else pInfo->setUnionID(pUnion->getUnionID());

	pInfo->setAdvancementLevel(getAdvancementClassLevel());
	pInfo->setAdvancementGoalExp(getAdvancementClassGoalExp());

	return pInfo;

	__END_DEBUG
	__END_CATCH
}


//----------------------------------------------------------------------
// Vampire Outlook Information
//----------------------------------------------------------------------
PCVampireInfo3 Vampire::getVampireInfo3 () const 
	throw()
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	m_VampireInfo.setX(m_X);
	m_VampireInfo.setY(m_Y);
	m_VampireInfo.setDir(m_Dir);
	m_VampireInfo.setCurrentHP(m_HP[ATTR_CURRENT]);
	m_VampireInfo.setMaxHP(m_HP[ATTR_MAX]);
	m_VampireInfo.setAttackSpeed(m_AttackSpeed[ATTR_CURRENT]);
	m_VampireInfo.setAlignment(m_Alignment);
	m_VampireInfo.setGuildID(m_GuildID);

	// by sigi. 2002.9.10
	m_VampireInfo.setRank(getRank());

	/*
	Item* pItem = m_pWearItem[WEAR_BODY];
	if (pItem!=NULL)
	{
		m_VampireInfo.setCoatType(pItem->getItemType());
	}
	else
	{
		m_VampireInfo.setCoatType(0);
	}
	*/


	if (m_Flag.test(Effect::EFFECT_CLASS_TRANSFORM_TO_WOLF)) 
	{
		m_VampireInfo.setShape(SHAPE_WOLF);
	} 
	else if (m_Flag.test(Effect::EFFECT_CLASS_TRANSFORM_TO_BAT)) 
	{
		m_VampireInfo.setShape(SHAPE_BAT);
	}
	else if (m_Flag.test(Effect::EFFECT_CLASS_TRANSFORM_TO_WERWOLF))
	{
		m_VampireInfo.setShape(SHAPE_WERWOLF);
	}
	else 
	{
		m_VampireInfo.setShape(SHAPE_NORMAL);
	}

	// 엽색약용
	m_VampireInfo.setBatColor(m_BatColor);
	m_VampireInfo.setSkinColor(m_SkinColor);
	m_VampireInfo.setMasterEffectColor(m_MasterEffectColor);

	GuildUnion* pUnion = GuildUnionManager::Instance().getGuildUnion(m_GuildID);
	if (pUnion == NULL ) m_VampireInfo.setUnionID(0);
	else m_VampireInfo.setUnionID(pUnion->getUnionID());

	m_VampireInfo.setAdvancementLevel(getAdvancementClassLevel());

	return m_VampireInfo;

	__END_DEBUG
	__END_CATCH
}

//----------------------------------------------------------------------
//
// get Extra Info
//
//----------------------------------------------------------------------
ExtraInfo* Vampire::getExtraInfo() const
	throw()
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	BYTE ItemCount = 0;

	ExtraInfo* pExtraInfo = new ExtraInfo();

	Item* pItem = m_pExtraInventorySlot->getItem();

	if (pItem != NULL) {
	
//		Item::ItemClass IClass = pItem->getItemClass();

		ExtraSlotInfo* pExtraSlotInfo = new ExtraSlotInfo();
		pItem->makePCItemInfo(*pExtraSlotInfo);

/*		pExtraSlotInfo->setObjectID(pItem->getObjectID());
		pExtraSlotInfo->setItemClass(pItem->getItemClass());
		pExtraSlotInfo->setItemType(pItem->getItemType());
		pExtraSlotInfo->setOptionType(pItem->getOptionTypeList());
		pExtraSlotInfo->setDurability(pItem->getDurability());
		pExtraSlotInfo->setSilver(pItem->getSilver());
		pExtraSlotInfo->setSilver(pItem->getEnchantLevel());

		if (IClass == Item::ITEM_CLASS_AR) 
		{
			AR* pAR = dynamic_cast<AR*>(pItem);
			pExtraSlotInfo->setItemNum(pAR->getBulletCount());
		} 
		else if (IClass == Item::ITEM_CLASS_SG) 
		{
			SG* pSG = dynamic_cast<SG*>(pItem);
			pExtraSlotInfo->setItemNum(pSG->getBulletCount());
		} 
		else if (IClass == Item::ITEM_CLASS_SMG) 
		{
			SMG* pSMG = dynamic_cast<SMG*>(pItem);
			pExtraSlotInfo->setItemNum(pSMG->getBulletCount());
		} 
		else if (IClass == Item::ITEM_CLASS_SR) 
		{
			SR* pSR = dynamic_cast<SR*>(pItem);
			pExtraSlotInfo->setItemNum(pSR->getBulletCount());
		} 
		else 
		{
			pExtraSlotInfo->setItemNum(pItem->getNum());
		}

		// 벨트라면 Sub 아이템의 추가 정보가 필요하다.
		if (IClass == Item::ITEM_CLASS_BELT) 
		{
			Belt* pBelt = dynamic_cast<Belt*>(pItem);
			Inventory* pBeltInventory = ((Belt*)pItem)->getInventory();
			BYTE SubItemCount = 0;

			// 포켓의 숫자만큼 아이템의 정보를 읽어 들인다.
			for (int i = 0; i < pBelt->getPocketCount(); i++) 
			{
				Item* pBeltItem = pBeltInventory->getItem(i, 0);

				if (pBeltItem != NULL) 
				{
					SubItemInfo* pSubItemInfo = new SubItemInfo();
					pSubItemInfo->setObjectID(pBeltItem->getObjectID());
					pSubItemInfo->setItemClass(pBeltItem->getItemClass());
					pSubItemInfo->setItemType(pBeltItem->getItemType());
					pSubItemInfo->setItemNum(pBeltItem->getNum());
					pSubItemInfo->setSlotID(i);

					pExtraSlotInfo->addListElement(pSubItemInfo);

					SubItemCount++;
				}
			}

			pExtraSlotInfo->setListNum(SubItemCount);

		}

		// 상의 하의 Main Color 지금은 그냥 0 으로 셋팅 해둔다.
		pExtraSlotInfo->setMainColor(0);*/
	
		pExtraInfo->addListElement(pExtraSlotInfo);

		ItemCount++;

	}

	pExtraInfo->setListNum(ItemCount);

	return pExtraInfo;

	__END_DEBUG
	__END_CATCH
}

//----------------------------------------------------------------------
//
// get Gear Info
//
//----------------------------------------------------------------------
GearInfo* Vampire::getGearInfo() const
	throw()
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	int ItemCount = 0;

	GearInfo* pGearInfo = new GearInfo();

	for (int i = 0; i < VAMPIRE_WEAR_MAX; i++) 
	{
		Item* pItem = m_pWearItem[i];

		if (pItem != NULL) 
		{
			//Item::ItemClass IClass = pItem->getItemClass();

			GearSlotInfo* pGearSlotInfo = new GearSlotInfo();
			pItem->makePCItemInfo(*pGearSlotInfo);

/*			pGearSlotInfo->setObjectID(pItem->getObjectID());
			pGearSlotInfo->setItemClass(pItem->getItemClass());
			pGearSlotInfo->setItemType(pItem->getItemType());
			pGearSlotInfo->setOptionType(pItem->getOptionTypeList());
			pGearSlotInfo->setDurability(pItem->getDurability());
			pGearSlotInfo->setSilver(pItem->getSilver());
			pGearSlotInfo->setEnchantLevel(pItem->getEnchantLevel());*/

			/*
			// 벨트라면 Sub 아이템의 추가 정보가 필요하다.
			if (IClass == Item::ITEM_CLASS_BELT) {

				// 아이템 인포를 받아온다.
				ItemInfo* pItemInfo = g_pItemInfoManager->getItemInfo(pItem->getItemClass(), pItem->getItemType());
	
				// 포켓의 숫자를 받아온다.
				BYTE PocketNum = ((BeltInfo*)pItemInfo)->getPocketCount();
	
				// 벨트의 인벤토리를 받아온다.
				Inventory* pBeltInventory = ((Belt*)pItem)->getInventory();
	
				BYTE SubItemCount = 0;
	
				// 포켓의 숫자만큼 아이템의 정보를 읽어 들인다.
				for (int i = 0; i < PocketNum ; i++) {
	
					Item* pBeltItem = pBeltInventory->getItem(i, 0);
	
					if (pBeltItem != NULL) {
	
						SubItemInfo* pSubItemInfo = new SubItemInfo();
						pSubItemInfo->setObjectID(pBeltItem->getObjectID());
						pSubItemInfo->setItemClass(pBeltItem->getItemClass());
						pSubItemInfo->setItemType(pBeltItem->getItemType());
						pSubItemInfo->setItemNum(pBeltItem->getNum());
						pSubItemInfo->setSlotID(i);
	
						pGearSlotInfo->addListElement(pSubItemInfo);
	
						SubItemCount++;
					}
				}
	
				pGearSlotInfo->setListNum(SubItemCount);
			}
			*/

			pGearSlotInfo->setSlotID(i);
	
			// 상의 하의 Main Color 지금은 그냥 0 으로 셋팅 해둔다.
//			pGearSlotInfo->setMainColor(0);
		
			pGearInfo->addListElement(pGearSlotInfo);

			ItemCount++;

		}
	}

	pGearInfo->setListNum(ItemCount);

	return pGearInfo;

	__END_DEBUG
	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
// get Inventory Info
//////////////////////////////////////////////////////////////////////////////
InventoryInfo* Vampire::getInventoryInfo() const throw() {
	__BEGIN_TRY
	__BEGIN_DEBUG

	BYTE ItemCount = 0;

    list<Item*> ItemList;
	InventoryInfo* pInventoryInfo = new InventoryInfo();
	VolumeHeight_t Height = m_pInventory->getHeight();
	VolumeWidth_t Width = m_pInventory->getWidth();

	for (int j = 0; j < Height; j++) {
		for (int i = 0; i < Width; i++) {
			if (m_pInventory->hasItem(i, j)) {
				Item* pItem = m_pInventory->getItem(i, j);
				VolumeWidth_t ItemWidth = pItem->getVolumeWidth();
//				Item::ItemClass IClass = pItem->getItemClass();

				list<Item*>::iterator itr = find(ItemList.begin(), ItemList.end(), pItem);

				if (itr == ItemList.end()) {
					ItemList.push_back(pItem);

					// InventorySlotInfo를 구성
					InventorySlotInfo* pInventorySlotInfo = new InventorySlotInfo();
					pItem->makePCItemInfo(*pInventorySlotInfo);
					pInventorySlotInfo->setInvenX(i);
					pInventorySlotInfo->setInvenY(j);

/*					pInventorySlotInfo->setObjectID(pItem->getObjectID());
					pInventorySlotInfo->setItemClass(pItem->getItemClass());
					pInventorySlotInfo->setItemType(pItem->getItemType());
					pInventorySlotInfo->setOptionType(pItem->getOptionTypeList());
					pInventorySlotInfo->setSilver(pItem->getSilver());
					pInventorySlotInfo->setDurability(pItem->getDurability());
					pInventorySlotInfo->setEnchantLevel(pItem->getEnchantLevel());
					pInventorySlotInfo->setInvenX(i);
					pInventorySlotInfo->setInvenY(j);
					pInventorySlotInfo->setItemNum(pItem->getNum());

					if (IClass == Item::ITEM_CLASS_AR) 
					{
						AR* pAR = dynamic_cast<AR*>(pItem);
						pInventorySlotInfo->setItemNum(pAR->getBulletCount());
					} 
					else if (IClass == Item::ITEM_CLASS_SG) 
					{
						SG* pSG = dynamic_cast<SG*>(pItem);
						pInventorySlotInfo->setItemNum(pSG->getBulletCount());
					} 
					else if (IClass == Item::ITEM_CLASS_SMG) 
					{
						SMG* pSMG = dynamic_cast<SMG*>(pItem);
						pInventorySlotInfo->setItemNum(pSMG->getBulletCount());
					} 
					else if (IClass == Item::ITEM_CLASS_SR) 
					{
						SR* pSR = dynamic_cast<SR*>(pItem);
						pInventorySlotInfo->setItemNum(pSR->getBulletCount());
					}
					else if (IClass == Item::ITEM_CLASS_PET_ITEM)
					{
						PetItem* pPetItem = dynamic_cast<PetItem*>(pItem);
						PetInfo* pPetInfo = pPetItem->getPetInfo();

						if (pPetInfo != NULL )
						{
							pInventorySlotInfo->setOptionType(list<OptionType_t>(pPetInfo->getPetOption()));
							pInventorySlotInfo->setDurability(pPetInfo->getPetHP());
							pInventorySlotInfo->setEnchantLevel(pPetInfo->getPetAttr());
							pInventorySlotInfo->setSilver(pPetInfo->getPetAttrLevel());
							pInventorySlotInfo->setItemNum(pPetInfo->getPetLevel());
						}
					}

					// 벨트라면 Sub 아이템의 추가 정보가 필요하다.
					if (IClass == Item::ITEM_CLASS_BELT) 
					{
						Belt* pBelt = dynamic_cast<Belt*>(pItem);
						Inventory* pBeltInventory = ((Belt*)pItem)->getInventory();

						BYTE SubItemCount = 0;

						// 포켓의 숫자만큼 아이템의 정보를 읽어 들인다.
						for (int i = 0; i < pBelt->getPocketCount() ; i++) 
						{
							Item* pBeltItem = pBeltInventory->getItem(i, 0);
							if (pBeltItem != NULL) 
							{
								SubItemInfo* pSubItemInfo = new SubItemInfo();
								pSubItemInfo->setObjectID(pBeltItem->getObjectID());
								pSubItemInfo->setItemClass(pBeltItem->getItemClass());
								pSubItemInfo->setItemType(pBeltItem->getItemType());
								pSubItemInfo->setItemNum(pBeltItem->getNum());
								pSubItemInfo->setSlotID(i);

								pInventorySlotInfo->addListElement(pSubItemInfo);

								SubItemCount++;
							}
						}

						pInventorySlotInfo->setListNum(SubItemCount);
					}

					pInventorySlotInfo->setMainColor(0);*/

					pInventoryInfo->addListElement(pInventorySlotInfo);
					ItemCount++;
					i = i + ItemWidth - 1;
				}
			}
		}
	}

	pInventoryInfo->setListNum(ItemCount);

	return pInventoryInfo;

	__END_DEBUG
	__END_CATCH
}

//----------------------------------------------------------------------
// getSkillInfo
//----------------------------------------------------------------------
void Vampire::sendVampireSkillInfo()
	throw()
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	VampireSkillInfo* pVampireSkillInfo = new VampireSkillInfo();

	BYTE SkillCount = 0;

	// 현재 시간, 남은 캐스팅 타임을 계산하기 위해
	Timeval currentTime;
	getCurrentTime(currentTime);

	map<SkillType_t, VampireSkillSlot*>::const_iterator itr = m_SkillSlot.begin();
	for (; itr != m_SkillSlot.end(); itr++)
	{
		VampireSkillSlot* pVampireSkillSlot = itr->second;
		Assert(pVampireSkillSlot != NULL);

		// AttackMelee 등의 기본 공격 기술 정보는 보내주지 않아야 한다.
		if (pVampireSkillSlot->getSkillType() >= SKILL_DOUBLE_IMPACT)
		{
			SubVampireSkillInfo* pSubVampireSkillInfo = new SubVampireSkillInfo();
			pSubVampireSkillInfo->setSkillType(pVampireSkillSlot->getSkillType());
			pSubVampireSkillInfo->setSkillTurn(pVampireSkillSlot->getInterval());
			// casting time 항목을 다음 캐스팅까지 남은 시간으로 한다.
			//pSubVampireSkillInfo->setCastingTime(pVampireSkillSlot->getCastingTime());
			pSubVampireSkillInfo->setCastingTime(pVampireSkillSlot->getRemainTurn(currentTime ));

			pVampireSkillInfo->addListElement(pSubVampireSkillInfo);

			SkillCount++;
		}
	}

	GCSkillInfo gcSkillInfo;
	gcSkillInfo.setPCType(PC_VAMPIRE);
	SkillType_t LearnSkillType = g_pSkillInfoManager->getSkillTypeByLevel(SKILL_DOMAIN_VAMPIRE , m_Level);

	// 현재 레벨에서 배울 수 있는 기술이 있는지 본다.
	if (LearnSkillType != 0) 
	{
		// 배울 수 있는 기술이 있고 배우지 않은 상태라면 배우라고 알려준다.
		if (hasSkill(LearnSkillType) == NULL) 
		{
			pVampireSkillInfo->setLearnNewSkill(true);
		}
	}

	pVampireSkillInfo->setListNum(SkillCount);

	gcSkillInfo.addListElement(pVampireSkillInfo);

	m_pPlayer->sendPacket(&gcSkillInfo);

	__END_DEBUG
	__END_CATCH
}



////////////////////////////////////////////////////////////////////////////////
//
//
// 기타 함수
//
//
////////////////////////////////////////////////////////////////////////////////
void Vampire::setGold(Gold_t gold)
	throw()
{
	__BEGIN_TRY

    // MAX_MONEY 를 넘어가는 걸 막는다
	// 2003.1.8  by bezz.
	m_Gold = min((Gold_t)MAX_MONEY, gold);

	__END_CATCH
}

void Vampire::setGoldEx(Gold_t gold)
	throw()
{
	__BEGIN_TRY

	setGold(gold);

	/*
	StringStream sql;
	sql << "Gold = " << (int)m_Gold;

	tinysave(sql.toString());
	*/

	// by sigi. 2002.5.15
	char pField[80];
	sprintf(pField, "Gold=%ld", m_Gold);
	tinysave(pField);

	__END_CATCH
}

void Vampire::increaseGoldEx(Gold_t gold)
	throw()
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	// MAX_MONEY 를 넘어가는 걸 막는다
	// 2003.1.8  by bezz.
	if (m_Gold + gold > MAX_MONEY )
		gold = MAX_MONEY - m_Gold;

	setGold(m_Gold+gold);

    Statement* pStmt = NULL;

	BEGIN_DB
	{
		pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
		pStmt->executeQuery("UPDATE Vampire SET Gold=Gold+%u WHERE NAME='%s'", gold, m_Name.c_str());
		SAFE_DELETE(pStmt);
	} 
	END_DB(pStmt)


	__END_DEBUG
	__END_CATCH
}

void Vampire::decreaseGoldEx(Gold_t gold)
	throw()
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	// 0 미만이 되는 걸 막는다. 0 미만이 되면 underflow 되서 난리가 난다.
	// 2003.1.8  by bezz.
	if (m_Gold < gold )
        gold = m_Gold;
	
	setGold(m_Gold-gold);

    Statement* pStmt = NULL;

	BEGIN_DB
	{
		pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
		pStmt->executeQuery("UPDATE Vampire SET Gold=Gold-%u WHERE NAME='%s'", gold, m_Name.c_str());
		SAFE_DELETE(pStmt);
	} 
	END_DB(pStmt)

	__END_DEBUG
	__END_CATCH
}

bool Vampire::checkGoldIntegrity()
{
	__BEGIN_TRY

	Statement* pStmt = NULL;
	bool ret = false;

	BEGIN_DB
	{
		pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
		Result* pResult = pStmt->executeQuery("SELECT Gold FROM Vampire WHERE NAME='%s'", m_Name.c_str());

		if (pResult->next() )
		{
			ret = pResult->getInt(1) == m_Gold;
		}

		SAFE_DELETE(pStmt);
	} 
	END_DB(pStmt)

	return ret;

	__END_CATCH
}

bool Vampire::checkStashGoldIntegrity()
{
	__BEGIN_TRY

	Statement* pStmt = NULL;
	bool ret = false;

	BEGIN_DB
	{
		pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
		Result* pResult = pStmt->executeQuery("SELECT StashGold FROM Vampire WHERE NAME='%s'", m_Name.c_str());

		if (pResult->next() )
		{
			ret = pResult->getInt(1) == m_StashGold;
		}

		SAFE_DELETE(pStmt);
	} 
	END_DB(pStmt)

	return ret;

	__END_CATCH
}


void Vampire::saveSilverDamage(Silver_t damage)
	throw()
{
	__BEGIN_TRY

	setSilverDamage(damage);

	/*
	StringStream sql;
	sql << "SilverDamage = " << (int)m_SilverDamage;
	tinysave(sql.toString());
	*/

	// by sigi. 2002.5.15
	char pField[80];
	sprintf(pField, "SilverDamage=%d", m_SilverDamage);
	tinysave(pField);

	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
// 소유 아이템 hearbeat
//////////////////////////////////////////////////////////////////////////////
void Vampire::heartbeat(const Timeval& currentTime)
    throw()
{
	__BEGIN_TRY
	__BEGIN_DEBUG

/*#ifdef __ACTIVE_QUEST__
	// by sigi. 2002.12.3
	if (m_pQuestManager!=NULL)
	{
		m_pQuestManager->heartbeat();
	}
#endif*/

	PlayerCreature::heartbeat(currentTime);

	// 주기적으로 HP를 회복시켜준다.
	if (m_HPRegenTime < currentTime)
	{
		Timeval diffTime = timediff(currentTime, m_HPRegenTime);

		if (diffTime.tv_sec > 0)
		{
			// 1. 살아있는 상태이고 (현재 HP가 0 초과)
			// 2. 코마 이펙트가 붙어있지 않다면.
			// 3. Mephisto 이펙트가 붙어있지 않다면.
			if (isAlive() 
				&& !isFlag(Effect::EFFECT_CLASS_COMA)
				&& (!isFlag(Effect::EFFECT_CLASS_MEPHISTO) || isFlag(Effect::EFFECT_CLASS_CASKET) )
			)
			{

				// by sigi. 2002.6.19
				bool bInCasket = isFlag(Effect::EFFECT_CLASS_CASKET);

				HP_t CurHP = m_HP[ATTR_CURRENT];
				HP_t NewHP = 0;

				// 관 속에 있는 경우는
				// SilverDamage를 먼저 치료한다.
				if (bInCasket && m_SilverDamage > 0)
				{
					NewHP = (10 + m_HPRegenBonus ) * diffTime.tv_sec;
					if (isFlag(Effect::EFFECT_CLASS_HAS_BLOOD_BIBLE ) ) NewHP/=2;

					int remainSilver = (int)m_SilverDamage - (int)NewHP;

					// SilverDamage를 다 치료하고 HP도 치료하는 경우
					if (remainSilver < 0)
					{
						m_SilverDamage = 0;
						NewHP = -remainSilver;
				
						HP_t MaxHP = m_HP[ATTR_MAX];
						m_HP[ATTR_CURRENT] = min((int)MaxHP, (int)(CurHP + NewHP));
					}
					// SilverDamage만 감소시키는 경우
					else
					{
						m_SilverDamage = remainSilver;
					}
				}
				else
				{
					HP_t MaxHP = m_HP[ATTR_MAX] - getSilverDamage();

					// Normal       : 2
					// Burrow(Hide) : 4
					// Casket       : 6
					// Wolf         : 2 (일반 상태로 간주)
					// Bat          : 0
					if (isFlag(Effect::EFFECT_CLASS_HIDE))
					{
						NewHP = (4 + m_HPRegenBonus ) * diffTime.tv_sec;
					}
					else if (isFlag(Effect::EFFECT_CLASS_TRANSFORM_TO_BAT))
					{
						NewHP = 0;
					}
					// by sigi. 2002.6.19
					else if (isFlag(Effect::EFFECT_CLASS_CASKET))
					{
						NewHP = (10 + m_HPRegenBonus ) * diffTime.tv_sec;
					}
					else
					{
						NewHP = (2 + m_HPRegenBonus ) * diffTime.tv_sec;
					}

					if (isFlag(Effect::EFFECT_CLASS_HAS_BLOOD_BIBLE ) ) NewHP/=2;
					m_HP[ATTR_CURRENT] = min((int)MaxHP, (int)(CurHP + NewHP));
				}
			}

			m_HPRegenTime.tv_sec = m_HPRegenTime.tv_sec + diffTime.tv_sec;
			m_HPRegenTime.tv_usec = m_HPRegenTime.tv_usec;
		}
	}

	/*

	list<Item*> ItemList;
	VolumeHeight_t Height = m_pInventory->getHeight();
	VolumeWidth_t Width = m_pInventory->getWidth();

	for (int j = 0; j < Height; j++) 
	{
		for (int i = 0 ; i < Width ; i ++) 
		{
			if (m_pInventory->hasItem(i, j)) 
			{
				Item* pItem = m_pInventory->getItem(i , j);
				VolumeWidth_t ItemWidth = pItem->getVolumeWidth();

				list<Item*>::iterator itr = find(ItemList.begin() , ItemList.end() , pItem);

				if (itr == ItemList.end()) 
				{
					ItemList.push_back(pItem);
					//아이템 크기의 다음 위치 부터 검색하기 위함.
					i = i + ItemWidth - 1;
				}
			}
		}
	}

	for (int i = 0; i < VAMPIRE_WEAR_MAX; i++) 
	{
		Item* pItem = m_pWearItem[i];
		if (pItem != NULL) ItemList.push_back(pItem);
	}

	Item* pSlotItem = m_pExtraInventorySlot->getItem();
	if (pSlotItem != NULL) 
	{
		ItemList.push_back(pSlotItem);
	}

	for (list<Item*>::iterator itr = ItemList.begin(); itr != ItemList.end(); itr++)
	{
		Item* pItem = *itr;
		pItem->getEffectManager()->heartbeat();
	}

	*/

	__END_DEBUG
	__END_CATCH
}

void Vampire::getVampireRecord(VAMPIRE_RECORD& record) const
	throw()
{
    __BEGIN_TRY

	record.pSTR[0]     = m_STR[0];
	record.pSTR[1]     = m_STR[1];
	record.pSTR[2]     = m_STR[2];

	record.pDEX[0]     = m_DEX[0];
	record.pDEX[1]     = m_DEX[1];
	record.pDEX[2]     = m_DEX[2];

	record.pINT[0]     = m_INT[0];
	record.pINT[1]     = m_INT[1];
	record.pINT[2]     = m_INT[2];

	record.pHP[0]      = m_HP[0];
	record.pHP[1]      = m_HP[1];

	record.pDamage[0]  = m_Damage[0];
	record.pDamage[1]  = m_Damage[1];

	record.Rank     = getRank();

	record.Defense     = m_Defense[0];
	record.ToHit       = m_ToHit[0];
	record.Protection  = m_Protection[0];
	record.AttackSpeed = m_AttackSpeed[0];

	__END_CATCH
}

void Vampire::setResurrectZoneIDEx(ZoneID_t id)
	throw()
{
	__BEGIN_TRY

	setResurrectZoneID(id);

	/*
	StringStream sql;
	sql << "ResurrectZone = " << (int)id;

	tinysave(sql.toString());
	*/

	// by sigi. 2002.5.15
	char pField[80];
	sprintf(pField, "ResurrectZone=%d", id);
	tinysave(pField);


	__END_CATCH
}

void Vampire::saveAlignment(Alignment_t alignment)
	    throw()
{
	__BEGIN_TRY

	setAlignment(alignment);

	/*
	StringStream sql;
	sql << "Alignment = " << (int)alignment;

	tinysave(sql.toString());
	*/
	// by sigi. 2002.5.15
	char pField[80];
	sprintf(pField, "Alignment=%d", alignment);
	tinysave(pField);

	__END_CATCH
}




//----------------------------------------------------------------------
// get debug string
//----------------------------------------------------------------------
string Vampire::toString () const
	throw()
{
	__BEGIN_TRY

	StringStream msg;

	msg << "Vampire("
		//<< "ObjectID:"   << (int)getObjectID()
		<< ",Name:"      << m_Name
		<< ",BatColor:" << (int)m_BatColor
		<< ",SkinColor:" << (int)m_SkinColor
		<< ",STR:"       << (int)m_STR[ATTR_CURRENT] << "/" << (int)m_STR[ATTR_MAX]
		<< ",DEX:"       << (int)m_DEX[ATTR_CURRENT] << "/" << (int)m_DEX[ATTR_MAX]
		<< ",INT:"       << (int)m_INT[ATTR_CURRENT] << "/" << (int)m_INT[ATTR_MAX]
		<< ",HP:"        << (int)m_HP[ATTR_CURRENT] << "/" << (int)m_HP[ATTR_MAX]
		<< ",Fame:"      << (int)m_Fame
//		<< ",Exp:"       << (int)m_Exp
//		<< ",ExpOffset:" << (int)m_ExpOffset
		<< ",Rank:"       << (int)getRank()
		<< ",RankGoalExp:" << (int)getRankGoalExp()
		<< ",Level:"     << (int)m_Level
		<< ",Bonus:"     << (int)m_Bonus
		//<< ",InMagics:'" << ??? << "'"
		<< ",Gold:"      << (int)m_Gold
		<< ",ZoneID:"    << (int)getZoneID()
		<< ",XCoord:"    << (int)m_X
		<< ",YCoord:"    << (int)m_Y
		<< ",Sight:"     << (int)m_Sight
		<< ")";

	return msg.toString();

	__END_CATCH
}

void Vampire::saveSkills(void) const 
	throw(Error)
{
	__BEGIN_TRY

	map<SkillType_t, VampireSkillSlot*>::const_iterator itr = m_SkillSlot.begin();
	for (; itr != m_SkillSlot.end(); itr++)
	{
		VampireSkillSlot* pVampireSkillSlot = itr->second;
		Assert(pVampireSkillSlot != NULL);

		// 기본 공격 스킬이 아니라면...
		if (pVampireSkillSlot->getSkillType() >= SKILL_DOUBLE_IMPACT)
		{
			pVampireSkillSlot->save(m_Name);
		}
	}

	__END_CATCH
}

IP_t Vampire::getIP(void) const
{
	Assert(m_pPlayer != NULL);
	Socket* pSocket = m_pPlayer->getSocket();
	Assert(pSocket != NULL);
	return pSocket->getHostIP();
}

void Vampire::saveGears(void) const
    throw(Error)
{
	__BEGIN_TRY

	// 장착하고 있는 아이템들을 저장한다.
	char pField[80];

	for (int i=0; i < Vampire::VAMPIRE_WEAR_MAX; i++)
	{
		Item* pItem = m_pWearItem[i];
		if (pItem != NULL)
		{
			Durability_t maxDurability = computeMaxDurability(pItem);
			if (pItem->getDurability() < maxDurability)
			{
				//pItem->save(m_Name, STORAGE_GEAR, 0, i, 0);
				// item저장 최적화. by sigi. 2002.5.13
				sprintf(pField, "Durability=%d", pItem->getDurability());
				pItem->tinysave(pField);
			}
		}
	}

	__END_CATCH
}


void Vampire::saveExps(void) const
    throw(Error)
{
	__BEGIN_TRY

	// 스킬 핸들러에서 쿼리 숫자를 줄이기 위해서 10으로 나누는 부분들은,
	// 서버 다운이 되지 않고, 정상적으로 로그아웃하는 경우에 
	// 세이브를 명시적으로 해주지 않으면 10 이하 올라간 부분은 날아가 버리게 된다.
	// 그러므로 여기서 세이브를 해 준다. 
	/*
	StringStream sql;
	sql << "UPDATE Vampire SET "
		<< "Alignment = " << m_Alignment
		<< ",Fame = " << m_Fame
		<< ",Exp = " << m_Exp
		<< ",GoalExp = " << m_GoalExp;

	if (m_SilverDamage != 0)
	{
		sql << ",SilverDamage = " << m_SilverDamage;
	}

	sql << " WHERE Name = '" << m_Name << "'";
	*/

	Statement* pStmt = NULL;

	char silverDam[40];
	if (m_SilverDamage != 0)
	{
		sprintf(silverDam, ",SilverDamage = %d", m_SilverDamage);
	}
	else silverDam[0]='\0';

	BEGIN_DB
	{
		pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
		pStmt->executeQuery("UPDATE Vampire SET Alignment=%d, Fame=%d, GoalExp=%lu%s, Rank=%d, RankGoalExp=%lu, AdvancementClass=%u, AdvancementGoalExp=%lu WHERE Name='%s'",
							m_Alignment, m_Fame, m_GoalExp, silverDam, getRank(), getRankGoalExp(), getAdvancementClassLevel(), getAdvancementClassGoalExp(), m_Name.c_str());

		SAFE_DELETE(pStmt);
	}
	END_DB(pStmt)

	__END_CATCH
}


//----------------------------------------------------------------------
// getShapeInfo
//----------------------------------------------------------------------
// login할때 처리를 빨리하기 위해서다.
//----------------------------------------------------------------------
// 일단 32bit로 32가지를 표현하는걸로도 충분하다고 본다.
// 언젠가? over되면 bitset을 써야겠지..
//
// (!) 색깔은 index색값이 아니고 optionType을 넣어서 사용한다.
//     클라이언트에서 옵션으로 색값을 찾아서 쓴다.
//
// colors[1]은 coatColor만 있기 때문이다.
//----------------------------------------------------------------------
void Vampire::getShapeInfo (DWORD& flag, Color_t colors[PCVampireInfo::VAMPIRE_COLOR_MAX]) const
//	throw()
{
	__BEGIN_DEBUG

	Item* 						pItem;
	//OptionInfo* 				pOptionInfo;
	int							vampireBit;
	int							vampireColor;
	WearPart					Part;

	// 초기화
	flag = 0;

	//-----------------------------------------------------------------
	// 복장
	//-----------------------------------------------------------------
	Part = WEAR_BODY;
	pItem = m_pWearItem[Part];
	vampireBit = 0;
	vampireColor = 0;

	if (pItem!=NULL && m_pRealWearingCheck[Part])
	{
		ItemType_t IType = pItem->getItemType();

		colors[vampireColor] = getItemShapeColor(pItem);

		//colors[vampireColor] = pItem->getOptionType();
		//flag |= (getVampireCoatType(IType) << vampireBit);

		// itemType을 넣어준다.
		flag = IType;
	} 
	else 
	{
		colors[vampireColor] = 377;
		//flag |= (VAMPIRE_COAT_BASIC << vampireBit);
		// 기본 옷 :  남자는 0, 여자는 1
		flag = (m_Sex? 0 : 1);
	}

	__END_DEBUG
}


//----------------------------------------------------------------------
// save InitialRank
//----------------------------------------------------------------------
// Rank, RankExp, RankGoalExp의 초기값을 저장한다.
//----------------------------------------------------------------------
void Vampire::saveInitialRank(void)
	throw()
{
	VAMPIRE_RECORD prev;
	getVampireRecord(prev);

	int curRank = max(1, (m_Level+3) / 4);
	m_pRank->SET_LEVEL(curRank);

/*	RankExp_t accumExp = 0;

	if (curRank!=1)
	{
		RankEXPInfo* pBeforeExpInfo = g_pRankEXPInfoManager[RANK_TYPE_VAMPIRE]->getRankEXPInfo(curRank-1);
		accumExp = pBeforeExpInfo->getAccumExp();
	}
	
	RankEXPInfo* pNextExpInfo = g_pRankEXPInfoManager[RANK_TYPE_VAMPIRE]->getRankEXPInfo(curRank);
	Exp_t NextGoalExp = pNextExpInfo->getGoalExp();

	setRankGoalExp(NextGoalExp);
*/

	char pField[80];
	sprintf(pField, "Rank=%d, RankExp=%lu, RankGoalExp=%lu",
					getRank(), getRankExp(), getRankGoalExp());
	tinysave(pField);
	setRankExpSaveCount(0);
	/*
	sendModifyInfo(prev);

	if (m_pZone != NULL)
	{
		GCOtherModifyInfo gcOtherModifyInfo;
		gcOtherModifyInfo.setObjectID(getObjectID());
		gcOtherModifyInfo.addShortData(MODIFY_RANK, curRank);

		m_pZone->broadcastPacket(m_X, m_Y, &gcOtherModifyInfo, this);
	}
	*/
}

bool
Vampire::addShape(Item::ItemClass IClass, ItemType_t IType, Color_t color)
{
	bool bisChange = false;

	switch (IClass)
	{
		case Item::ITEM_CLASS_VAMPIRE_COAT:
		{
			bisChange = true;

			m_VampireInfo.setCoatColor(color);
			m_VampireInfo.setCoatType(IType);
		}
		break;

		default:
			break;
	}

	return bisChange;
}


bool
Vampire::removeShape(Item::ItemClass IClass, bool bSendPacket)
{
    bool bisChange = false;

	switch (IClass)
	{
		case Item::ITEM_CLASS_VAMPIRE_COAT :
		{
			m_VampireInfo.setCoatColor(377);
			m_VampireInfo.setCoatType(0);

			if (bSendPacket)	// by sigi. 2002.11.6
			{
				GCTakeOff pkt;
				pkt.setObjectID(getObjectID());
				pkt.setSlotID((SlotID_t)ADDON_COAT);
				m_pZone->broadcastPacket(getX(), getY(), &pkt, this);
			}
		}
		break;

		default :
			return false;
	}

	return bisChange;
}

Color_t 
Vampire::getItemShapeColor(Item* pItem, OptionInfo* pOptionInfo) const
{
	Color_t color;

	if (pItem->isTimeLimitItem())
	{
		// 퀘스트는 특정한 색깔로 대체해서 처리한다.
		color = QUEST_COLOR;
	}
	else if (pItem->isUnique())
	{
		// 유니크는 특정한 색깔로 대체해서 처리한다.
		color = UNIQUE_COLOR;
	}
	// 외부에서 이미 OptionInfo를 찾은 경우
	else if (pOptionInfo != NULL) 
	{
		color = pOptionInfo->getColor();
	}
	// 아니면.. 첫번째 옵션의 색깔을 지정한다.
	else if (pItem->getFirstOptionType() != 0)
	{
		OptionInfo* pOptionInfo = g_pOptionInfoManager->getOptionInfo(pItem->getFirstOptionType());
		color = pOptionInfo->getColor();
	}
	else 
	{
		// default 색
		color = 377;
	}

	return color;
}

bool Vampire::canPlayFree()
	throw(Error)
{
	__BEGIN_TRY

	return m_Level <= g_pVariableManager->getVariable(FREE_PLAY_VAMPIRE_LEVEL);

	__END_CATCH
}


bool Vampire::isPayPlayAvaiable() 
	throw(Error)
{
	__BEGIN_TRY

	if (m_pPlayer==NULL)
		return false;

	GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(m_pPlayer);

#ifdef __CONNECT_BILLING_SYSTEM__
	if (pGamePlayer->isPayPlaying())
	{
		// 완전 무료 사용자. ㅋㅋ
		if (pGamePlayer->getPayType()==PAY_TYPE_FREE)
			return true;

		// 제한된 레벨까지 play가능
		if (m_Level <= g_pVariableManager->getVariable(FREE_PLAY_VAMPIRE_LEVEL))
		{
			return true;
		}
	}

	return false;

// 애드빌 빌링을 사용하지 않고 사용자 제한을 하는 경우
#elif defined(__PAY_SYSTEM_FREE_LIMIT__)

	if (!pGamePlayer->isPayPlaying())
	{
		// 제한된 레벨까지 play가능
		if (m_Level <= g_pVariableManager->getVariable(FREE_PLAY_VAMPIRE_LEVEL))
		{
			return true;
		}

		return false;
	}

	return true;

#else

	return pGamePlayer->isPayPlaying();

#endif


	__END_CATCH
}

void Vampire::initPetQuestTarget()
{
	int minClass = 1, maxClass = 1;

	if (getLevel() <= 50 )
	{
		minClass = 8; maxClass = 9;
	}
	else if (getLevel() <= 60 )
	{
		minClass = maxClass = 9;
	}
	else if (getLevel() <= 70 )
	{
		minClass = maxClass = 10;
	}
	else if (getLevel() <= 80 )
	{
		minClass = 10; maxClass = 11;
	}
	else if (getLevel() <= 90 )
	{
		minClass = 10; maxClass = 11;
	}
	else if (getLevel() <= 110 )
	{
		minClass = 11; maxClass = 12;
	}
	else if (getLevel() <= 130 )
	{
		minClass = 11; maxClass = 12;
	}
	else
	{
		minClass = 12; maxClass = 13;
	}

	m_TargetMonster = g_pMonsterInfoManager->getRandomMonsterByClass(minClass, maxClass);
	m_TargetNum = 80;
	m_TimeLimit = 3600;
}
