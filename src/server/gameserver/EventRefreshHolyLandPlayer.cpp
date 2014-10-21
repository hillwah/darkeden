//////////////////////////////////////////////////////////////////////////////
// Filename    : EventRefreshHolyLandPlayer.cpp
// Written by  : bezz
// Description : 
//////////////////////////////////////////////////////////////////////////////

#include "EventRefreshHolyLandPlayer.h"
#include "HolyLandManager.h"
//#include "BloodBibleBonusManager.h"
#include "Zone.h"
#include "ZoneGroupManager.h"
#include "ZoneGroup.h"

//#include "GCHolyLandBonusInfo.h"

#include <map>

//////////////////////////////////////////////////////////////////////////////
// class EventRefreshHolyLandPlayer member methods
//////////////////////////////////////////////////////////////////////////////

EventRefreshHolyLandPlayer::EventRefreshHolyLandPlayer(GamePlayer* pGamePlayer )
	throw()
	:Event(pGamePlayer)
{
}

void EventRefreshHolyLandPlayer::activate () 
	throw(Error)
{
	__BEGIN_TRY

	const map<ZoneGroupID_t, ZoneGroup*>& zoneGroups = g_pZoneGroupManager->getZoneGroups();

	map<ZoneGroupID_t, ZoneGroup*>::const_iterator itr = zoneGroups.begin();

	for (; itr != zoneGroups.end(); ++itr) {
		const map< ZoneID_t, Zone* >& zones = itr->second->getZones();
		map< ZoneID_t, Zone* >::const_iterator zItr = zones.begin();

		for (; zItr != zones.end(); ++zItr )
			zItr->second->setRefreshHolyLandPlayer(true);
	}

	// �ƴ��� ������ �ִ� �÷��̾���� ������ ���� ����Ѵ�.(���� ���� ���ʽ�)
//	g_pHolyLandManager->refreshHolyLandPlayers();

/*	// �ƴ��� ���� ������ ���� ���� ���ʽ� ������ �Ѹ���.
	GCHolyLandBonusInfo gcHolyLandBonusInfo;
	g_pBloodBibleBonusManager->makeHolyLandBonusInfo(gcHolyLandBonusInfo);
	g_pHolyLandManager->broadcast(&gcHolyLandBonusInfo);
*/
	__END_CATCH
}

string EventRefreshHolyLandPlayer::toString () const 
	throw()
{
	StringStream msg;
	msg << "EventRefreshHolyLandPlayer("
		<< ")";
	return msg.toString();
}
