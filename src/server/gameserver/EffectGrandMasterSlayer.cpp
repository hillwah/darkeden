//////////////////////////////////////////////////////////////////////////////
// Filename    : EffectGrandMasterSlayer.cpp
// Written by  : ��
// Description : 
//////////////////////////////////////////////////////////////////////////////

#include "EffectGrandMasterSlayer.h"
#include "Creature.h"
#include "Slayer.h"
#include "Player.h"
#include "Zone.h"
#include "GCAddEffect.h"
#include "GCRemoveEffect.h"

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
EffectGrandMasterSlayer::EffectGrandMasterSlayer(Creature* pCreature)
	throw(Error)
{
	__BEGIN_TRY

	setTarget(pCreature);

	__END_CATCH
}

Effect::EffectClass EffectGrandMasterSlayer::getSendEffectClass() const throw()
{
	Slayer* pSlayer = dynamic_cast<Slayer*>(m_pTarget);
	if (pSlayer == NULL ) return getEffectClass();

	SkillLevel_t level = pSlayer->getHighestSkillDomainLevel();

	if (level == 150 ) return Effect::EFFECT_CLASS_GRAND_MASTER_SLAYER_150;
	else if (level >= 130 ) return Effect::EFFECT_CLASS_GRAND_MASTER_SLAYER_130;
	else return getEffectClass();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void EffectGrandMasterSlayer::affect()
	throw(Error)
{
	__BEGIN_TRY

	Creature* pCreature = dynamic_cast<Creature*>(m_pTarget);
	affect(pCreature);

	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void EffectGrandMasterSlayer::affect(Creature* pCreature)
	throw(Error)
{
	__BEGIN_TRY

	Assert(pCreature->isSlayer());

	pCreature->setFlag(Effect::EFFECT_CLASS_GRAND_MASTER_SLAYER);

	// ������ �ѷ��ش�.
	GCAddEffect gcAddEffect;
	gcAddEffect.setObjectID(pCreature->getObjectID());
	gcAddEffect.setEffectID(getSendEffectClass());
	gcAddEffect.setDuration(999999);
	pCreature->getZone()->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcAddEffect);

	__END_CATCH
}

void EffectGrandMasterSlayer::unaffect() 
	throw(Error)
{
	__BEGIN_TRY	

	Creature* pCreature = dynamic_cast<Creature *>(m_pTarget);
	unaffect(pCreature);

	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void EffectGrandMasterSlayer::unaffect(Creature* pCreature)
	throw(Error)
{
	__BEGIN_TRY
	__BEGIN_DEBUG

	Assert(pCreature != NULL);

	// �ɷ�ġ�� ���������� �ǵ����� ���ؼ��� �÷��׸� ����,
	// initAllStat�� �ҷ��� �Ѵ�.
	pCreature->removeFlag(Effect::EFFECT_CLASS_GRAND_MASTER_SLAYER);

	Zone* pZone = pCreature->getZone();
	Assert(pZone != NULL);

	GCRemoveEffect gcRemoveEffect;
	gcRemoveEffect.setObjectID(pCreature->getObjectID());
	gcRemoveEffect.addEffectList(Effect::EFFECT_CLASS_GRAND_MASTER_SLAYER);
	pZone->broadcastPacket(pCreature->getX(), pCreature->getY(), &gcRemoveEffect);

	__END_DEBUG
	__END_CATCH
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
string EffectGrandMasterSlayer::toString()
	const throw()
{
	__BEGIN_TRY

	StringStream msg;
	msg << "EffectGrandMasterSlayer("
		<< "ObjectID:" << getObjectID()
		<< ")";
	return msg.toString();

	__END_CATCH
}
