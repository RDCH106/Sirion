/*
Transmog display vendor
Code by Rochet2
Ideas LilleCarl

ScriptName for NPC:
NPC_TransmogDisplayVendor

Compatible with Transmogrification 6.1 by Rochet2
http://rochet2.github.io/Transmogrification
*/

#include "..\Auxiliares\TransmorgVendor\TransmogVendor_menu.h"
#include "TemporarySummon.h"


class NPC_TransmogDisplayVendor : public CreatureScript
{
public:
	NPC_TransmogDisplayVendor() : CreatureScript("NPC_TransmogDisplayVendor") { } // If you change this, also change in Player.cpp: if (creature->GetScriptName() == "NPC_TransmogDisplayVendor")

	struct npc_TransmogDisplayVendorAI : public ScriptedAI
	{
		npc_TransmogDisplayVendorAI(Creature* creature) : ScriptedAI(creature) { }

		bool GossipHello(Player* player) override
		{
			if (me->IsSummon())
			{				
				if (player != me->ToTempSummon()->GetSummoner())
				{					
					me->Whisper("Lo siento, solo puede utilizarme quien me invoco, invocame mediante el uso de los parches, o buscame en las ciudades", LANG_UNIVERSAL, player);
					//Intento esto para cerrarlo
					AddGossipItemFor(player, 0, "", SENDER_END, 0);
					SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
					return false;
				}

			}

			return TransmorgVendor_Gossip::GossipHello(player, me);
		}

		bool GossipSelect(Player *player, uint32 menuId, uint32 gossipListId) override
		{
			return TransmorgVendor_Gossip::GossipSelect(player, me, menuId, gossipListId);
		}

		void UpdateAI(uint32 /*diff*/) override
		{
			if (me->IsSummon())
			{
				if (!me->SelectNearestPlayer(1)  || me->SelectNearestPlayer(1) != me->ToTempSummon()->GetSummoner())
					me->ToTempSummon()->UnSummon();
			}
		}

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_TransmogDisplayVendorAI(creature);
	}


};

#if !TRANSMOGRIFICATION_ALREADY_INSTALLED
class Player_Transmogrify : public PlayerScript
{
public:
	Player_Transmogrify() : PlayerScript("Player_Transmogrify") { }

	std::vector<ObjectGuid> GetItemList(const Player* player) const
	{
		std::vector<ObjectGuid> itemlist;

		// Copy paste from Player::GetItemByGuid(guid)

		for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
			if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
				itemlist.push_back(pItem->GetGUID());

		for (uint8 i = KEYRING_SLOT_START; i < CURRENCYTOKEN_SLOT_END; ++i)
			if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
				itemlist.push_back(pItem->GetGUID());

		for (int i = BANK_SLOT_ITEM_START; i < BANK_SLOT_BAG_END; ++i)
			if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
				itemlist.push_back(pItem->GetGUID());

		for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
			if (Bag* pBag = player->GetBagByPos(i))
				for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
					if (Item* pItem = pBag->GetItemByPos(j))
						itemlist.push_back(pItem->GetGUID());

		for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
			if (Bag* pBag = player->GetBagByPos(i))
				for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
					if (Item* pItem = pBag->GetItemByPos(j))
						itemlist.push_back(pItem->GetGUID());

		return itemlist;
	}

	void OnSave(Player* player) override
	{
		uint32 lowguid = player->GetGUID().GetCounter();
		SQLTransaction trans = CharacterDatabase.BeginTransaction();
		trans->PAppend("DELETE FROM `custom_transmogrification` WHERE `Owner` = %u", lowguid);

		if (!player->transmogMap.empty())
		{
			// Only save items that are in inventory / bank / etc
			std::vector<ObjectGuid> items = GetItemList(player);
			for (std::vector<ObjectGuid>::const_iterator it = items.begin(); it != items.end(); ++it)
			{
				TransmogMapType::const_iterator it2 = player->transmogMap.find(*it);
				if (it2 == player->transmogMap.end())
					continue;

				trans->PAppend("REPLACE INTO custom_transmogrification (GUID, FakeEntry, Owner) VALUES (%u, %u, %u)", it2->first.GetCounter(), it2->second, lowguid);
			}
		}

		if (trans->GetSize()) // basically never false
			CharacterDatabase.CommitTransaction(trans);
	}

	void OnLogin(Player* player, bool /*firstLogin*/) override
	{
		QueryResult result = CharacterDatabase.PQuery("SELECT GUID, FakeEntry FROM custom_transmogrification WHERE Owner = %u", player->GetGUID().GetCounter());

		if (result)
		{
			do
			{
				Field* field = result->Fetch();
				ObjectGuid itemGUID(HighGuid::Item, 0, field[0].GetUInt32());
				uint32 fakeEntry = field[1].GetUInt32();
				// Only load items that are in inventory / bank / etc
				if (sObjectMgr->GetItemTemplate(fakeEntry) && player->GetItemByGuid(itemGUID))
				{
					player->transmogMap[itemGUID] = fakeEntry;
				}
				else
				{
					// Ignore, will be erased on next save.
					// Additionally this can happen if an item was deleted from DB but still exists for the player
					// TC_LOG_ERROR("custom.transmog", "Item entry (Entry: %u, itemGUID: %u, playerGUID: %u) does not exist, ignoring.", fakeEntry, GUID_LOPART(itemGUID), player->GetGUID().GetCounter());
					// CharacterDatabase.PExecute("DELETE FROM custom_transmogrification WHERE FakeEntry = %u", fakeEntry);
				}
			} while (result->NextRow());

			if (!player->transmogMap.empty())
			{
				for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
				{
					if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
					{
						player->SetVisibleItemSlot(slot, item);
						if (player->IsInWorld())
							item->SendUpdateToPlayer(player);
					}
				}
			}
		}
	}

	void OnLogout(Player* player) override
	{
		TransmogDisplayVendorMgr::selectionStore.RemoveSelection(player->GetGUID().GetCounter());
	}
};
#endif

class PREP_TransmogDisplayVendor : public WorldScript
{
public:
	PREP_TransmogDisplayVendor() : WorldScript("PREP_TransmogDisplayVendor") { }

	void OnStartup() override
	{
		for (uint32 v : TransmogDisplayVendorMgr::AllowedItems)
			TransmogDisplayVendorMgr::Allowed.push_back(v);
		for (uint32 v : TransmogDisplayVendorMgr::NotAllowedItems)
			TransmogDisplayVendorMgr::NotAllowed.push_back(v);
		TC_LOG_INFO("server.loading", "Creando lista de items transmutables...");
		// initialize .. for reload in future?
		for (uint32 i = 0; i < MAX_ITEM_SUBCLASS_WEAPON + MAX_ITEM_SUBCLASS_ARMOR; ++i)
			for (uint32 j = 0; j < MAX_INVTYPE; ++j)
				for (uint32 k = 0; k < MAX_ITEM_QUALITY; ++k)
					delete TransmogDisplayVendorMgr::optionMap[i][j][k], TransmogDisplayVendorMgr::optionMap[i][j][k] = NULL;

		std::unordered_set<uint32> displays;
		ItemTemplateContainer const* its = sObjectMgr->GetItemTemplateStore();
		for (ItemTemplateContainer::const_iterator itr = its->begin(); itr != its->end(); ++itr)
		{
			if (itr->second.Class != ITEM_CLASS_WEAPON && itr->second.Class != ITEM_CLASS_ARMOR)
				continue;
			if (!TransmogDisplayVendorMgr::SuitableForTransmogrification(NULL, &itr->second))
				continue;
			if (displays.find(itr->second.DisplayInfoID) != displays.end()) // skip duplicate item displays
				continue;
			TransmogDisplayVendorMgr::EntryVector* oM = TransmogDisplayVendorMgr::optionMap[(itr->second.Class != ITEM_CLASS_WEAPON ? MAX_ITEM_SUBCLASS_WEAPON : 0) + itr->second.SubClass][TransmogDisplayVendorMgr::getCorrectInvType(itr->second.InventoryType)][itr->second.Quality];
			if (!oM)
			{
				oM = new TransmogDisplayVendorMgr::EntryVector();
				TransmogDisplayVendorMgr::optionMap[(itr->second.Class != ITEM_CLASS_WEAPON ? MAX_ITEM_SUBCLASS_WEAPON : 0) + itr->second.SubClass][TransmogDisplayVendorMgr::getCorrectInvType(itr->second.InventoryType)][itr->second.Quality] = oM;
			}
			if (oM->size() < MAX_VENDOR_ITEMS * 3)
			{
				oM->push_back(itr->second.ItemId);
				displays.insert(itr->second.DisplayInfoID);
			}
			else
			{
				TC_LOG_INFO("server.loading", "Too many items for transmogrification: Class: %u SubClass: %u InventoryType: %u Quality: %u", itr->second.Class, itr->second.SubClass, TransmogDisplayVendorMgr::getCorrectInvType(itr->second.InventoryType), itr->second.Quality);
			}
		}

		// resize entry lists
		for (uint32 i = 0; i < MAX_ITEM_SUBCLASS_WEAPON + MAX_ITEM_SUBCLASS_ARMOR; ++i)
			for (uint32 j = 0; j < MAX_INVTYPE; ++j)
				for (uint32 k = 0; k < MAX_ITEM_QUALITY; ++k)
					if (TransmogDisplayVendorMgr::optionMap[i][j][k])
						TransmogDisplayVendorMgr::optionMap[i][j][k]->resize(TransmogDisplayVendorMgr::optionMap[i][j][k]->size());
		TransmogDisplayVendorMgr::Lista = displays;
#if !TRANSMOGRIFICATION_ALREADY_INSTALLED
		TC_LOG_INFO("custom.transmog", "Deleting non-existing transmogrification entries...");
		CharacterDatabase.DirectExecute("DELETE FROM custom_transmogrification WHERE NOT EXISTS (SELECT 1 FROM item_instance WHERE item_instance.guid = custom_transmogrification.GUID)");
#endif
		TC_LOG_INFO("server.loading", "Lista de transmutables creada.");
	}

	void OnShutdown() override
	{
		for (uint32 i = 0; i < MAX_ITEM_SUBCLASS_WEAPON + MAX_ITEM_SUBCLASS_ARMOR; ++i)
			for (uint32 j = 0; j < MAX_INVTYPE; ++j)
				for (uint32 k = 0; k < MAX_ITEM_QUALITY; ++k)
					delete TransmogDisplayVendorMgr::optionMap[i][j][k], TransmogDisplayVendorMgr::optionMap[i][j][k] = NULL;
	}
};



void AddSC_NPC_TransmogDisplayVendor()
{
	new NPC_TransmogDisplayVendor();
	new PREP_TransmogDisplayVendor();

#if !TRANSMOGRIFICATION_ALREADY_INSTALLED
	new Player_Transmogrify();
#endif
}
