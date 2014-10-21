////////////////////////////////////////////////////////////////////////////////
// Filename    : ActionGiveCommonEventItem.cpp
// Written By  : 
// Description :
////////////////////////////////////////////////////////////////////////////////
#include "ActionGiveCommonEventItem.h"
#include "Item.h"
#include "Slayer.h"
#include "Vampire.h"
#include "Ousters.h"
#include "Player.h"
#include "Inventory.h"
#include "ItemFactoryManager.h"
#include "ItemUtil.h"
#include "OptionInfo.h"
#include "Zone.h"
#include "VariableManager.h"
#include "DB.h"

#include "GCCreateItem.h"
#include "GCNPCResponse.h"

#include <list>


struct EVENT_ITEM_TEMPLATE
{
	Item::ItemClass		ItemClass;
	ItemType_t			ItemType;
	Grade_t				ItemGrade;
	string				ItemOption;
	uint				Ratio;
	string				ItemName;
};

const int EVENT_ITEM_200505_SLAYER_MAX = 24;
const EVENT_ITEM_TEMPLATE Event200505SlayerTemplate[EVENT_ITEM_200505_SLAYER_MAX] =
{
	{ Item::ITEM_CLASS_COAT,				10,		4,		"DUR+5",		80,		"�Ĺ� ����(M)" },
	{ Item::ITEM_CLASS_TROUSER,				10,		4,		"DUR+5",		80,		"�Ĺ� ������(M)" },
	{ Item::ITEM_CLASS_COAT,				11,		4,		"VIS+3",		80,		"�Ĺ� ����(W)" },
	{ Item::ITEM_CLASS_TROUSER,				11,		4,		"VIS+3",		80,		"�Ĺ� ������(W)" },
	{ Item::ITEM_CLASS_GLOVE,				5,		4,		"DEF+5",		80,		"ƾ �÷���Ʈ ��Ʋ��" },
	{ Item::ITEM_CLASS_SHOES,				5,		4,		"ASPD+5",		80,		"��� ����" },
	{ Item::ITEM_CLASS_BELT,				2,		4,		"RES+5",		80,		"�̵�� ��Ʈ" },
	{ Item::ITEM_CLASS_SHIELD,				5,		4,		"DAM+4",		80,		"�巡�� ����" },
	{ Item::ITEM_CLASS_BLADE,				3,		4,		"BLRES+3",		80,		"������ ���̵�" },
	{ Item::ITEM_CLASS_SG,					3,		4,		"TOHIT+3",		80,		"AM-99 ����" },
	{ Item::ITEM_CLASS_SMG,					3,		4,		"INT+5",		80,		"B-INTER" },
	{ Item::ITEM_CLASS_AR,					3,		4,		"ACRES+3",		80,		"MK-2 G2" },
	{ Item::ITEM_CLASS_SR,					3,		4,		"ATTR+3",		80,		"X-45T �丶ȣũ" },
	{ Item::ITEM_CLASS_NECKLACE,			4,		4,		"TOHIT+4",		80,		"ũ����Ʈ ��ũ����" },
	{ Item::ITEM_CLASS_BRACELET,			4,		4,		"DEF+4",		80,		"������ �극�̽���" },
	{ Item::ITEM_CLASS_NECKLACE,			5,		4,		"CURES+2",		80,		"��� ����" },
	{ Item::ITEM_CLASS_BRACELET,			5,		4,		"HP+5",			80,		"������ �극�̽���" },
	{ Item::ITEM_CLASS_RING,				5,		4,		"VIS+3",		80,		"����� ����" },
	{ Item::ITEM_CLASS_CARRYING_RECEIVER,	0,		4,		"",				20,		"ĳ�� ���ù� 1��" },
	{ Item::ITEM_CLASS_RESURRECT_ITEM,		0,		0,		"",				200,	"��Ȱ ��ũ��" },
	{ Item::ITEM_CLASS_RESURRECT_ITEM,		1,		0,		"",				100,	"������ ��ũ��" },
	{ Item::ITEM_CLASS_EVENT_ETC,			13,		0,		"",				3000,	"���� ����" },
	{ Item::ITEM_CLASS_EVENT_ETC,			3,		0,		"",				4740,	"�巡�� ����" },
	{ Item::ITEM_CLASS_EVENT_TREE,			27,		0,		"",				500,	"12�ð� �˸���" }
};

const int EVENT_ITEM_200505_VAMPIRE_MAX = 19;
const EVENT_ITEM_TEMPLATE Event200505VampireTemplate[EVENT_ITEM_200505_VAMPIRE_MAX] =
{
	{ Item::ITEM_CLASS_VAMPIRE_COAT,		6,		4,		"CRI+9",		110,	"���̵� �κ�" },
	{ Item::ITEM_CLASS_VAMPIRE_COAT,		8,		4,		"VIS+3",		110,	"��Ƽ-�� ��" },
	{ Item::ITEM_CLASS_VAMPIRE_COAT,		7,		4,		"DUR+5",		110,	"��������Ʈ Ŭ��" },
	{ Item::ITEM_CLASS_VAMPIRE_COAT,		9,		4,		"VIS+3",		110,	"��Ƽ-�� ����" },
	{ Item::ITEM_CLASS_VAMPIRE_NECKLACE,	3,		4,		"TOHIT+5",		110,	"�� ��ũ����" },
	{ Item::ITEM_CLASS_VAMPIRE_BRACELET,	2,		4,		"DEF+5",		110,	"��� �극�̽���" },
	{ Item::ITEM_CLASS_VAMPIRE_RING,		3,		4,		"CURES+3",		110,	"��Ʈ ��" },
	{ Item::ITEM_CLASS_VAMPIRE_EARRING,		3,		4,		"HP+5",			110,	"��� �̾" },
	{ Item::ITEM_CLASS_VAMPIRE_AMULET,		2,		4,		"PORES+3",		110,	"����" },
	{ Item::ITEM_CLASS_VAMPIRE_WEAPON,		3,		4,		"RES+5",		110,	"���̳θ� ��Ŭ" },
	{ Item::ITEM_CLASS_VAMPIRE_WEAPON,		4,		4,		"ATTR+3",		110,	"��Ƽ ũ�ο�" },
	{ Item::ITEM_CLASS_VAMPIRE_WEAPON,		5,		4,		"INT+5",		110,	"Ĺ�� ũ�ο�" },
	{ Item::ITEM_CLASS_VAMPIRE_WEAPON,		6,		4,		"DAM+4",		110,	"�ǽ�Ʈ ��Ŭ" },
	{ Item::ITEM_CLASS_PERSONA,				0,		4,		"",				20,		"�丣�ҳ� 1��" },
	{ Item::ITEM_CLASS_RESURRECT_ITEM,		0,		0,		"",				200,	"��Ȱ ��ũ��" },
	{ Item::ITEM_CLASS_RESURRECT_ITEM,		1,		0,		"",				100,	"������ ��ũ��" },
	{ Item::ITEM_CLASS_EVENT_ETC,			13,		0,		"",				3000,	"���� ����" },
	{ Item::ITEM_CLASS_EVENT_ETC,			3,		0,		"",				4750,	"�巡�� ����" },
	{ Item::ITEM_CLASS_EVENT_TREE,			27,		0,		"",				500,	"12�ð� �˸���" }
};

const int EVENT_ITEM_200505_OUSTERS_MAX = 19;
const EVENT_ITEM_TEMPLATE Event200505OustersTemplate[EVENT_ITEM_200505_OUSTERS_MAX] =
{
	{ Item::ITEM_CLASS_OUSTERS_COAT,		4,		4,		"DUR+5",		110,	"�������� ��Ʈ" },
	{ Item::ITEM_CLASS_OUSTERS_BOOTS,		4,		4,		"ASPD+5",		110,	"�������� ����" },
	{ Item::ITEM_CLASS_OUSTERS_BOOTS,		4,		4,		"VIS+3",		110,	"�������� ����" },
	{ Item::ITEM_CLASS_OUSTERS_ARMSBAND,	4,		4,		"CURES+3",		110,	"����� �Ͻ����" },
	{ Item::ITEM_CLASS_OUSTERS_CIRCLET,		3,		4,		"HP+5",			110,	"�ǹٳ� ��Ŭ��" },
	{ Item::ITEM_CLASS_OUSTERS_PENDENT,		3,		4,		"TOHIT+4",		110,	"������ ���Ʈ" },
	{ Item::ITEM_CLASS_OUSTERS_RING,		3,		4,		"CURES+2",		110,	"������ ���� ���� ��" },
	{ Item::ITEM_CLASS_OUSTERS_STONE,		10,		4,		"VIS+3",		110,	"������ ���ɼ�1" },
	{ Item::ITEM_CLASS_OUSTERS_STONE,		6,		4,		"MP+5",			110,	"���� ���ɼ�2" },
	{ Item::ITEM_CLASS_OUSTERS_WRISTLET,	4,		4,		"ASPD+5",		110,	"���� �н��÷��� ����Ʋ��" },
	{ Item::ITEM_CLASS_OUSTERS_WRISTLET,	15,		4,		"HP+5",			110,	"���� ��ũ���� ����Ʋ��" },
	{ Item::ITEM_CLASS_OUSTERS_CHAKRAM,		3,		4,		"DAM+5",		110,	"���� íũ��" },
	{ Item::ITEM_CLASS_OUSTERS_CHAKRAM,		4,		4,		"ACRES+3",		110,	"�ƴϸ� íũ��" },
	{ Item::ITEM_CLASS_FASCIA,				0,		4,		"",				20,		"���̻� 1��" },
	{ Item::ITEM_CLASS_RESURRECT_ITEM,		0,		0,		"",				200,	"��Ȱ ��ũ��" },
	{ Item::ITEM_CLASS_RESURRECT_ITEM,		1,		0,		"",				100,	"������ ��ũ��" },
	{ Item::ITEM_CLASS_EVENT_ETC,			13,		0,		"",				3000,	"���� ����" },
	{ Item::ITEM_CLASS_EVENT_ETC,			3,		0,		"",				4750,	"�巡�� ����" },
	{ Item::ITEM_CLASS_EVENT_TREE,			27,		0,		"",				500,	"12�ð� �˸���" }
};

////////////////////////////////////////////////////////////////////////////////
// ActionGiveCommonEventItem
////////////////////////////////////////////////////////////////////////////////
ActionGiveCommonEventItem::ActionGiveCommonEventItem()
{
}

////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
ActionGiveCommonEventItem::~ActionGiveCommonEventItem()
{
}

////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////
void ActionGiveCommonEventItem::read(PropertyBuffer & propertyBuffer)
    throw(Error)
{
    __BEGIN_TRY

	try
	{
		// read script id
		m_Type = propertyBuffer.getProperty("Type");
	}
	catch (NoSuchElementException & nsee)
	{
		throw Error(nsee.toString());
	}

	__END_CATCH
}

////////////////////////////////////////////////////////////////////////////////
// �׼��� �����Ѵ�.
////////////////////////////////////////////////////////////////////////////////
void ActionGiveCommonEventItem::execute(Creature * pCreature1 , Creature * pCreature2) 
	throw(Error)
{
	__BEGIN_TRY

	Assert(pCreature1 != NULL);
	Assert(pCreature2 != NULL);
	Assert(pCreature1->isNPC());
	Assert(pCreature2->isPC());

	PlayerCreature* pPC = dynamic_cast<PlayerCreature*>(pCreature2);
	Assert(pPC != NULL);

	Player* pPlayer = pPC->getPlayer();
	Assert(pPlayer != NULL);

	Inventory* pInventory = pPC->getInventory();
	Assert (pInventory != NULL);

	Zone* pZone = pPC->getZone();
	Assert (pZone != NULL);

	// �̺�Ʈ �Ⱓ�� �ƴ� ���
	if (g_pVariableManager->getVariable(FAMILY_COIN_EVENT ) == 0 )
	{
		GCNPCResponse response;
		response.setCode(NPC_RESPONSE_QUIT_DIALOGUE);
		pPlayer->sendPacket(&response);

		return;
	}

	// �κ��丮 ���� üũ
	_TPOINT pt;
	if (!pInventory->getEmptySlot(2, 3, pt ) )
	{
		GCNPCResponse response;
		response.setCode(NPC_RESPONSE_QUIT_DIALOGUE);
		pPlayer->sendPacket(&response);

		return;
	}


	// �йи� ���� üũ
	if (!pInventory->hasEnoughNumItem(Item::ITEM_CLASS_EVENT_ETC, 18, 9 ) )
	{
		GCNPCResponse response;
		response.setCode(NPC_RESPONSE_QUIT_DIALOGUE);
		pPlayer->sendPacket(&response);

		return;
	}

	// ������ ���� ����
	int ratio = rand() % 10000 + 1;

	Item::ItemClass itemClass;
	ItemType_t itemType;
	Grade_t itemGrade;
	string itemOption;

	int sum = 0;
	bool bFind = false;
	
	int itemIndex = 0;
	if (pPC->isSlayer() )
	{
		Slayer* pSlayer = dynamic_cast<Slayer*>(pPC);
		Assert(pSlayer != NULL);

		for (int i = 0; i < EVENT_ITEM_200505_SLAYER_MAX; ++i )
		{
			sum += Event200505SlayerTemplate[i].Ratio;
			if (sum >= ratio )
			{
				bFind = true;
				itemIndex = i;

				itemClass = Event200505SlayerTemplate[i].ItemClass;
				itemType = Event200505SlayerTemplate[i].ItemType;
				itemGrade = Event200505SlayerTemplate[i].ItemGrade;
				itemOption = Event200505SlayerTemplate[i].ItemOption;

				break;
			}
		}
	}
	else if (pPC->isVampire() )
	{
		Vampire* pVampire = dynamic_cast<Vampire*>(pPC);
		Assert(pVampire != NULL);

		for (int i = 0; i < EVENT_ITEM_200505_VAMPIRE_MAX; ++i )
		{
			sum += Event200505VampireTemplate[i].Ratio;
			if (sum >= ratio )
			{
				bFind = true;
				itemIndex = i;

				itemClass = Event200505VampireTemplate[i].ItemClass;
				itemType = Event200505VampireTemplate[i].ItemType;
				itemGrade = Event200505VampireTemplate[i].ItemGrade;
				itemOption = Event200505VampireTemplate[i].ItemOption;

				break;
			}
		}
	}
	else if (pPC->isOusters() )
	{
		Ousters* pOusters = dynamic_cast<Ousters*>(pPC);
		Assert(pOusters != NULL);

		for (int i = 0; i < EVENT_ITEM_200505_OUSTERS_MAX; ++i )
		{
			sum += Event200505OustersTemplate[i].Ratio;
			if (sum >= ratio )
			{
				bFind = true;
				itemIndex = i;

				itemClass = Event200505OustersTemplate[i].ItemClass;
				itemType = Event200505OustersTemplate[i].ItemType;
				itemGrade = Event200505OustersTemplate[i].ItemGrade;
				itemOption = Event200505OustersTemplate[i].ItemOption;

				break;
			}
		}
	}

	if (bFind )
	{
		// ������ ���� �� �߰�
		list<OptionType_t> options;

		// ������ �ɼ� üũ
		if (itemOption != "" )
		{
			options.push_back(g_pOptionInfoManager->getOptionType(itemOption ));
		}

		Item* pItem = g_pItemFactoryManager->createItem(itemClass, itemType, options);
		Assert(pItem != NULL);

		// ������ �޼� üũ
		if (itemGrade != 0 )
		{
			pItem->setGrade(itemGrade);
		}

		pZone->registerObject(pItem);

		if (pInventory->addItem(pItem, pt ) )
		{
			// �йи� ������ ���ش�.
			pInventory->decreaseNumItem(Item::ITEM_CLASS_EVENT_ETC, 18, 9, pPlayer);

			// DB �� ������ ����
			pItem->create(pPC->getName(), STORAGE_INVENTORY, 0, pt.x, pt.y);

			// itemTraceLog �� �����.
			remainTraceLog(pItem, "Event200505", pPC->getName(), ITEM_LOG_CREATE, DETAIL_EVENTNPC);

			// ����� Ŭ���̾�Ʈ�� �˸���
			GCCreateItem gcCreateItem;
			gcCreateItem.setObjectID(pItem->getObjectID());
			gcCreateItem.setItemClass(pItem->getItemClass());
			gcCreateItem.setItemType(pItem->getItemType());
			gcCreateItem.setGrade(pItem->getGrade());
			gcCreateItem.setOptionType(pItem->getOptionTypeList());
			gcCreateItem.setDurability(pItem->getDurability());
			gcCreateItem.setItemNum(pItem->getNum());
			gcCreateItem.setInvenX(pt.x);
			gcCreateItem.setInvenY(pt.y);

			pPlayer->sendPacket(&gcCreateItem);
		}

		Statement* pStmt = NULL;

		BEGIN_DB
		{
			pStmt = g_pDatabaseManager->getConnection("DARKEDEN")->createStatement();
			pStmt->executeQuery("UPDATE EventItemCount2 SET Count = Count + 1 WHERE Race = %d AND ItemIndex = %d",
									pPC->getRace(), itemIndex);
		}
		END_DB(pStmt )
	}

	// ��ȭâ �ݱ�
	GCNPCResponse response;
	response.setCode(NPC_RESPONSE_QUIT_DIALOGUE);
	pPlayer->sendPacket(&response);

	__END_CATCH
}


////////////////////////////////////////////////////////////////////////////////
// get debug string
////////////////////////////////////////////////////////////////////////////////
string ActionGiveCommonEventItem::toString () const 
	throw()
{
	__BEGIN_TRY

	StringStream msg;
	msg << "ActionGiveCommonEventItem("
		<< "Type:" << m_Type
	    << ")";
	return msg.toString();

	__END_CATCH
}

