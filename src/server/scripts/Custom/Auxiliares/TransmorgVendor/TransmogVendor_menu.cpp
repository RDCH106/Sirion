/*
Transmog display vendor
Code by Rochet2
Ideas LilleCarl

ScriptName for NPC:
NPC_TransmogDisplayVendor

Compatible with Transmogrification 6.1 by Rochet2
http://rochet2.github.io/Transmogrification
*/

#include "TransmogVendor_menu.h"


uint32 nDespawnTime = 60 * MINUTE * IN_MILLISECONDS;

enum TransmogDisplayVendorActions
{
    ACTION_ONLY_MY_CLASS,
    ACTION_REST_CLASS,
    ACTION_ADD_PAGE,
    ACTION_REMOVE_PAGE
};



uint32 GetMaxSubclass(Player* player)
{
    uint32 ret = 0;
    switch (player->getClass())
    {
    case CLASS_WARRIOR:
        ret = 4;
        break;
    case CLASS_PALADIN:
        ret = 4;
        break;
    case CLASS_HUNTER:
        ret = 3;
        break;
    case CLASS_ROGUE:
        ret = 2;
        break;
    case CLASS_PRIEST:
        ret = 1;
        break;
    case CLASS_DEATH_KNIGHT:
        ret = 4;
        break;
    case CLASS_SHAMAN:
        ret = 3;
        break;
    case CLASS_MAGE:
        ret = 1;
        break;
    case CLASS_WARLOCK:
        ret = 1;
        break;
    case CLASS_MONK:
        ret = 2;
        break;
    case CLASS_DRUID:
        ret = 2;
        break;
    default:
        break;
    }

    return ret;
}

bool OnGossipSelect(Player *player, WorldObject* source, uint32 sender, uint32 action)
{
    Qualities* Calidades;
    WorldSession* session = player->GetSession();
    player->PlayerTalkClass->ClearMenus();
    switch (sender)
    {
    case SENDER_SELECT_VENDOR: // action = slot
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, action);
        if (!item)
        {
            if (const char* slotname = TransmogDisplayVendorMgr::getSlotName(action, player->GetSession()))
                session->SendNotification("No hay objeto equipado en el slot de %s", slotname);
            TransmorgVendor_Gossip::GossipHello(player, source);
            return true;
        }
        const ItemTemplate * itemTemplate = item->GetTemplate();
        //AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, (std::string)"Actualizar " + TransmogDisplayVendorMgr::getItemName(itemTemplate, session), sender, action);

        std::string query = "";
        if (itemTemplate->Class == 4 && itemTemplate->SubClass >= 1 && itemTemplate->SubClass <= 4)
        {
            // Esto es ropa, y la selecciono segun el nivel maximo de material
            query = Trinity::StringFormat("SELECT Quality FROM item_template where class = %u and subclass <= %u and subclass >= 1 and entry in (select entry from items_obtenidos where cuenta = %u) group by Quality;", itemTemplate->Class, itemTemplate->SubClass, player->GetSession()->GetAccountId());
        }
        else
            query = Trinity::StringFormat("SELECT Quality FROM item_template where class = %u and subclass = %u and entry in (select entry from items_obtenidos where cuenta = %u) group by Quality;", itemTemplate->Class, itemTemplate->SubClass, player->GetSession()->GetAccountId());

        QueryResult resultado = WorldDatabase.PQuery(query);
        if (resultado)
        {

            int nCantidad = resultado->GetRowCount();
            Calidades = new Qualities[nCantidad];

            int nIteraciones = 0;
            query = "";
            if (itemTemplate->Class == 4 && itemTemplate->SubClass >= 1 && itemTemplate->SubClass <= 4)
            {
                // Esto es ropa, y la selecciono segun el nivel maximo de material
                query = Trinity::StringFormat("SELECT Quality,displayid,entry FROM item_template where class = %u and subclass <= %u and subclass >= 1 and entry in (select entry from items_obtenidos where cuenta = %u) group by Quality,displayid order by Quality asc;", itemTemplate->Class, itemTemplate->SubClass, player->GetSession()->GetAccountId());
            }
            else
                query = Trinity::StringFormat("SELECT Quality,displayid,entry FROM item_template where class = %u and subclass = %u and entry in (select entry from items_obtenidos where cuenta = %u) group by Quality,displayid order by Quality asc;", itemTemplate->Class, itemTemplate->SubClass, player->GetSession()->GetAccountId());

            resultado = WorldDatabase.PQuery(query);
            if (resultado)
            {
                do
                {
                    Field* fields = resultado->Fetch();
                    ItemTemplate const* curtemp = sObjectMgr->GetItemTemplate(fields[2].GetInt32());

                    if (nIteraciones == 0 && Calidades[nIteraciones].Quality != fields[0].GetInt8())
                    {
                        Calidades[nIteraciones].Quality = fields[0].GetInt8();
                        if (!TransmogDisplayVendorMgr::CanTransmogrifyItemWithItem(player, itemTemplate, curtemp))
                        {
                            Calidades[nIteraciones].Size = 0;
                        }
                        else
                        {
                            Calidades[nIteraciones].Size = 1;
                            Calidades[nIteraciones].DisplayID = fields[1].GetInt32();
                        }
                        nIteraciones++;
                    }
                    else if (nIteraciones > 0 && Calidades[nIteraciones - 1].Quality != fields[0].GetInt8())
                    {
                        Calidades[nIteraciones].Quality = fields[0].GetInt8();
                        if (!TransmogDisplayVendorMgr::CanTransmogrifyItemWithItem(player, itemTemplate, curtemp))
                        {
                            Calidades[nIteraciones].Size = 0;
                        }
                        else
                        {
                            Calidades[nIteraciones].Size = 1;
                            Calidades[nIteraciones].DisplayID = fields[1].GetInt32();
                        }
                        nIteraciones++;
                    }
                    else
                    {
                        if (TransmogDisplayVendorMgr::CanTransmogrifyItemWithItem(player, itemTemplate, curtemp))
                        {
                            Calidades[nIteraciones - 1].Size++;
                        }
                    }
                } while (resultado->NextRow());

                uint32 nMenus = 1;
                for (int nCalidad = 0; nCalidad < nCantidad; nCalidad++)
                {
                    if (Calidades[nCalidad].Size == 0) continue;
                    else if (Calidades[nCalidad].Size == 1)
                    {
                        if (Calidades[nCalidad].DisplayID == itemTemplate->DisplayInfoID)	continue;
                    }
                    for (uint32 count = 0; count*MAX_VENDOR_ITEMS < Calidades[nCalidad].Size; ++count)
                    {
                        if ((nMenus >= (GOSSIP_MAX_MENU_ITEMS-2)))
                            continue;
                        std::ostringstream ss;
                        ss << TransmogDisplayVendorMgr::getQualityName(Calidades[nCalidad].Quality);
                        if (count)
                            ss << " [" << count << "]";                        
                        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, ss.str().c_str(), Calidades[nCalidad].Quality, count);
                        nMenus++;
                    }
                }
            }

        }


        if (player->PlayerTalkClass->GetGossipMenu().GetMenuItemCount() == 0) // Si no pongo el itemMenu de Actualizar ........... (item), la cuenta de items del menu solo falla si es 0, no <= 1 
        {
            if (const char* slotname = TransmogDisplayVendorMgr::getSlotName(action, player->GetSession()))
                session->SendNotification("No hay transfiguraciones disponibles para %s", slotname);
            TransmorgVendor_Gossip::GossipHello(player, source);
            return true;
        }

        SelectionStore::Selection temp = { item->GetEntry(), static_cast<uint8>(action), 0, 0 }; // entry, slot, offset, quality
        TransmogDisplayVendorMgr::selectionStore.SetSelection(player->GetGUID().GetCounter(), temp);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Atras...", SENDER_BACK, 0);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());
        break;
    }
    case SENDER_BACK: // Back
    {
        TransmorgVendor_Gossip::GossipHello(player, source);
        break;
    }
    case SENDER_REMOVE_ALL: // Remove TransmogDisplayVendorMgrs
    {
        bool removed = false;
        for (uint8 Slot = EQUIPMENT_SLOT_START; Slot < EQUIPMENT_SLOT_END; Slot++)
        {
            if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, Slot))
            {
                if (!TransmogDisplayVendorMgr::GetFakeEntry(newItem))
                    continue;
                TransmogDisplayVendorMgr::DeleteFakeEntry(player, newItem);
                removed = true;
            }
        }
        if (removed)
        {
            session->SendAreaTriggerMessage("Transfiguracion retirada de los objetos equipados");
            player->PlayDirectSound(3337);
        }
        else
        {
            session->SendNotification("No tienes equipados objetos transfigurados");
        }
        OnGossipSelect(player, source, SENDER_REMOVE_MENU, 0);
        break;
    }
    case SENDER_REMOVE_ONE: // Remove TransmogDisplayVendorMgr from single item
    {
        const char* slotname = TransmogDisplayVendorMgr::getSlotName(action, player->GetSession());
        if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, action))
        {
            if (TransmogDisplayVendorMgr::GetFakeEntry(newItem))
            {
                TransmogDisplayVendorMgr::DeleteFakeEntry(player, newItem);
                if (slotname)
                    session->SendAreaTriggerMessage("Removida transfiguracion %s", slotname);
                player->PlayDirectSound(3337);
            }
            else if (slotname)
            {
                session->SendNotification("No hay transfiguracion en el slot %s", slotname);
            }
        }
        else if (slotname)
        {
            session->SendNotification("No hay objeto equipado en el slot %s", slotname);
        }
        OnGossipSelect(player, source, SENDER_REMOVE_MENU, 0);
        break;
    }
    case SENDER_REMOVE_MENU:
    {
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
        {
            const char* slotname = TransmogDisplayVendorMgr::getSlotName(slot, player->GetSession());
            if (!slotname)
                continue;
            std::ostringstream ss;
            ss << "Quitar transfiguracion de " << slotname << "?";
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, (std::string)"Quitar transfiguracion de " + slotname, SENDER_REMOVE_ONE, slot, ss.str().c_str(), 0, false);
        }
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Quitar todas las transfiguraciones", SENDER_REMOVE_ALL, 0, "Estas seguro?", 0, false);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Atras...", SENDER_BACK, 0);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());
        break;
    }
    case SENDER_END:
    {
        CloseGossipMenuFor(player);
        break;
    }
    case SENDER_EXPANSIONES:
    {
        QueryResult result = WorldDatabase.PQuery("select distinct c.idcategory, c.NombreCategoria from custom_npc_tmorg as t inner join custom_npc_tmorg_set as s on s.idset = t.idset and (s.classMask = 0 or (s.classMask & Pow(2,%u) = Pow(2,%u))) inner join custom_npc_tmorg_category as c on c.idcategory = t.idcategoria inner join item_template as i on i.entry = t.iditem and i.class = 4 and i.subclass >= 1 and i.subclass <= %u inner join items_obtenidos as io on io.entry = t.iditem and io.cuenta = %u", player->getClass() - 1, player->getClass() - 1, GetMaxSubclass(player), player->GetSession()->GetAccountId());
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, fields[1].GetString(), SENDER_FILTRO, fields[0].GetInt32());

            } while (result->NextRow());
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Atras...", SENDER_BACK, 0);

            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());

        }
        else
        {
            player->GetSession()->SendNotification("No dispones de ningun conjunto utilizable en el personaje");
            TransmorgVendor_Gossip::GossipHello(player, source);
        }
        break;
    }
    case SENDER_FILTRO:
    {
        player->SetTmorgCategory(action);
        uint32 clase = 0, todos = 0;
        QueryResult result = WorldDatabase.PQuery("select count(*) from custom_npc_tmorg as t inner join custom_npc_tmorg_set as s on s.idset = t.idset and (s.classMask & Pow(2,%u) = Pow(2,%u)) inner join custom_npc_tmorg_category as c on c.idcategory = t.idcategoria  and c.idcategory = %u inner join item_template as i on i.entry = t.iditem and i.class = 4 and i.subclass >= 1 and i.subclass <= %u inner join items_obtenidos as io on io.entry = t.iditem and io.cuenta = %u limit 30 offset %u", player->getClass() - 1, player->getClass() - 1, player->GetTmorgCategory(), GetMaxSubclass(player), player->GetSession()->GetAccountId(), 29 * player->GetTmorgOffset());
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                clase = fields[0].GetInt32();
            } while (result->NextRow());
        }
        result = WorldDatabase.PQuery("select count(*) from custom_npc_tmorg as t inner join custom_npc_tmorg_set as s on s.idset = t.idset and s.classMask = 0 inner join custom_npc_tmorg_category as c on c.idcategory = t.idcategoria  and c.idcategory = %u inner join item_template as i on i.entry = t.iditem and i.class = 4 and i.subclass >= 1 and i.subclass <= %u inner join items_obtenidos as io on io.entry = t.iditem and io.cuenta = %u limit 30 offset %u", player->GetTmorgCategory(), GetMaxSubclass(player), player->GetSession()->GetAccountId(), 29 * player->GetTmorgOffset());
        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                todos = fields[0].GetInt32();
            } while (result->NextRow());
        }
        if (clase > 0)
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Solo mi clase", SENDER_CONJUNTOS, ACTION_ONLY_MY_CLASS);
        if (todos > 0)
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Resto de clases", SENDER_CONJUNTOS, ACTION_REST_CLASS);

        AddGossipItemFor(player, GOSSIP_ICON_TALK, "Atras...", SENDER_EXPANSIONES, 0);
        player->SetTmorgOffset(0);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());

        break;
    }
    case SENDER_CONJUNTOS:
    {
        switch (action)
        {
        case ACTION_ONLY_MY_CLASS:
            player->SetTmorgOnlyMyClass(true);
            break;
        case ACTION_REST_CLASS:
            player->SetTmorgOnlyMyClass(false);
            break;
        case ACTION_ADD_PAGE:
            player->SetTmorgOffset(player->GetTmorgOffset() + 1);
            break;
        case ACTION_REMOVE_PAGE:
            player->SetTmorgOffset(player->GetTmorgOffset() - 1);
            break;
        default:
            player->GetSession()->SendNotification("Algo ha ido mal...");
            ChatHandler handler = ChatHandler(player->GetSession());
            handler.PSendSysMessage("Algo ha ido mal... reintentalo en unos minutos, si el problema persiste avisa a un GM indicandole el codigo de error->T001");
            return false;
            break;
        }


        QueryResult result;
        if (player->GetTmorgOnlyMyClass())  //'Solo mi clase'
            result = WorldDatabase.PQuery("select distinct s.idset,s.NombreSet from custom_npc_tmorg as t inner join custom_npc_tmorg_set as s on s.idset = t.idset and (s.classMask & Pow(2,%u) = Pow(2,%u)) inner join custom_npc_tmorg_category as c on c.idcategory = t.idcategoria  and c.idcategory = %u inner join item_template as i on i.entry = t.iditem and i.class = 4 and i.subclass >= 1 and i.subclass <= %u inner join items_obtenidos as io on io.entry = t.iditem and io.cuenta = %u limit 30 offset %u", player->getClass() - 1, player->getClass() - 1, player->GetTmorgCategory(), GetMaxSubclass(player), player->GetSession()->GetAccountId(), 29 * player->GetTmorgOffset());
        else  //'Solo objetos sin requisito de clase'
            result = WorldDatabase.PQuery("select distinct s.idset,s.NombreSet from custom_npc_tmorg as t inner join custom_npc_tmorg_set as s on s.idset = t.idset and s.classMask = 0 inner join custom_npc_tmorg_category as c on c.idcategory = t.idcategoria  and c.idcategory = %u inner join item_template as i on i.entry = t.iditem and i.class = 4 and i.subclass >= 1 and i.subclass <= %u inner join items_obtenidos as io on io.entry = t.iditem and io.cuenta = %u limit 30 offset %u", player->GetTmorgCategory(), GetMaxSubclass(player), player->GetSession()->GetAccountId(), 29 * player->GetTmorgOffset());

        if (result)
        {
            uint32 nItems = 0;
            bool bNext = false;
            do
            {
                if (nItems >= 29)
                {
                    bNext = true;
                    break;
                }
                Field* fields = result->Fetch();
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, fields[1].GetString(), SENDER_SET, fields[0].GetInt32());
                nItems++;

            } while (result->NextRow());
            if (bNext)
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Siguiente...->", SENDER_CONJUNTOS, ACTION_ADD_PAGE);
            if (player->GetTmorgOffset() > 0)
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "<-Previo...", SENDER_CONJUNTOS, ACTION_REMOVE_PAGE);

            AddGossipItemFor(player, GOSSIP_ICON_TALK, "Atras...", SENDER_FILTRO, player->GetTmorgCategory());

            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());

        }
        else
        {
            player->GetSession()->SendNotification("No dispones de ningun conjunto utilizable en el personaje");
            TransmorgVendor_Gossip::GossipHello(player, source);
        }

        break;
    }
    case SENDER_SET: // Show items you can use
    {
        SelectionStore::Selection selection;
        selection.offset = action;
        selection.quality = sender;
        selection.slot = 100;
        uint32 slot = 0; // slot
        TransmogDisplayVendorMgr::selectionStore.SetSelection(player->GetGUID().GetCounter(), selection);

        CloseGossipMenuFor(player);

        TC_LOG_DEBUG("network", "WORLD: Sent SMSG_LIST_INVENTORY");

        Creature* vendor = player->GetNPCIfCanInteractWith(source->GetGUID(), UNIT_NPC_FLAG_VENDOR);
        if (!vendor)
        {
            TempSummon* vendor_temp = source->SummonCreature(960001, source->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, nDespawnTime);
            if (!vendor_temp)
            {
                TC_LOG_DEBUG("network", "WORLD: SendListInventory - Unit (GUID: %u) not found or you can not interact with him.", source->GetGUID().GetCounter());
                player->SendSellError(SELL_ERR_CANT_FIND_VENDOR, NULL, ObjectGuid::Empty, 0);
                return true;
            }
            vendor_temp->SetVisible(true);
            //vendor_temp->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, vendor_temp->GetFollowAngle());
            vendor = vendor_temp;
        }
        else
        {
            if (vendor->HasUnitState(UNIT_STATE_MOVING))
                vendor->StopMoving();
        }

        if (player->HasUnitState(UNIT_STATE_DIED))
            player->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);




        uint32 item_amount = 0;
        uint8 count = 0;
        uint32 itemCount = 0;

        std::string query = "";
        query = Trinity::StringFormat("select count(t.iditem) from custom_npc_tmorg as t inner join items_obtenidos as io on io.entry = t.iditem and io.cuenta = %u where t.idset = %u", player->GetSession()->GetAccountId(), action);

        QueryResult resultado = WorldDatabase.PQuery(query);
        if (resultado)
        {
            Field* fields = resultado->Fetch();
            itemCount = fields[0].GetInt64();
        }


        WorldPacket data(SMSG_LIST_INVENTORY, 8 + 1 + itemCount * 8 * 4);
        data << uint64(vendor->GetGUID());

        size_t countPos = data.wpos();
        data << uint8(count);
       

        query = Trinity::StringFormat("select t.iditem from custom_npc_tmorg as t inner join items_obtenidos as io on io.entry = t.iditem and io.cuenta = %u where t.idset = %u", player->GetSession()->GetAccountId(), action);
        resultado = WorldDatabase.PQuery(query);
        if (resultado)
        {
            uint32 cost = sWorld->getIntConfig(CONFIG_TMORG_COST) * player->getLevel() / 80;
            if (cost < 195) cost = 195; //Minimo, 1 plata 95 cobre

            do
            {
                Field* fields = resultado->Fetch();
                ItemTemplate const* curtemp = sObjectMgr->GetItemTemplate(fields[0].GetInt32());
                if (curtemp)
                {
                    if (!TransmogDisplayVendorMgr::CanTransmogrifyItemConjuntos(player, curtemp))
                        continue;
                    

                    data << uint32(count + 1);
                    data << uint32(curtemp->ItemId);
                    data << uint32(curtemp->DisplayInfoID);
                    data << int32(0xFFFFFFFF);
                    data << uint32(cost); // coste
                    data << uint32(curtemp->MaxDurability);
                    data << uint32(curtemp->BuyCount);
                    data << uint32(0);
                    ++item_amount;
                    count++;
                }

            } while (resultado->NextRow());
            //Revisar donde se llama a la transfiguracion, ahora mismo no la ejecuta

        }

        if (!item_amount)
        {
            session->SendAreaTriggerMessage("No hay transfiguraciones disponibles para el objeto equipado");
            OnGossipSelect(player, source, SENDER_SELECT_VENDOR, slot);
            return true;
        }
        else
        {
            data.put<uint8>(countPos, item_amount);
            session->SendPacket(&data);
        }
        break;
    }
    default: // Show items you can use
    {
        if (sender >= MAX_ITEM_QUALITY) // sender = quality, action = iterator
            return false; // cheat

        SelectionStore::Selection selection;
        if (!TransmogDisplayVendorMgr::selectionStore.GetSelection(player->GetGUID().GetCounter(), selection))
            return false; // cheat
        if (selection.offset != 0 || selection.quality != 0)
            return false; // cheat (something is off)

        selection.offset = action;
        selection.quality = sender;
        uint32 slot = selection.slot; // slot
        TransmogDisplayVendorMgr::selectionStore.SetSelection(player->GetGUID().GetCounter(), selection);

        if (const ItemTemplate* itemTemplate = sObjectMgr->GetItemTemplate(selection.item))
        {
            if (!TransmogDisplayVendorMgr::SuitableForTransmogrification(player, itemTemplate))
            {
                player->GetSession()->SendNotification("El objeto equipado no puede ser transfigurado");
                OnGossipSelect(player, source, SENDER_SELECT_VENDOR, slot);
                return true;
            }
            selection.offset = action;
            selection.quality = sender;
            uint32 slot = selection.slot; // slot
            TransmogDisplayVendorMgr::selectionStore.SetSelection(player->GetGUID().GetCounter(), selection);

            CloseGossipMenuFor(player);

            TC_LOG_DEBUG("network", "WORLD: Sent SMSG_LIST_INVENTORY");

            Creature* vendor = player->GetNPCIfCanInteractWith(source->GetGUID(), UNIT_NPC_FLAG_VENDOR);
            if (!vendor)
            {
                TempSummon* vendor_temp = source->SummonCreature(990000, source->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, nDespawnTime);
                if (!vendor_temp)
                {
                    TC_LOG_DEBUG("network", "WORLD: SendListInventory - Unit (GUID: %u) not found or you can not interact with him.", source->GetGUID().GetCounter());
                    player->SendSellError(SELL_ERR_CANT_FIND_VENDOR, NULL, ObjectGuid::Empty, 0);
                    return true;
                }
                vendor_temp->SetVisible(true);
                //vendor_temp->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, vendor_temp->GetFollowAngle());
                vendor = vendor_temp;
            }
            else
            {
                if (vendor->HasUnitState(UNIT_STATE_MOVING))
                    vendor->StopMoving();
            }

            if (player->HasUnitState(UNIT_STATE_DIED))
                player->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);




            uint32 item_amount = 0;
            uint8 count = 0;
            uint32 itemCount = 0;
            ItemTemplate const* itemProto = sObjectMgr->GetItemTemplate(selection.item);

            std::string query = "";
            if (itemTemplate->Class == 4 && itemTemplate->SubClass >= 1 && itemTemplate->SubClass <= 4)
            {
                // Esto es ropa, y la selecciono segun el nivel maximo de material
                query = Trinity::StringFormat("SELECT count(entry) FROM item_template where class = %u and subclass <= %u and subclass >= 1 and quality = %u and entry in (select entry from items_obtenidos where cuenta = %u) group by displayid limit 150 offset %u;", itemProto->Class, itemProto->SubClass, selection.quality, player->GetSession()->GetAccountId(), selection.offset * 150);
            }
            else
                query = Trinity::StringFormat("SELECT count(entry) FROM item_template where class = %u and subclass = %u and quality = %u and entry in (select entry from items_obtenidos where cuenta = %u) group by displayid limit 150 offset %u;", itemProto->Class, itemProto->SubClass, selection.quality, player->GetSession()->GetAccountId(), selection.offset * 150);

            QueryResult resultado = WorldDatabase.PQuery(query);
            if (resultado)
            {
                Field* fields = resultado->Fetch();
                itemCount = fields[0].GetInt64();
            }


            WorldPacket data(SMSG_LIST_INVENTORY, 8 + 1 + itemCount * 8 * 4);
            data << uint64(vendor->GetGUID());

            size_t countPos = data.wpos();
            data << uint8(count);

            if (itemTemplate->Class == 4 && itemTemplate->SubClass >= 1 && itemTemplate->SubClass <= 4)
            {
                // Esto es ropa, y la selecciono segun el nivel maximo de material
                query = Trinity::StringFormat("SELECT entry FROM item_template where class = %u and subclass <= %u and subclass >= 1 and quality = %u and entry in (select entry from items_obtenidos where cuenta = %u) group by displayid limit 150 offset %u;", itemProto->Class, itemProto->SubClass, selection.quality, player->GetSession()->GetAccountId(), selection.offset * 150);
            }
            else
                query = Trinity::StringFormat("SELECT entry FROM item_template where class = %u and subclass = %u and quality = %u and entry in (select entry from items_obtenidos where cuenta = %u) group by displayid limit 150 offset %u;", itemProto->Class, itemProto->SubClass, selection.quality, player->GetSession()->GetAccountId(), selection.offset * 150);
            resultado = WorldDatabase.PQuery(query);
            if (resultado)
            {

                uint32 cost = sWorld->getIntConfig(CONFIG_TMORG_COST) * player->getLevel() / 80;
                if (cost < 195) cost = 195; //Minimo, 1 plata 95 cobre

                do
                {
                    Field* fields = resultado->Fetch();
                    ItemTemplate const* curtemp = sObjectMgr->GetItemTemplate(fields[0].GetInt32());
                    if (curtemp)
                    {
                        if (!TransmogDisplayVendorMgr::CanTransmogrifyItemWithItem(player, itemProto, curtemp))
                            continue;
                        data << uint32(count + 1);
                        data << uint32(curtemp->ItemId);
                        data << uint32(curtemp->DisplayInfoID);
                        data << int32(0xFFFFFFFF);
                        data << uint32(cost); // coste
                        data << uint32(curtemp->MaxDurability);
                        data << uint32(curtemp->BuyCount);
                        data << uint32(0);
                        ++item_amount;
                        count++;
                    }

                } while (resultado->NextRow());
                //Revisar donde se llama a la transfiguracion, ahora mismo no la ejecuta

            }

            if (!item_amount)
            {
                session->SendAreaTriggerMessage("No hay transfiguraciones disponibles para el objeto equipado");
                OnGossipSelect(player, source, SENDER_SELECT_VENDOR, slot);
                return true;
            }
            else
            {
                data.put<uint8>(countPos, item_amount);
                session->SendPacket(&data);
            }
        }
        else
        {
            session->SendNotification("Equipado objeto invalido");
            OnGossipSelect(player, source, SENDER_SELECT_VENDOR, slot);
            return true;
        }
        break;
    }
    }
    return true;
}

bool TransmorgVendor_Gossip::GossipHello(Player* player, WorldObject* source)
{
    CloseGossipMenuFor(player);
    ClearGossipMenuFor(player);

    TransmogDisplayVendorMgr::selectionStore.RemoveSelection(player->GetGUID().GetCounter());
    WorldSession* session = player->GetSession();
    //AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "Para hacer negocios contigo, necesito 30 monedas de oro por cada pieza", SENDER_BACK, 0);

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; slot++)
    {
        // if (player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        if (const char* slotName = TransmogDisplayVendorMgr::getSlotName(slot, session))
            AddGossipItemFor(player, GOSSIP_ICON_TRAINER, slotName, SENDER_SELECT_VENDOR, slot);
    }
    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Conjuntos", SENDER_EXPANSIONES, 0);
    AddGossipItemFor(player, GOSSIP_ICON_TRAINER, "Deshacer Transfiguraciones", SENDER_REMOVE_MENU, 0);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());
    return true;
}

bool TransmorgVendor_Gossip::GossipSelect(Player *player, WorldObject* source, uint32 menuId, uint32 gossipListId)
{
    uint32 sender = player->PlayerTalkClass->GetGossipOptionSender(gossipListId);
    uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
    OnGossipSelect(player, source, sender, action);

    return true;
}

