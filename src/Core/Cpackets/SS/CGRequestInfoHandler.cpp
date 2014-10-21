//////////////////////////////////////////////////////////////////////////////
// Filename    : CGRequestInfo.cpp
// Written By  : excel96
// Description : 
//////////////////////////////////////////////////////////////////////////////

#include "CGRequestInfo.h"

#ifdef __GAME_SERVER__
	#include "GamePlayer.h"
	#include "Zone.h"
	#include "Slayer.h"
	#include "Vampire.h"
	#include "Ousters.h"
	#include "GCOtherModifyInfo.h"
	#include "GCOtherGuildName.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// ������ ���� ������ ��û�ϴ� ����̴�.
//////////////////////////////////////////////////////////////////////////////
void CGRequestInfoHandler::execute(CGRequestInfo* pPacket , Player* pPlayer) throw(ProtocolException, Error) {
	__BEGIN_TRY __BEGIN_DEBUG_EX

#ifdef __GAME_SERVER__

	Assert(pPacket != NULL);
	Assert(pPlayer != NULL);

	GamePlayer* pGamePlayer = dynamic_cast<GamePlayer*>(pPlayer);
	Assert(pGamePlayer != NULL);

	switch (pPacket->getCode()) {
		//------------------------------------------------------------
		// �ٸ� ĳ������ ������ ��û�ϴ� ���
		//------------------------------------------------------------
		case CGRequestInfo::REQUEST_CHARACTER_INFO:
		{
			Creature* pCreature = pGamePlayer->getCreature();
			Assert(pCreature != NULL);

			Zone* pZone = pCreature->getZone();
			Assert(pZone != NULL);

			Creature* pTargetCreature = pZone->getCreature(pPacket->getValue());

			if (pTargetCreature != NULL && pTargetCreature->isPC()) {
				// ������ ����� ��� ����� �ƴ� ��츸 �ȴ�.
				if (pTargetCreature->isSlayer()) {
					Slayer* pSlayer = dynamic_cast<Slayer*>(pTargetCreature);
					if (pSlayer->getCompetenceShape() != 1)
						return;
				}
				else if (pTargetCreature->isVampire()) {
					Vampire* pVampire = dynamic_cast<Vampire*>(pTargetCreature);
					if (pVampire->getCompetenceShape() != 1)
						return;
				}
				else if (pTargetCreature->isOusters()) {
					Ousters* pOusters = dynamic_cast<Ousters*>(pTargetCreature);
					if (pOusters->getCompetenceShape() != 1)
						return;
				}

				bool bSendInfo = false;

				if (pZone == NULL)
                    return;

				GCOtherModifyInfo gcOtherModifyInfo;
				GCOtherGuildName gcOtherGuildName;

				// �� ���� �κ�
				// ���� ������ �����ش�.
				if ((pCreature->isSlayer() || pCreature->getCompetenceShape() != 1) && pTargetCreature->isSlayer()) {
					Slayer* pSlayer = dynamic_cast<Slayer*>(pTargetCreature);

					gcOtherModifyInfo.addShortData(MODIFY_BASIC_STR, pSlayer->getSTR(ATTR_BASIC));
					gcOtherModifyInfo.addShortData(MODIFY_CURRENT_STR, pSlayer->getSTR(ATTR_CURRENT));
					gcOtherModifyInfo.addShortData(MODIFY_BASIC_DEX, pSlayer->getDEX(ATTR_BASIC));
					gcOtherModifyInfo.addShortData(MODIFY_CURRENT_DEX, pSlayer->getDEX(ATTR_CURRENT));
					gcOtherModifyInfo.addShortData(MODIFY_BASIC_INT, pSlayer->getINT(ATTR_BASIC));
					gcOtherModifyInfo.addShortData(MODIFY_CURRENT_INT, pSlayer->getINT(ATTR_CURRENT));
					gcOtherModifyInfo.addLongData(MODIFY_FAME, pSlayer->getFame());

                    Level_t advLVL = pSlayer->getAdvancementClassLevel();
                    if (advLVL > 105)
                        advLVL = 105;

                    if (pSlayer->getSkillDomainLevel(SKILL_DOMAIN_SWORD) == 150)
                        gcOtherModifyInfo.addShortData(MODIFY_SWORD_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_SWORD) + advLVL);
                    else
                        gcOtherModifyInfo.addShortData(MODIFY_SWORD_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_SWORD));
                    if (pSlayer->getSkillDomainLevel(SKILL_DOMAIN_BLADE) == 150)
                        gcOtherModifyInfo.addShortData(MODIFY_BLADE_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_BLADE) + advLVL);
                    else
                        gcOtherModifyInfo.addShortData(MODIFY_BLADE_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_BLADE));
                    if (pSlayer->getSkillDomainLevel(SKILL_DOMAIN_HEAL) == 150)
                        gcOtherModifyInfo.addShortData(MODIFY_HEAL_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_HEAL) + advLVL);
                    else
                        gcOtherModifyInfo.addShortData(MODIFY_HEAL_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_HEAL));
                    if (pSlayer->getSkillDomainLevel(SKILL_DOMAIN_ENCHANT) == 150)
                        gcOtherModifyInfo.addShortData(MODIFY_ENCHANT_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_ENCHANT) + advLVL);
                    else
                        gcOtherModifyInfo.addShortData(MODIFY_ENCHANT_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_ENCHANT));
                    if (pSlayer->getSkillDomainLevel(SKILL_DOMAIN_GUN) == 150)
                        gcOtherModifyInfo.addShortData(MODIFY_GUN_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_GUN) + advLVL);
                    else
                        gcOtherModifyInfo.addShortData(MODIFY_GUN_DOMAIN_LEVEL, pSlayer->getSkillDomainLevel(SKILL_DOMAIN_GUN));
					gcOtherModifyInfo.addLongData(MODIFY_ALIGNMENT, pSlayer->getAlignment());
					gcOtherModifyInfo.addShortData(MODIFY_GUILDID, pSlayer->getGuildID());
					gcOtherModifyInfo.addShortData(MODIFY_RANK, pSlayer->getRank());

					bSendInfo = true;
				} else if ((pCreature->isVampire() || pCreature->getCompetenceShape() != 1) && pTargetCreature->isVampire()) {
					Vampire* pVampire = dynamic_cast<Vampire*>(pTargetCreature);

					gcOtherModifyInfo.addShortData(MODIFY_BASIC_STR, pVampire->getSTR(ATTR_BASIC));
					gcOtherModifyInfo.addShortData(MODIFY_CURRENT_STR, pVampire->getSTR(ATTR_CURRENT));
					gcOtherModifyInfo.addShortData(MODIFY_BASIC_DEX, pVampire->getDEX(ATTR_BASIC));
					gcOtherModifyInfo.addShortData(MODIFY_CURRENT_DEX, pVampire->getDEX(ATTR_CURRENT));
					gcOtherModifyInfo.addShortData(MODIFY_BASIC_INT, pVampire->getINT(ATTR_BASIC));
					gcOtherModifyInfo.addShortData(MODIFY_CURRENT_INT, pVampire->getINT(ATTR_CURRENT));
					gcOtherModifyInfo.addLongData(MODIFY_FAME, pVampire->getFame());

                    Level_t advLVL = pVampire->getAdvancementClassLevel();
                    if (advLVL > 105)
                        advLVL = 105;

                    gcOtherModifyInfo.addShortData(MODIFY_LEVEL, pVampire->getLevel() + advLVL);
                    gcOtherModifyInfo.addLongData(MODIFY_ALIGNMENT, pVampire->getAlignment());
					gcOtherModifyInfo.addShortData(MODIFY_GUILDID, pVampire->getGuildID());
					gcOtherModifyInfo.addShortData(MODIFY_RANK, pVampire->getRank());

					bSendInfo = true;
				} else if ((pCreature->isOusters() || pCreature->getCompetenceShape() != 1) && pTargetCreature->isOusters()) {
					Ousters* pOusters = dynamic_cast<Ousters*>(pTargetCreature);

					gcOtherModifyInfo.addShortData(MODIFY_BASIC_STR, pOusters->getSTR(ATTR_BASIC));
					gcOtherModifyInfo.addShortData(MODIFY_CURRENT_STR, pOusters->getSTR(ATTR_CURRENT));
					gcOtherModifyInfo.addShortData(MODIFY_BASIC_DEX, pOusters->getDEX(ATTR_BASIC));
					gcOtherModifyInfo.addShortData(MODIFY_CURRENT_DEX, pOusters->getDEX(ATTR_CURRENT));
					gcOtherModifyInfo.addShortData(MODIFY_BASIC_INT, pOusters->getINT(ATTR_BASIC));
					gcOtherModifyInfo.addShortData(MODIFY_CURRENT_INT, pOusters->getINT(ATTR_CURRENT));
					gcOtherModifyInfo.addLongData(MODIFY_FAME, pOusters->getFame());

                    Level_t advLVL = pOusters->getAdvancementClassLevel();
                    if (advLVL > 105)
                        advLVL = 105;

                    gcOtherModifyInfo.addShortData(MODIFY_LEVEL, pOusters->getLevel() + advLVL);
					gcOtherModifyInfo.addLongData(MODIFY_ALIGNMENT, pOusters->getAlignment());
					gcOtherModifyInfo.addShortData(MODIFY_GUILDID, pOusters->getGuildID());
					gcOtherModifyInfo.addShortData(MODIFY_RANK, pOusters->getRank());

					bSendInfo = true;
				}

				if (bSendInfo) {
					gcOtherModifyInfo.setObjectID(pPacket->getValue());

					pGamePlayer->sendPacket(&gcOtherModifyInfo);
					
					if (pTargetCreature->isPC()) {
						PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pTargetCreature);

						if (pPC != NULL && pPC->getGuildID() != pPC->getCommonGuildID()) {
							gcOtherGuildName.setObjectID(pPC->getObjectID());
							gcOtherGuildName.setGuildID(pPC->getGuildID());
							gcOtherGuildName.setGuildName(pPC->getGuildName());

							pGamePlayer->sendPacket(&gcOtherGuildName);
						}
					}
				}
			}
		}
		break;
	}
#endif

	__END_DEBUG_EX __END_CATCH
}
