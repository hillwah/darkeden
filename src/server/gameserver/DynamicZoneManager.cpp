/////////////////////////////////////////////////////////////////////////////
// DynamicZoneManager.cpp
// �������� ����� ��������ϴ� ���� �����ϴ� ��
/////////////////////////////////////////////////////////////////////////////

// include files
#include "DynamicZoneManager.h"
#include "DynamicZoneGroup.h"
#include "DynamicZoneInfo.h"
#include "Assert1.h"

// global variable
DynamicZoneManager* g_pDynamicZoneManager = NULL;

// ���� ����� ������ DynamicZoneID ���� ��ġ
// ���鶧���� 1 �� ����
const ZoneID_t StartDynamicZoneID = 15001;

// constructor
DynamicZoneManager::DynamicZoneManager()
	: m_DynamicZoneID(StartDynamicZoneID )
{
	m_Mutex.setName("DynamicZoneManager");
}

// destructor
DynamicZoneManager::~DynamicZoneManager()
{
	clear();
}

void DynamicZoneManager::init()
{
	// DynamicZoneGroup �߰�. ����
	
	{
		// ���� �Ա�
		DynamicZoneGroup* pDynamicZoneGroup = new DynamicZoneGroup();
		pDynamicZoneGroup->setDynamicZoneType(DYNAMIC_ZONE_GATE_OF_ALTER);
		pDynamicZoneGroup->setTemplateZoneID(g_pDynamicZoneInfoManager->getDynamicZoneInfo(DYNAMIC_ZONE_GATE_OF_ALTER )->getTemplateZoneID());

		addDynamicZoneGroup(pDynamicZoneGroup);
	}

	{
		// ���� ����
		DynamicZoneGroup* pDynamicZoneGroup = new DynamicZoneGroup();
		pDynamicZoneGroup->setDynamicZoneType(DYNAMIC_ZONE_ALTER_OF_BLOOD);
		pDynamicZoneGroup->setTemplateZoneID(g_pDynamicZoneInfoManager->getDynamicZoneInfo(DYNAMIC_ZONE_ALTER_OF_BLOOD )->getTemplateZoneID());

		addDynamicZoneGroup(pDynamicZoneGroup);
	}

	{
		// �����̾� �ɿ��� �ſ�
		DynamicZoneGroup* pDynamicZoneGroup = new DynamicZoneGroup();
		pDynamicZoneGroup->setDynamicZoneType(DYNAMIC_ZONE_SLAYER_MIRROR_OF_ABYSS);
		pDynamicZoneGroup->setTemplateZoneID(g_pDynamicZoneInfoManager->getDynamicZoneInfo(DYNAMIC_ZONE_SLAYER_MIRROR_OF_ABYSS )->getTemplateZoneID());

		addDynamicZoneGroup(pDynamicZoneGroup);
	}

	{
		// �����̾� �ɿ��� �ſ�
		DynamicZoneGroup* pDynamicZoneGroup = new DynamicZoneGroup();
		pDynamicZoneGroup->setDynamicZoneType(DYNAMIC_ZONE_VAMPIRE_MIRROR_OF_ABYSS);
		pDynamicZoneGroup->setTemplateZoneID(g_pDynamicZoneInfoManager->getDynamicZoneInfo(DYNAMIC_ZONE_VAMPIRE_MIRROR_OF_ABYSS )->getTemplateZoneID());

		addDynamicZoneGroup(pDynamicZoneGroup);
	}

	{
		// �ƿ콺���� �ɿ��� �ſ�
		DynamicZoneGroup* pDynamicZoneGroup = new DynamicZoneGroup();
		pDynamicZoneGroup->setDynamicZoneType(DYNAMIC_ZONE_OUSTERS_MIRROR_OF_ABYSS);
		pDynamicZoneGroup->setTemplateZoneID(g_pDynamicZoneInfoManager->getDynamicZoneInfo(DYNAMIC_ZONE_OUSTERS_MIRROR_OF_ABYSS )->getTemplateZoneID());

		addDynamicZoneGroup(pDynamicZoneGroup);
	}
}

void DynamicZoneManager::clear()
{
	HashMapDynamicZoneGroupItor itr = m_DynamicZoneGroups.begin();
	HashMapDynamicZoneGroupItor endItr = m_DynamicZoneGroups.end();

	for (; itr != endItr; ++itr )
	{
		SAFE_DELETE(itr->second);
	}

	m_DynamicZoneGroups.clear();
}

void DynamicZoneManager::addDynamicZoneGroup(DynamicZoneGroup* pDynamicZoneGroup )
{
	Assert(pDynamicZoneGroup != NULL);

	HashMapDynamicZoneGroupItor itr = m_DynamicZoneGroups.find(pDynamicZoneGroup->getDynamicZoneType());

	if (itr != m_DynamicZoneGroups.end() )
	{
		cerr << "Duplicated DynamicZoneGroup. DynamicZoneManager::addDynamicZoneGroup" << endl;
		Assert(false);
	}

	m_DynamicZoneGroups[ pDynamicZoneGroup->getDynamicZoneType() ] = pDynamicZoneGroup;
}

DynamicZoneGroup* DynamicZoneManager::getDynamicZoneGroup(int dynamicZoneType )
{
	HashMapDynamicZoneGroupItor itr = m_DynamicZoneGroups.find(dynamicZoneType);

	if (itr == m_DynamicZoneGroups.end() )
	{
		return NULL;
	}

	return itr->second;
}

ZoneID_t DynamicZoneManager::getNewDynamicZoneID()
{
	ZoneID_t zoneID = 0;

	__ENTER_CRITICAL_SECTION(m_Mutex )

	zoneID = m_DynamicZoneID++;

	__LEAVE_CRITICAL_SECTION(m_Mutex )

	return zoneID;
}

bool DynamicZoneManager::isDynamicZone(ZoneID_t zoneID )
{
	return zoneID >= StartDynamicZoneID;
}

