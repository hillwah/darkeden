#include "GQuestGiveOustersExpElement.h"
#include "PlayerCreature.h"
#include "Ousters.h"
#include "skill/SkillUtil.h"
#include "GCModifyInformation.h"
#include "GCSystemMessage.h"
#include "Player.h"

GQuestElement::ResultType GQuestGiveOustersExpElement::checkCondition(PlayerCreature* pPC ) const
{
	if (!pPC->isOusters() ) return FAIL;

	GCModifyInformation gcMI;
	Ousters* pOusters = dynamic_cast<Ousters*>(pPC);
	increaseOustersExp(pOusters, m_Amount, gcMI);

	pOusters->getPlayer()->sendPacket(&gcMI);

	GCSystemMessage gcSM;
	gcSM.setMessage("����ġ�� ȹ���߽��ϴ�.");
	pOusters->getPlayer()->sendPacket(&gcSM);

	return OK;
}

GQuestGiveOustersExpElement* GQuestGiveOustersExpElement::makeElement(XMLTree* pTree)
{
	GQuestGiveOustersExpElement* pRet = new GQuestGiveOustersExpElement;

	pTree->GetAttribute("amount", pRet->m_Amount);

	return pRet;
}

GQuestGiveOustersExpElement g_GiveOustersExpElement;
