//////////////////////////////////////////////////////////////////////////////
// Filename    : CGSayHandler.cc
// Written By  : elca@ewestsoft.com
// Description :
//////////////////////////////////////////////////////////////////////////////

#include "CGSay.h"

#ifdef __GAME_SERVER__
	#include <stdio.h>

	#include "GamePlayer.h"
    #include "ZoneGroupManager.h"
    #include "ZoneInfoManager.h"
    #include "ZonePlayerManager.h"
	#include "DB.h"
    #include "InfoClassManager.h"
    #include "ItemInfoManager.h"
	#include "ItemFactoryManager.h"
	#include "Monster.h"
	#include "IncomingPlayerManager.h"
	#include "Zone.h"
	#include "ZoneUtil.h"
	#include "EffectShutDown.h"
	#include "NPC.h"
	#include "Slayer.h"
	#include "Vampire.h"
	#include "Ousters.h"
	#include "PCFinder.h"
	#include "NPCInfo.h"
	#include "VSDateTime.h"
	#include "Guild.h"
	#include "Relic.h"
	#include "RelicUtil.h"
	#include "CombatInfoManager.h"
	#include "ClientManager.h"
	#include "OptionInfo.h"
	#include "ItemUtil.h"
	#include "MonsterManager.h"
	#include "PCManager.h"
	#include <list>
	#include "StringPool.h"
	#include "GoodsInventory.h"
	#include "NicknameBook.h"

	#include "PacketUtil.h"

	#include "skill/Sniping.h"

	#include "Gpackets/GCUpdateInfo.h"
    #include "Gpackets/GCSystemMessage.h"
    #include "Gpackets/GCDeleteObject.h"
    #include "Gpackets/GCAddVampire.h"
    #include "Gpackets/GCAddSlayer.h"
    #include "Gpackets/GCAddOusters.h"
	#include "Gpackets/GCSay.h"
	#include "Gpackets/GCCreateItem.h"
	#include "Gpackets/GCAddEffect.h"
	#include "Gpackets/GCAddEffectToTile.h"
	#include "Gpackets/GCRemoveEffect.h"
	#include "Gpackets/GCModifyNickname.h"

	#include "Mine.h"
	#include "Properties.h"
	#include "VariableManager.h"

	#include "Properties.h"
	#include "GameServerInfoManager.h"
	#include "LoginServerManager.h"
	#include "Gpackets/GGCommand.h"
	#include "ZoneInfoManager.h"
	#include "ZoneGroupManager.h"
	#include "UniqueItemManager.h"
	#include "MasterLairManager.h"
	#include "EffectGhost.h"
	#include "EventManager.h"
	#include "EventReloadInfo.h"
	#include "CreatureUtil.h"
	#include "ShrineInfoManager.h"
	#include "CastleInfoManager.h"
	#include "war/WarSystem.h"
	#include "EffectLoud.h"
	#include "EffectMute.h"
	#include "LevelWarManager.h"

	#include "LogNameManager.h"
	#include "war/RaceWarLimiter.h"
	#include "war/WarScheduler.h"

	#include "PKZoneInfoManager.h"
	#include "PCManager.h"

	#include "SharedServerManager.h"
	#include "GuildManager.h"
	#include "GameServerGroupInfoManager.h"

	#include "BillingPlayerManager.h"

	#include "mission/QuestManager.h"
	#include "mission/MonsterKillQuestStatus.h"

	#include "Gpackets/GSRequestGuildInfo.h"
	#include "Gpackets/GCActiveGuildList.h"
	#include "Gpackets/GCNoticeEvent.h"
	#include "Gpackets/GSModifyGuildMember.h"
	#include "Gpackets/GCStatusCurrentHP.h"
	#include "Gpackets/GCNotifyWin.h"
	#include "Gpackets/GCMiniGameScores.h"
	#include "Gpackets/GCPetStashList.h"

	#include "Cpackets/CGModifyGuildIntro.h"
	#include "Cpackets/CGModifyGuildMemberIntro.h"
	#include "Cpackets/CGSkillToNamed.h"
    #include "Cpackets/CGStashList.h"
    
	#include "item/SlayerPortalItem.h"
	#include "item/OustersSummonItem.h"
	#include "item/PetItem.h"

	#include "ctf/FlagManager.h"
	
	#include "EventZoneInfo.h"
	#include "EffectCastingTrap.h"
	#include "GDRLairManager.h"
	#include "SiegeManager.h"

//	#include "gameguard/CSAuth.h"

	#include "GuildUnion.h"
	#include "DynamicZoneManager.h"
	#include "DynamicZoneGroup.h"
	#include "DynamicZone.h"
	#include "GQuestManager.h"
	#include "EffectDeleteItem.h"
    #include <map>
#endif


void CGSayHandler::execute(CGSay* pPacket , Player* pPlayer) throw(ProtocolException, Error) {
	__BEGIN_TRY __BEGIN_DEBUG_EX
#ifdef __GAME_SERVER__

	Assert(pPacket != NULL);
	Assert(pPlayer != NULL);

	try {
    	GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(pPlayer);

    	// 서버에서 클라이언트로 전송하므로 GC- 패킷을 사용해야 한다.
    	GCSay gcSay;
    	
    	Creature* pCreature = pGamePlayer->getCreature();

    	// 크리처 이름과 메시지를 패킷에 대입한다.
    	gcSay.setObjectID(pCreature->getObjectID());	
    	gcSay.setColor(pPacket->getColor());

    	string msg = pPacket->getMessage();

    	gcSay.setMessage(msg);

    	bool Success = true;

    	uint i = msg.find_first_of('*' , 0);

    	if (i == 0) {
    		Success = false;

    		// 길드용 임시 명령들
    		if (msg.substr(i + 1, 4) == "zone" && !(pCreature->isPLAYER()))
                opzone(msg, i);
    		else if (msg.substr(i + 1, 5) == "guild" && pCreature->isGOD())
    			opguild(msg, i, pCreature);
    		/////////////////////
    		// world명령어는 도우미 케릭터도 쓸 수 있다.
    		if (msg.substr(i + 1, 5) == "world" && !(pCreature->isPLAYER())) {
    			// 현재 서버
    			uint j = msg.find_first_of(' ', i + 1);
    			string command = msg.substr(j + 1, msg.size() - j - 1).c_str();
    			uint k = command.find_first_of('*', 0);
    			opExecute(pCreature, pGamePlayer, command, k);

    			// 다른 server에도 보낸다. bSameWorldOnly==true
    			opworld(pGamePlayer, msg, i, true);
    		}
    		else if (msg.substr(i + 1 , 8) == "allworld" && (pCreature->isDM() || pCreature->isGOD())) {
    			// 현재 서버	
    			uint j = msg.find_first_of(' ', i + 1);
    			string command = msg.substr(j + 1, msg.size() - j - 1).c_str();
    			uint k = command.find_first_of('*', 0);
    			opExecute(pCreature, pGamePlayer, command, k);

    			// 다른 server에도 보낸다. bSameWorldOnly==false
    			opworld(pGamePlayer, msg, i, false);
    		}
    		else
    			opExecute(pCreature, pGamePlayer, msg, i);
    	}

    	// 채팅 로그를 남긴다. by sigi. 2002.10.30
    	if (LogNameManager::getInstance().isExist(pCreature->getName()))
    		filelog("chatLog.txt", "[Say] %s> %s", pCreature->getName().c_str(), msg.c_str());

    	// invisibility상태면 푼다.
    	if (pCreature->isFlag(Effect::EFFECT_CLASS_INVISIBILITY)) {
    		Zone* pZone = pCreature->getZone();
    		Assert(pZone);
    		addVisibleCreature(pZone, pCreature, true);
    	}

    	if (pCreature->isFlag(Effect::EFFECT_CLASS_SNIPING_MODE))
    		g_Sniping.checkRevealRatio(pCreature, 20, 10);

    	bool isVampire = false;
    	// 뱀파이어가 늑대, 박쥐이면 말할수없다.
    	if (pCreature->isVampire()) {
    		Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);
    		
    		if (pVampire->isFlag(Effect::EFFECT_CLASS_TRANSFORM_TO_WOLF) ||
                pVampire->isFlag(Effect::EFFECT_CLASS_TRANSFORM_TO_BAT) ||
                pVampire->isFlag(Effect::EFFECT_CLASS_TRANSFORM_TO_WERWOLF))
                    Success = false;

    		isVampire = true;
    	}

    	if (pGamePlayer->isPenaltyFlag(PENALTY_TYPE_MUTE))
    		Success = false;

    	if (pCreature->isFlag(Effect::EFFECT_CLASS_MUTE))
    		Success = false;

    	if (Success) {
    		// 주변 PC들에게 브로드캐스트한다.
//		pCreature->getZone()->broadcastSayPacket(pCreature->getX() , pCreature->getY() , &gcSay , pCreature, isVampire);
    		pCreature->getZone()->broadcastPacket(pCreature->getX() , pCreature->getY() , &gcSay , pCreature);
    	}
	}
    catch (Throwable & t) {
		//cout << t.toString() << endl;
	}

#endif
	__END_DEBUG_EX __END_CATCH
}

#ifdef __GAME_SERVER__


void CGSayHandler::opExecute(Creature* pCreature, GamePlayer* pGamePlayer, string msg, int i) throw(ProtocolException, Error) {
	__BEGIN_TRY

	if (msg.substr(i + 1 , 4) == "save" && (pCreature->isDM() || pCreature->isGOD()))
        opsave(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 4) == "wall" && !(pCreature->isPLAYER()))
		opwall(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 8) == "shutdown" && (pCreature->isDM() || pCreature->isGOD()))
		opshutdown(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 3) == "pay" && !(pCreature->isPLAYER()))
		oppay(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 4) == "kick" && !(pCreature->isPLAYER()))
		opkick(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 4) == "mute" && !(pCreature->isPLAYER()))
		opmute(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 8) == "denychat" && !(pCreature->isPLAYER()))
		opdenychat(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 8) == "freezing" && !(pCreature->isPLAYER()))
		opfreezing(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 4) == "deny" && (pCreature->isDM() || pCreature->isGOD()))
		opdeny(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 4) == "info" && !(pCreature->isPLAYER()))
		opinfo(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 5) == "trace" && !(pCreature->isPLAYER()))
		optrace(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 4) == "warp" && !(pCreature->isPLAYER()))
		opwarp(pGamePlayer, msg, i);

	else if (msg.substr(i + 1  , 6) == "create" && (pCreature->isGOD()))
		opcreate(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 6) == "summon" && (pCreature->isGOD() || pCreature->isDM()))
		opsummon(pGamePlayer, msg, i);
	
	else if (msg.substr(i + 1 , 5) == "grant" && pCreature->isGOD())
		opgrant(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 7) == "command" && (pCreature->isGOD() || pCreature->isDM()))
		opcommand(pGamePlayer, msg, i);

	else if (msg.substr(i + 1 , 3) == "fun" && !(pCreature->isPLAYER()))
		opfun(pGamePlayer, msg, i);

	else if (msg.substr(i + 1, 18) == "billing disconnect" && pCreature->isGOD()) {
		#ifdef __CONNECT_BILLING_SYSTEM__
		g_pBillingPlayerManager->setForceDisconnect();
		#endif
	}

	/*
	if (msg.substr(i+1 , 7) == "mrecall" && (pCreature->isGOD() || pCreature->isDM())) {

		Zone* pZone = pCreature->getZone();

		uint j = msg.find_first_of(' ' , i+1);

		BYTE MonsterType = atoi(msg.substr(j+1, msg.size()-j-1).c_str());

		Monster* pMonster = new Monster(MonsterType);

		pZone->addCreature(pMonster, pCreature->getX(), pCreature->getY(), pCreature->getDir(), true);

	}
	*/

	else if (msg.substr(i + 1 , 6) == "recall")
		oprecall(pGamePlayer, msg, i);
	else if (msg.substr(i + 1 , 4) == "user" && !(pCreature->isPLAYER()))
		opuser(pGamePlayer, msg, i);
	else if (msg.substr(i + 1 , 6) == "notice" && (pCreature->isGOD() || pCreature->isDM()))
		opnotice(pGamePlayer, msg, i);

	// 각종 함수용 값 세팅용 함수
	// set type value 형태로 정의된다.
	// 2002.5.8 별 확률을 자율적으로 조절하기 위해서 만듬
	else if (msg.substr(i + 1, 3) == "set" && (pCreature->isGOD()))
		opset(pGamePlayer, msg, i);

	else if (msg.substr(i + 1, 4) == "load" && (pCreature->isGOD()))
		opload(pGamePlayer, msg, i);

	else if (msg.substr(i + 1, 4) == "view" && (pCreature->isGOD() || pCreature->isDM()))
		opview(pGamePlayer, msg, i);

	else if (msg.substr(i + 1, 6) == "combat" && (pCreature->isGOD() || pCreature->isDM()))
		opcombat(pGamePlayer, msg, i);

	else if (msg.substr(i + 1, 4) == "find" || msg.substr(i+1, 4) == "어디")
		opfind(pGamePlayer, msg, i);

	else if (msg.substr(i + 1, 6) == "credit")
		opcredit(pGamePlayer, msg, i);

	else if (msg.substr(i + 1, 9) == "soulchain" && (!pCreature->isPLAYER()))
		opsoulchain(pGamePlayer, msg, i);

	else if (msg.substr(i + 1, 3) == "log" && (pCreature->isGOD() || pCreature->isDM()))
		oplog(pGamePlayer, msg, i);
	else if (msg.substr(i + 1, 10) == "bug_report") {
		uint j = msg.find_first_of(' ', i + 1);
		if (j == string::npos) {
			filelog("bugreport.log", "%s", msg.c_str());
			return;
		}
 
		string report = msg.substr(j + 1, msg.size() - j - 1);
		if ((j = report.find_first_of('\'')) != string::npos)
            report[j] = '_';
		if ((j = report.find_first_of('\\')) != string::npos)
            report[j] = '_';

		Statement* pStmt = NULL;

		try {
			BEGIN_DB {
				pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
				pStmt->executeQuery("INSERT INTO BugReportLog(PlayerID, Name, ReportTime, ReportLog) VALUES ('%s', '%s', now(), '%s')", pGamePlayer->getID().c_str(), pCreature->getName().c_str(), report.c_str());

				SAFE_DELETE(pStmt);
			} END_DB(pStmt)
				// 누가 이상한거 날리면 무시하자
		}
        catch(...) {
            filelog("bugreport.log", "%s", msg.c_str());
        }
	}
	else if (msg.substr(i + 1, 11) == "CrashReport") {
		uint j = msg.find_first_of(' ', i + 1);
		uint k = msg.find_first_of(' ', j + 13);
		uint l = msg.find_first_of(' ', k + 1);
		uint m = msg.find_first_of(' ', l + 1);
		if (j == string::npos || k == string::npos || l == string::npos || m == string::npos) {
			filelog("CrashReport.log", "[%s] %s", pGamePlayer->getID().c_str(), msg.c_str());
			return;
		}

		string ExecutableTime = msg.substr(j + 1, k - j - 1);
		string Version = msg.substr(k + 1, l - k - 1);
		string Address = msg.substr(l + 1, m - l - 1);
		string Message = msg.substr(m + 1, msg.size() - m - 1);

		//cout << ExecutableTime << endl << Version << endl << Address << endl << Message << endl;

		Statement* pStmt = NULL;

		try {
			BEGIN_DB {
				pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
				pStmt->executeQuery("INSERT INTO CrashLog (PlayerID, Name, ReportTime, ExecutableTime, Version, Address, Message) VALUES ('%s', '%s', now(), '%s', '%s', '%s', '%s')", pGamePlayer->getID().c_str(), pCreature->getName().c_str(), ExecutableTime.c_str(), Version.c_str(), Address.c_str(), Message.c_str());

				SAFE_DELETE(pStmt);
			} END_DB(pStmt)
				// 누가 이상한거 날리면 무시하자
		}
        catch(...) {
            filelog("CrashReport.log", "%s", msg.c_str());
        }
	}
    else if (msg.substr(i + 1, 9) == "openstash") {
        if (pCreature->isSlayer()) {
            Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);
            if (pSlayer->getStashNum() < 1) return;
        } else if (pCreature->isVampire()) {
            Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);
            if (pVampire->getStashNum() < 1) return;
        } else {
            Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);
            if (pOusters->getStashNum() < 1) return;
        }
        CGStashList _CGStashList;
        _CGStashList.setObjectID(pCreature->getObjectID());
        _CGStashList.execute(pGamePlayer);
    }

	__END_CATCH
}


void CGSayHandler::opcombat(GamePlayer* pGamePlayer, string msg, int i) throw(ProtocolException, Error) {
	__BEGIN_TRY
	
	if (pGamePlayer == NULL)
        return;

	Creature* pCreature = pGamePlayer->getCreature();
	if (pCreature == NULL)
        return;

	filelog("change.txt", "[%s] %s", pCreature->getName().c_str(), msg.c_str());

//	Creature* pCreature = pGamePlayer->getCreature();
	GCSystemMessage gcSystemMessage;

	// 임시로 막아둔것
	{
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_DO_NOT_SUPPORT_OLD_WAR));
		pGamePlayer->sendPacket (&gcSystemMessage);

		return;
	}

	StringStream message;

	uint j = msg.find_first_of(' ', i + 1);
	uint z = msg.find_first_of(' ', j + 1);

	string set_type = msg.substr(j + 1, z - j - 1);

	if (set_type == "start") {
		if (g_pCombatInfoManager->isCombat() ||
           g_pCombatInfoManager->isSlayerBonus() ||
           g_pCombatInfoManager->isVampireBonus()) {
			//cout << "이미 전쟁중입니다" << endl;
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_COMBAT_ALEADY_START));

			pGamePlayer->sendPacket(&gcSystemMessage);
		} else {
			//cout << "전쟁이 시작되었습니다" << endl;
//			message << "전쟁이 시작되었습니다";
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_COMBAT_START));

			g_pZoneGroupManager->broadcast(&gcSystemMessage);

			// by sigi. 2002.7.5
			for (int i = 0; i < maxRelic; i++) {
				const RelicInfo* pRelicInfo = dynamic_cast<RelicInfo*>(g_pRelicInfoManager->getItemInfo(i));

				int ZoneNum = pRelicInfo->zoneID;

				ZoneInfo* pZoneInfo = NULL;

				try {
					pZoneInfo = g_pZoneInfoManager->getZoneInfo(ZoneNum);
                }
				catch (NoSuchElementException&) {
					throw Error("전쟁존이 없다.");
                }

				ZoneGroup* pZoneGroup = NULL;

				try {
					pZoneGroup = g_pZoneGroupManager->getZoneGroup(pZoneInfo->getZoneGroupID());
                }
				catch (NoSuchElementException&) {
					throw Error("전쟁존이 없다.2");
                }

				Zone* pZone = pZoneGroup->getZone(ZoneNum);

				pZone->addRelicItem(i);
			}

			g_pCombatInfoManager->setCombat(true);
		}
	}
	else if (set_type == "end") {
		if (!g_pCombatInfoManager->isCombat() &&
           (g_pCombatInfoManager->isSlayerBonus() || g_pCombatInfoManager->isVampireBonus())) {
			//cout << "전쟁을 종료합니다." << endl;
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_COMBAT_END));
			g_pZoneGroupManager->broadcast(&gcSystemMessage);

			// by sigi. 2002.7.5
			for (int i=0; i < maxRelic; i++) {
				const RelicInfo* pRelicInfo = dynamic_cast<RelicInfo*>(g_pRelicInfoManager->getItemInfo(i));

				int ZoneNum = pRelicInfo->zoneID;

				ZoneInfo* pZoneInfo = NULL;

				try {
					pZoneInfo = g_pZoneInfoManager->getZoneInfo(ZoneNum);
                }
				catch (NoSuchElementException&) {
					throw Error("전쟁존이 없다.");
                }

				ZoneGroup* pZoneGroup = NULL;

				try {
					pZoneGroup = g_pZoneGroupManager->getZoneGroup(pZoneInfo->getZoneGroupID());
                }
				catch (NoSuchElementException&) {
					throw Error("전쟁존이 없다.2");
                }

				Zone* pZone = pZoneGroup->getZone(ZoneNum);

				pZone->deleteRelicItem();
				g_pCombatInfoManager->setRelicOwner(i, CombatInfoManager::RELIC_OWNER_NULL);
			}

			g_pCombatInfoManager->computeModify();
			g_pCombatInfoManager->setCombat(false);
		} else {
			//cout << "전쟁중이 아니거나 종료할 수 없습니다." << endl;
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_CANNOT_END_COMBAT));
			
			pGamePlayer->sendPacket(&gcSystemMessage);
		}
	}

	__END_CATCH
}


void CGSayHandler::opview(GamePlayer* pGamePlayer, string msg, int i) throw(ProtocolException, Error) {
	__BEGIN_TRY

	if (pGamePlayer==NULL)
        return;

	Creature* pCreature = pGamePlayer->getCreature();
	GCSystemMessage gcSystemMessage;
	StringStream message;

	uint j = msg.find_first_of(' ' , i + 1);
	uint z = msg.find_first_of(' ' , j + 1);

	string set_type = msg.substr(j + 1, z - j - 1);

	//cout << "modifier : " << pCreature->getName() << " set type : " << set_type << endl;

	if (set_type == "star") {
	    message << "별 출현 확률: 1/" << g_pVariableManager->getStar();
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "evnet_activate") {
		if (g_pVariableManager->getEventActivate() == 1)
			message << "이벤트 진행중";
		else
			message << "이벤트 정지중";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "event_ratio") {
	    message << "이벤트 몬스터 출현 확률 1/" << g_pVariableManager->getEventRatio();
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "exp_ratio") {
	    message << "경험치 획득률: " << g_pVariableManager->getExpRatio();
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "item_prob_ratio") {
	    message << "아이템 획득률: " << g_pVariableManager->getItemProbRatio();
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "combat_bonus_time") {
	    message << "전쟁 승리 유지시간: " << g_pVariableManager->getCombatBonusTime() << "분";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "combat_bonus_slayer_hp_ratio") {
	    message << "슬레이어 전쟁 HP보너스 증가치: +" << g_pVariableManager->getCombatSlayerHPBonusRatio() << "%";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "combat_bonus_vampire_hp_ratio") {
	    message << "뱀파이어 전쟁 HP보너스 증가치: +" << g_pVariableManager->getCombatVampireHPBonusRatio() << "%";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "combat_bonus_slayer_damage") {
	    message << "슬레이어 전쟁 Damage보너스 증가치: +" << g_pVariableManager->getCombatSlayerDamageBonus();
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "combat_bonus_vampire_damage") {
	    message << "뱀파이어 전쟁 Damage보너스 증가치: +" << g_pVariableManager->getCombatVampireDamageBonus();
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "premium_exp_bonus_percent") {
	    message << "프리미엄 사용자 경험치 보너스: " << g_pVariableManager->getPremiumExpBonusPercent() << "%";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "premium_item_probe_percent") {
	    message << "프리미엄 존 아이템 루팅 확률: " << g_pVariableManager->getPremiumItemProbePercent() << "%";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "zone_group_balancing_minute") {
	    message << "ZoneGroupBalancing 시간: " << g_pVariableManager->getZoneGroupBalancingMinute() << "분";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "zone_group_next_balancing_time") { 
		const Timeval& tv = g_pClientManager->getBalanceZoneGroupTime();
		Timeval currentTime;
		getCurrentTime(currentTime);
	    message << "다음 ZoneGroupBalancing 시간: " << (tv.tv_sec - currentTime.tv_sec)/60 << "분 후";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "gamble_item_type_ratio") {
	    message << "갬블 ItemType확률: " << g_pVariableManager->getGambleItemTypeRatio() << "%";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "gamble_item_option_ratio") {
	    message << "갬블 ItemOption확률: " << g_pVariableManager->getGambleItemOptionRatio() << "%";
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "summon_motorcycle") {
	    message << "오토바이 소환 기능 : " << (g_pVariableManager->isSummonMotorcycle()? "ON":"OFF");
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "enemy_limit_time") {
		message << "몬스터 적 유효 시간: " << g_pVariableManager->getEnemyLimitTime() << "초";
		gcSystemMessage.setMessage(message.toString());
 	} else if (set_type == "rare_item_ratio") {
		message << "레어 아이템 루팅 확률: " << g_pVariableManager->getRareItemRatio() << "%";
		gcSystemMessage.setMessage(message.toString());
 	} else if (set_type == "unique_item_ratio") {
		message << "유니크 아이템 루팅 확률: " << g_pVariableManager->getUniqueItemRatio() << "/10000";
		gcSystemMessage.setMessage(message.toString());
 	} else if (set_type == "active_master_lair") {
	    message << "마스터 레어 이벤트 : " << (g_pVariableManager->isActiveMasterLair()? "ON":"OFF");
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "retry_master_lair") {
	    message << "마스터 레어에 죽고 다시 들어가기 : " << (g_pVariableManager->isRetryMasterLair()? "ON":"OFF");
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "harvest_festival_item_ratio") {
		message << "추석 송편 아이템 루팅 확률: 1/" << g_pVariableManager->getHarvestFestivalItemRatio();
		gcSystemMessage.setMessage(message.toString());
 	} else if (set_type == "master_blood_drain_start_hp") {
		message << "흡혈 시작 때의 HP : " << g_pVariableManager->getMasterBloodDrainStartHP() << "%";
		gcSystemMessage.setMessage(message.toString());
 	} else if (set_type == "master_blood_drain_start_bd") {
		message << "흡혈 시작 때의 흡혈 확률 : " << g_pVariableManager->getMasterBloodDrainStartBD() << "%";
		gcSystemMessage.setMessage(message.toString());
 	} else if (set_type == "master_blood_drain_end_hp") {
		message << "흡혈 끝(?) 때의 HP : " << g_pVariableManager->getMasterBloodDrainEndHP() << "%";
		gcSystemMessage.setMessage(message.toString());
 	} else if (set_type == "master_blood_drain_end_bd") {
		message << "흡혈 끝(?) 때의 흡혈 확률 : " << g_pVariableManager->getMasterBloodDrainEndBD() << "%";
		gcSystemMessage.setMessage(message.toString());
 	} else if (set_type == "chief_monster") {
	    message << "치프 몬스터 등장 : " << (g_pVariableManager->isActiveChiefMonster()? "ON":"OFF");
		gcSystemMessage.setMessage(message.toString());
	} else if (set_type == "chief_monster_rare_item_percent") {
		message << "치프 몬스터 레어 아이템 루팅 확률: " << g_pVariableManager->getChiefMonsterRareItemPercent() << "%";
		gcSystemMessage.setMessage(message.toString());
 	} else if (set_type == "newbie_transport_to_guild") {
		message << "능력치 40짜리 길드로 이동시키기: " << (g_pVariableManager->isNewbieTransportToGuild()? "ON":"OFF");
		gcSystemMessage.setMessage(message.toString());
 	} else {
		// by sigi. 2002.11.19
		VariableType vt = g_pVariableManager->getVariableType(set_type);

		if (vt != VARIABLE_MAX) {
			const string& msg = g_pVariableManager->toString(vt);
			gcSystemMessage.setMessage(msg);
		}
        else
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_WRONG_VARIABLE_NAME));
	}

	// 자신에게만 시스템 메시지 보내기 (수정되었을시)
	pGamePlayer->sendPacket(&gcSystemMessage);

	__END_CATCH
}
#endif

#ifdef __GAME_SERVER__


void CGSayHandler::opload(GamePlayer* pGamePlayer, string msg, int i) throw(ProtocolException, Error) {
	__BEGIN_TRY
	
	GCSystemMessage gcSystemMessage;
	StringStream message;

	uint j = msg.find_first_of(' ' , i + 1);
	uint z = msg.find_first_of(' ' , j + 1);
	
	string load_type = msg.substr(j + 1, z - j - 1);
	string load_value = "0";
	
	if (z != string::npos)
		load_value = msg.substr(z + 1);

	if (load_type.length() < 1)
		return;

	if (pGamePlayer != NULL) {
		Creature* pCreature = pGamePlayer->getCreature();
		filelog("change.txt", "[LOAD] %s , %s , %s", pCreature->getName().c_str(), load_type.c_str(), load_value.c_str());
	}

	EventReloadInfo* pEvent = NULL;

	if (load_type == "master_lair_info") {
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::MASTER_LAIR_INFO);
//		StringStream msg;
//		msg << "마스터 레어 정보를 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_MASTER_LAIR_INFO));
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (load_type == "monster_info") {
	//	string MonsterName = load_value;
		string MonsterName = "all";
		int SpriteType = 0;

		// 일단 '이름'으로 몬스터 SpriteType을 찾아본다.
//		int SpriteType = g_pMonsterInfoManager->getSpriteTypeByName(MonsterName);

//		if (SpriteType==0)
//		{
			// 이름으로 없다면.. 값 자체가 숫자(SpriteType)인가?
//			SpriteType = atoi(MonsterName.c_str());
//		}	

		bool bExist = true;
//		try 
//		{
//			g_pMonsterInfoManager->getMonsterTypeBySprite(SpriteType);
//		} catch (Throwable&) {
//			bExist = false;
//		}

//		StringStream msg;
        char msg[100];

		if (bExist || MonsterName=="all") {
			// all인 경우는 SpriteType이 0이다.
			pEvent =  new EventReloadInfo(pGamePlayer,  EventReloadInfo::MONSTER_INFO, SpriteType);

			if (SpriteType!=0) {
                sprintf(msg, g_pStringPool->c_str(STRID_LOAD_MONSTER_INFO), load_value.c_str());
//				msg << "몬스터 정보(" << load_value.c_str() << ")를 load합니다.";
			} else {
                sprintf(msg, g_pStringPool->c_str(STRID_LOAD_ALL_MONSTER_INFO));
//				msg << "모든 몬스터 정보를 load합니다.";
			}
		} else {
            sprintf(msg, g_pStringPool->c_str(STRID_LOAD_WRONG_MONSTER_INFO), load_value.c_str());
//			msg << "몬스터(" << load_value.c_str() << ")가 잘못 지정되었습니다.";
		}

		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (load_type == "monster_ai") {
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::MONSTER_AI);
//		StringStream msg;
//		msg << "몬스터 AI정보(DirectiveSet)를 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_DIRECTIVESET));
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (load_type == "zone_info") {
		pEvent =  new EventReloadInfo(pGamePlayer, EventReloadInfo::ZONE_INFO);
//		StringStream msg;
//		msg << "존 정보를 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_ZONE));
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (load_type == "zone") {
		int zoneID = atoi(load_value.c_str());

		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::ZONE, zoneID);

//		StringStream msg;
//		msg << "존을 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_ZONE));
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (load_type == "log_user") {
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::LOG_USER_INFO);
//		StringStream msg;
//		msg << "LogUser 정보를 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_LOG_USER));
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (load_type == "item_info") {
		Item::ItemClass ItemClass = g_pItemFactoryManager->getItemClassByName(load_value);

		// ItemClass가 MAX라면 이름으로는 못 찾았다는 말이다. 
		// 이 경우에는 아이템 클래스가 숫자로 바로 오지는 않았는지 검사해야 한다.
		if (ItemClass == Item::ITEM_CLASS_MAX) {
			int temp = atoi(load_value.c_str());
			if (temp < 0 || temp >= Item::ITEM_CLASS_MAX)
				return;
			else
				ItemClass = (Item::ItemClass)(temp);
		}

		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::ITEM_INFO, ItemClass);

//		StringStream msg;
//		msg << load_value << " Info를 load합니다.";

        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_LOAD_ITEM_INFO), load_value.c_str());
        gcSystemMessage.setMessage(msg);
	} else if (load_type == "option_info") {
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::OPTION_INFO);
	//	StringStream msg;
	//	msg << "OptionInfo 정보를 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_OPTION_INFO));
	} else if (load_type == "rank_bonus_info") {
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::RANK_BONUS_INFO);
//		StringStream msg;
//		msg << "RankBonusInfo 정보를 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_RANK_BONUS_INFO));
	} else if (load_type == "string_pool") {
        pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::STRING_POOL);
        gcSystemMessage.setMessage("reload StringPool");
    } else if (load_type == "war_schedule_info") {
		ZoneID_t zoneID = atoi(load_value.c_str());

		// 설정 안된 경우는 현재 Creature가 있는 존
		if (zoneID == 0) {
			if (pGamePlayer != NULL) {
				Creature* pCreature = pGamePlayer->getCreature();
				Assert(pCreature != NULL);

				Zone* pZone = pCreature->getZone();
				Assert(pZone != NULL);

				zoneID = pZone->getZoneID();
			}
		}

		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::WAR_SCHEDULE_INFO, zoneID);

		StringStream msg;
		msg << g_pStringPool->getString(STRID_LOAD_WAR_SCHEDULE_INFO);

		if (!g_pVariableManager->isWarActive())
			msg << g_pStringPool->getString(STRID_WAR_OFF);

		gcSystemMessage.setMessage(msg.toString().c_str());

	} else if (load_type == "blood_bible_owner") {
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::BLOOD_BIBLE_OWNER);

//		StringStream msg;
//		msg << "BloodBibleOwner 정보를 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_BLOOD_BIBLE_OWNER_INFO));
	} else if (load_type == "sweeper_owner") {
		int level = atoi(load_value.c_str());
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::SWEEPER_OWNER, level);

//		StringStream msg;
//		msg << "BloodBibleOwner 정보를 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_BLOOD_BIBLE_OWNER_INFO));
	} else if (load_type == "race_war_pc_limit") {
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::RACE_WAR_PC_LIMIT);

//		StringStream msg;
//		msg << "RaceWarPCLimit 정보를 load합니다.";
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_RACE_WAR_PC_LIMIT_INFO));
	} else if (load_type == "npc") {
		ZoneID_t zoneID = (ZoneID_t)(atoi(load_value.c_str()));
		Zone* pZone = getZoneByZoneID(zoneID);
		if (pZone != NULL) {
			// 성 외에는 NPC 가 사라지는 일이 없으니 -_-a
			CastleInfo* pCastleInfo = g_pCastleInfoManager->getCastleInfo(zoneID);

			if (pCastleInfo != NULL)
				pZone->loadNPCs(pCastleInfo->getRace());
		}
	} else if (load_type == "goods_list_info") {
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::GOODS_LIST_INFO);

		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_GOODS_LIST_INFO));
	}
/*	else if (load_type == "quest_info")
	{
		pEvent = new EventReloadInfo(pGamePlayer, EventReloadInfo::QUEST_INFO);

		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_LOAD_QUEST_INFO));
	}*/
	else
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_WRONG_VARIABLE_NAME));

	if (pEvent != NULL)
		g_pClientManager->addEvent(pEvent);

	// 자신에게만 시스템 메시지 보내기 (수정되었을시)
	if (pGamePlayer != NULL)
		pGamePlayer->sendPacket(&gcSystemMessage);

	__END_CATCH
}


void CGSayHandler::opset(GamePlayer* pGamePlayer, string msg, int i) throw(ProtocolException, Error) {
	__BEGIN_TRY

	// [!!!] GGCommand를 통해서 여기가 처리될때에는
	// pGamePlayer가 NULL일 수 있으므로.. 꼭 체크를 해야한다! by sigi.2002.12.23
	
	GCSystemMessage gcSystemMessage;
	StringStream message;

	uint j = msg.find_first_of(' ' , i + 1);
	uint z = msg.find_first_of(' ' , j + 1);
	
	string set_type = msg.substr(j + 1, z - j - 1);
	string set_value = msg.substr(z + 1);

	if (set_value.length() < 1)
		return;

	if (pGamePlayer != NULL) {
		Creature* pCreature = pGamePlayer->getCreature();
		filelog("change.txt", "%s , %s , %s", pCreature->getName().c_str(), set_type.c_str(), set_value.c_str());
	}

	if (set_type == "star") {
		g_pVariableManager->setStar(atoi(set_value.c_str()));
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_STAR_RATIO), set_value.c_str());
//	    message << "별 출현 확률이 1/" << set_value << "로 수정되었습니다";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "event_activate") {
		if (set_value == "start") {
			g_pVariableManager->setEventActivate(1);
//	    	message << "이벤트가 시작되었습니다";
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_EVENT_START));
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		} else {
			g_pVariableManager->setEventActivate(0);
//	    	message << "이벤트가 중지되었습니다";
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_EVENT_END));
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
	} else if (set_type == "event_ratio") {
		g_pVariableManager->setEventRatio(atoi(set_value.c_str()));
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_EVENT_MONSTER_RATIO), set_value.c_str());
//	    message << "이벤트 몬스터 출현 확률이 의해서 1/" << set_value << "로 수정되었습니다";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "exp_ratio") {
		int value = atoi(set_value.c_str());
		if (value < 100)
			return;

		g_pVariableManager->setExpRatio(value);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_EXP_RATIO), set_value.c_str());
//	    message << "경험치 획득률이" << set_value << "%로 수정되었습니다";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "item_prob_ratio") {
		int value = atoi(set_value.c_str());
		if (value < value)
			return;

		g_pVariableManager->setItemProbRatio(value);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_ITEM_PROBE_RATIO), set_value.c_str());
//	    message << "아이템 획득률이 " << set_value << "%로 수정되었습니다";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "combat_bonus_time") {
		int bonusTime = atoi(set_value.c_str());
		if (bonusTime < 1 || bonusTime > 14400)
			return;

		g_pVariableManager->setCombatBonusTime(bonusTime);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_COMBAT_BONUS_TIME), set_value.c_str());
//	    message << "전쟁 승리 유지시간이 " << set_value << "분으로 수정되었습니다";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "combat_bonus_slayer_hp_ratio") {
		int bonus= atoi(set_value.c_str());
		if (bonus < 0 || bonus > 100)
			return;

		g_pVariableManager->setCombatSlayerHPBonusRatio(bonus);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_COMBAT_SLAYER_BONUS_HP), set_value.c_str());
//	    message << "슬레이어 전쟁 HP보너스가 +" << set_value << "% 로 수정되었습니다";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "combat_bonus_vampire_hp_ratio") {
		int bonus= atoi(set_value.c_str());
		if (bonus < 0 || bonus > 100)
			return;

		g_pVariableManager->setCombatVampireHPBonusRatio(bonus);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_COMBAT_VAMPIRE_BONUS_HP), set_value.c_str());
//	    message << "뱀파이어 전쟁 HP보너스가 +" << set_value << "% 로 수정되었습니다";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "combat_bonus_slayer_damage") {
		int bonus= atoi(set_value.c_str());
		if (bonus < 0 || bonus > 20)
			return;

		g_pVariableManager->setCombatSlayerDamageBonus(bonus);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_COMBAT_SLAYER_BONUS_DAMAGE), set_value.c_str());
//	    message << "슬레이어 전쟁 데미지보너스가 +" << set_value << " 로 수정되었습니다";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "combat_bonus_vampire_damage") {
		int bonus= atoi(set_value.c_str());
		if (bonus < 0 || bonus > 20)
			return;

		g_pVariableManager->setCombatVampireDamageBonus(bonus);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_COMBAT_VAMPIRE_BONUS_DAMAGE), set_value.c_str());
//	    message << "뱀파이어 전쟁 데미지보너스가 +" << set_value << " 로 수정되었습니다";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "premium_exp_bonus_percent") {
		int bonus= atoi(set_value.c_str());
		if (bonus < 100 || bonus > 1000)
			return;

		g_pVariableManager->setPremiumExpBonusPercent(bonus);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_PREMIUM_EXP_RATIO), set_value.c_str());
//	    message << "프리미엄 사용자의 경험치 보너스가 " << set_value << "% 로 수정되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "premium_item_probe_percent") {
		int bonus= atoi(set_value.c_str());
		if (bonus < 100 || bonus > 1000)
			return;

		g_pVariableManager->setPremiumItemProbePercent(bonus);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_PREMIUM_ITEM_PROBE_RATIO), set_value.c_str());
//	    message << "프리미엄 존의 아이템 루팅 확률이 " << set_value << "% 로 수정되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "zone_group_balancing_minute") {
		int minute= atoi(set_value.c_str());
		if (minute <= 0 || minute > 30*24*60)
			return;

		g_pVariableManager->setZoneGroupBalancingMinute(minute);
		g_pClientManager->setBalanceZoneGroup(minute);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_ZONE_GROUP_BALANCING_TIME), set_value.c_str());
//	    message << "ZoneGroupBalancing 주기가 " << set_value << "분으로 수정되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "gamble_item_type_ratio") {
		int ratio= atoi(set_value.c_str());
		if (ratio <= 10 || ratio > 1000)
			return;

		g_pVariableManager->setGambleItemTypeRatio(ratio);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_GAMBLE_ITEM_TYPE_RATIO), set_value.c_str());
//	    message << "갬블 ItemType확률이 " << set_value << "%로 수정되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "gamble_item_option_ratio") {
		int ratio= atoi(set_value.c_str());
		if (ratio <= 10 || ratio > 1000)
			return;

		g_pVariableManager->setGambleItemOptionRatio(ratio);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_GAMBLE_ITEM_OPTION_RATIO), set_value.c_str());
//	    message << "갬블 ItemOption확률이 " << set_value << "%로 수정되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "summon_motorcycle") {
		if (set_value == "on") {
			g_pVariableManager->setSummonMotorcycle(true);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_SUMMON_MOTORCYCLE), "ON");
//			message << "오토바이 소환 기능이 ON 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		} else if (set_value == "off") {
			g_pVariableManager->setSummonMotorcycle(false);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_SUMMON_MOTORCYCLE), "OFF");
//			message << "오토바이 소환 기능이 OFF 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
	} else if (set_type == "enemy_limit_time") {
		int enemy_limit_time = atoi(set_value.c_str());
		if (enemy_limit_time <= 180 || enemy_limit_time >= 3600)
			return;

		g_pVariableManager->setEnemyLimitTime(enemy_limit_time);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_MONSTER_FORGET_TIME), set_value.c_str());
//	    message << "몬스터의 연속 적 인식시간이" << set_value << "초로 수정되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "rare_item_ratio") {
		int ratio = atoi(set_value.c_str());
		if (ratio < 0 || ratio > 100000)
			return;

		g_pVariableManager->setRareItemRatio(ratio);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_RARE_ITEM_RATIO), set_value.c_str());
//		message << "레어 아이템 루팅 확률이 " << set_value << "%로 수정되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "unique_item_ratio") {
		int ratio = atoi(set_value.c_str());
		if (ratio < 0 || ratio > 10000)
			return;

		g_pVariableManager->setUniqueItemRatio(ratio);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_UNIQUE_ITEM_RATIO), set_value.c_str());
//		message << "유니크 아이템 루팅 확률이 " << set_value << "/10000 으로 수정되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "active_master_lair") {
		MasterLairManager* pMasterLairManager = NULL;

		if (pGamePlayer != NULL) {
			Creature* pCreature = pGamePlayer->getCreature();
			Zone* pZone = pCreature->getZone();
			pMasterLairManager = pZone->getMasterLairManager();
		}

		if (set_value=="on") {
			g_pVariableManager->setActiveMasterLair(true);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_MASTER_LAIR_ACTIVATE), "ON");
//			message << "마스터 레어 이벤트가 ON 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());

			if (pMasterLairManager != NULL)
				pMasterLairManager->startEvent();
		} else if (set_value=="off") {
			g_pVariableManager->setActiveMasterLair(false);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_MASTER_LAIR_ACTIVATE), "OFF");
//			message << "마스터 레어 이벤트가 OFF 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());

			if (pMasterLairManager != NULL)
				pMasterLairManager->stopEvent();
		}
	} else if (set_type == "retry_master_lair") {
		if (set_value=="on")
		{
			g_pVariableManager->setRetryMasterLair(true);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_RETRY_MASTER_LAIR), "ON");
//			message << "마스터 레어에 죽고 다시 들어가기가 ON 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
		else if (set_value=="off")
		{
			g_pVariableManager->setRetryMasterLair(false);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_RETRY_MASTER_LAIR), "OFF");
//			message << "마스터 레어에 죽고 다시 들어가기가 OFF 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
	} else if (set_type == "harvest_festival_item_ratio") {
		int ratio = atoi(set_value.c_str());
		if (ratio < 0 || ratio > 10000)
			return;

		g_pVariableManager->setHarvestFestivalItemRatio(ratio);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_HARVEST_FESTIVAL_ITEM_RATIO), set_value.c_str());
//		message << "추석 송편 아이템 루팅 확률이 1/" << ratio << "이 되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "master_blood_drain_start_hp") {
		int percent = atoi(set_value.c_str());
		if (percent < 0 || percent > 100)
			return;

		g_pVariableManager->setMasterBloodDrainStartHP(percent);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_MASTER_BLOOD_DRAIN_START_HP), set_value.c_str());
//		message << "마스터의 흡혈 시작 HP가 " << percent << "%가 되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "master_blood_drain_start_bd") {
		int percent = atoi(set_value.c_str());
		if (percent < 0 || percent > 100)
			return;

		g_pVariableManager->setMasterBloodDrainStartBD(percent);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_MASTER_BLOOD_DRAIN_START_BD), set_value.c_str());
//		message << "마스터의 흡혈 시작 때의 흡혈 확률이 " << percent << "%가 되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "master_blood_drain_end_hp") {
		int percent = atoi(set_value.c_str());
		if (percent < 0 || percent > 100)
			return;

		g_pVariableManager->setMasterBloodDrainEndHP(percent);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_MASTER_BLOOD_DRAIN_END_HP), set_value.c_str());
//		message << "마스터의 흡혈 끝(?) HP가 " << percent << "%가 되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "master_blood_drain_end_bd") {
		int percent = atoi(set_value.c_str());
		if (percent < 0 || percent > 100)
			return;

		g_pVariableManager->setMasterBloodDrainEndBD(percent);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_MASTER_BLOOD_DRAIN_END_BD), set_value.c_str());
//		message << "마스터의 흡혈 끝(?) 상태의 흡혈 확률이 " << percent << "%가 되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "chief_monster") {
		if (set_value == "on") {
			g_pVariableManager->setActiveChiefMonster(true);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_CHIEF_MONSTER), "ON");
//			message << "치프 몬스터 기능이 ON 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
		else if (set_value == "off") {
			g_pVariableManager->setActiveChiefMonster(false);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_CHIEF_MONSTER), "OFF");
//			message << "치프 몬스터 기능이 OFF 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
	} else if (set_type == "chief_monster_rare_item_percent") {
		int ratio = atoi(set_value.c_str());
		if (ratio < 0 || ratio > 100)
			return;

		g_pVariableManager->setChiefMonsterRareItemPercent(ratio);
        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_SET_CHIEF_MONSTER_RARE_ITEM_RATIO), set_value.c_str());
//		message << "치프 몬스터 레어 아이템 루팅 확률이 " << set_value << "%로 수정되었습니다.";
		gcSystemMessage.setMessage(msg);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
	} else if (set_type == "newbie_transport_to_guild") {
		if (set_value == "on") {
			g_pVariableManager->setNewbieTransportToGuild(true);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_NEWBIE_TRANSPORT_TO_GUILD), "ON");
//			message << "능력치 40짜리, 길드로 이동시키기 기능이 ON 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		} else if (set_value == "off") {
			g_pVariableManager->setNewbieTransportToGuild(false);
            char msg[100];
            sprintf(msg, g_pStringPool->c_str(STRID_SET_NEWBIE_TRANSPORT_TO_GUILD), "OFF");
//			message << "능력치 40짜리, 길드로 이동시키기 기능이 OFF 되었습니다.";
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
	} else if (set_type == "xmas_2002") {
		uint l = set_value.find_first_of(' ' , 0);
		uint m = set_value.find_first_of(' ' , l + 1);
		
		string value_firecracker = set_value.substr(0, l);
		string value_treepart = set_value.substr(l + 1, m - l - 1);
		string value_giftbox = set_value.substr(m + 1);

		g_pVariableManager->setVariable(CHRISTMAS_FIRE_CRACKER_RATIO, value_firecracker);
		const string& msg1 = g_pVariableManager->toString(CHRISTMAS_FIRE_CRACKER_RATIO);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		gcSystemMessage.setMessage(msg1);
		
		if (pGamePlayer != NULL)
			pGamePlayer->sendPacket(&gcSystemMessage);

		g_pVariableManager->setVariable(CHRISTMAS_TREE_PART_RATIO, value_treepart);
		const string& msg2 = g_pVariableManager->toString(CHRISTMAS_TREE_PART_RATIO);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		gcSystemMessage.setMessage(msg2);

		if (pGamePlayer != NULL)
			pGamePlayer->sendPacket(&gcSystemMessage);

		g_pVariableManager->setVariable(CHRISTMAS_GIFT_BOX_RATIO, value_giftbox);
		const string& msg3 = g_pVariableManager->toString(CHRISTMAS_GIFT_BOX_RATIO);
		filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());

		gcSystemMessage.setMessage(msg3);

	} else if (set_type =="ItemTaxRatio") {
		if (pGamePlayer != NULL) {
			Creature* pCreature = pGamePlayer->getCreature();
			Zone* pZone = pCreature->getZone();
			if (set_value.size() != 0) {
				int Tax = atoi(set_value.c_str());
				g_pCastleInfoManager->setItemTaxRatio(pZone, Tax);
			}
            else
				gcSystemMessage.setMessage(g_pStringPool->getString(STRID_WRONG_ITEM_TAX_RATIO));
		}
	} else {
		// by sigi. 2002.11.19
		VariableType vt = g_pVariableManager->getVariableType(set_type);

		if (vt != VARIABLE_MAX) {
			g_pVariableManager->setVariable(vt, set_value);
			const string& msg = g_pVariableManager->toString(vt);
			gcSystemMessage.setMessage(msg);
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());

			if (vt == PREMIUM_HALF_EVENT) {
				GCNoticeEvent gcNoticeEvent;

				if (g_pVariableManager->getVariable(vt))
					gcNoticeEvent.setCode(NOTICE_EVENT_PREMIUM_HALF_START);
				else
					gcNoticeEvent.setCode(NOTICE_EVENT_PREMIUM_HALF_END);

				// 유료존에만 뿌린다.
				getZoneByZoneID(61)->broadcastPacket(&gcNoticeEvent);		// 페로나북동
				getZoneByZoneID(64)->broadcastPacket(&gcNoticeEvent);		// 페로나남서
				getZoneByZoneID(1007)->broadcastPacket(&gcNoticeEvent);		// 라센성내부(바토리마을)
			} else if (vt == TODAY_IS_HOLYDAY) {
				GCNoticeEvent gcNoticeEvent;
				gcNoticeEvent.setCode(NOTICE_EVENT_HOLYDAY);
				gcNoticeEvent.setParameter(g_pVariableManager->getVariable(vt));

				g_pZoneGroupManager->broadcast(&gcNoticeEvent);
			} else if (vt == CROWN_PRICE) {
				GCNoticeEvent gcNoticeEvent;
				gcNoticeEvent.setCode(NOTICE_EVENT_CROWN_PRICE);
				gcNoticeEvent.setParameter(g_pVariableManager->getVariable(vt));

				g_pZoneGroupManager->broadcast(&gcNoticeEvent);
			}
		}
		else
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_WRONG_VARIABLE_NAME));
	}

	// 자신에게만 시스템 메시지 보내기 (수정되었을시)
	if (pGamePlayer != NULL)
		pGamePlayer->sendPacket(&gcSystemMessage);

	__END_CATCH
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opzone(string msg, int i) throw(ProtocolException, Error) {
	__BEGIN_TRY

	//////////////
	// Zone Info
	//////////////
/*	ZoneInfo* pZoneInfo = new ZoneInfo();
	pZoneInfo->setZoneID(10001);
	pZoneInfo->setZoneGroupID(6);
	pZoneInfo->setZoneType("NORMAL_FIELD");
	pZoneInfo->setZoneLevel(0);
	pZoneInfo->setZoneAccessMode("PUBLIC");
	pZoneInfo->setZoneOwnerID("");
	pZoneInfo->setPayPlay("");

	pZoneInfo->setSMPFilename("team_hdqrs.smp");
	pZoneInfo->setSSIFilename("team_hdqrs.ssi");
	pZoneInfo->setFullName("team");
	pZoneInfo->setShortName("team");

	g_pZoneInfoManager->addZoneInfo(pZoneInfo);

	/////////
	// Zone
	/////////
	Zone* pZone = new Zone(10001);
	Assert(pZone != NULL);

	ZoneGroup* pZoneGroup = g_pZoneGroupManager->getZoneGroup(6);
	Assert(pZoneGroup != NULL);

	pZone->setZoneGroup(pZoneGroup);
	pZoneGroup->addZone(pZone);
	pZone->init();
*/
	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opguild(string msg, int i, Creature* pCreature) throw(ProtocolException, Error) {
	__BEGIN_TRY

	i = i + 7;

	uint j = msg.find_first_of(' ', i);
	string command = msg.substr(i, j - i);
	string variable = trim(msg.substr(j+1));

	GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(pCreature->getPlayer());

	if (command == "changeGuildMaster")
	{
		int a = variable.find_first_of(' ', 0);
		GuildID_t guildID = atoi(variable.substr(0, a).c_str());
		string master = trim(variable.substr(a+1));

		Guild* pGuild = g_pGuildManager->getGuild(guildID);
		if (pGuild == NULL) 
		{
			GCSystemMessage msg;
			msg.setMessage("No such Guild");
			pGamePlayer->sendPacket(&msg);

			return;
		}

		GuildMember* pGuildMember = pGuild->getMember(master);
		if (pGuildMember == NULL)
		{
			GCSystemMessage msg;
			msg.setMessage("No such Guild Member");
			pGamePlayer->sendPacket(&msg);

			return;
		}

		if (pGuildMember->getRank() != GuildMember::GUILDMEMBER_RANK_NORMAL
		  && pGuildMember->getRank() != GuildMember::GUILDMEMBER_RANK_SUBMASTER)
		{
			GCSystemMessage msg;
			msg.setMessage("New Guild Member must be submaster or normal member");
			pGamePlayer->sendPacket(&msg);

			return;
		}

		Statement* pStmt = NULL;
		Result* pResult = NULL;

		BEGIN_DB
		{
			pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
			
			if (pGuild->getRace() == Guild::GUILD_RACE_SLAYER)
			{
				pResult = pStmt->executeQuery("SELECT Fame, BladeLevel, SwordLevel, GunLevel, HealLevel, EnchantLevel FROM Slayer WHERE Name = '%s'", master.c_str());
				int i = 0;

				if (!pResult->next())
				{
					SAFE_DELETE(pStmt);
					return;
				}

				Fame_t			Fame			= pResult->getInt(++i);
				SkillLevel_t 	BladeLevel		= pResult->getInt(++i);
				SkillLevel_t 	SwordLevel		= pResult->getInt(++i);
				SkillLevel_t 	GunLevel		= pResult->getInt(++i);
				SkillLevel_t 	HealLevel		= pResult->getInt(++i);
				SkillLevel_t 	EnchantLevel	= pResult->getInt(++i);

				SkillDomainType_t	highestDomain;
				SkillLevel_t		maxLevel;

				if (BladeLevel > SwordLevel)
				{
					maxLevel = BladeLevel;
					highestDomain = SKILL_DOMAIN_BLADE;
				}
				else
				{
					maxLevel = SwordLevel;
					highestDomain = SKILL_DOMAIN_SWORD;
				}
				if (GunLevel > maxLevel)
				{
					maxLevel = GunLevel;
					highestDomain = SKILL_DOMAIN_GUN;
				}
				if (HealLevel > maxLevel)
				{
					maxLevel = HealLevel;
					highestDomain = SKILL_DOMAIN_HEAL;
				}
				if (EnchantLevel > maxLevel)
				{
					maxLevel = EnchantLevel;
					highestDomain = SKILL_DOMAIN_ENCHANT;
				}

				if (maxLevel < REQUIRE_SLAYER_MASTER_SKILL_DOMAIN_LEVEL)
				{
					SAFE_DELETE(pStmt);

					GCSystemMessage msg;
					msg.setMessage("Master Level Limit Error");
					pGamePlayer->sendPacket(&msg);

					return;
				}

				if (Fame < REQUIRE_SLAYER_MASTER_FAME[highestDomain])
				{
					SAFE_DELETE(pStmt);

					GCSystemMessage msg;
					msg.setMessage("Master Fame Limit Error");
					pGamePlayer->sendPacket(&msg);

					return;
				}
			}
			else if (pGuild->getRace() == Guild::GUILD_RACE_VAMPIRE) 
			{
				pResult = pStmt->executeQuery("SELECT Level FROM Vampire WHERE Name = '%s'", master.c_str());

				if (!pResult->next())
				{
					SAFE_DELETE(pStmt);
					return;
				}
				
				Level_t Level = pResult->getInt(1);
				if (Level < REQUIRE_VAMPIRE_MASTER_LEVEL)
				{
					SAFE_DELETE(pStmt);

					GCSystemMessage msg;
					msg.setMessage("Master Level Limit Error");
					pGamePlayer->sendPacket(&msg);

					return;
				}
			}
			else if (pGuild->getRace() == Guild::GUILD_RACE_OUSTERS) 
			{
				pResult = pStmt->executeQuery("SELECT Level FROM Ousters WHERE Name = '%s'", master.c_str());

				if (!pResult->next())
				{
					SAFE_DELETE(pStmt);
					return;
				}

				Level_t Level = pResult->getInt(1);
				if (Level < REQUIRE_OUSTERS_MASTER_LEVEL)
				{
					SAFE_DELETE(pStmt);

					GCSystemMessage msg;
					msg.setMessage("Master Level Limit Error");
					pGamePlayer->sendPacket(&msg);

					return;
				}
			}
			else
			{
				SAFE_DELETE(pStmt);
				return;
			}

			SAFE_DELETE(pStmt);
		}
		END_DB(pStmt)

		GSModifyGuildMember gsPacket;
		gsPacket.setGuildID(pGuild->getID());
		gsPacket.setName(master);
		gsPacket.setGuildMemberRank(GuildMember::GUILDMEMBER_RANK_MASTER);
		gsPacket.setSender(pCreature->getName());

		g_pSharedServerManager->sendPacket(&gsPacket);
	}

	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opsave(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	GCSystemMessage gcSystemMessage;

	gcSystemMessage.setMessage(g_pStringPool->getString(STRID_SAVE_YOUR_DATA));

	Statement* pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();

	Result* pResult = pStmt->executeQuery("SELECT MAX(ZoneGroupID) FROM ZoneGroupInfo");

	pResult->next();

	BYTE GroupCount = pResult->getInt(1) + 1;

	for (int i = 1; i < GroupCount; i++) 
	{
		ZoneGroup* pZoneGroup = NULL;

		try 
		{
			pZoneGroup = g_pZoneGroupManager->getZoneGroup(i);
		} 
		catch (NoSuchElementException&) 
		{
			SAFE_DELETE(pStmt);
			throw Error("Critical Error : ZoneInfoManager에 해당 존그룹이 존재하지 않습니다.");
		}

		ZonePlayerManager* pZonePlayerManager = pZoneGroup->getZonePlayerManager();

		pZonePlayerManager->broadcastPacket(&gcSystemMessage);
		pZonePlayerManager->save();
	}

	SAFE_DELETE(pStmt);

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opwall(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);

	GCSystemMessage gcSystemMessage;

	gcSystemMessage.setType(SYSTEM_MESSAGE_OPERATOR);
	gcSystemMessage.setMessage(msg.substr(j+1, msg.size()-j-1).c_str());

	if (pGamePlayer != NULL)
	{
		Creature* pCreature = pGamePlayer->getCreature();
		filelog("change.txt", "[Wall]%s, %s", pCreature->getName().c_str(), gcSystemMessage.getMessage().c_str());
	}

	g_pZoneGroupManager->broadcast(&gcSystemMessage);

	/*
	Statement* pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();

	Result* pResult = pStmt->executeQuery("SELECT MAX(ZoneGroupID) FROM ZoneGroupInfo");

	pResult->next();

	BYTE GroupCount = pResult->getInt(1) + 1;

	for (int i = 1; i < GroupCount; i++) {

		ZoneGroup* pZoneGroup;

		try 
		{
			pZoneGroup = g_pZoneGroupManager->getZoneGroup(i);
		} 
		catch (NoSuchElementException&) 
		{
			SAFE_DELETE(pStmt);
			throw Error("Critical Error : ZoneInfoManager에 해당 존그룹이 존재하지 않습니다.");
		}

		ZonePlayerManager* pZonePlayerManager = pZoneGroup->getZonePlayerManager();

		pZonePlayerManager->broadcastPacket_NOBLOCKED(&gcSystemMessage);

	}

	SAFE_DELETE(pStmt);
	*/

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opshutdown(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	// 아무 zone에나 붙이면 되는데..
	// multithread때메.. lock잘 걸고 effect를 붙여야 한다.
	uint j = msg.find_first_of(' ' , i+1);
	Turn_t dTime = atoi(msg.substr(j+1, msg.size()-j-1).c_str());

	if (pGamePlayer != NULL)
	{
		Creature* pCreature = pGamePlayer->getCreature();
		filelog("change.txt", "[ShutDown]%s, %d", pCreature->getName().c_str(), (int)dTime);
	}

	EffectShutDown* pEffectShutDown = new EffectShutDown();
	pEffectShutDown->setNextTime(100);
	pEffectShutDown->setDelay(100);
	pEffectShutDown->setDeadline(dTime);


	if (pGamePlayer != NULL)
	{
		Creature* pCreature = pGamePlayer->getCreature();

		Zone* pZone = pCreature->getZone();
		(pZone->getObjectRegistry()).registerObject(pEffectShutDown);

		pZone->addEffect(pEffectShutDown);
	}
	else
	{
		int ZoneNum = 1003;

		ZoneInfo* pZoneInfo = NULL;
		try {
			pZoneInfo = g_pZoneInfoManager->getZoneInfo(ZoneNum);
		} catch (NoSuchElementException&) {
			throw Error("shutdown할려는데 zoneID없다");
		}

		ZoneGroup* pZoneGroup = NULL;

		try {
			pZoneGroup = g_pZoneGroupManager->getZoneGroup(pZoneInfo->getZoneGroupID());
		} catch (NoSuchElementException&) {
			throw Error("shutdown할려는데 zoneGroup에 뭐 없다.");
		}

		Zone* pZone = pZoneGroup->getZone(ZoneNum);

		(pZone->getObjectRegistry()).registerObject(pEffectShutDown);

		pZone->addEffect_LOCKING(pEffectShutDown);
	}

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opkick(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);
	string Name = msg.substr(j+1, msg.size()-j-1).c_str();

	Creature* pTargetCreature = NULL;

	// NoSuch제거. by sigi. 2002.5.2
	__ENTER_CRITICAL_SECTION((*g_pPCFinder))

	pTargetCreature = g_pPCFinder->getCreature_LOCKED(Name);
	if (pTargetCreature==NULL) 
	{
		g_pPCFinder->unlock();
		return;
	}

	Player* pTargetPlayer = pTargetCreature->getPlayer();
	GamePlayer* pTargetGamePlayer = dynamic_cast<GamePlayer*>(pTargetPlayer);
	pTargetGamePlayer->setPenaltyFlag(PENALTY_TYPE_KICKED);
	pTargetGamePlayer->setItemRatioBonusPoint(1);

	if (pGamePlayer != NULL)
	{
		Creature* pCreature = pGamePlayer->getCreature();
		filelog("change.txt", "[Kick]%s --> %s", pCreature->getName().c_str(), Name.c_str());
	}

	__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opmute(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint time = 0;
	uint j = msg.find_first_of(' ' , i+1);
	uint k = msg.find_first_of(' ' , j+1);
	string Name;
	if (k < msg.size())
	{
		Name = msg.substr(j+1, k-j-1).c_str();
		time = (uint)atoi(msg.substr(k+1,msg.size()-k-1).c_str());
	}
	else
	{
		Name = msg.substr(j+1, msg.size()-j-1).c_str();
	}

	Creature* pTargetCreature = NULL;
	// NoSuch제거. by sigi. 2002.5.2
	__ENTER_CRITICAL_SECTION((*g_pPCFinder))

	pTargetCreature = g_pPCFinder->getCreature_LOCKED(Name);
	if (pTargetCreature==NULL) 
	{
		g_pPCFinder->unlock();
		return;
	}

	Player* pTargetPlayer = pTargetCreature->getPlayer();
	GamePlayer* pTargetGamePlayer = dynamic_cast<GamePlayer*>(pTargetPlayer);

	if (time == 0)
	{
		pTargetGamePlayer->setPenaltyFlag(PENALTY_TYPE_MUTE);
	}
	else
	{
		Effect* pEffect = NULL;
		if (pTargetCreature->isFlag(Effect::EFFECT_CLASS_MUTE))
		{
			pEffect = pTargetCreature->findEffect(Effect::EFFECT_CLASS_MUTE);

			pEffect->setDeadline(time*600);
			pEffect->save(pTargetCreature->getName());
		}
		else
		{
			pEffect = new EffectMute(pTargetCreature);
			pEffect->setDeadline(time*600);
			pEffect->create(pTargetCreature->getName());
			pTargetCreature->addEffect(pEffect);
			pTargetCreature->setFlag(pEffect->getEffectClass());
		}

		GCAddEffect gcAddEffect;
		gcAddEffect.setObjectID(pTargetCreature->getObjectID());
		gcAddEffect.setEffectID(pEffect->getSendEffectClass());
		gcAddEffect.setDuration(time*600);

		pTargetGamePlayer->sendPacket(&gcAddEffect);

	}

	__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opdenychat(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint level = 0;
	uint time = 0;
	uint j = msg.find_first_of(' ' , i+1);
	uint k = msg.find_first_of(' ' , j+1);
	string Name;
	if (k < msg.size())
	{
		level = (uint)atoi(msg.substr(j+1,k-j-1).c_str());
		Name = msg.substr(k+1,msg.size()-k-1).c_str();
	}
	else
	{
		return;
	}

	Creature* pTargetCreature = NULL;

	if (level == 1)
	{
		// level1 = 1시간
		time = 1 * 60;
	}
	else if (level == 2)
	{
		// level2 = 6시간
		time = 6 * 60;
	}
	else if (level == 3)
	{
		// level3 = 12시간
		time = 12 * 60;
	}
	else if (level == 4)
	{
		// level4 = 24시간
		time = 24 * 60;
	}
	else if (level == 5)
	{
		// level5 = 168시간
		time = 168 * 60;
	}

	// NoSuch제거. by sigi. 2002.5.2
	__ENTER_CRITICAL_SECTION((*g_pPCFinder))

	pTargetCreature = g_pPCFinder->getCreature_LOCKED(Name);
	if (pTargetCreature==NULL) 
	{
		g_pPCFinder->unlock();
		return;
	}

	Player* pTargetPlayer = pTargetCreature->getPlayer();
	GamePlayer* pTargetGamePlayer = dynamic_cast<GamePlayer*>(pTargetPlayer);

	if (time == 0)
	{
		pTargetGamePlayer->setPenaltyFlag(PENALTY_TYPE_MUTE);
	}
	else
	{
		Effect* pEffect = NULL;
		if (pTargetCreature->isFlag(Effect::EFFECT_CLASS_MUTE))
		{
			pEffect = pTargetCreature->findEffect(Effect::EFFECT_CLASS_MUTE);

			pEffect->setDeadline(time*600);
			pEffect->save(pTargetCreature->getName());
		}
		else
		{
			pEffect = new EffectMute(pTargetCreature);
			pEffect->setDeadline(time*600);
			pEffect->create(pTargetCreature->getName());
			pTargetCreature->addEffect(pEffect);
			pTargetCreature->setFlag(pEffect->getEffectClass());
		}

		GCAddEffect gcAddEffect;
		gcAddEffect.setObjectID(pTargetCreature->getObjectID());
		gcAddEffect.setEffectID(pEffect->getSendEffectClass());
		gcAddEffect.setDuration(time*600);

		pTargetGamePlayer->sendPacket(&gcAddEffect);

	}

	__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opfreezing(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);
	string Name = msg.substr(j+1, msg.size()-j-1).c_str();

	Creature* pTargetCreature = NULL;

	// NoSuch제거. by sigi. 2002.5.2
	__ENTER_CRITICAL_SECTION((*g_pPCFinder))

	pTargetCreature = g_pPCFinder->getCreature_LOCKED(Name);
	if (pTargetCreature==NULL) 
	{
		g_pPCFinder->unlock();
		return;
	}

	Player* pTargetPlayer = pTargetCreature->getPlayer();
	GamePlayer* pTargetGamePlayer = dynamic_cast<GamePlayer*>(pTargetPlayer);
	pTargetGamePlayer->setPenaltyFlag(PENALTY_TYPE_FREEZING);

	__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opdeny(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);
	string Name = msg.substr(j+1, msg.size()-j-1).c_str();

	Statement* pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
	Result* pResult = pStmt->executeQuery("SELECT PlayerID FROM Slayer where Name='%s'", Name.c_str());

	string PlayerID;

	if (pResult->next()) 
	{
		string PlayerID = pResult->getString(1);

		Statement* pStmt2 = g_pDatabaseManager->getConnection((int)Thread::self())->createStatement();
		pStmt2->executeQuery("UPDATE Player set Access='DENY' where PlayerID ='%s'", PlayerID.c_str());
		SAFE_DELETE(pStmt2);
	}

	SAFE_DELETE(pStmt);

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opinfo(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;
	
	uint j = msg.find_first_of(' ' , i+1);
	string Name = msg.substr(j+1, msg.size()-j-1).c_str();

	Creature* pTargetCreature = NULL;
	// NoSuch제거. by sigi. 2002.5.2
	__ENTER_CRITICAL_SECTION((*g_pPCFinder))

	pTargetCreature = g_pPCFinder->getCreature_LOCKED(Name);
	if (pTargetCreature==NULL) 
	{
		g_pPCFinder->unlock();
		return;
	}

	StringStream msg;

	msg << "Name : " << Name
		<< " Host : " << pTargetCreature->getPlayer()->getSocket()->getHost();

	if (pTargetCreature->isSlayer()) {

		Slayer* pSlayer = dynamic_cast<Slayer*>(pTargetCreature);

		msg << " STR : " << (int)pSlayer->getSTR() << " / " << pSlayer->getSTR(ATTR_MAX)
			<< " DEX : " << (int)pSlayer->getDEX() << " / " << pSlayer->getDEX(ATTR_MAX)
			<< " INT : " << (int)pSlayer->getINT() << " / " << pSlayer->getINT(ATTR_MAX)
			<< " DAM : " << (int)pSlayer->getDamage() << " / " << pSlayer->getDamage(ATTR_MAX) << " / " << pSlayer->getDamage(ATTR_BASIC)
			<< " Defense  : " << (int)pSlayer->getDefense()
			<< " TOHIT : " << (int)pSlayer->getToHit()
			<< " HP : " << (int)pSlayer->getHP() << " / " << pSlayer->getHP(ATTR_MAX)
			<< " MP : " << (int)pSlayer->getMP() << " / " << pSlayer->getMP(ATTR_MAX)
			<< " Fame : " << (int)pSlayer->getFame()
			<< " Gold : " << (int)pSlayer->getGold()
			<< " StashGold : " << (int)pSlayer->getStashGold();

	} else if (pTargetCreature->isVampire()) {

		Vampire* pVampire= dynamic_cast<Vampire*>(pTargetCreature);

		msg << " STR : " << pVampire->getSTR() << " / " << pVampire->getSTR(ATTR_MAX)
			<< " DEX : " << pVampire->getDEX() << " / " << pVampire->getDEX(ATTR_MAX)
			<< " INT : " << pVampire->getINT() << " / " << pVampire->getINT(ATTR_MAX)
			<< " Defense  : " << (int)pVampire->getDefense()
			<< " TOHIT : " << (int)pVampire->getToHit()
			<< " HP : " << (int)pVampire->getHP() << " / " << pVampire->getHP(ATTR_MAX)
			<< " Fame : " << (int)pVampire->getFame()
			<< " Gold : " << (int)pVampire->getGold()
			<< " StashGold : " << (int)pVampire->getStashGold();

	} else if (pTargetCreature->isOusters()) {

		Ousters* pOusters= dynamic_cast<Ousters*>(pTargetCreature);

		msg << " STR : " << pOusters->getSTR() << " / " << pOusters->getSTR(ATTR_MAX)
			<< " DEX : " << pOusters->getDEX() << " / " << pOusters->getDEX(ATTR_MAX)
			<< " INT : " << pOusters->getINT() << " / " << pOusters->getINT(ATTR_MAX)
			<< " Defense  : " << (int)pOusters->getDefense()
			<< " TOHIT : " << (int)pOusters->getToHit()
			<< " HP : " << (int)pOusters->getHP() << " / " << pOusters->getHP(ATTR_MAX)
			<< " Fame : " << (int)pOusters->getFame()
			<< " Gold : " << (int)pOusters->getGold()
			<< " StashGold : " << (int)pOusters->getStashGold();

	}

	GCSystemMessage gcSystemMessage;

	gcSystemMessage.setMessage(msg.toString());

	pGamePlayer->sendPacket (&gcSystemMessage);

	__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opfind(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;
	uint j = msg.find_first_of(' ' , i+1);
	string Name = msg.substr(j+1, msg.size()-j-1).c_str();

	Statement* pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
	Result* pResult = pStmt->executeQuery("SELECT ServerID FROM UserIPInfo where Name='%s'", Name.c_str());

	static WorldID_t WorldID = g_pConfig->getPropertyInt("WorldID");

	if (pResult->getRowCount() != 0) {

		pResult->next();
		
		ServerID_t ServerID = pResult->getInt(1);

		string ServerName = g_pGameServerGroupInfoManager->getGameServerGroupInfo(ServerID, WorldID)->getGroupName();

        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_PLAYER_IN_GAMESERVER),
                        Name.c_str(), ServerName.c_str());

		GCSystemMessage gcSystemMessage;
		gcSystemMessage.setMessage(msg);
		pGamePlayer->sendPacket (&gcSystemMessage);

	} else {
//		StringStream msg;
//		msg <<  Name << " 님은 비접속 중입니다.";

        char msg[100];
        sprintf(msg, g_pStringPool->c_str(STRID_PLAYER_NOT_IN_GAMESERVER),
                        Name.c_str());

		GCSystemMessage gcSystemMessage;
		gcSystemMessage.setMessage(msg);
		pGamePlayer->sendPacket (&gcSystemMessage);
	}


	SAFE_DELETE(pStmt);

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
// 여기 이름 남기고.. 열심히~ 케케.. 
// GCSystemMessage가 256자가 한계니까 조심하길. by sigi. 2003.3.22
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opcredit(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY

	if (pGamePlayer==NULL) return;

	static map<string, string> Credits;

	// 하드코딩으로 유지하길... ㅇㅎㅎ  by sigi.
	if (Credits.empty())
	{
		Credits["윤창필"] = "서버코어(1999.9~2000.9), 기획(2001.3~2002.3)";
		Credits["김건우"] = "서버, 기획(1999.9~2002.3)";
		Credits["김진태"] = "클라이언트UI, 툴(1999.9~2001.1)";
		Credits["임춘근"] = "서버, DB(1999.9~2000.?)";
		Credits["김성민"] = "서버, DB(2000.10~2002.3)";
		Credits["윤영식"] = "클라이언트메인(1999.9~2002.3), 서버(2002.4~2003.3).아 청춘이여~T_T; unuis@hanmail.net";
		Credits["지성욱"] = "클라이언트UI, 툴(2001.2~2002.3), 클라이언트메인(2002.3~2003.3)";
		Credits["장홍창"] = "서버, DB(2002.1~2002.12)";
		Credits["배재형"] = "서버(2002.4~)";
		Credits["손희승"] = "클라이언트UI, 툴(2002.6~2003.1), 클라이언트메인(2003.1~)";
		Credits["이희원"] = "DB, 운영팀지원(2002.6~)";
		Credits["양승명"] = "서버(2002.11~)";
		Credits["홍상민"] = "서버(2002.12~)";
	}

	uint j = msg.find_first_of(' ' , i+1);
	string Name = msg.substr(j+1, msg.size()-j-1).c_str();

	map<string, string>::const_iterator itr = Credits.find(Name);

	if (itr!=Credits.end())
	{
		StringStream msg;
		msg << "[" << itr->first << "] " << itr->second;

		GCSystemMessage gcSystemMessage;
		gcSystemMessage.setMessage(msg.toString());
		pGamePlayer->sendPacket(&gcSystemMessage);
	}

	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opuser(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;

	Statement* pStmt = g_pDatabaseManager->getDistConnection("PLAYER_DB")->createStatement();
	//Statement* pStmt = g_pDatabaseManager->getConnection((int)Thread::self())->createStatement();
	Result* pResult = pStmt->executeQuery("SELECT Count(*) FROM Player where LogOn='GAME' OR LogOn='LOGON'");

	pResult->next();

	int GroupCount = pResult->getInt(1);

//	StringStream msg;
//	msg << "현재 동시 접속자 : " << GroupCount << "명";

    char msg[100];
    sprintf(msg, g_pStringPool->c_str(STRID_CURRENT_NUMBER_OF_PLAYER),
                    GroupCount);

	GCSystemMessage gcSystemMessage;
	gcSystemMessage.setMessage(msg);
	pGamePlayer->sendPacket (&gcSystemMessage);

	SAFE_DELETE(pStmt);

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::optrace(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;

	Creature* pCreature = pGamePlayer->getCreature();

	uint j = msg.find_first_of(' ' , i+1);
	uint k = msg.find_first_of(' ' , j+1);

	bool isNPCTrace = false;

	if (msg.size() > k && !strncasecmp(msg.substr(j+1, k-j-1).c_str(),"NPC",3))
		isNPCTrace = true;

	string Name;

	if (isNPCTrace)
	{
		Name = msg.substr(k+1, msg.size()-k-1);
	}
	else
	{
		Name = msg.substr(j+1, msg.size()-j-1);
	}

	Creature* pTargetCreature = NULL;

	// NoSuch제거. by sigi. 2002.5.2
	__ENTER_CRITICAL_SECTION((*g_pPCFinder))
		
	if (isNPCTrace)
	{
		NPC *pNPC = g_pPCFinder->getNPC_LOCKED(Name);
		pTargetCreature = dynamic_cast<Creature*>(pNPC);
	}
	else 
	{
		pTargetCreature = g_pPCFinder->getCreature_LOCKED(Name);
	}

	if (pTargetCreature==NULL) 
	{
		g_pPCFinder->unlock();
		return;
	}

	ZoneID_t ZoneNum = pTargetCreature->getZoneID();
	Coord_t ZoneX = pTargetCreature->getX();
	Coord_t ZoneY = pTargetCreature->getY();

	Assert((int)ZoneX < 256);
	Assert((int)ZoneY < 256);
	Assert(pCreature->isPC());

	transportCreature(pCreature, ZoneNum, ZoneX, ZoneY, false);

	__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::oppay(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;

	char str[80];

	if (pGamePlayer->isBillingPlayAvaiable())
	{
		strcpy(str, "[애드빌] 빌링 시스템에 접속하여 게임하는 중입니다.");
	}
	else if (pGamePlayer->isPayPlaying())
	{
		Timeval currentTime;
		getCurrentTime(currentTime);
		Timeval payTime = pGamePlayer->getPayPlayTime(currentTime);

		if (pGamePlayer->getPayPlayType()==PAY_PLAY_TYPE_PERSON)
		{
			strcpy(str, "[Metrotech][개인] ");
		}
		else
		{
			strcpy(str, "[Metrotech][PC방] ");
		}

		if (pGamePlayer->getPayType()==PAY_TYPE_FREE)
		{	
			strcat(str, "유료서비스지만 무료 계정입니다.");
		}
		else if (pGamePlayer->getPayType()==PAY_TYPE_PERIOD)
		{	
			sprintf(str, "%s%s 까지 사용가능합니다.", str, pGamePlayer->getPayPlayAvailableDateTime().toString().c_str());
		}
		else
		{
			sprintf(str, "%s사용시간 : %d / %d 분", str, (int)(payTime.tv_sec/60), (int)pGamePlayer->getPayPlayAvailableHours());
		}
	}
	else
	{	
		strcpy(str, "[Metrotech] 무료 게임 중입니다.");
	}

	GCSystemMessage gcSystemMessage;
	gcSystemMessage.setMessage(str);
	pGamePlayer->sendPacket (&gcSystemMessage);

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opfun(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;

	uint j = msg.find_first_of(' ' , i+1);
	uint k = msg.find_first_of(' ' , j+1);
	uint l = msg.find_first_of(' ' , k+1);

	string      FunType		= trim(msg.substr(j+1, k-j-1));
	int 		value1    	= atoi(msg.substr(k+1, l-k-1).c_str());
	int 		value2    	= atoi(msg.substr(l+1, msg.size()-l-1).c_str());


	if (FunType=="mine")
	{
		int ItemType = value1;
		int ItemNum  = value2;

		ItemType = max(0, ItemType);
		ItemType = min((int)g_pMineInfoManager->getInfoCount()-1, ItemType);

		ItemNum = max(1, ItemNum);
		ItemNum = min(36, ItemNum);

		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature!=NULL);

		/*
		if (!pCreature->isSlayer())
		{
			GCSystemMessage gcSystemMessage;
			gcSystemMessage.setMessage("slayer 아니면 안된다~");
			pGamePlayer->sendPacket (&gcSystemMessage);
			return;
		}
		*/

		Zone* pZone = pCreature->getZone();
		Assert(pZone!=NULL);

		ItemInfo* pItemInfo = g_pItemInfoManager->getItemInfo(Item::ITEM_CLASS_MINE, ItemType);
		if (pItemInfo==NULL) return;

		Damage_t MinDamage = pItemInfo->getMinDamage();
		Damage_t MaxDamage = pItemInfo->getMaxDamage();

		for (int i=0; i<ItemNum; i++)
		{
			Mine * pInstallMine = new Mine();
			Assert(pInstallMine != NULL);

			ObjectRegistry& OR = pZone->getObjectRegistry();
			OR.registerObject(pInstallMine);

			int SkillLevel = 100;
			Damage_t RealDamage = MinDamage + (max (0, ((int)MaxDamage * (int)SkillLevel / 100) - MinDamage));

			pInstallMine->setItemType(ItemType);
			pInstallMine->setDir(pCreature->getDir());
			pInstallMine->setDamage(RealDamage);
			pInstallMine->setInstallerName(pCreature->getName());
			pInstallMine->setInstallerPartyID(pCreature->getPartyID());
			pInstallMine->setFlag(Effect::EFFECT_CLASS_INSTALL);

			TPOINT pt = pZone->addItem(pInstallMine, pCreature->getX(), pCreature->getY(), true, 6000);

			addInstalledMine(pZone, pInstallMine, pt.x, pt.y);
		}

		GCSystemMessage gcSystemMessage;
		gcSystemMessage.setMessage("지뢰를 설치했습니다.");
		pGamePlayer->sendPacket (&gcSystemMessage);
	}
	else
	{
		GCSystemMessage gcSystemMessage;
		gcSystemMessage.setMessage("뭐꼬~!? 똑바로 모하나");
		pGamePlayer->sendPacket (&gcSystemMessage);
	}

	__END_DEBUG_EX __END_CATCH
}



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opwarp(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

#ifdef __GAME_SERVER__

	if (pGamePlayer==NULL)	 return;

	Creature* pCreature = pGamePlayer->getCreature();

	uint j = msg.find_first_of(' ' , i+1);
	uint k = msg.find_first_of(' ' , j+1);
	uint l = msg.find_first_of(' ' , k+1);

	string      ZoneName = trim(msg.substr(j+1, k-j-1));
	ZoneCoord_t ZoneX    = atoi(msg.substr(k+1, l-k-1).c_str());
	ZoneCoord_t ZoneY    = atoi(msg.substr(l+1, msg.size()-l-1).c_str());

	int ZoneID = 0;

	// ZoneName은 존의 이름일 수도 있고, 존의 ID일 수도 있다.
	// ZoneInfo가 NULL이 아니라는 말은 이 문자열이 존의 이름이라는 뜻이고,
	// Zoneinfo가 NULL이라는 말은 이 문자열이 존의 ID이라는 말이다.
	// (물론 사용자의 입력 실수도 있겠지만, 이는 무시한다.)
	ZoneInfo* pZoneInfo = g_pZoneInfoManager->getZoneInfoByName(ZoneName);
	if (pZoneInfo != NULL)
	{
		ZoneID = pZoneInfo->getZoneID();
	}
	else
	{
		ZoneID = atoi(ZoneName.c_str());
	}

	if (pCreature->isPC() && ZoneX < 256 && ZoneY < 256)
	{
		// 시험삼아서 실제로 그런 존이 있는지를 체크한다.
		// 없다면 리턴한다.
		try
		{
			Zone* pZone = getZoneByZoneID(ZoneID);
			// evade warning
			pZone = NULL;
		}
		catch (Error)
		{
			return;
		}

		// 그런 존이 있다는 것이 확인되었다면 이동시킨다. 
		try
		{
			transportCreature(pCreature, ZoneID, ZoneX, ZoneY, false);
		}
		catch (Throwable & t)
		{
			cerr << t.toString() << endl;
		}
	}

#endif

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opcreate(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;

	Creature* pCreature = pGamePlayer->getCreature();

	uint j = msg.find_first_of(' ' , i+1);	// class
	uint k = msg.find_first_of(' ' , j+1);	// type
	uint l = msg.find_first_of(' ' , k+1);	// option
	uint lNum = msg.find_first_of('[' , k+1);	// num
	uint rNum = msg.find_first_of(']' , k+1);
	uint lTime = msg.find_first_of('(' , k+1);	// num
	uint rTime = msg.find_first_of(')' , k+1);
	uint lGrade = msg.find_first_of('{' , k+1);	// num
	uint rGrade = msg.find_first_of('}' , k+1);

	string optional;

	if (lNum < rNum)
		optional = msg.substr(lNum+1 ,rNum-lNum-1).c_str();

	ItemType_t ItemType   = 255;
	ItemType_t OptionType = 255;

	string ItemClassName = trim(msg.substr(j+1, k-j-1));
	Item::ItemClass ItemClass = g_pItemFactoryManager->getItemClassByName(ItemClassName);

	// ItemClass가 MAX라면 이름으로는 못 찾았다는 말이다. 
	// 이 경우에는 아이템 클래스가 숫자로 바로 오지는 않았는지 검사해야 한다.
	if (ItemClass == Item::ITEM_CLASS_MAX) {
		int temp = atoi(ItemClassName.c_str());
		if (temp < 0 || temp >= Item::ITEM_CLASS_MAX)
			return;
		else
			ItemClass = (Item::ItemClass)(temp);
	}

    if (ItemClass == Item::ITEM_CLASS_BLOOD_BIBLE ||
		ItemClass == Item::ITEM_CLASS_CASTLE_SYMBOL ||
		ItemClass == Item::ITEM_CLASS_COUPLE_RING ||
		ItemClass == Item::ITEM_CLASS_VAMPIRE_COUPLE_RING ||
		ItemClass == Item::ITEM_CLASS_SWEEPER ||
		ItemClass == Item::ITEM_CLASS_BLOOD_BIBLE_SIGN) {
        GCSystemMessage gcSystemMessage;
        gcSystemMessage.setMessage(g_pStringPool->getString(STRID_CANNOT_CREATE_ITEM));

        pGamePlayer->sendPacket(&gcSystemMessage);

        return;
    }

	if (ItemClass == Item::ITEM_CLASS_CORPSE
//		|| ItemClass == Item::ITEM_CLASS_KEY
		|| ItemClass == Item::ITEM_CLASS_MOTORCYCLE
		|| isRelicItem(ItemClass) && optional != "force") {
		// 시체 만들지 마랑~~ by sigi
		GCSystemMessage gcSystemMessage;
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_CANNOT_CREATE_ITEM));

		pGamePlayer->sendPacket(&gcSystemMessage);

		return;
	}

	ItemType   = atoi(msg.substr(k+1, l-k-1).c_str());

	list<OptionType_t> optionTypes;

	
	uint pos = l, previous = l;
	while (previous < string::npos && pos < string::npos) {
		pos = msg.find_first_of(' ' , previous+1);
		string optionString = trim(msg.substr(previous+1, pos-previous-1));

		//cout << "optionString: " << optionString.c_str() << endl;

		if (optionString.size()==0)
			break;

		OptionInfo* pOptionInfo = NULL;

		//try {
			pOptionInfo = g_pOptionInfoManager->getOptionInfo(optionString);

			if (pOptionInfo==NULL)
			{
				pOptionInfo = g_pOptionInfoManager->getOptionInfo(atoi(optionString.c_str()));

				if (pOptionInfo==NULL)
					break;
			}
				
			/*
		} catch (NoSuchElementException&) {
			try {
				pOptionInfo = g_pOptionInfoManager->getOptionInfo(atoi(optionString.c_str()));
			} catch (NoSuchElementException&) {
				break;
			}
		}
		*/

		if (pOptionInfo!=NULL) {
			OptionType = pOptionInfo->getType();

			if (OptionType!=0) {
				//cout << "optionType = " << (int)OptionType << endl;
				optionTypes.push_back(OptionType);
			}
		}

		previous = pos;
	}


	// 정상적으로 생성할 수 있는 아이템이 아니라면 리턴한다.
	if (!g_pItemInfoManager->isPossibleItem(ItemClass, ItemType, optionTypes)) {
		//cerr << "Cannot create item" << endl;
		StringStream msg;
		msg << g_pStringPool->getString(STRID_CANNOT_CREATE_ITEM_2)
			<< ItemClass2ShortString[ItemClass]
			<< ", " << (int)ItemType
			<< ", " << getOptionTypeToString(optionTypes);

		GCSystemMessage gcSystemMessage;
		gcSystemMessage.setMessage(msg.toString().c_str());

		pGamePlayer->sendPacket(&gcSystemMessage);
	
		return;
	}

	//cout << "createItemOptions : " << getOptionTypeToString(optionTypes) << endl;
	// create로 생성한 유니크 아이템도 개수 제한을 시켜야할까?
	ItemInfo* pItemInfo = g_pItemInfoManager->getItemInfo(ItemClass, ItemType);
	Assert(pItemInfo!=NULL);

	// 유니크 아이템인 경우
	/*
	if (pItemInfo->isUnique())
	{
		if (!UniqueItemManager::isPossibleCreate(ItemClass, ItemType)) 
		{
			GCSystemMessage gcSystemMessage;
			gcSystemMessage.setMessage("더 이상 만들 수 없는 유니크 아이템입니다.");

			pGamePlayer->sendPacket(&gcSystemMessage);

			return;
		}
	}
	*/

	Item* pItem = g_pItemFactoryManager->createItem((Item::ItemClass)ItemClass, ItemType, optionTypes);
	pItem->setCreateType(Item::CREATE_TYPE_CREATE);
	pItem->setNum(1);

	if (ItemClass == Item::ITEM_CLASS_PET_ITEM) {
		PetInfo* pPetInfo = new PetInfo;
		pPetInfo->setPetType(ItemType);
		pPetInfo->setPetCreatureType(432);
		pPetInfo->setPetLevel(1);
		pPetInfo->setPetExp(0);
		pPetInfo->setPetHP(1000);
		pPetInfo->setPetAttr(0xff);
		pPetInfo->setPetOption(0);
		pPetInfo->setFoodType(0);
		pPetInfo->setFeedTime(VSDateTime::currentDateTime());

		PetItem* pPetItem = dynamic_cast<PetItem*>(pItem);
		if (pPetItem != NULL)
		{
			pPetItem->setPetInfo(pPetInfo);
			pPetInfo->setPetItem(pPetItem);
		}
		else 
		{
			//cout << "-_-;;;;" << endl;
		}
	}

	//cout << "createItemOptions : " << getOptionTypeToString(pItem->getOptionTypeList()) << endl;

	if (isStackable(pItem->getItemClass())
		&& lNum < rNum)
	{
		int itemNum = atoi(msg.substr(lNum+1 ,rNum-lNum-1).c_str());
	
		itemNum = max(1, min(itemNum, ItemMaxStack[pItem->getItemClass()]));

		pItem->setNum(itemNum);
	}

	Assert(pItem != NULL);

	Zone* pZone = pCreature->getZone();

	ObjectRegistry & objectregistery = pZone->getObjectRegistry();
	objectregistery.registerObject(pItem);

	GCCreateItem gcCreateItem;

	if (pCreature->isPC()) 
	{
		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pCreature);

		Inventory* pInventory = pPC->getInventory();

		Assert(pInventory != NULL);

		TPOINT p;

		if (pInventory->getEmptySlot(pItem, p)) 
		{
			pInventory->addItem(p.x, p.y, pItem);
			if (lGrade < rGrade)
			{
				int grade = atoi(msg.substr(lGrade+1, rGrade-lGrade-1).c_str());
				pItem->setGrade(grade);
			}

			pItem->create(pPC->getName(), STORAGE_INVENTORY, 0, p.x, p.y);
			//pItem->whenPCTake(pPC);

			if (lTime < rTime)
			{
				int time = atoi(msg.substr(lTime+1, rTime-lTime-1).c_str());
				if (time > 0)
				{
					pPC->addTimeLimitItem(pItem, time);
					pPC->sendTimeLimitItemInfo();
				}
				else
				{
					pPC->addTimeLimitItem(pItem, 2592000);
					pPC->sendTimeLimitItemInfo();
				}
			}
			else 
			{
				pPC->addTimeLimitItem(pItem, 2592000);
				pPC->sendTimeLimitItemInfo();
			}

			makeGCCreateItem(&gcCreateItem, pItem, p.x, p.y);
			pGamePlayer->sendPacket(&gcCreateItem);
		} 
		else 
		{
			//cerr << "아이템 창조에 실패하였습니다" << endl;
			SAFE_DELETE(pItem);
		}
	} 
/*	else if (pCreature->isVampire())
	{
		Vampire* pVampire= dynamic_cast<Vampire*>(pCreature);

		Inventory* pInventory = pVampire->getInventory();

		Assert(pInventory != NULL);

		TPOINT p;

		if (pInventory->getEmptySlot(pItem, p)) 
		{
			pInventory->addItem(p.x, p.y, pItem);

			makeGCCreateItem(&gcCreateItem, pItem, p.x, p.y);

			pGamePlayer->sendPacket(&gcCreateItem);

			pItem->create(pVampire->getName(), STORAGE_INVENTORY, 0, p.x, p.y);
		} 
		else 
		{
			//cout << "아이템 창조에 실패하였습니다" << endl;
			SAFE_DELETE(pItem);
		}
	}
	else if (pCreature->isOusters())
	{
		Ousters* pOusters= dynamic_cast<Ousters*>(pCreature);

		Inventory* pInventory = pOusters->getInventory();

		Assert(pInventory != NULL);

		TPOINT p;

		if (pInventory->getEmptySlot(pItem, p)) 
		{
			pInventory->addItem(p.x, p.y, pItem);

			makeGCCreateItem(&gcCreateItem, pItem, p.x, p.y);

			pGamePlayer->sendPacket(&gcCreateItem);

			pItem->create(pOusters->getName(), STORAGE_INVENTORY, 0, p.x, p.y);
		} 
		else 
		{
			//cout << "아이템 창조에 실패하였습니다" << endl;
			SAFE_DELETE(pItem);
		}
	}*/

	if (pItem != NULL)
	{
		if (isRelicItem(pItem))
		{
			addRelicEffect(pCreature, pItem);
		}

		// 유니크 아이템인 경우 개수 체크해준다.
		if (pItemInfo->isUnique())
		{
			pItem->setUnique();
			//UniqueItemManager::createItem(ItemClass, ItemType);
			filelog("uniqueItem.txt", "[OpCreate] %s %s", pCreature->getName().c_str(), pItem->toString().c_str());
		}


		// 로그를 남긴다.
		Statement* pStmt = NULL;
		BEGIN_DB
		{
			StringStream sql;
			sql << "INSERT INTO OpCreate (OpName, DateTime, ItemDesc) VALUES ("
				<< "'" << pCreature->getName() << "',"
				<< "'" << VSDateTime::currentDateTime().toString() << "',"
				<< "'" << pItem->toString() << "'"
				<< ")";

			pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
			pStmt->executeQuery(sql.toString());
			SAFE_DELETE(pStmt);
		}
		END_DB(pStmt);

		if (pItem != NULL && pItem->isTraceItem())
		{
			remainTraceLog(pItem, "GOD", pCreature->getName(), ITEM_LOG_CREATE, DETAIL_COMMAND);
		}
	}

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opgrant(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;

	Creature* pCreature = pGamePlayer->getCreature();

	uint j = msg.find_first_of(' ' , i+1);
	uint k = msg.find_first_of(' ' , j+1);

	BYTE Competence = atoi(msg.substr(j+1, k-j-1).c_str());
	
	Zone* pZone = pCreature->getZone();

	string Name = msg.substr(k+1, msg.size()-k-1).c_str();

	Creature* pTargetCreature = NULL;

	//try
	//{
	pTargetCreature = pZone->getCreature(Name);

	// NoSuch제거. by sigi. 2002.5.2
	if (pTargetCreature!=NULL)
	{
		if (pTargetCreature->isSlayer()) 
		{
			Slayer* pSlayer = dynamic_cast<Slayer*>(pTargetCreature);
			pSlayer->setCompetence(Competence);
			//cout << "set new Competence : " << (int)pSlayer->getCompetence() << endl;
		} 
		else if (pTargetCreature->isVampire()) 
		{
			Vampire* pVampire= dynamic_cast<Vampire*>(pTargetCreature);
			pVampire->setCompetence(Competence);
			//cout << "set new Competence : " << (int)pVampire->getCompetence() << endl;
		}
		else if (pTargetCreature->isOusters()) 
		{
			Ousters* pOusters= dynamic_cast<Ousters*>(pTargetCreature);
			pOusters->setCompetence(Competence);
			//cout << "set new Competence : " << (int)pOusters->getCompetence() << endl;
		}
	}
	//}
	//catch (NoSuchElementException& nsee)
	//{
		//cout << nsee.toString() << endl;
	//	pTargetCreature = NULL;
	//}

	//cout << "Creature Name : " << Name << endl;

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::oprecall(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;

	Creature* pCreature = pGamePlayer->getCreature();

	PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pCreature);
	if (pPC->isPLAYER())
	{
		if (g_pGuildManager->isGuildMaster(pPC->getGuildID(), pPC))
		{
			if (!SiegeManager::Instance().isSiegeZone(pPC->getZoneID()))
			{
				return;
			}
		}
		else
		{
			return;
		}
	}


	//Zone* pCallZone = pCreature->getZone();

	/*
	uint j = msg.find_first_of(' ' , i+1);

	string Name = msg.substr(j+1, msg.size()-j-1).c_str();
	*/

	Creature* pTCreature = NULL;

	uint j = msg.find_first_of(' ' , i+1);

	string Name;

	while (j < msg.size()) 
	{
		i = msg.find_first_of(' ' , j+1);

		Name = msg.substr(j+1, i - j -1).c_str();

		//cout << "Name : (" <<  Name << ")" << endl;

		// NoSuch제거. by sigi. 2002.5.2
		__ENTER_CRITICAL_SECTION((*g_pPCFinder))

		pTCreature = g_pPCFinder->getCreature_LOCKED(Name);

		if (pTCreature==NULL) 
		{
			g_pPCFinder->unlock();
			return;
		}
			
		//if (pTCreature != NULL) 
		{
			// 불행히도 PCFinder에서 찾아온 Creature는 const이다.
			Zone* pTargetZone = pTCreature->getZone();

			Assert(pTargetZone != NULL);

			Creature* pTargetCreature = NULL;
			//try 
			//{
				// NoSuch제거. by sigi. 2002.5.2
				pTargetCreature = pTargetZone->getCreature(pTCreature->getObjectID());
			//}
			//catch (NoSuchElementException) 
			//{
			//}

			if (pTargetCreature != NULL) 
			{
				// 현재 죽어있는 놈은 데리고 올 수 없다.
				if (pTargetCreature->isEffect(Effect::EFFECT_CLASS_COMA)) 
				{
					g_pPCFinder->unlock();
					return;
				}

				// 소환자의 존과 좌표.
				ZoneID_t ZoneNum = pCreature->getZoneID();
				Coord_t ZoneX = pCreature->getX();
				Coord_t ZoneY = pCreature->getY();

				if (SiegeManager::Instance().isSiegeZone(pPC->getZoneID()))
				{
					Effect::EffectClass eClass = Effect::EFFECT_CLASS_MAX;
					if (pCreature->isFlag(Effect::EFFECT_CLASS_SIEGE_DEFENDER))
					{
						ZoneX = 172;
						ZoneY = 38;
						eClass = Effect::EFFECT_CLASS_SIEGE_DEFENDER;
					}
					else if (pCreature->isFlag(Effect::EFFECT_CLASS_SIEGE_REINFORCE))
					{
						ZoneX = 172;
						ZoneY = 38;
						eClass = Effect::EFFECT_CLASS_SIEGE_REINFORCE;
					}
					else if (pCreature->isFlag(Effect::EFFECT_CLASS_SIEGE_ATTACKER_1))
					{
						ZoneX = 20;
						ZoneY = 232;
						eClass = Effect::EFFECT_CLASS_SIEGE_ATTACKER_1;
					}
					else if (pCreature->isFlag(Effect::EFFECT_CLASS_SIEGE_ATTACKER_2))
					{
						ZoneX = 20;
						ZoneY = 232;
						eClass = Effect::EFFECT_CLASS_SIEGE_ATTACKER_2;
					}
					else if (pCreature->isFlag(Effect::EFFECT_CLASS_SIEGE_ATTACKER_3))
					{
						ZoneX = 20;
						ZoneY = 232;
						eClass = Effect::EFFECT_CLASS_SIEGE_ATTACKER_3;
					}
					else if (pCreature->isFlag(Effect::EFFECT_CLASS_SIEGE_ATTACKER_4))
					{
						ZoneX = 20;
						ZoneY = 232;
						eClass = Effect::EFFECT_CLASS_SIEGE_ATTACKER_4;
					}
					else if (pCreature->isFlag(Effect::EFFECT_CLASS_SIEGE_ATTACKER_5))
					{
						ZoneX = 20;
						ZoneY = 232;
						eClass = Effect::EFFECT_CLASS_SIEGE_ATTACKER_5;
					}

					if (eClass != Effect::EFFECT_CLASS_MAX && !pTargetCreature->isFlag(eClass))
					{
						if (!pTargetCreature->isPC() || dynamic_cast<PlayerCreature*>(pTargetCreature)->getGuildID() != pPC->getGuildID())
						{
							g_pPCFinder->unlock();
							return;
						}

						addSimpleCreatureEffect(pTargetCreature, eClass, 72000);
					}
				}

				Assert((int)ZoneX < 256);
				Assert((int)ZoneY < 256);

				Assert(pTargetCreature->isPC());

				transportCreature(pTargetCreature, ZoneNum, ZoneX, ZoneY, false);
			}
		}

		__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

		j = i;

	}

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opmrecall(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opnotice(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	//Creature* pCreature = pGamePlayer->getCreature();

	uint j = msg.find_first_of(' ' , i+1);
	uint k = msg.find_first_of(' ' , j+1);

	string noticemsg = msg.substr(j+1, k-j-1).c_str();

	StringStream sql;

	sql << "INSERT INTO quick1001 (content) VALUES (" << noticemsg << ")";

	Connection* pConnection = new Connection("211.117.52.124", "darkBBS2002", "elcastle", "elca005", 3306);
	Statement* pStmt = pConnection->createStatement();
	pStmt->executeQuery(sql.toString());

	SAFE_DELETE(pStmt);
	SAFE_DELETE(pConnection);
	
	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opsummon(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	if (pGamePlayer==NULL)	 return;

	Creature* pCreature = pGamePlayer->getCreature();

	uint j = msg.find_first_of(' ' , i+1); 
	uint k = msg.find_first_of(' ' , j+1); // j~k : name
	uint l = msg.find_first_of(' ' , k+1); // k~l : num
	uint m = msg.find_first_of(' ' , l+1); // l~m : mad

	uint o = msg.find_first_of('{' , j);
	uint p = msg.find_first_of('}' , j+1);

#ifdef __UNDERWORLD__

	uint e = msg.find_first_of('[', i+1);
	uint v = msg.find_first_of(']', i+1); // e~v : event 여부

	string EventFlag = msg.substr(e+1,v-e-1);
	
	if (strstr(EventFlag.c_str(), "EventAll") != NULL)
	{
		//	이벤트용 코드 (몬스터타입도 그냥 하드코딩 -_-;)
		Zone* pZone = pCreature->getZone();
		Coord_t ZoneX = pCreature->getX();
		Coord_t ZoneY = pCreature->getY();

		TPOINT pt;

		pt = findSuitablePosition(pZone, ZoneX, ZoneY, Creature::MOVE_MODE_WALKING);

		if (pt.x == -1 || (pZone->getZoneLevel(pt.x, pt.y) & SAFE_ZONE))
			return;

		Monster *pMonster = NULL;
		Monster *pMonster1 = NULL;
		Monster *pMonster2 = NULL;

		try 
		{
			pMonster = new Monster(599);
			pMonster1 = new Monster(599);
			pMonster2 = new Monster(599);
			
			if (pMonster == NULL) return;

			pMonster->setName("언더월드");

			pMonster->setTreasure(true);
			pMonster->setUnderworld(true);

			pMonster1->setTreasure(true);
			pMonster2->setTreasure(true);

			pMonster->setClanType(CLAN_VAMPIRE_MONSTER);	
			pMonster1->setClanType(CLAN_VAMPIRE_MONSTER);	
			pMonster2->setClanType(CLAN_VAMPIRE_MONSTER);	

			pZone->addCreature(pMonster , pt.x , pt.y , Directions(rand()%8));
			pZone->addCreature(pMonster1 , pt.x , pt.y , Directions(rand()%8));
			pZone->addCreature(pMonster2 , pt.x , pt.y , Directions(rand()%8));


		}
		catch (...)
		{
			SAFE_DELETE(pMonster);
			return;
		}

		return;
	}
	else if (strstr(EventFlag.c_str(), "Event") != NULL)
	{
		//	이벤트용 코드 (몬스터타입도 그냥 하드코딩 -_-;)
		Zone* pZone = pCreature->getZone();
		Coord_t ZoneX = pCreature->getX();
		Coord_t ZoneY = pCreature->getY();

		TPOINT pt;

		pt = findSuitablePosition(pZone, ZoneX, ZoneY, Creature::MOVE_MODE_WALKING);

		if (pt.x == -1 || (pZone->getZoneLevel(pt.x, pt.y) & SAFE_ZONE))
			return;

		Monster *pMonster = NULL;

		try 
		{
			pMonster = new Monster(599);
			
			if (pMonster == NULL) return;

			pMonster->setName("언더월드");

			pMonster->setTreasure(true);
			pMonster->setUnderworld(true);

			pMonster->setClanType(CLAN_VAMPIRE_MONSTER);	

			pZone->addCreature(pMonster , pt.x , pt.y , Directions(rand()%8));

		}
		catch (...)
		{
			SAFE_DELETE(pMonster);
			return;
		}

		return;
	}

#endif


	SpriteType_t	SpriteType	= 0;
	MonsterType_t   MonsterType = 0;
	int 			MonsterNum	= 0;

	string MonsterName = msg.substr(j+1, k-j-1);

//	cout << MonsterName << endl;

	// SpriteType이 아니고 MonsterType이 설정된 경우
	if (o!=string::npos && p!=string::npos)
	{
		MonsterType = atoi(msg.substr(o+1, p-o-1).c_str());
	}
	else if (strstr(MonsterName.c_str(),"치프") != NULL)
	{
		// 치프몬스터 소환하기 ㅡ.ㅡ; by DEW
		MonsterType = g_pMonsterInfoManager->getChiefMonsterTypeByName(MonsterName);
	}
	else
	{
		SpriteType = g_pMonsterInfoManager->getSpriteTypeByName(MonsterName);

		if (SpriteType==0)
		{
			SpriteType = atoi(msg.substr(j+1, k-j-1).c_str());
		}
	}

	MonsterNum = atoi(msg.substr(k+1, msg.size()-k-1).c_str());

	// 1~30 사이
	MonsterNum = max(1, MonsterNum);
	MonsterNum = min(30, MonsterNum);

	// ClanType
	// "mad" monsters -_-;
	string mad = trim(msg.substr(l+1, m-l-1));

	SUMMON_INFO summonInfo;
	if (mad=="mad")
	{
		summonInfo.canScanEnemy = true;	
	
		string group = trim(msg.substr(m+1, msg.size()-m-1));

		if (group=="group")
		{
			summonInfo.clanType = SUMMON_INFO::CLAN_TYPE_RANDOM_GROUP;	
			summonInfo.clanID = rand()%90+2;
		}
		else
		{
			summonInfo.clanType = SUMMON_INFO::CLAN_TYPE_RANDOM_EACH;	
		}
	}
	else
	{
		summonInfo.canScanEnemy = false;	
		summonInfo.clanType = SUMMON_INFO::CLAN_TYPE_DEFAULT;	
	}

	Zone* pZone = pCreature->getZone();
	Coord_t ZoneX = pCreature->getX();
	Coord_t ZoneY = pCreature->getY();

	try 
	{
		// 일반 존이라면 몬스터를 생성한다.
		if (!(pZone->getZoneLevel() & SAFE_ZONE))
		{
			filelog("summon.txt", "[%s] ZoneID=%d, %s", pCreature->getName().c_str(), pCreature->getZone()->getZoneID(), msg.c_str());

			// monsterInfo에 있는건지 없는건지 체크하기 위해서..
			// 없으면 NoSuchElementException이 뜬다.
			if (SpriteType!=0)
			{
				g_pMonsterInfoManager->getMonsterTypeBySprite(SpriteType);
				addMonstersToZone(pZone, ZoneX, ZoneY, SpriteType, MonsterType, MonsterNum, summonInfo);
			}
			else if (MonsterType!=0)
			{
				g_pMonsterInfoManager->getMonsterInfo(MonsterType);
				addMonstersToZone(pZone, ZoneX, ZoneY, SpriteType, MonsterType, MonsterNum, summonInfo);
			}
		}
	} 
	catch (Throwable& t) 
	{
		cout << t.toString() << endl;
		filelog("summon.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), t.toString().c_str());
	}


	__END_DEBUG_EX __END_CATCH

}

static const Effect::EffectClass FirecrackerEffects[] =
{
	Effect::EFFECT_CLASS_FIRE_CRACKER_1,                // 0
	Effect::EFFECT_CLASS_FIRE_CRACKER_2,                // 1
	Effect::EFFECT_CLASS_FIRE_CRACKER_3,                // 2
	Effect::EFFECT_CLASS_DRAGON_FIRE_CRACKER,           // 3
	Effect::EFFECT_CLASS_FIRE_CRACKER_4,                // 4
	Effect::EFFECT_CLASS_FIRE_CRACKER_VOLLEY_1,         // 5
	Effect::EFFECT_CLASS_FIRE_CRACKER_VOLLEY_2,         // 6
	Effect::EFFECT_CLASS_FIRE_CRACKER_VOLLEY_3,         // 7
	Effect::EFFECT_CLASS_FIRE_CRACKER_VOLLEY_4,         // 8
	Effect::EFFECT_CLASS_FIRE_CRACKER_WIDE_VOLLEY_1,    // 9
	Effect::EFFECT_CLASS_FIRE_CRACKER_WIDE_VOLLEY_2,    // 10
	Effect::EFFECT_CLASS_FIRE_CRACKER_WIDE_VOLLEY_3,    // 11
	Effect::EFFECT_CLASS_FIRE_CRACKER_WIDE_VOLLEY_4,    // 12
	Effect::EFFECT_CLASS_FIRE_CRACKER_STORM             // 13
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opcommand(GamePlayer* pGamePlayer , string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);
	uint k = msg.find_first_of(' ' , j+1);
	string command = trim(msg.substr(j+1, k-j-1)); 
	string value1 = msg.substr(k+1, msg.size()-k-1).c_str();

	if (pGamePlayer != NULL)
	{
		Creature* pCreature = pGamePlayer->getCreature();
		filelog("change.txt", "%s , %s , %s", pCreature->getName().c_str(), command.c_str(), value1.c_str());
	}

	GCSystemMessage gcSystemMessage;
	gcSystemMessage.setMessage("nothing");
	bool bSendPacket = true;

	if (command=="balanceZoneGroup")
	{
		bool defaultZoneGroup = false;

		if (value1=="default")
			defaultZoneGroup = true;

		// 즉시~
		g_pClientManager->setBalanceZoneGroup(0, true, defaultZoneGroup);

		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_ZONE_GROUP_BALANCING));
	}
	else if (command=="regenMasterLair")
	{
		if (pGamePlayer==NULL) return;
		Creature* pCreature = pGamePlayer->getCreature();
		if (pCreature==NULL) return;
		Zone* pZone = pCreature->getZone();
		Assert(pZone!=NULL);

		if (pZone->isMasterLair())
		{
			// 즉시~
			MasterLairManager* pMasterLairManager = pZone->getMasterLairManager();
			Assert(pMasterLairManager!=NULL);

			pMasterLairManager->startEvent();

			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_MASTER_LAIR_REGEN));
		}
		else
		{
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_NOT_IN_MASTER_LAIR));
		}
	}
	else if (command=="showMasterLairStatus")
	{
		if (pGamePlayer==NULL) return;
		Creature* pCreature = pGamePlayer->getCreature();
		if (pCreature==NULL) return;
		Zone* pZone = pCreature->getZone();
		Assert(pZone!=NULL);

		if (pZone->isMasterLair())
		{
			MasterLairManager* pMasterLairManager = pZone->getMasterLairManager();
			Assert(pMasterLairManager!=NULL);

			gcSystemMessage.setMessage(pMasterLairManager->toString());
		}
		else
		{
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_NOT_IN_MASTER_LAIR));
		}
	}
	else if (command=="invincible")
	{
		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature!=NULL);

		bool bInvincible = pCreature->isFlag(Effect::EFFECT_CLASS_NO_DAMAGE);

		if (value1=="on")
		{
            char msg[50];
            sprintf(msg, g_pStringPool->c_str(STRID_INVINCIBLE), "ON");

			if (bInvincible)
				gcSystemMessage.setMessage(msg);
			else
			{
				pCreature->setFlag(Effect::EFFECT_CLASS_NO_DAMAGE);
				gcSystemMessage.setMessage(msg);
			}
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
		else if (value1=="off")
		{

            char msg[50];
            sprintf(msg, g_pStringPool->c_str(STRID_INVINCIBLE), "OFF");

			StringStream message;
			if (!bInvincible)
				gcSystemMessage.setMessage(msg);
			else
			{
				pCreature->removeFlag(Effect::EFFECT_CLASS_NO_DAMAGE);
				gcSystemMessage.setMessage(msg);
			}
			
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
		else
		{
			bSendPacket = false;
		}
	}
	else if (command=="ghost")
	{
		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature!=NULL);

		bool bGhost = pCreature->isFlag(Effect::EFFECT_CLASS_GHOST);

		if (value1=="on")
		{

            char msg[50];
            sprintf(msg, g_pStringPool->c_str(STRID_GHOST), "ON");

			if (bGhost)
				gcSystemMessage.setMessage(msg);
			else
			{
				Zone* pZone = pCreature->getZone();
				Assert(pZone!=NULL);

				Tile& rTile = pZone->getTile(pCreature->getX(), pCreature->getY());	
				if (!rTile.isAirBlocked() && !rTile.hasPortal())
				{
					// 주변의 PC들에게 크리처가 사라졌다는 사실을 브로드캐스트한다.
					GCDeleteObject gcDeleteObject(pCreature->getObjectID());
					pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcDeleteObject, pCreature);

					rTile.deleteCreature(pCreature->getObjectID());
					pCreature->setMoveMode(Creature::MOVE_MODE_FLYING);
					rTile.addCreature(pCreature);

					GCAddEffect gcAddEffect;
					gcAddEffect.setObjectID(pCreature->getObjectID());
					gcAddEffect.setEffectID(Effect::EFFECT_CLASS_GHOST);
					gcAddEffect.setDuration(999999);
					pGamePlayer->sendPacket(&gcAddEffect);

					EffectGhost* pEffect = new EffectGhost(pCreature);
					pCreature->getEffectManager()->addEffect(pEffect);
					pCreature->setFlag(Effect::EFFECT_CLASS_GHOST);

					gcSystemMessage.setMessage(msg);
				}
				else
				{
					gcSystemMessage.setMessage(g_pStringPool->getString(STRID_AIR_BLOCKED));
				}
			}
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
		else if (value1=="off")
		{
            char msg[50];
            sprintf(msg, g_pStringPool->c_str(STRID_GHOST), "OFF");

			StringStream message;
			if (!bGhost)
				gcSystemMessage.setMessage(msg);
			else
			{
				pCreature->getEffectManager()->deleteEffect(Effect::EFFECT_CLASS_GHOST);
				pCreature->removeFlag(Effect::EFFECT_CLASS_GHOST);
				gcSystemMessage.setMessage(msg);

				GCRemoveEffect gcRemoveEffect;
				gcRemoveEffect.setObjectID(pCreature->getObjectID());
				gcRemoveEffect.addEffectList(Effect::EFFECT_CLASS_GHOST);
				pGamePlayer->sendPacket(&gcRemoveEffect);

				Zone* pZone = pCreature->getZone();
				Assert(pZone!=NULL);

				Tile& rTile = pZone->getTile(pCreature->getX(), pCreature->getY());	
				if (!rTile.isGroundBlocked() && !rTile.hasPortal())
				{
					rTile.deleteCreature(pCreature->getObjectID());
					pCreature->setMoveMode(Creature::MOVE_MODE_WALKING);
					rTile.addCreature(pCreature);

					if (pCreature->isSlayer())
					{
						Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);

						GCAddSlayer gcAddSlayer;
						makeGCAddSlayer(&gcAddSlayer, pSlayer);
						pZone->broadcastPacket(&gcAddSlayer, pCreature);
					}
					else if (pCreature->isVampire())
					{
						Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);

						GCAddVampire gcAddVampire;
						makeGCAddVampire(&gcAddVampire, pVampire);
						pZone->broadcastPacket(&gcAddVampire, pCreature);
					}
					else if (pCreature->isOusters())
					{
						Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);

						GCAddOusters gcAddOusters;
						makeGCAddOusters(&gcAddOusters, pOusters);
						pZone->broadcastPacket(&gcAddOusters, pCreature);
					}
				}
				else
				{
					gcSystemMessage.setMessage(g_pStringPool->getString(STRID_GROUND_BLOCKED));
				}

			}
			
			filelog("change.txt", "[%s]%s", (pGamePlayer==NULL?"Nobody":pGamePlayer->getCreature()->getName().c_str()), gcSystemMessage.toString().c_str());
		}
		else
		{
			bSendPacket = false;
		}
	}
	/*
	else if (command=="twiceMoney")
	{
		gcSystemMessage.setMessage("돈을 두배로 만듭니다.");

		Creature* pCreature = pGamePlayer->getCreature();
		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pCreature);

		pPC->setGold(pPC->getGold()*2);

		// 패킷 뭐 보내줘야 하는데.. -_-;
	}
	*/
	else if (command=="clearInventory")
	{
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_CLEAR_INVENTORY));

		Creature* pCreature = pGamePlayer->getCreature();
		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pCreature);
		Inventory* pInventory = pPC->getInventory();

		int i, j;

		try 
		{
			if (pInventory != NULL)
			{
				for (j = 0 ; j < pInventory->getHeight() ; j++) 
				{
					for (i = 0 ; i < pInventory->getWidth() ; i++) 
					{
						Item* pItem = pInventory->getItem(i, j);

						if (pItem != NULL) 
						{
							// 열쇠가 아니면 지운다.
							if (pItem->getItemClass() != Item::ITEM_CLASS_KEY
								&& !isRelicItem(pItem)
								)
							{
								// 유니크 아이템인 경우 개수를 줄인다.
								if (pItem->isUnique())
								{
									//UniqueItemManager::deleteItem(pItem->getItemClass(), pItem->getItemType());
									filelog("uniqueItem.txt", "[ClearInventory] %s", pItem->toString().c_str());
								}

								pInventory->deleteItem(i,j);
								pItem->whenPCLost(pPC);
								pItem->destroy();

								// ItemTrace Log 를 남긴다
								if (pItem != NULL && pItem->isTraceItem())
								{
									remainTraceLog(pItem, pCreature->getName(), "GOD", ITEM_LOG_DELETE, DETAIL_OPCLEAR);
								}

								SAFE_DELETE(pItem);
							}
						} // end of if (pItem != NULL)
					} // end of for
				} // end of for
			}

			// inventory정리 패킷이 없어서.. -_-;
			transportCreature(pCreature, pCreature->getZone()->getZoneID(), pCreature->getX(), pCreature->getY(), false);
		} 
		catch (Throwable & t) 
		{ 
			gcSystemMessage.setMessage(t.toString().c_str());
		}
	
	} else if (command=="clearRankBonus") {
		gcSystemMessage.setMessage("Rank Bonus Clear");

		Creature* pCreature = pGamePlayer->getCreature();
		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pCreature);

		pPC->clearRankBonus();
		if (pPC->isSlayer()) {
			Slayer* pSlayer = dynamic_cast<Slayer*>(pPC);
			Assert(pSlayer != NULL);
			pSlayer->initAllStat();
		} else if (pPC->isVampire()) {
			Vampire* pVampire = dynamic_cast<Vampire*>(pPC);
			Assert(pVampire != NULL);
			pVampire->initAllStat();
		} else if (pPC->isOusters()) {
			Ousters* pOusters = dynamic_cast<Ousters*>(pPC);
			Assert(pOusters != NULL);
			pOusters->initAllStat();
		}
		pPC->sendRankBonusInfo();
	} else if (command=="setCastleOwner") {
		Creature* pCreature = pGamePlayer->getCreature();
		Zone* pZone = pCreature->getZone();
		const string& Name = value1;

		bSendPacket = false;

		if (pZone->isCastle()) {
			// *command setCastleOwner SlayerCommon 이 넘어온 경우 Slayer 공용성
			if (value1 == "SlayerCommon")
				g_pCastleInfoManager->modifyCastleOwner(pZone->getZoneID(), RACE_SLAYER, 99);
			// *command setCastleOwner VampireCommon 이 넘어온 경우 Vampire 공용성
			else if (value1 == "VampireCommon")
				g_pCastleInfoManager->modifyCastleOwner(pZone->getZoneID(), RACE_VAMPIRE, 0);
			else if (value1 == "OustersCommon")
				g_pCastleInfoManager->modifyCastleOwner(pZone->getZoneID(), RACE_OUSTERS, 66);
			// *command setCastleOwner 캐릭터이름 이 넘어온 경우 그 캐릭터의 길드성
			else {
				GuildID_t guildID;
				Race_t race;
				if (getRaceFromDB(Name, race)) {
					if (getGuildIDFromDB(Name, race, guildID))
						g_pCastleInfoManager->modifyCastleOwner(pZone->getZoneID(), race, guildID);
					else {
						gcSystemMessage.setMessage(g_pStringPool->getString(STRID_DO_NOT_BELONG_TO_GUILD));
						bSendPacket = true;
					}
				} else {
					gcSystemMessage.setMessage(g_pStringPool->getString(STRID_NO_SUCH_CHARACTOR));
					bSendPacket = true;
				}
			}
				
		}
		else 
		{
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_NOT_IN_CASTLE));
			bSendPacket = true;
		}

	} else if (command=="setCastleOwnerGuild") {
		uint j = value1.find_first_of(' ' , 0);
		ZoneID_t zoneID = (ZoneID_t)atoi(trim(value1.substr(0, j)).c_str());
		GuildID_t guildID = (GuildID_t)atoi(trim(value1.substr(j+1, value1.size()-j-1)).c_str());

//		cout << zoneID << endl;
//		cout << guildID << endl;

		bSendPacket = false;

		Zone* pZone = getZoneByZoneID(zoneID);
		Guild* pGuild = g_pGuildManager->getGuild(guildID);

		if (pZone != NULL && pZone->isCastle() && pGuild != NULL)
			g_pCastleInfoManager->modifyCastleOwner(zoneID, pGuild->getRace(), guildID);
		else if (pZone != NULL && pZone->isCastle() && guildID == 99)
			g_pCastleInfoManager->modifyCastleOwner(pZone->getZoneID(), RACE_SLAYER, 99);
		// *command setCastleOwner VampireCommon 이 넘어온 경우 Vampire 공용성
		else if (pZone != NULL && pZone->isCastle() && guildID == 0)
			g_pCastleInfoManager->modifyCastleOwner(pZone->getZoneID(), RACE_VAMPIRE, 0);
		else if (pZone != NULL && pZone->isCastle() && guildID == 66)
			g_pCastleInfoManager->modifyCastleOwner(pZone->getZoneID(), RACE_OUSTERS, 66);
	} else if (command == "showWarList") {
		// 현재 진행중인 전쟁의 목록을 보여준다.

		g_pWarSystem->broadcastWarList(pGamePlayer);
		bSendPacket = false;
	} else if (command == "startRaceWar") {
		if (!g_pVariableManager->isWarActive())
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_WAR_OFF_DO_WAR_ACITIVE_ON));
		else if (g_pWarSystem->startRaceWar())
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_RACE_WAR_START));
		else
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_ALREADY_WAR_STARTED_OF_SERVER_ERROR));
		bSendPacket = true;
	} else if (command == "startWar") {
		Creature* pCreature = pGamePlayer->getCreature();
		Zone* pZone = pCreature->getZone();

		WarScheduler* pScheduler = pZone->getWarScheduler();
		if (pScheduler != NULL) {
			Schedule* pSchedule = pScheduler->getRecentSchedule();
			if (pSchedule != NULL) pSchedule->setScheduledTime(VSDateTime::currentDateTime());
		}
	} else if (command == "removeWar") {
		ZoneID_t zoneID = atoi(value1.c_str());

		if (zoneID==0) {
			if (pGamePlayer != NULL) {
				Creature* pCreature = pGamePlayer->getCreature();
				Zone* pZone = pCreature->getZone();

				zoneID = pZone->getZoneID();
			}
			else
				return;
		}

		if (g_pWarSystem->removeWar(zoneID)) {
//			StringStream msg;
//			msg << "[" << (int)zoneID << "] 의 전쟁을 제거했습니다.";
			char msg[100];
			sprintf(msg, g_pStringPool->c_str(STRID_GUILD_WAR_REMOVED),
							(int)zoneID);
			gcSystemMessage.setMessage(msg);
		} else {
//			StringStream msg;
//			msg << "[" << (int)zoneID << "] 에는 진행중인 전쟁이 없습니다.";
			char msg[100];
			sprintf(msg, g_pStringPool->c_str(STRID_NO_GUILD_WAR_IN_ACTIVE),
							(int)zoneID);
			gcSystemMessage.setMessage(msg);
		}
		bSendPacket = true;
	} else if (command == "removeRaceWar") {
		if (g_pWarSystem->removeRaceWar()) {
//			StringStream msg;
//			msg << "종족 전쟁을 제거했습니다.";
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_RACE_WAR_REMOVED));
		} else {
//			StringStream msg;
//			msg << "진행중인 종족 전쟁이 없습니다.";
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_NO_RACE_WAR_IN_ACTIVE));
		}
		bSendPacket = true;
	} else if (command == "LevelWar") {
		if (!g_pVariableManager->isWarActive())
			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_WAR_OFF_DO_WAR_ACITIVE_ON));
		if (g_pVariableManager->isActiveLevelWar()) {
			ZoneID_t zoneID = atoi(value1.c_str());

			if (zoneID != 1131 && zoneID != 1132 && zoneID != 1133 && zoneID != 1134)
				return;

			Zone* pZone = getZoneByZoneID(zoneID);

			Assert(pZone != NULL);

			LevelWarManager* pLevelWarManager = pZone->getLevelWarManager();

			if (pLevelWarManager != NULL) {
				gcSystemMessage.setMessage("LevelWar advanced.");
				pLevelWarManager->manualStart();
			}
		}
	}
	else if (command == "saveBloodBibleOwner") {
		g_pShrineInfoManager->saveBloodBibleOwner();

		StringStream msg;
		gcSystemMessage.setMessage(g_pStringPool->getString(STRID_SAVE_BLOOD_BIBLE_OWNER_INFO_IN_DB));

		bSendPacket = true;
	} else if (command=="killAllMonster") {
		if (pGamePlayer != NULL) {
			Creature* pCreature = pGamePlayer->getCreature();
			Zone* pZone = pCreature->getZone();
			MonsterManager* pMM = pZone->getMonsterManager();
			map< ObjectID_t, Creature* > & monsters = pMM->getCreatures();
			map< ObjectID_t, Creature* >::iterator itr = monsters.begin();

			for (; itr!=monsters.end(); itr++) {
				Creature* pCreature = itr->second;
				Monster* pMonster = dynamic_cast<Monster*>(pCreature);

				if (!pMonster->isFlag(Effect::EFFECT_CLASS_NO_DAMAGE))
					pMonster->setHP(0);
			}

			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_KILL_ALL_MONSTER_IN_ZONE));
		}
	} else if (command=="killAllPC") {
		if (pGamePlayer != NULL) {
			Creature* pCreature = pGamePlayer->getCreature();
			Zone* pZone = pCreature->getZone();
			PCManager* pPM = (PCManager*)pZone->getPCManager();
			map< ObjectID_t, Creature* > & pcs = pPM->getCreatures();
			map< ObjectID_t, Creature* >::iterator itr = pcs.begin();

			for (; itr!=pcs.end(); itr++) {
				Creature* pCreature = itr->second;
				PlayerCreature* pPlayerCreature = dynamic_cast<PlayerCreature*>(pCreature);

				if (!pPlayerCreature->isFlag(Effect::EFFECT_CLASS_NO_DAMAGE)) {
					if (pPlayerCreature->isSlayer())
						dynamic_cast<Slayer*>(pPlayerCreature)->setHP(0);
					if (pPlayerCreature->isVampire())
						dynamic_cast<Vampire*>(pPlayerCreature)->setHP(0);
					if (pPlayerCreature->isOusters())
						dynamic_cast<Ousters*>(pPlayerCreature)->setHP(0);
				}
			}

			gcSystemMessage.setMessage(g_pStringPool->getString(STRID_KILL_ALL_MONSTER_IN_ZONE));
		}
	} else if (command=="showZonePCNum") {
		if (pGamePlayer != NULL) {
			WORD num = pGamePlayer->getCreature()->getZone()->getPCManager()->getSize();
			char msg[100];
			sprintf(msg, g_pStringPool->getString(STRID_PC_NUM).c_str(), num);
			gcSystemMessage.setMessage(msg);
		}
	} else if (command=="showPKZonePCNum") {
		if (pGamePlayer != NULL) {
			ZoneID_t zoneID = pGamePlayer->getCreature()->getZone()->getZoneID();
			if (g_pPKZoneInfoManager->isPKZone(zoneID)) {
				int num = g_pPKZoneInfoManager->getPKZoneInfo(zoneID)->getCurrentPCNum();

				char msg[100];
				sprintf(msg, g_pStringPool->getString(STRID_PC_NUM).c_str(), num);
				gcSystemMessage.setMessage(msg);
			}
			else
				bSendPacket=false;
		}
	} else if (command=="setPKZonePCNum") {
		if (pGamePlayer != NULL) {
			ZoneID_t zoneID = pGamePlayer->getCreature()->getZone()->getZoneID();

			uint j = value1.find_first_of(' ' , 0);
			int num = atoi(trim(value1.substr(0, j)).c_str());

			if (g_pPKZoneInfoManager->isPKZone(zoneID))
				g_pPKZoneInfoManager->getPKZoneInfo(zoneID)->setCurrentPCNum(num);
		}
	} else if (command=="suicide") {
		Creature* pCreature = pGamePlayer->getCreature();
		if (pCreature != NULL) {
			if (pCreature->isSlayer()) {
				Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);
				pSlayer->setHP(0);
			} else if (pCreature->isVampire()) {
				Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);
				pVampire->setHP(0);
			} else if (pCreature->isOusters()) {
				Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);
				pOusters->setHP(0);
			}
		}

		bSendPacket=false;
	} else if (command=="heal") {
		Creature* pCreature = pGamePlayer->getCreature();
		if (pCreature != NULL) {
			Zone* pZone = pCreature->getZone();

			if (pCreature->isFlag(Effect::EFFECT_CLASS_COMA)) {
				// 타겟의 이펙트 매니저에서 코마 이펙트를 삭제한다.
				pCreature->deleteEffect(Effect::EFFECT_CLASS_COMA);
				pCreature->removeFlag(Effect::EFFECT_CLASS_COMA);

				// 코마 이펙트가 날아갔다고 알려준다.
				GCRemoveEffect gcRemoveEffect;
				gcRemoveEffect.setObjectID(pCreature->getObjectID());
				gcRemoveEffect.addEffectList((EffectID_t)Effect::EFFECT_CLASS_COMA);
				pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcRemoveEffect);

				// 이펙트 정보를 다시 보내준다.
				pCreature->getEffectManager()->sendEffectInfo(pCreature, pZone, pCreature->getX(), pCreature->getY());
			}

			HP_t hp = 0;
			MP_t mp = 0;

			if (pCreature->isSlayer()) {
				Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);
				hp = pSlayer->getHP(ATTR_MAX);
				pSlayer->setHP(hp);
				mp = pSlayer->getMP(ATTR_MAX);
				pSlayer->setMP(mp);
			} else if (pCreature->isVampire()) {
				Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);
				hp = pVampire->getHP(ATTR_MAX);
				pVampire->setHP(hp);

				if (pVampire->getSilverDamage() != 0) {
					pVampire->setSilverDamage(0);
					GCModifyInformation gcMI;
					gcMI.addShortData(MODIFY_SILVER_DAMAGE,0);
					pGamePlayer->sendPacket(&gcMI);
				}
			} else if (pCreature->isOusters()) {
				Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);
				hp = pOusters->getHP(ATTR_MAX);
				pOusters->setHP(hp);

				if (pOusters->getSilverDamage() != 0) {
					pOusters->setSilverDamage(0);
					GCModifyInformation gcMI;
					gcMI.addShortData(MODIFY_SILVER_DAMAGE,0);
					pGamePlayer->sendPacket(&gcMI);
				}

				mp = pOusters->getMP(ATTR_MAX) * 2;
				pOusters->setMP(mp);
			}

			if (hp != 0) {
				// 주위에 체력이 채워졌다는 사실을 알린다.
				GCStatusCurrentHP gcStatusCurrentHP;
				gcStatusCurrentHP.setObjectID(pCreature->getObjectID());
				gcStatusCurrentHP.setCurrentHP(hp);
				pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcStatusCurrentHP);
			}
			
			if (mp != 0) {
				GCModifyInformation gcMI;
				gcMI.addShortData(MODIFY_CURRENT_MP,mp);
				pGamePlayer->sendPacket(&gcMI);
			}
		}

		bSendPacket=false;
	} else if (command=="setGold") {
		Gold_t gold = atoi(value1.c_str());
		Creature* pCreature = pGamePlayer->getCreature();

		if (pCreature != NULL) {
			GCModifyInformation gcMI;

			if (pCreature->isSlayer()) {
				Slayer* pSlayer = dynamic_cast<Slayer*>(pCreature);
				pSlayer->setGoldEx(gold);

				gcMI.addLongData(MODIFY_GOLD, pSlayer->getGold());
			} else if (pCreature->isVampire()) {
				Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);
				pVampire->setGoldEx(gold);

				gcMI.addLongData(MODIFY_GOLD, pVampire->getGold());
			} else if (pCreature->isOusters()) {
				Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);
				pOusters->setGoldEx(gold);

				gcMI.addLongData(MODIFY_GOLD, pOusters->getGold());
			}

			pGamePlayer->sendPacket(&gcMI);
		}

		// 돈로그 남길 액수면 돈로그 남긴다
		if (gold >= g_pVariableManager->getMoneyTraceLogLimit()) {
			if (gold > 2000000000)
				gold = 2000000000;

			remainMoneyTraceLog("GOD", pCreature->getName(), ITEM_LOG_CREATE, DETAIL_COMMAND, gold);
		}


		bSendPacket=false;
	} else if (command=="Quest") {
		Creature* pCreature = pGamePlayer->getCreature();
		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pCreature);

		if (pPC != NULL && value1=="Complete") {
			QuestManager* pQM = pPC->getQuestManager();
			if (pQM != NULL) {
				MonsterKillQuestStatus* pQS = dynamic_cast<MonsterKillQuestStatus*>(pQM->getQuestStatusByQuestClass(QUEST_CLASS_MONSTER_KILL));
				if (pQS != NULL) {
					pQS->completeQuest();
					pPC->sendCurrentQuestInfo();

					bSendPacket=false;
				}
			}
		}
	} else if (command == "QuestEnding") {
		GCNoticeEvent gcNoticeEvent;
		gcNoticeEvent.setCode(NOTICE_EVENT_START_QUEST_ENDING);

		pGamePlayer->sendPacket(&gcNoticeEvent);
		bSendPacket=false;
	} else if (command == "NotifyWin") {
		int j = value1.find_first_of(' ', 0);
		string name = value1.substr(0,j);
		int giftID = atoi(trim(value1.substr(j+1, value1.size())).c_str());

		GCNotifyWin gcNW;
		gcNW.setGiftID(giftID);
		gcNW.setName(name);

		g_pZoneGroupManager->broadcast(&gcNW);
		bSendPacket = false;
	} else if (command == "Horn") {
		GCNoticeEvent gcNE;
		gcNE.setCode(NOTICE_EVENT_RUN_HORN);

		pGamePlayer->sendPacket(&gcNE);
		bSendPacket = false;
	} else if (command == "Loud") {
		Creature* pCreature = pGamePlayer->getCreature();

		int j = value1.find_first_of(' ', 0);
		string name = value1.substr(0,j);
		int time = atoi(trim(value1.substr(j+1, value1.size())).c_str());

		Effect* pEffect = NULL;
		if (pCreature->isFlag(Effect::EFFECT_CLASS_LOUD)) {
			pEffect = pCreature->findEffect(Effect::EFFECT_CLASS_LOUD);

			pEffect->setDeadline(time*10);
		} else {
			pEffect = new EffectLoud(pCreature);
			pEffect->setDeadline(time*10);
			pCreature->addEffect(pEffect);
			pCreature->setFlag(pEffect->getEffectClass());
		}

		GCAddEffect gcAddEffect;
		gcAddEffect.setObjectID(pCreature->getObjectID());
		gcAddEffect.setEffectID(pEffect->getSendEffectClass());
		gcAddEffect.setDuration(time*10);

		pCreature->getZone()->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcAddEffect, pCreature);
		pGamePlayer->sendPacket(&gcAddEffect);

		bSendPacket = false;
	}
	else if (command == "Game") {
		int j = value1.find_first_of(' ', 0);
		string name = value1.substr(0,j);
		
		GCNoticeEvent gcNE;
		gcNE.setCode(NOTICE_EVENT_MINI_GAME);

		if (name == "Mine")
			gcNE.setParameter(0);
		else if (name == "Nemo")
			gcNE.setParameter(1);
		else if (name == "Push")
			gcNE.setParameter(2);
		else if (name == "Mine")
			gcNE.setParameter(3);
		else if (name == "Arrow")
			gcNE.setParameter(4);
		else 
			gcNE.setParameter(atoi(name.c_str()));

/*		if (gcNE.getParameter() < 3)
		{
			pGamePlayer->sendPacket(&gcNE);

			GCMiniGameScores gcScores;
			gcScores.setGameType((GameType)gcNE.getParameter());

			Statement* pStmt = NULL;

			BEGIN_DB
			{
				pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
				Result* pResult = pStmt->executeQuery(
						"SELECT Name, Score FROM MiniGameScores WHERE Type = %u ORDER BY Score limit 10",gcScores.getGameType());

				int i=0;

				if (pResult->next())
				{
					string name = pResult->getString(1);
					WORD score = (WORD)pResult->getInt(2);

					gcScores.setScore(i,name,score);
					i++;
				}

				SAFE_DELETE(pStmt);
			}
			END_DB(pStmt)

			pGamePlayer->sendPacket(&gcScores);
			bSendPacket = false;
		} */
	} else if (command == "changeSex") {
		Creature* pCreature = pGamePlayer->getCreature();
		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pCreature);

		if (pPC != NULL)
			if (changeSexEx(pPC)) bSendPacket = false;
	}
/*	else if (command == "createBulletin")
	{
		int bulletinLevel = atoi(value1.c_str());

		string createMessage = "*create EventTree ";
		switch (bulletinLevel)
		{
			case 1:
				createMessage = createMessage + "26";
				break;
			case 2:
				createMessage = createMessage + "27";
				break;
			case 3:
				createMessage = createMessage + "28";
				break;
			default:
				createMessage = createMessage + "26";
				break;
		}
		opcreate(pGamePlayer, createMessage, 0);
		bSendPacket = false;
	}
	else if (command == "createFirecraker")
	{
		string createMessage = "*create EventETC " + value1;
		opcreate(pGamePlayer, createMessage, 0);
		bSendPacket = false;
	} */ 
	else if (command == "Firecraker") {
		Creature* pCreature = pGamePlayer->getCreature();
		if (pCreature != NULL) {
			Zone* pZone = pCreature->getZone();
			
			if (isAbleToUseTileSkill(pCreature) ||
				!(pZone->getZoneLevel(pCreature->getX(), pCreature->getY()) & COMPLETE_SAFE_ZONE) ||
				atoi(value1.c_str()) > 0 || atoi(value1.c_str()) < 13) {
				Effect::EffectClass effectClass = FirecrackerEffects[ atoi(value1.c_str()) ];

				GCAddEffectToTile gcAddEffectToTile;
				gcAddEffectToTile.setObjectID(pCreature->getObjectID());
				gcAddEffectToTile.setEffectID(effectClass);
				gcAddEffectToTile.setXY(pCreature->getX(), pCreature->getY());
				gcAddEffectToTile.setDuration(10);

				pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcAddEffectToTile);
				bSendPacket = false;
			}
		}
	} else if (command == "Bulletin") {
		Creature* pCreature = pGamePlayer->getCreature();
		if (pCreature != NULL) {
			Zone* pZone = pCreature->getZone();

			uint l = value1.find_first_of(' ' , 0);

			int bulletinLevel = atoi(value1.substr(0, l).c_str());
			string bulletinMessage = value1.substr(l+1, value1.size()-l-1) + "%" + pCreature->getName();

			MonsterType_t MType = 0;
			int time 			= 0;

			switch (bulletinLevel) {
//				case 0:
//					MType = 482;
//					time = g_pVariableManager->getVariable(CHRISTMAS_TREE_DECAY_TIME) / 10;
//					break;
				case 1:
					MType = 650;
					time = 21600;
					break;
				case 2:
					MType = 650;
					time = 43200;
					break;
				case 3:
					MType = 650;
					time = 86400;
					break;
				default:
					break;
			}

			if (MType != 0 && l <= value1.size()) {
				if (createBulletinBoard(pZone, pCreature->getX(), pCreature->getY(), MType, bulletinMessage, VSDateTime::currentDateTime().addSecs(time)))
					bSendPacket = false;
			}
		}
	} else if (command == "SetHP") {
		Creature* pCreature = pGamePlayer->getCreature();
		if (pCreature != NULL) {
			Zone* pZone = pCreature->getZone();

			HP_t hp = (HP_t)atoi(value1.c_str());
			if (pCreature->isSlayer())
				dynamic_cast<Slayer*>(pCreature)->setHP(hp);
			else if (pCreature->isVampire())
				dynamic_cast<Vampire*>(pCreature)->setHP(hp);
			else if (pCreature->isOusters())
				dynamic_cast<Ousters*>(pCreature)->setHP(hp);

			GCStatusCurrentHP gcHP;
			gcHP.setObjectID(pCreature->getObjectID());
			gcHP.setCurrentHP(hp);

			pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcHP);
			bSendPacket = false;
		}
	} else if (command == "ResetAttr") {
		Creature* pCreature = pGamePlayer->getCreature();
		char buffer[100];

		if (pCreature != NULL) {
			if (pCreature->isSlayer())
				gcSystemMessage.setMessage("슬레이어는 능력치 초기화를 지원하지 않습니다.");
			else if (pCreature->isVampire()) {
				Vampire* pVampire = dynamic_cast<Vampire*>(pCreature);
				if (pVampire != NULL);
				{
					VAMPIRE_RECORD prev;
					pVampire->getVampireRecord(prev);

					pVampire->setSTR(20, ATTR_BASIC);
					pVampire->setDEX(20, ATTR_BASIC);
					pVampire->setINT(20, ATTR_BASIC);

					pVampire->setBonus(3 * (pVampire->getLevel()-1));

					pVampire->initAllStat();
					pVampire->sendModifyInfo(prev);

					sprintf(buffer, "STR=20, DEX=20, INTE=20, Bonus=%d", pVampire->getBonus());
					pVampire->tinysave(buffer);
				}
			} else if (pCreature->isOusters()) {
				Ousters* pOusters = dynamic_cast<Ousters*>(pCreature);
				if (pOusters != NULL);
				{
					OUSTERS_RECORD prev;
					pOusters->getOustersRecord(prev);

					pOusters->setSTR(10, ATTR_BASIC);
					pOusters->setDEX(10, ATTR_BASIC);
					pOusters->setINT(10, ATTR_BASIC);

					pOusters->setBonus(3 * (pOusters->getLevel()-1) + 15);

					pOusters->initAllStat();
					pOusters->sendModifyInfo(prev);

					sprintf(buffer, "STR=10, DEX=10, INTE=10, Bonus=%d", pOusters->getBonus());
					pOusters->tinysave(buffer);
				}
			}
		}
	} else if (command == "CTF") {
		g_pFlagManager->manualStart();
		gcSystemMessage.setMessage("CTF advanced.");
	} else if (command == "ViewDamage") {
		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature!=NULL);

		bool bViewDamage = pCreature->isFlag(Effect::EFFECT_CLASS_VIEW_HP);

		if (value1 == "on") {
			if (!bViewDamage) {
				addSimpleCreatureEffect(pCreature, Effect::EFFECT_CLASS_VIEW_HP, -1, false);

				GCAddEffect gcAddEffect;
				gcAddEffect.setObjectID(pCreature->getObjectID());
				gcAddEffect.setEffectID(Effect::EFFECT_CLASS_VIEW_HP);
				gcAddEffect.setDuration(65535);

				pGamePlayer->sendPacket(&gcAddEffect);
			}
		} else if (value1 == "off") {
			if (bViewDamage) {
				Effect* pEffect = pCreature->findEffect(Effect::EFFECT_CLASS_VIEW_HP);
				if (pEffect != NULL) pEffect->setDeadline(0);
			}
		}
	} else if (command == "GoodsReload") {
		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pGamePlayer->getCreature());
		if (pPC != NULL) {
			pPC->loadGoods();
			pPC->registerGoodsInventory(pPC->getZone()->getObjectRegistry());
			gcSystemMessage.setMessage("Goods relading..");
		}
	} else if (command == "PetStash") {
		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pGamePlayer->getCreature());
		if (pPC != NULL) {
			GCPetStashList gcPetStashList;
			makeGCPetStashList(&gcPetStashList, pPC);

			gcPetStashList.setCode(0);
			pGamePlayer->sendPacket(&gcPetStashList);
			bSendPacket = false;
		}
	} else if (command == "ZoneEvent") {
		Creature* pCreature = pGamePlayer->getCreature();
		EventZoneInfo* pEventZoneInfo = EventZoneInfoManager::Instance().getEventZoneInfo(pCreature->getZone()->getZoneID());
		if (pEventZoneInfo == NULL) 
			gcSystemMessage.setMessage("이벤트 존이 아닙니다.");
		else if (value1 == "on") {
			WORD	EventID = pEventZoneInfo->getEventID();
			ZoneEventInfo* pZoneEventInfo = EventZoneInfoManager::Instance().getZoneEventInfo(EventID);
			EventZoneInfo* pCurrentEventZoneInfo = pZoneEventInfo->getCurrentEventZoneInfo();

			if (pCurrentEventZoneInfo != NULL && pCurrentEventZoneInfo != pEventZoneInfo)
				gcSystemMessage.setMessage("이미 다른 이벤트가 켜져있습니다.");
			else if (pCurrentEventZoneInfo == NULL) {
				pEventZoneInfo->turnOn();
				gcSystemMessage.setMessage("이벤트를 켰습니다.");
			}
			else gcSystemMessage.setMessage("이미 이벤트가 켜져있습니다.");
		} else if (value1 == "off") {
			if (pEventZoneInfo->isEventOn()) {
				pEventZoneInfo->turnOff();
				gcSystemMessage.setMessage("이벤트를 껐습니다.");
			}
			else gcSystemMessage.setMessage("이벤트가 켜져있지 않습니다.");
		}
	} else if (command == "EventZonePCLimit") {
		Creature* pCreature = pGamePlayer->getCreature();
		EventZoneInfo* pEventZoneInfo = EventZoneInfoManager::Instance().getEventZoneInfo(pCreature->getZone()->getZoneID());
		if (pEventZoneInfo == NULL)
			gcSystemMessage.setMessage("이벤트 존이 아닙니다.");
		else {
			WORD lim = (WORD)atoi(value1.c_str());
			pEventZoneInfo->setPCLimit(lim);
			char buffer[100];
			sprintf(buffer, "인원제한 : %u", lim);
			gcSystemMessage.setMessage(buffer);
		}
	} else if (command == "KickOutAll") {
		Creature* pCreature = pGamePlayer->getCreature();
		PCManager* pPCManager = (PCManager*)pCreature->getZone()->getPCManager();
		pPCManager->transportAllCreatures(1303, 46, 49);
		gcSystemMessage.setMessage("모든 유저를 바나툴로 쫓아냈습니다.");
	} else if (command == "StartTrap") {
		Creature* pCreature = pGamePlayer->getCreature();
		if (pCreature->getZoneID() == 1410) {
			Zone* pZone = pCreature->getZone();

			EffectCastingIcicleTrap* pEffect = new EffectCastingIcicleTrap(Effect::EFFECT_CLASS_ICICLE_DROP, pZone);
			pZone->registerObject(pEffect);

			pEffect->setStartXY(116, 66);
			pEffect->setLength(48);
			pEffect->setTick(5);
			pEffect->setUnit(10);
			pEffect->setDir(7);
			pEffect->setNextTime(0);
			pEffect->setDeadline(6000);
			pZone->addEffect(pEffect);

			{
				EffectCastingIceWall* pEffect = new EffectCastingIceWall(pZone);

				pEffect->setStartXY(63, 19);
				pEffect->setLength(48);
				pEffect->setWallLength(5);
				pEffect->setTick(15);
				pEffect->setDir(1);
				pEffect->setNextTime(0);
				pEffect->setDeadline(6000);
				pZone->addEffect(pEffect);
			}

			pEffect = new EffectCastingIcicleTrap(Effect::EFFECT_CLASS_ICICLE_AUGER_LARGE, pZone);

			pEffect->setStartXY(15, 72);
			pEffect->setLength(47);
			pEffect->setTick(10);
			pEffect->setUnit(15);
			pEffect->setDir(3);
			pEffect->setLarge(true);
			pEffect->setNextTime(0);
			pEffect->setDeadline(6000);
			pZone->addEffect(pEffect);

			{
				EffectCastingSideTrap* pEffect = new EffectCastingSideTrap(pZone);

				pEffect->setStartXY(68, 119);
				pEffect->setLength(27);
				pEffect->setTick(10);
				pEffect->setUnit(5);
				pEffect->setDir(5);
				pEffect->setNextTime(0);
				pEffect->setDeadline(6000);
				pZone->addEffect(pEffect);
			}

			gcSystemMessage.setMessage("트랩을 깔았습니다.");
		}
		else
			gcSystemMessage.setMessage("트랩을 만들 수 있는 존이 아닙니다.");

	} else if (command == "SMSTest") {
		GCNoticeEvent gcNE;
		gcNE.setCode(NOTICE_EVENT_SEND_SMS);
		pGamePlayer->sendPacket(&gcNE);
	} else if (command == "ForceNick") {
		int j = value1.find_first_of(' ', 0);
		string name = trim(value1.substr(0,j));
		string nick = trim(value1.substr(j+1, value1.size()));

		//cout << "ForceNick " << name << " : " << nick << endl;;

		Creature* pTargetCreature;
		Creature* pCreature = pGamePlayer->getCreature();

		__ENTER_CRITICAL_SECTION((*g_pPCFinder))

		pTargetCreature = g_pPCFinder->getCreature_LOCKED(name);

		__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

		if (pTargetCreature == NULL || pTargetCreature->getZone() != pCreature->getZone() || !pTargetCreature->isPC())
			gcSystemMessage.setMessage("이 존에서 해당 캐릭터를 찾을 수 없습니다.");
		else {
			PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pTargetCreature);
			Statement* pStmt;

			BEGIN_DB {
				pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
				pStmt->executeQuery("REPLACE INTO NicknameBook (nID, OwnerID, NickType, Nickname, NickIndex, Time) VALUES "
						"(100, '%s', %u, '%s', 0, now())", pPC->getName().c_str(), NicknameInfo::NICK_CUSTOM_FORCED, nick.c_str());
				SAFE_DELETE(pStmt);
			} END_DB(pStmt)

			NicknameBook* pNickbook = pPC->getNicknameBook();
			NicknameInfo* pNick = pNickbook->getNicknameInfo(100);
			SAFE_DELETE(pNick);

			pNick = new NicknameInfo;
			pNick->setNicknameID(100);
			pNick->setNicknameType(NicknameInfo::NICK_CUSTOM_FORCED);
			pNick->setNickname(nick);
			pNickbook->setNicknameInfo(100, pNick);

			pPC->setNickname(pNick);
			GCModifyNickname gcMN;
			gcMN.setObjectID(pPC->getObjectID());
			gcMN.setNicknameInfo(pNick);

			pPC->getZone()->broadcastPacket(pPC->getX(), pPC->getY(), &gcMN);
			pPC->getPlayer()->sendPacket(pNickbook->getNicknameBookListPacket());
		}
	} else if (command == "RemoveNick") {
		string name = trim(value1);
		//cout << "RemoveNick " << name << endl;

		Creature* pTargetCreature;
		Creature* pCreature = pGamePlayer->getCreature();

		__ENTER_CRITICAL_SECTION((*g_pPCFinder))

		pTargetCreature = g_pPCFinder->getCreature_LOCKED(name);

		__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

		if (pTargetCreature == NULL || pTargetCreature->getZone() != pCreature->getZone() || !pTargetCreature->isPC())
			gcSystemMessage.setMessage("이 존에서 해당 캐릭터를 찾을 수 없습니다.");
		else {
			PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pTargetCreature);

			NicknameBook* pNickbook = pPC->getNicknameBook();
			NicknameInfo* pNick = pNickbook->getNicknameInfo(100);

			if (pNick == NULL)
				gcSystemMessage.setMessage("지울 강제닉이 없습니다.");
			else {
				Statement* pStmt;

				BEGIN_DB {
					pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
					pStmt->executeQuery("DELETE FROM NicknameBook WHERE OwnerID='%s' AND nID=100",
							pPC->getName().c_str());
					SAFE_DELETE(pStmt);
				} END_DB(pStmt)

				if (pPC->getNickname() == pNick) {
					pPC->setNickname(NULL);
					GCModifyNickname gcMN;
					gcMN.setObjectID(pPC->getObjectID());

					NicknameInfo noNick;
					noNick.setNicknameType(NicknameInfo::NICK_NONE);
					gcMN.setNicknameInfo(&noNick);

					pPC->getZone()->broadcastPacket(pPC->getX(), pPC->getY(), &gcMN);
				}

				pNickbook->setNicknameInfo(100, NULL);
				SAFE_DELETE(pNick);
				pPC->getPlayer()->sendPacket(pNickbook->getNicknameBookListPacket());
			}
		}
	}
	else if (command == "StartGDRLair") {
		if (GDRLairManager::Instance().getCurrentState() == GDR_LAIR_IDLE) {
			GDRLairIdle* pState = dynamic_cast<GDRLairIdle*>(GDRLairManager::Instance().getCurrentState_Object());
			if (pState != NULL) {
				pState->expire();
				gcSystemMessage.setMessage("질드레 레어를 시작합니다.");
			}
		}
		else if (GDRLairManager::Instance().getCurrentState() == GDR_LAIR_ENTRANCE) {
			GDRLairEntrance* pState = dynamic_cast<GDRLairEntrance*>(GDRLairManager::Instance().getCurrentState_Object());
			if (pState != NULL) {
				pState->expire();
				gcSystemMessage.setMessage("질드레 레어를 엽니다.");
			}
		}
	}
	else if (command == "ResetGDRLair") {
		if (GDRLairManager::Instance().getCurrentState() != GDR_LAIR_IDLE) {
			GDRLairManager::Instance().reset();
			gcSystemMessage.setMessage("질드레 레어를 리셋합니다.");
		}
	} else if (command == "GuildRecall" && pGamePlayer != NULL) {
		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature != NULL);

		int j = value1.find_first_of(' ', 0);
		int k = value1.find_first_of(' ', j+1);

		if (k == string::npos) k = value1.size();
		GuildID_t gid = atoi(trim(value1.substr(0,j)).c_str());
		int num = atoi(trim(value1.substr(j+1, k)).c_str());
		int side = 0;

		if (k != value1.size()) {
			string sidestr = trim(value1.substr(k+1, value1.size()));
			side = atoi(sidestr.c_str());
			if (side == 0) {
				if (sidestr == "defense")
					side = 1;
				else if (sidestr == "reinforce")
					side = 2;
				else if (sidestr == "attack1")
					side = 3;
				else if (sidestr == "attack2")
					side = 4;
				else if (sidestr == "attack3")
					side = 5;
				else if (sidestr == "attack4")
					side = 6;
				else if (sidestr == "attack5")
					side = 7;
			}
		}

		__ENTER_CRITICAL_SECTION((*g_pPCFinder))

		list<Creature*> clist = g_pPCFinder->getGuildCreatures(gid, 200);

		for (list<Creature*>::const_iterator itr = clist.begin(); itr != clist.end(); ++itr) {
			Creature* pTargetCreature = *itr;
			if (pTargetCreature == NULL) continue;
			
			// 소환자의 존과 좌표.
			ZoneID_t ZoneNum = pCreature->getZoneID();
			Coord_t ZoneX = pCreature->getX();
			Coord_t ZoneY = pCreature->getY();

			if (pCreature->getZoneID() != pTargetCreature->getZoneID())
				pTargetCreature->getZone()->lock();

			for (int i=0; i<7; ++i)
				deleteCreatureEffect(pTargetCreature, (Effect::EffectClass)(Effect::EFFECT_CLASS_SIEGE_DEFENDER + i));

			if (side < 8 && side > 0) {
				//cout << "side : " << side << endl;
				addSimpleCreatureEffect(pTargetCreature, (Effect::EffectClass)(Effect::EFFECT_CLASS_SIEGE_DEFENDER + side - 1));
			}

			if (pCreature->getZoneID() != pTargetCreature->getZoneID())
				pTargetCreature->getZone()->unlock();

			transportCreature(pTargetCreature, ZoneNum, ZoneX, ZoneY, false);
			num--;
			if (num <= 0) break;
		}

		__LEAVE_CRITICAL_SECTION((*g_pPCFinder))
	} else if (command == "ResetSiege") {
		SiegeManager::Instance().reset(pGamePlayer->getCreature()->getZoneID());
	} else if (command == "InitSiege") {
		SiegeManager::Instance().start(pGamePlayer->getCreature()->getZoneID());
	}
/*	else if (command == "ReloadAuthTable")
	{
		LoadAuthTable((g_pConfig->getProperty("HomePath") + "/data/CSAuth.tab").c_str());
		LoadAuthIndex((g_pConfig->getProperty("HomePath") + "/data/CSAuth.idx").c_str());
		gcSystemMessage.setMessage("엔프로텍트 인증 테이블을 다시 로드했습니다.");
	}
	else if (command == "SetAuthIndex")
	{
		DWORD index = atoi(trim(value1).c_str());
		SetAuthIndex(index);
		char buffer[100];
		sprintf(buffer, "엔프로텍트 인증 인덱스를 %u로 재설정했습니다.", index);
		gcSystemMessage.setMessage(buffer);
	}
*/
	else if (command == "IAmAttacker") {
		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature != NULL);

		addSimpleCreatureEffect(pCreature, Effect::EFFECT_CLASS_SIEGE_ATTACKER_1, 600);
		gcSystemMessage.setMessage("공성 공격측 1번으로 1분간 변신합니다.");
	} else if (command == "IAmDefender") {
		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature != NULL);

		addSimpleCreatureEffect(pCreature, Effect::EFFECT_CLASS_SIEGE_DEFENDER, 600);
		gcSystemMessage.setMessage("공성 수비측으로 1분간 변신합니다.");
	} else if (command == "IAmReinforce") {
		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature != NULL);

		addSimpleCreatureEffect(pCreature, Effect::EFFECT_CLASS_SIEGE_REINFORCE, 600);
		gcSystemMessage.setMessage("공성 수비 원군측으로 1분간 변신합니다.");
	} else if (command == "showpcstat") {
		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature != NULL);
		vector<uint> num = pCreature->getZone()->getPCManager()->getPCNumByRace();
		char buffer[200];
		sprintf(buffer, "슬레이어 %u명, 뱀파이어 %u명, 아우스터즈 %u명", num[RACE_SLAYER], num[RACE_VAMPIRE], num[RACE_OUSTERS]);
		gcSystemMessage.setMessage(buffer);
	} else if (command == "advanceclass") {
		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pGamePlayer->getCreature());
		Assert(pPC != NULL);

		pPC->increaseAdvancementClassExp(1);
		GCModifyInformation gcMI;
		gcMI.addShortData(MODIFY_ADVANCEMENT_CLASS_LEVEL, pPC->getAdvancementClassLevel());
		pGamePlayer->sendPacket(&gcMI);
	} else if (command == "addDynamicZone") {
		int DynamicZoneType = atoi(trim(value1).c_str());

		DynamicZone* pDynamicZone = g_pDynamicZoneManager->getDynamicZoneGroup(DynamicZoneType)->getAvailableDynamicZone();

		char zoneID[32];
		sprintf(zoneID, "%u - %u", pDynamicZone->getTemplateZoneID(), pDynamicZone->getZoneID());

		gcSystemMessage.setMessage(zoneID);
	} else if (command == "enterDynamicZone") {
		ZoneID_t DynamicZoneType = atoi(trim(value1).c_str());

		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pGamePlayer->getCreature());
		Assert(pPC != NULL);

		pPC->getGQuestManager()->enterDynamicZone(DynamicZoneType);
	} else if (command == "clearDynamicZone") {
		ZoneID_t DynamicZoneType = atoi(trim(value1).c_str());

		PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pGamePlayer->getCreature());
		Assert(pPC != NULL);

		pPC->getGQuestManager()->clearDynamicZone(DynamicZoneType);
	} else if (command == "setTimeOutAllZoneEffect") {
		Creature* pCreature = pGamePlayer->getCreature();
		Assert(pCreature != NULL);
		pCreature->getZone()->getEffectManager()->setTimeOutAllEffect();
		gcSystemMessage.setMessage("존 이펙트 다 타임아웃 시키기");
	} else if (command == "printTile") {
		Zone* pZone = pGamePlayer->getCreature()->getZone();

		for (int y=0; y<pZone->getHeight(); ++y) {
			for (int x=0; x<pZone->getWidth(); ++x) {
				if (pZone->getTile(x,y).getEffect(Effect::EFFECT_CLASS_DELETE_TILE)  != NULL) {
					//cout << "#";
				} else {
					//cout << ".";
				}
			}
			cout << endl;
		}
	}
	else 
		bSendPacket = false;

	if (pGamePlayer!=NULL && bSendPacket) {
		pGamePlayer->sendPacket(&gcSystemMessage);
		filelog("change.txt", "[%s] %s", pGamePlayer->getCreature()->getName().c_str(), gcSystemMessage.getMessage().c_str());  
	}

	__END_DEBUG_EX __END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void CGSayHandler::opworld(GamePlayer* pGamePlayer , string msg, int i, bool bSameWorldOnly)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);
	string command = msg.substr(j+1, msg.size()-j-1).c_str();

	// packet
	GGCommand ggCommand;
	ggCommand.setCommand(command);


	// 각 server로 보낸다.
	HashMapGameServerInfo** pGameServerInfos = g_pGameServerInfoManager->getGameServerInfos();


	static int myWorldID = g_pConfig->getPropertyInt("WorldID");
	static int myServerID = g_pConfig->getPropertyInt("ServerID");

	int maxWorldID = g_pGameServerInfoManager->getMaxWorldID();
	int maxServerGroupID = g_pGameServerInfoManager->getMaxServerGroupID();


	for (int worldID=1; worldID<maxWorldID; worldID++) {
		for (int groupID=0; groupID<maxServerGroupID; groupID++) {
			HashMapGameServerInfo& gameServerInfo = pGameServerInfos[worldID][groupID];

			if (!gameServerInfo.empty()) {
				HashMapGameServerInfo::const_iterator itr = gameServerInfo.begin();
				for (; itr != gameServerInfo.end(); itr++) {
					GameServerInfo* pGameServerInfo = itr->second;

					if (pGameServerInfo->getWorldID()==myWorldID) {
						if (pGameServerInfo->getGroupID()!=myServerID)
							g_pLoginServerManager->sendPacket(pGameServerInfo->getIP(), pGameServerInfo->getUDPPort(), &ggCommand);
					}
					// 다른 World인 경우. 같은 world에만 뿌리는게 아니라면..
					else if (!bSameWorldOnly)
						g_pLoginServerManager->sendPacket(pGameServerInfo->getIP(), pGameServerInfo->getUDPPort(), &ggCommand);
				}
			}
		}
	}

	__END_DEBUG_EX __END_CATCH
}

void CGSayHandler::opsoulchain(GamePlayer* pPlayer, string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);

	string Name = msg.substr(j+1, msg.size()-j-1).c_str();

	CGSkillToNamed packet;
	packet.setSkillType(SKILL_SOUL_CHAIN);
	packet.setTargetName(Name);

	packet.execute(pPlayer);

	__END_DEBUG_EX __END_CATCH
}

void CGSayHandler::oplog(GamePlayer* pPlayer, string msg, int i)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);
	uint k = msg.find_first_of(' ' , j+1);
	uint sec = 0;
	string name;

	if (k == string::npos)
		name = msg.substr(j+1, msg.size()-j-1).c_str();
	else {
		name = trim(msg.substr(j+1, k-j-1)); 
		sec = (uint)atoi(msg.substr(k+1, msg.size()-k-1).c_str());
	}

	Creature* pTargetCreature = NULL;

	__ENTER_CRITICAL_SECTION((*g_pPCFinder))

	pTargetCreature = g_pPCFinder->getCreature_LOCKED(name);
	if (pTargetCreature==NULL) {
		g_pPCFinder->unlock();
		return;
	}

	GamePlayer* pTargetGamePlayer = dynamic_cast<GamePlayer*>(pTargetCreature->getPlayer());

	if (pTargetGamePlayer == NULL) {
		g_pPCFinder->unlock();
		return;
	}

	if (sec == 0)
		sec = 600;

	if (pTargetGamePlayer->startPacketLog(sec)) {
		char msg[100];
		sprintf(msg, "%s의 PacketLog를 남깁니다(%u초)", name.c_str(), sec);

		GCSystemMessage gcMsg;
		gcMsg.setMessage(msg);
		pPlayer->sendPacket(&gcMsg);
	}

	__LEAVE_CRITICAL_SECTION((*g_pPCFinder))

	__END_DEBUG_EX __END_CATCH
}

void CGSayHandler::opmodifyunioninfo(GamePlayer* pGamePlayer , string msg, int i, bool bSameWorldOnly)
	throw(ProtocolException, Error) {
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);
	uint gID = (uint)atoi(msg.substr(j+1, msg.size()-j-1).c_str());


	//cout << "ModifyunionInfo->sharedserver() : " << (int)gID <<  endl;

	sendGCOtherModifyInfoGuildUnionByGuildID(gID);
	
		
	__END_DEBUG_EX __END_CATCH
	
}

void CGSayHandler::oprefreshguildunion(GamePlayer* pGamePlayer , string msg, int i, bool bSameWorldOnly)
	throw(ProtocolException, Error)
{
	__BEGIN_TRY __BEGIN_DEBUG_EX

	uint j = msg.find_first_of(' ' , i+1);
	string command = msg.substr(j+1, msg.size()-j-1).c_str();

	//cout << "GuildUnionManager->reload()" << endl;

	GuildUnionManager::Instance().reload();	
	
	
/*
	// packet
	GGCommand ggCommand;
	ggCommand.setCommand(command);


	// 각 server로 보낸다.
	HashMapGameServerInfo** pGameServerInfos = g_pGameServerInfoManager->getGameServerInfos();


	static int myWorldID = g_pConfig->getPropertyInt("WorldID");
	static int myServerID = g_pConfig->getPropertyInt("ServerID");

	int maxWorldID = g_pGameServerInfoManager->getMaxWorldID();
	int maxServerGroupID = g_pGameServerInfoManager->getMaxServerGroupID();


	for (int worldID=1; worldID<maxWorldID; worldID++)
	{
		for (int groupID=0; groupID<maxServerGroupID; groupID++)
		{
			HashMapGameServerInfo& gameServerInfo = pGameServerInfos[worldID][groupID];

			if (!gameServerInfo.empty())
			{
				HashMapGameServerInfo::const_iterator itr = gameServerInfo.begin();
				for (; itr != gameServerInfo.end(); itr++)
				{
					GameServerInfo* pGameServerInfo = itr->second;

					if (pGameServerInfo->getWorldID()==myWorldID)
					{
						// 현재 서버가 아닌 경우에만..(위에서 처리했으므로)
						if (pGameServerInfo->getGroupID()==myServerID)
						{
						}
						else
						{
							g_pLoginServerManager->sendPacket(pGameServerInfo->getIP(),
																pGameServerInfo->getUDPPort(),
																&ggCommand);
						}
					}
					// 다른 World인 경우. 같은 world에만 뿌리는게 아니라면..
					else if (!bSameWorldOnly)
					{
						g_pLoginServerManager->sendPacket(pGameServerInfo->getIP(),
																pGameServerInfo->getUDPPort(),
																&ggCommand);

					}
				}
			}
		}
	}
*/
	__END_DEBUG_EX __END_CATCH
}


#endif
