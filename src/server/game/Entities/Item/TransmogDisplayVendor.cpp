/*
Transmog display vendor
Code by Rochet2
Ideas LilleCarl

ScriptName for NPC:
NPC_TransmogDisplayVendor

Compatible with Transmogrification 6.1 by Rochet2
http://rochet2.github.io/Transmogrification
*/

#include "TransmogDisplayVendorConf.h"
#include "Common.h"
#include "Creature.h"
#include "QueryResult.h"
#include "ScriptedGossip.h"
#include "SharedDefines.h"
#include "DatabaseEnv.h"
#include "Transaction.h"
#include "TemporarySummon.h"
#include "Field.h"
#include "DBCStructure.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Language.h"
#include "Log.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "GameEventMgr.h"
#include <sstream>
#include <string>

// Config start

// Edit Transmogrification compatibility in TransmogDisplayVendorConf.h


// A multiplier for the default gold cost (change to 0.0f for no default cost)

const float TransmogDisplayVendorMgr::ScaledCostModifier = 1.1f;
// Cost added on top of other costs (can be negative)
const int32 TransmogDisplayVendorMgr::CopperCost = 0;
// For custom gold cost set ScaledCostModifier to 0.0f and CopperCost to what ever cost you want

const bool TransmogDisplayVendorMgr::RequireToken = false;
const uint32 TransmogDisplayVendorMgr::TokenEntry = 49426;
const uint32 TransmogDisplayVendorMgr::TokenAmount = 1;

const bool TransmogDisplayVendorMgr::AllowPoor = true;
const bool TransmogDisplayVendorMgr::AllowCommon = true;
const bool TransmogDisplayVendorMgr::AllowUncommon = true;
const bool TransmogDisplayVendorMgr::AllowRare = true;
const bool TransmogDisplayVendorMgr::AllowEpic = true;
const bool TransmogDisplayVendorMgr::AllowLegendary = true;
const bool TransmogDisplayVendorMgr::AllowArtifact = true;
const bool TransmogDisplayVendorMgr::AllowHeirloom = true;

const bool TransmogDisplayVendorMgr::AllowMixedArmorTypes = true;
const bool TransmogDisplayVendorMgr::AllowMixedWeaponTypes = false;
const bool TransmogDisplayVendorMgr::AllowFishingPoles = false;

const bool TransmogDisplayVendorMgr::IgnoreReqRace = false;
const bool TransmogDisplayVendorMgr::IgnoreReqClass = false;
const bool TransmogDisplayVendorMgr::IgnoreReqSkill = true;
const bool TransmogDisplayVendorMgr::IgnoreReqSpell = true;
const bool TransmogDisplayVendorMgr::IgnoreReqLevel = true;
const bool TransmogDisplayVendorMgr::IgnoreReqEvent = true;
const bool TransmogDisplayVendorMgr::IgnoreReqStats = true;

// Example AllowedItems[] = { 123, 234, 345 };
const std::set<uint32> TransmogDisplayVendorMgr::AllowedItems = {};
const std::set<uint32> TransmogDisplayVendorMgr::NotAllowedItems = {};
//Lista de ids visualizables
std::unordered_set<uint32> TransmogDisplayVendorMgr::Lista;


// Config end

std::vector<uint32> TransmogDisplayVendorMgr::Allowed;
std::vector<uint32> TransmogDisplayVendorMgr::NotAllowed;
SelectionStore TransmogDisplayVendorMgr::selectionStore;
TransmogDisplayVendorMgr::EntryVector* TransmogDisplayVendorMgr::optionMap[MAX_ITEM_SUBCLASS_WEAPON + MAX_ITEM_SUBCLASS_ARMOR][MAX_INVTYPE][MAX_ITEM_QUALITY];

uint32 TransmogDisplayVendorMgr::GetFakeEntry(const Item* item)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::GetFakeEntry");

	Player* owner = item->GetOwner();

	if (!owner)
		return 0;
	if (owner->transmogMap.empty())
		return 0;

	TransmogMapType::const_iterator it = owner->transmogMap.find(item->GetGUID());
	if (it == owner->transmogMap.end())
		return 0;
	return it->second;
}
void TransmogDisplayVendorMgr::DeleteFakeEntry(Player* player, Item* item)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::DeleteFakeEntry");

	if (player->transmogMap.erase(item->GetGUID()) != 0)
		UpdateItem(player, item);
}
void TransmogDisplayVendorMgr::SetFakeEntry(Player* player, Item* item, uint32 entry)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::SetFakeEntry");

	player->transmogMap[item->GetGUID()] = entry;
	UpdateItem(player, item);
}
void TransmogDisplayVendorMgr::UpdateItem(Player* player, Item* item)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::UpdateItem");

	if (item->IsEquipped())
	{
		player->SetVisibleItemSlot(item->GetSlot(), item);
		if (player->IsInWorld())
			item->SendUpdateToPlayer(player);
	}
}
const char* TransmogDisplayVendorMgr::getSlotName(uint8 slot, WorldSession* /*session*/)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::TransmogDisplayVendorMgr::getSlotName");

	switch (slot)
	{
	case EQUIPMENT_SLOT_HEAD: return  "Cabeza";//"Head";// session->GetTrinityString(LANG_SLOT_NAME_HEAD);
	case EQUIPMENT_SLOT_SHOULDERS: return  "Hombreras";// "Shoulders";// session->GetTrinityString(LANG_SLOT_NAME_SHOULDERS);
	case EQUIPMENT_SLOT_BODY: return  "Camisa";// "Shirt";// session->GetTrinityString(LANG_SLOT_NAME_BODY);
	case EQUIPMENT_SLOT_CHEST: return  "Pecho";// "Chest";// session->GetTrinityString(LANG_SLOT_NAME_CHEST);
	case EQUIPMENT_SLOT_WAIST: return  "Cintura";// "Waist";// session->GetTrinityString(LANG_SLOT_NAME_WAIST);
	case EQUIPMENT_SLOT_LEGS: return  "Piernas";// "Legs";// session->GetTrinityString(LANG_SLOT_NAME_LEGS);
	case EQUIPMENT_SLOT_FEET: return  "Pies";// "Feet";// session->GetTrinityString(LANG_SLOT_NAME_FEET);
	case EQUIPMENT_SLOT_WRISTS: return  "Brazos";// "Wrists";// session->GetTrinityString(LANG_SLOT_NAME_WRISTS);
	case EQUIPMENT_SLOT_HANDS: return  "Manos";// "Hands";// session->GetTrinityString(LANG_SLOT_NAME_HANDS);
	case EQUIPMENT_SLOT_BACK: return  "Espalda";// "Back";// session->GetTrinityString(LANG_SLOT_NAME_BACK);
	case EQUIPMENT_SLOT_MAINHAND: return  "Mano Derecha";// "Main hand";// session->GetTrinityString(LANG_SLOT_NAME_MAINHAND);
	case EQUIPMENT_SLOT_OFFHAND: return  "Mano Izquierda";// "Off hand";// session->GetTrinityString(LANG_SLOT_NAME_OFFHAND);
	case EQUIPMENT_SLOT_RANGED: return  "A Distancia";// "Ranged";// session->GetTrinityString(LANG_SLOT_NAME_RANGED);
	case EQUIPMENT_SLOT_TABARD: return  "Tabardo";// "Tabard";// session->GetTrinityString(LANG_SLOT_NAME_TABARD);
	default: return NULL;
	}
}
uint32 TransmogDisplayVendorMgr::GetSpecialPrice(ItemTemplate const* proto)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::GetSpecialPrice");

	uint32 cost = proto->BuyPrice < 10000 ? 10000 : proto->BuyPrice;
	return cost;
}

bool TransmogDisplayVendorMgr::CanTransmogrifyItemConjuntos(Player* player, ItemTemplate const* target)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::CanTransmogrifyItemWithItem");

	if (!target)
		return false;
	if (target->Class == ITEM_CLASS_WEAPON)
		return false;

	if (target->InventoryType == INVTYPE_BAG ||
		target->InventoryType == INVTYPE_RELIC ||
		// target->InventoryType == INVTYPE_BODY ||
		target->InventoryType == INVTYPE_FINGER ||
		target->InventoryType == INVTYPE_TRINKET ||
		target->InventoryType == INVTYPE_AMMO ||
		target->InventoryType == INVTYPE_QUIVER)
		return false;

	if (!SuitableForTransmogrification(player, target)) // if (!transmogrified->CanTransmogrify() || !transmogrifier->CanBeTransmogrified())
		return false;

	return true;
}


bool TransmogDisplayVendorMgr::CanTransmogrifyItemWithItem(Player* player, ItemTemplate const* target, ItemTemplate const* source)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::CanTransmogrifyItemWithItem");

	if (!target || !source)
		return false;

	if (source->ItemId == target->ItemId)
		return false;

	if (source->DisplayInfoID == target->DisplayInfoID)
		return false;

	if (source->Class != target->Class)
		return false;

	if (source->InventoryType == INVTYPE_BAG ||
		source->InventoryType == INVTYPE_RELIC ||
		// source->InventoryType == INVTYPE_BODY ||
		source->InventoryType == INVTYPE_FINGER ||
		source->InventoryType == INVTYPE_TRINKET ||
		source->InventoryType == INVTYPE_AMMO ||
		source->InventoryType == INVTYPE_QUIVER)
		return false;

	if (target->InventoryType == INVTYPE_BAG ||
		target->InventoryType == INVTYPE_RELIC ||
		// target->InventoryType == INVTYPE_BODY ||
		target->InventoryType == INVTYPE_FINGER ||
		target->InventoryType == INVTYPE_TRINKET ||
		target->InventoryType == INVTYPE_AMMO ||
		target->InventoryType == INVTYPE_QUIVER)
		return false;

	if (!SuitableForTransmogrification(player, target) || !SuitableForTransmogrification(player, source)) // if (!transmogrified->CanTransmogrify() || !transmogrifier->CanBeTransmogrified())
		return false;

	if (IsRangedWeapon(source->Class, source->SubClass) != IsRangedWeapon(target->Class, target->SubClass))
		return false;

	if (source->SubClass != target->SubClass && !IsRangedWeapon(target->Class, target->SubClass))
	{
		if (source->Class == ITEM_CLASS_ARMOR && !AllowMixedArmorTypes)
			return false;
		if (source->Class == ITEM_CLASS_WEAPON && !AllowMixedWeaponTypes)
			return false;
	}

	if (source->InventoryType != target->InventoryType)
	{
		if (source->Class == ITEM_CLASS_WEAPON && !((IsRangedWeapon(target->Class, target->SubClass) ||
			((target->InventoryType == INVTYPE_WEAPON || target->InventoryType == INVTYPE_2HWEAPON) &&
			(source->InventoryType == INVTYPE_WEAPON || source->InventoryType == INVTYPE_2HWEAPON)) ||
				((target->InventoryType == INVTYPE_WEAPONMAINHAND || target->InventoryType == INVTYPE_WEAPONOFFHAND) &&
			(source->InventoryType == INVTYPE_WEAPON || source->InventoryType == INVTYPE_2HWEAPON)))))
			return false;
		if (source->Class == ITEM_CLASS_ARMOR &&
			!((source->InventoryType == INVTYPE_CHEST || source->InventoryType == INVTYPE_ROBE) &&
			(target->InventoryType == INVTYPE_CHEST || target->InventoryType == INVTYPE_ROBE)))
			return false;
	}

	return true;
}
bool TransmogDisplayVendorMgr::SuitableForTransmogrification(Player* player, ItemTemplate const* proto)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::SuitableForTransmogrification");

	// ItemTemplate const* proto = item->GetTemplate();
	if (!proto)
		return false;

	if (proto->Class != ITEM_CLASS_ARMOR &&
		proto->Class != ITEM_CLASS_WEAPON)
		return false;

	// Skip all checks for allowed items
	if (IsAllowed(proto->ItemId))
		return true;

	if (IsNotAllowed(proto->ItemId))
		return false;

	if (!AllowFishingPoles && proto->Class == ITEM_CLASS_WEAPON && proto->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE)
		return false;

	if (!IsAllowedQuality(proto->Quality)) // (proto->Quality == ITEM_QUALITY_LEGENDARY)
		return false;

	if (player)
	{
		if ((proto->Flags2 & ITEM_FLAGS_EXTRA_HORDE_ONLY) && player->GetTeam() != HORDE)
			return false;

		if ((proto->Flags2 & ITEM_FLAGS_EXTRA_ALLIANCE_ONLY) && player->GetTeam() != ALLIANCE)
			return false;

		if (!IgnoreReqClass && (proto->AllowableClass & player->getClassMask()) == 0)
			return false;

		if (!IgnoreReqRace && (proto->AllowableRace & player->getRaceMask()) == 0)
			return false;

		if (!IgnoreReqSkill && proto->RequiredSkill != 0)
		{
			if (player->GetSkillValue(proto->RequiredSkill) == 0)
				return false;
			else if (player->GetSkillValue(proto->RequiredSkill) < proto->RequiredSkillRank)
				return false;
		}

		if (!IgnoreReqSpell && proto->RequiredSpell != 0 && !player->HasSpell(proto->RequiredSpell))
			return false;

		if (!IgnoreReqLevel && player->getLevel() < proto->RequiredLevel)
			return false;
	}

	// If World Event is not active, prevent using event dependant items
	if (!IgnoreReqEvent && proto->HolidayId && !IsHolidayActive((HolidayIds)proto->HolidayId))
		return false;

	if (!IgnoreReqStats)
	{
		if (!proto->RandomProperty && !proto->RandomSuffix)
		{
			bool found = false;
			for (uint8 i = 0; i < proto->StatsCount; ++i)
			{
				if (proto->ItemStat[i].ItemStatValue != 0)
				{
					found = true;
					break;
				}
			}
			if (!found)
				return false;
		}
	}

	return true;
}

bool TransmogDisplayVendorMgr::IsRangedWeapon(uint32 Class, uint32 SubClass)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::IsRangedWeapon");

	return Class == ITEM_CLASS_WEAPON && (
		SubClass == ITEM_SUBCLASS_WEAPON_BOW ||
		SubClass == ITEM_SUBCLASS_WEAPON_GUN ||
		SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW);
}
bool TransmogDisplayVendorMgr::IsAllowed(uint32 entry)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::IsAllowed");

	return std::find(Allowed.begin(), Allowed.end(), entry) != Allowed.end();
}
bool TransmogDisplayVendorMgr::IsNotAllowed(uint32 entry)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::IsNotAllowed");

	return std::find(NotAllowed.begin(), NotAllowed.end(), entry) != NotAllowed.end();
}
bool TransmogDisplayVendorMgr::IsAllowedQuality(uint32 quality)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::IsAllowedQuality");

	switch (quality)
	{
	case ITEM_QUALITY_POOR: return AllowPoor;
	case ITEM_QUALITY_NORMAL: return AllowCommon;
	case ITEM_QUALITY_UNCOMMON: return AllowUncommon;
	case ITEM_QUALITY_RARE: return AllowRare;
	case ITEM_QUALITY_EPIC: return AllowEpic;
	case ITEM_QUALITY_LEGENDARY: return AllowLegendary;
	case ITEM_QUALITY_ARTIFACT: return AllowArtifact;
	case ITEM_QUALITY_HEIRLOOM: return AllowHeirloom;
	default: return false;
	}
}

uint32 GetTmorgSlot(uint32 InventoryType)
{
	uint32 ret = EQUIPMENT_SLOT_END;

	switch (InventoryType)
	{
	case INVTYPE_HEAD:
		ret = EQUIPMENT_SLOT_HEAD;
		break;
	case INVTYPE_NECK:
		ret = EQUIPMENT_SLOT_NECK;
		break;
	case INVTYPE_SHOULDERS:
		ret = EQUIPMENT_SLOT_SHOULDERS;
		break;
	case INVTYPE_BODY:
		ret = EQUIPMENT_SLOT_BODY;
		break;
	case INVTYPE_CHEST:
	case INVTYPE_ROBE:
		ret = EQUIPMENT_SLOT_CHEST;
		break;
	case INVTYPE_WAIST:
		ret = EQUIPMENT_SLOT_WAIST;
		break;
	case INVTYPE_LEGS:
		ret = EQUIPMENT_SLOT_LEGS;
		break;
	case INVTYPE_FEET:
		ret = EQUIPMENT_SLOT_FEET;
		break;
	case INVTYPE_WRISTS:
		ret = EQUIPMENT_SLOT_WRISTS;
		break;
	case INVTYPE_HANDS:
		ret = EQUIPMENT_SLOT_HANDS;
		break;
	case INVTYPE_CLOAK:
		ret = EQUIPMENT_SLOT_BACK;
		break;

	default:
		break;
	}

	return ret;
}

void TransmogDisplayVendorMgr::HandleTransmogrify(Player* player, Creature* creature, uint32 vendorslot, uint32 itemEntry, bool no_cost)
{
	TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::HandleTransmogrify");

	SelectionStore::Selection selection;
	if (!selectionStore.GetSelection(player->GetGUID().GetCounter(), selection))
		return; // cheat, no slot selected



	uint8 slot;

	const ItemTemplate* itemTransmogrifier = NULL;
	// guid of the transmogrifier item, if it's not 0
	if (itemEntry)
	{
		itemTransmogrifier = sObjectMgr->GetItemTemplate(itemEntry);
		if (!itemTransmogrifier)
		{
			TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::HandleTransmogrify - %s (%s) tried to transmogrify with an invalid item entry %u.", player->GetName().c_str(), player->GetGUID().ToString().c_str(), itemEntry);
			return; // LANG_ERR_TRANSMOG_MISSING_SRC_ITEM
		}
	}

	slot = selection.slot != 100 ? selection.slot : GetTmorgSlot(itemTransmogrifier->InventoryType);

	const char* slotname = TransmogDisplayVendorMgr::getSlotName(slot, player->GetSession());
	if (!slotname)
		return;

	// slot of the transmogrified item
	if (slot >= EQUIPMENT_SLOT_END)
	{
		TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::HandleTransmogrify - %s (%s) tried to transmogrify item %u with a wrong slot (%u) when transmogrifying items.", player->GetName().c_str(), player->GetGUID().ToString().c_str(), itemEntry, slot);
		return; // LANG_ERR_TRANSMOG_INVALID_SLOT
	}



	// transmogrified item
	Item* itemTransmogrified = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
	if (!itemTransmogrified)
	{
		TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::HandleTransmogrify - %s (%s) tried to transmogrify an invalid item in a valid slot (slot: %u).", player->GetName().c_str(), player->GetGUID().ToString().c_str(), slot);
		player->GetSession()->SendNotification("No hay objeto en el slot %s", slotname);
		return; // LANG_ERR_TRANSMOG_MISSING_DEST_ITEM
	}

	if (!itemTransmogrifier) // reset look newEntry
	{
		DeleteFakeEntry(player, itemTransmogrified);
	}
	else
	{
		if (!CanTransmogrifyItemWithItem(player, itemTransmogrified->GetTemplate(), itemTransmogrifier))
		{
			TC_LOG_DEBUG("custom.transmog", "TransmogDisplayVendorMgr::HandleTransmogrify - %s (%s) failed CanTransmogrifyItemWithItem (%u with %u).", player->GetName().c_str(), player->GetGUID().ToString().c_str(), itemTransmogrified->GetEntry(), itemTransmogrifier->ItemId);
			player->GetSession()->SendNotification("El objeto equipado no es adecuado para la transfiguracion seleccionada");
			return; // LANG_ERR_TRANSMOG_INVALID_ITEMS
		}

		if (uint32 fakeEntry = GetFakeEntry(itemTransmogrified))
		{
			if (const ItemTemplate* fakeItemTemplate = sObjectMgr->GetItemTemplate(fakeEntry))
			{
				if (fakeItemTemplate->DisplayInfoID == itemTransmogrifier->DisplayInfoID)
				{
					player->GetSession()->SendNotification("Tu [%s] ya esta transfigurado con [%s]", slotname, getItemName(itemTransmogrifier, player->GetSession()).c_str());
					return;
				}
			}
		}

		if (!no_cost)
		{
			if (RequireToken)
			{
				if (player->HasItemCount(TokenEntry, TokenAmount))
				{
					player->DestroyItemCount(TokenEntry, TokenAmount, true);
				}
				else
				{
					player->GetSession()->SendNotification("No tienes suficientes %ss", getItemName(sObjectMgr->GetItemTemplate(TransmogDisplayVendorMgr::TokenEntry), player->GetSession()).c_str());
					return; // LANG_ERR_TRANSMOG_NOT_ENOUGH_TOKENS
				}
			}


			uint32 cost = sWorld->getIntConfig(CONFIG_TMORG_COST) * player->getLevel() / 80;
			if (cost < 195) cost = 195; //Minimo, 1 plata 95 cobre

			if (cost) // 0 cost if reverting look
			{
				if (!player->HasEnoughMoney(cost))
				{
					player->GetSession()->SendNotification("No tienes suficiente dinero");
					return; // LANG_ERR_TRANSMOG_NOT_ENOUGH_MONEY
				}

				#pragma warning(suppress: 4146)
				player->ModifyMoney(-cost, false);

			}

			SetFakeEntry(player, itemTransmogrified, itemTransmogrifier->ItemId);

			itemTransmogrified->UpdatePlayedTime(player);

			itemTransmogrified->SetOwnerGUID(player->GetGUID());
			itemTransmogrified->SetNotRefundable(player);
			itemTransmogrified->ClearSoulboundTradeable(player);


		}

		player->PlayDirectSound(3337);
		player->CastSpell(player, 48335, true, NULL, NULL, player->GetGUID());
		creature->CastSpell(creature, 48335);
		player->GetSession()->SendAreaTriggerMessage("Transfigurado %s", slotname);

		/*	CloseGossipMenuFor(player);
			if (creature->IsSummon()) creature->ToTempSummon()->UnSummon();
			player->SendGossipRequest(TRANSMORG_GOSSIP_HELLO_REQUEST);*/

			//return LANG_ERR_TRANSMOG_OK;
	}
}

const char * TransmogDisplayVendorMgr::getQualityName(uint32 quality)
{
	switch (quality)
	{
	case ITEM_QUALITY_POOR: return "|CFF545454[Pobre]";
	case ITEM_QUALITY_NORMAL: return "|CFF9d9d9d[Comun]";
	case ITEM_QUALITY_UNCOMMON: return "|CFF0B5F00[Poco Comun]";
	case ITEM_QUALITY_RARE: return "|CFF004C97[Raro]";
	case ITEM_QUALITY_EPIC: return "|CFF6711A2[Epico]";
	case ITEM_QUALITY_LEGENDARY: return "|CFF995516[Legendario]";
	case ITEM_QUALITY_ARTIFACT: return "|CFF735C15[Artefacto]";
	case ITEM_QUALITY_HEIRLOOM: return "|CFF0C7362[Reliquia]";
	default: return "|CFFe5cc80[ESPECIAL]";
	}
}

std::string TransmogDisplayVendorMgr::getItemName(const ItemTemplate * itemTemplate, WorldSession * session)
{
	std::string name = itemTemplate->Name1;
	//int loc_idx = session->GetSessionDbLocaleIndex();
	LocaleConstant loc_const = session->GetSessionDbcLocale();
	if (loc_const)//loc_idx >= 0)
		if (ItemLocale const* il = sObjectMgr->GetItemLocale(itemTemplate->ItemId))
			sObjectMgr->GetLocaleString(il->Name, loc_const, name);
	//sObjectMgr->GetLocaleString(il->Name, loc_idx, name);
	return name;
}

uint32 TransmogDisplayVendorMgr::getCorrectInvType(uint32 inventorytype)
{
	switch (inventorytype)
	{
	case INVTYPE_WEAPONMAINHAND:
	case INVTYPE_WEAPONOFFHAND:
		return INVTYPE_WEAPON;
	case INVTYPE_RANGEDRIGHT:
		return INVTYPE_RANGED;
	case INVTYPE_ROBE:
		return INVTYPE_CHEST;
	default:
		return inventorytype;
	}
}
