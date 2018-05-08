#include "Promo.h"

enum GuildPromoGossipSender
{
    GUILD_PROMO_GOSSIP_ACTION_ACTIVAR = 0,
    GUILD_PROMO_GOSSIP_ACTION_RECLAMAR,
    GUILD_PROMO_GOSSIP_ACTION_SALIR,
    GUILD_PROMO_GOSSIP_ACTION_MAX
};

enum GuildPromoGossipAction
{
    GUILD_PROMO_GOSSIP_SENDER_MAIN = GUILD_PROMO_GOSSIP_ACTION_MAX,
    GUILD_PROMO_GOSSIP_SENDER_SET,
    GUILD_PROMO_GOSSIP_SENDER_MAX
};

enum MenuSets
{
    CUERO_DUDU_TANK = 813,
    PLACAS_HEALER = 814,
    CUERO_DUDU_HEAL = 815,
    PLACAS_DPS = 816,
    SHAMAN_HEAL = 817,
    MALLA_CAZADORES_SHAMAN = 818,
    TELA_DPS = 819,
};
uint32 nItemBuffos = 200013;
uint32 nMinOnline = 10;
uint32 nLevelMin = 5;
uint32 nOro = 1;
uint32 nGuildDaysDuration = 7;
uint32 nPlayerDays = 10;
uint32 nItemPromoIndividual = 0;
uint32 nItemExtenderPromo = 0;

uint32 PromoGuildGossip1 = 500000;
uint32 PromoGuildGossip2 = 500001;
uint32 PromoGuildGossip3 = 500002;
uint32 BoosterGossip = 500003;


void CreateItemInstance(Player* player, uint32 ItemId, bool AddGem = true)
{
    ItemPosCountVec dest;
    Item* item;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, ItemId, 1);
    if (msg == EQUIP_ERR_OK) //Determina si va a ser añadido o enviado
    {
        item = player->StoreNewItem(dest, ItemId, true);
    }
    else
    {
        item = Item::CreateItem(ItemId, 1, nullptr);
    }
    //Si hay que engemarlo, le añadimos las gemas
    if (AddGem)
    {
        item->SetEnchantment(SOCK_ENCHANTMENT_SLOT, 3532, player, 0, 0);
        item->SetEnchantment(SOCK_ENCHANTMENT_SLOT_2, 3532, player, 0, 0);
        item->SetEnchantment(SOCK_ENCHANTMENT_SLOT_3, 3532, player, 0, 0);
    }

    //Seteo el owner para enviarlo y que lo pueda recibir
    item->SetOwnerGUID(player->GetGUID());

    //Lo ligamos
    item->SetBinding(true);
    //

    if (msg == EQUIP_ERR_OK) //Determina si va a ser añadido o enviado
    {
        player->SendNewItem(item, 1, true, false);
    }
    else
    {
        player->EnviarPorCorreo(item);
    }
}

bool GiveItemSet(Player* player, uint32 setID)
{
    if (setID == 0)
    {
        return false;
    }
    bool found = false;
    ItemTemplateContainer const* its = sObjectMgr->GetItemTemplateStore();
    for (ItemTemplateContainer::const_iterator itr = its->begin(); itr != its->end(); ++itr)
    {
        if (itr->second.ItemSet == setID)
        {

            found = true;
            CreateItemInstance(player, itr->second.ItemId);
        }
    }
    if (found)
    {
        //Creo las bolsas
        CreateItemInstance(player, 4500, false);
        CreateItemInstance(player, 4500, false);
        CreateItemInstance(player, 4500, false);
        CreateItemInstance(player, 4500, false);
    }

    //Para el caso del dudu...
    if (player->getClass() == CLASS_DRUID)
    {
        if (setID == CUERO_DUDU_TANK)
            player->AddItem(42574, 1);
        else if (setID == CUERO_DUDU_HEAL)
            player->AddItem(42576, 1);
    }

    return found;
}

void GiveItemOffset(Player* player)
{
    //Damos los items para todosç   
    player->AddItem(35948, 40);
    player->AddItem(200013, 20);

    //Damos los items dependientes de clase
    switch (player->getClass())
    {
    case CLASS_WARLOCK:
        player->AddItem(43252, 1);
        player->AddItem(43498, 1);
        player->AddItem(49227, 1);
        player->AddItem(36472, 1);
        player->AddItem(36554, 1);
        player->AddItem(36668, 1);
        player->AddItem(36972, 1);
        player->AddItem(6265, 9);
        player->AddItem(33445, 40);
        player->AddItem(37758, 1);
        player->AddItem(37756, 1);
        break;
    case CLASS_MAGE:
        player->AddItem(43252, 1);
        player->AddItem(43498, 1);
        player->AddItem(49227, 1);
        player->AddItem(36472, 1);
        player->AddItem(36554, 1);
        player->AddItem(36668, 1);
        player->AddItem(36972, 1);
        player->AddItem(17020, 40);
        player->AddItem(17032, 40);
        player->AddItem(33445, 40);
        player->AddItem(37758, 1);
        player->AddItem(37756, 1);
        player->AddItem(17056, 40);

        break;
    case CLASS_PRIEST:
        player->AddItem(43252, 1);
        player->AddItem(43498, 1);
        player->AddItem(49227, 1);
        player->AddItem(36472, 1);
        player->AddItem(36554, 1);
        player->AddItem(36668, 1);
        player->AddItem(36972, 1);
        player->AddItem(44615, 40);
        player->AddItem(33445, 40);
        player->AddItem(37758, 1);
        player->AddItem(37756, 1);
        player->AddItem(17056, 40);
        break;
    case CLASS_DEATH_KNIGHT:
        player->AddItem(43250, 1);
        player->AddItem(43482, 1);
        player->AddItem(36526, 2);
        player->AddItem(42618, 1);
        player->AddItem(36540, 1);
        player->AddItem(37166, 1);
        player->AddItem(37201, 40);
        player->AddItem(37794, 1);
        player->AddItem(41238, 1);
        break;
    case CLASS_WARRIOR:
        player->AddItem(43250, 1);
        player->AddItem(43482, 1);
        player->AddItem(36526, 2);
        player->AddItem(36458, 1);
        player->AddItem(36540, 1);
        player->AddItem(36981, 1);
        player->AddItem(37166, 1);
        player->AddItem(37794, 1);
        player->AddItem(41238, 1);

        break;
    case CLASS_PALADIN:
        player->AddItem(43250, 1);
        player->AddItem(43482, 1);
        player->AddItem(36527, 1);
        player->AddItem(42611, 1);
        player->AddItem(36542, 1);
        player->AddItem(36458, 1);
        player->AddItem(37166, 1);
        player->AddItem(21177, 40);
        player->AddItem(17033, 40);
        player->AddItem(33445, 40);
        player->AddItem(37794, 1);
        player->AddItem(41238, 1);

        break;
    case CLASS_HUNTER:
        player->AddItem(3031, 1000);
        player->AddItem(43482, 1);
        player->AddItem(43251, 1);
        player->AddItem(36626, 1);
        player->AddItem(36611, 1);
        player->AddItem(37166, 1);
        player->AddItem(33445, 40);
        player->AddItem(37823, 1);
        player->AddItem(37797, 1);

        break;
    case CLASS_ROGUE:
        player->AddItem(43251, 1);
        player->AddItem(43482, 1);
        player->AddItem(44505, 2);
        player->AddItem(36981, 1);
        player->AddItem(37166, 1);
        player->AddItem(43231, 1);
        player->AddItem(43233, 1);
        player->AddItem(43235, 1);
        player->AddItem(5237, 1);
        player->AddItem(3775, 1);
        player->AddItem(37823, 1);
        player->AddItem(38441, 1);
        break;
    case CLASS_SHAMAN:
        player->AddItem(43253, 1);
        player->AddItem(43252, 1);
        player->AddItem(36570, 2);
        player->AddItem(36500, 1);
        player->AddItem(36458, 1);
        player->AddItem(36972, 1);
        player->AddItem(17057, 40);
        player->AddItem(33445, 40);
        player->AddItem(37758, 1);
        player->AddItem(37756, 1);
        break;
    case CLASS_DRUID:
        player->AddItem(43252, 1);
        player->AddItem(43498, 1);
        player->AddItem(49227, 1);
        player->AddItem(36472, 1);
        player->AddItem(36554, 1);
        player->AddItem(36972, 1);
        player->AddItem(44605, 40);
        player->AddItem(44614, 40);
        player->AddItem(33445, 40);
        player->AddItem(37758, 1);
        player->AddItem(37756, 1);
        break;
    default:
        break;
    }

    //Damos los items dependientes de faccion
    if (player->GetTeam() == ALLIANCE)
        player->AddItem(45131, 1);
    else
        player->AddItem(45219, 1);

}

void AddAuras(Player* player)
{
    player->AddAura(48074, player);
    player->AddAura(48170, player);
    player->AddAura(48162, player);
    player->AddAura(43002, player);
    player->AddAura(48470, player);
    player->AddAura(25898, player);
    player->AddAura(53760, player);
    player->AddAura(57399, player);
    player->AddAura(53307, player);
    player->AddAura(48441, player);
    player->AddAura(15366, player);
    player->AddAura(66068, player);
    player->AddAura(35076, player);

}

void SetReputation(Player* player)
{
    switch (player->getRace())
    {

    case RACE_HUMAN:
        player->SetReputation(72, 62999);
        break;
    case RACE_ORC:
        player->SetReputation(76, 62999);
        break;
    case RACE_DWARF:
        player->SetReputation(47, 62999);
        break;
    case RACE_NIGHTELF:
        player->SetReputation(69, 62999);
        break;
    case RACE_UNDEAD_PLAYER:
        player->SetReputation(68, 62999);
        break;
    case RACE_TAUREN:
        player->SetReputation(81, 62999);
        break;
    case RACE_GNOME:
        player->SetReputation(54, 62999);
        break;
    case RACE_TROLL:
        player->SetReputation(530, 62999);
        break;
    case RACE_BLOODELF:
        player->SetReputation(911, 62999);
        break;
    case RACE_DRAENEI:
        player->SetReputation(930, 62999);
        break;
    case RACE_HUARGUEN_WOLF:
    case RACE_HUARGUEN_HUMAN:
        player->SetReputation(72, 62999);
        player->LearnSpell(458, false);
        break;
    case RACE_PANDAREN_ALI:
        player->SetReputation(72, 62999);
        player->LearnSpell(458, false);
        break;

    case RACE_PANDAREN_HOR:
        player->SetReputation(76, 62999);
        player->LearnSpell(6654, false);
        break;
    case RACE_GOBLIN:
        player->SetReputation(76, 62999);
        player->LearnSpell(6654, false);
        break;
    default:
        break;
    }
}

void LearnMount(Player* player)
{
    //Enseñamos equitaciones hasta 150% y vuelo en clima frio
    player->LearnSpell(33388, false);
    player->LearnSpell(33391, false);
    player->LearnSpell(34090, false);
    player->LearnSpell(54197, false);

    //Damos los items dependientes de faccion
    if (player->GetTeam() == ALLIANCE)
        player->LearnSpell(32235, false);
    else
        player->LearnSpell(32243, false);
}

void LearnDualSpec(Player* player)
{
    player->CastSpell(player, 63680, true, NULL, NULL, player->GetGUID());
    player->CastSpell(player, 63624, true, NULL, NULL, player->GetGUID());
}

void SendRoleMenu(Player* player, ObjectGuid source)
{
    uint32 GossipText = PromoGuildGossip2;
    if (source.IsItem())
        GossipText = BoosterGossip;
    switch (player->getClass())
    {
    case CLASS_DEATH_KNIGHT:
        AddGossipItemFor(player, 0, "DPS-TANQUE", GUILD_PROMO_GOSSIP_SENDER_SET, PLACAS_DPS);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    case CLASS_DRUID:
        AddGossipItemFor(player, 0, "DPS", GUILD_PROMO_GOSSIP_SENDER_SET, CUERO_DUDU_HEAL);
        AddGossipItemFor(player, 0, "FERAL", GUILD_PROMO_GOSSIP_SENDER_SET, CUERO_DUDU_TANK);
        AddGossipItemFor(player, 0, "SANADOR", GUILD_PROMO_GOSSIP_SENDER_SET, CUERO_DUDU_HEAL);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    case CLASS_HUNTER:
        AddGossipItemFor(player, 0, "DPS", GUILD_PROMO_GOSSIP_SENDER_SET, MALLA_CAZADORES_SHAMAN);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    case CLASS_MAGE:
        AddGossipItemFor(player, 0, "DPS", GUILD_PROMO_GOSSIP_SENDER_SET, TELA_DPS);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    case CLASS_PALADIN:
        AddGossipItemFor(player, 0, "DPS-TANQUE", GUILD_PROMO_GOSSIP_SENDER_SET, PLACAS_DPS);
        AddGossipItemFor(player, 0, "SANADOR", GUILD_PROMO_GOSSIP_SENDER_SET, PLACAS_HEALER);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    case CLASS_PRIEST:
        AddGossipItemFor(player, 0, "DPS-SANADOR", GUILD_PROMO_GOSSIP_SENDER_SET, TELA_DPS);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    case CLASS_ROGUE:
        AddGossipItemFor(player, 0, "DPS", GUILD_PROMO_GOSSIP_SENDER_SET, CUERO_DUDU_TANK);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    case CLASS_WARLOCK:
        AddGossipItemFor(player, 0, "DPS", GUILD_PROMO_GOSSIP_SENDER_SET, TELA_DPS);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    case CLASS_WARRIOR:
        AddGossipItemFor(player, 0, "DPS-TANQUE", GUILD_PROMO_GOSSIP_SENDER_SET, PLACAS_DPS);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    case CLASS_SHAMAN:
        AddGossipItemFor(player, 0, "DPS ELEMENTAL", GUILD_PROMO_GOSSIP_SENDER_SET, MALLA_CAZADORES_SHAMAN);
        AddGossipItemFor(player, 0, "DPS MEJORA-SANADOR", GUILD_PROMO_GOSSIP_SENDER_SET, SHAMAN_HEAL);
        AddGossipItemFor(player, 3, "Atras", GUILD_PROMO_GOSSIP_SENDER_SET, GUILD_PROMO_GOSSIP_ACTION_SALIR);
        SendGossipMenuFor(player, GossipText, source);
        break;
    default:
        player->GetSession()->SendNotification("Clase no reconocida");
        CloseGossipMenuFor(player);
        break;
    }
}

bool Promocion::GossipSelect(Player *player, uint32 menuId, uint32 gossipListId, ObjectGuid source)
{
    uint32 sender = player->PlayerTalkClass->GetGossipOptionSender(gossipListId);
    uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
    ClearGossipMenuFor(player);
    CloseGossipMenuFor(player);
    switch (sender)
    {

    case GUILD_PROMO_GOSSIP_SENDER_MAIN:
        switch (action)
        {
        case GUILD_PROMO_GOSSIP_ACTION_ACTIVAR:
            player->GetGuild()->ActivatePromo(nGuildDaysDuration);
            if (source.IsItem())
            {
                player->DestroyItemCount(nItemExtenderPromo, 1, true, false);
            }
            CloseGossipMenuFor(player);
            break;
        case GUILD_PROMO_GOSSIP_ACTION_RECLAMAR:
            SendRoleMenu(player, source);
            break;
        case GUILD_PROMO_GOSSIP_ACTION_SALIR:
            CloseGossipMenuFor(player);
            break;
        default:
            break;
        }
        break;
    case GUILD_PROMO_GOSSIP_SENDER_SET:

        //Si pulsan atras, salimos
        if (action == GUILD_PROMO_GOSSIP_ACTION_SALIR)
        {
            //Si es criatura, es el NPC y queremos volver atras
            if (source.IsCreature())
                GossipHello(player, source);
            else  //Si no es criatura, no hay menu anterior, asi que cerramos
                CloseGossipMenuFor(player);
            return true;
        }

        //Da el set
        if (!GiveItemSet(player, action))
            CloseGossipMenuFor(player);

        LearnDualSpec(player);

        //Damos el offset
        GiveItemOffset(player);

        //Desactivamos el anuncio global de level
        player->SetLevelAnnouncerState(false);

        //Sube a nivel 80
        player->GiveLevel(80);

        //Da el oro
        player->SetMoney(player->GetMoney() + (nOro * 10000));

        //Enseñamos la montura y la equitacion
        LearnMount(player);

        //Sube Reputaciones
        SetReputation(player);

        //Da los buffos               
        AddAuras(player);

        //Activa la promo si es criatura
        if (source.IsCreature()) player->ActivatePromo(nPlayerDays);
        else if (source.IsItem())
        {
            player->DestroyItemCount(nItemPromoIndividual, 1, true, false);
        }
        //Activamos el anuncio global de level
        player->SetLevelAnnouncerState(true);

        //Teleporto al player a las camaras heladas
        AccionesWorldQueueScript accion;
        accion.nIdCharacter = player->GetGUID();
        accion.nActionType = TYPE_TELEPORT;
        accion.nMap = 571;
#pragma warning(push)
#pragma warning(disable: 4305)
        accion.lfX = 5636.55;
        accion.lfY = 2064.03;
        accion.lfZ = 798.059;
        accion.lfO = 4.62278;
#pragma warning(pop)
        sWorldQueueScript->Encolar(accion);

        CloseGossipMenuFor(player);
        break;
    default:
        break;
    }


    return true;
}

bool Promocion::GossipHello(Player *player, ObjectGuid source)
{
    ClearGossipMenuFor(player);
    CloseGossipMenuFor(player);


    if (source.IsCreature()) //Si es player, lanzamos PromoGuild
    {
        if (player->IsGameMaster())
        {
            AddGossipItemFor(player, 8, "Depurar promocion", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_RECLAMAR);
            if (player->GetGuild())
                AddGossipItemFor(player, 8, "Depurar activacion hermandad", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_ACTIVAR);
            AddGossipItemFor(player, 8, "Salir", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
            SendGossipMenuFor(player, PromoGuildGossip1, source);
            return true;
        }
        else if (!player->GetGuild())
        {
            ChatHandler chat(player->GetSession());
            std::string msg = "No tienes hermandad";
            chat.SendSysMessage(msg.c_str());
            AddGossipItemFor(player, 8, "Entiendo...", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
            AddGossipItemFor(player, 8, "Salir", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
            SendGossipMenuFor(player, PromoGuildGossip3, source);
            return true;
        }
       /* else if (player->GetGuild()->GetLeaderGUID() == player->GetGUID() && !player->GetGuild()->TakedPromo() &&
            player->GetGuild()->GetOnlineCount(nLevelMin) >= nMinOnline)
        {
            AddGossipItemFor(player, 8, "Activar promocion para mi hermandad", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_ACTIVAR, "Estas seguro de que quieres activarla?", 0, false);
            AddGossipItemFor(player, 8, "Salir", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
            SendGossipMenuFor(player, PromoGuildGossip1, source);
            return true;
        }*/
        else if (/*player->GetGuild()->CanTakePromo() &&*/ !player->HasPromoInAccount())
        {
            AddGossipItemFor(player, 8, "Estoy de acuerdo con los terminos y condiciones de la pagina web", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_RECLAMAR);
            AddGossipItemFor(player, 8, "Salir", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
            SendGossipMenuFor(player, PromoGuildGossip1, source);
            return true;
        }
        else
        {
            ChatHandler chat(player->GetSession());
            std::string msg = "";
            if (player->GetGuild()->GetLeaderGUID() == player->GetGUID())
            {
/*                if (player->GetGuild()->GetOnlineCount(nLevelMin) < nMinOnline)
                {
                    msg = "No hay ";
                    msg += std::to_string(nMinOnline);
                    msg += " personas online con nivel ";
                    msg += std::to_string(nLevelMin);
                    msg += " o superior.";
                }
                else if (player->GetGuild()->TakedPromo())
                {
                    msg = "La promocion ya ha sido activada";
                }
                else*/ if (player->HasPromoInAccount())
                {
                    msg = "Ya se ha solicitado promocion desde esta cuenta";
                }
                else
                {
                    msg = "No cumples los terminos y requisitos de la web, revisalos en https://www.sirionserver.com/promoguild";
                }
            }
            else
            {
                if (!player->GetGuild()->CanTakePromo())
                {
                    msg = "La promocion no esta activa, tu lider debe activarla";
                }
                else if (player->HasPromoInAccount())
                {
                    msg = "Ya se ha solicitado promocion desde esta cuenta";
                }
                else
                {
                    msg = "No cumples los terminos y requisitos de la web, revisalos en https://www.sirionserver.com/promoguild";
                }
            }
            chat.SendSysMessage(msg.c_str());
            AddGossipItemFor(player, 8, "Entiendo...", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
            AddGossipItemFor(player, 8, "Salir", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
            SendGossipMenuFor(player, PromoGuildGossip3, source);
            return true;
        }

    }
    else if (source.IsItem())
    {
        if (player->IsCurrentItemGossip(GUILD_PROMO_EXTEND))
        {
            if (player->GetGuild() && player->GetGuild()->GetLeaderGUID() == player->GetGUID() && player->GetGuild()->GetOnlineCount(nLevelMin) >= nMinOnline)
            {
                AddGossipItemFor(player, 8, "Extender promocion para mi hermandad", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_ACTIVAR, "Estas seguro de que quieres activarla?", 0, false);
                AddGossipItemFor(player, 8, "Salir", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
                SendGossipMenuFor(player, PromoGuildGossip1, source);
                return true;
            }
            else
            {
                ChatHandler chat(player->GetSession());
                std::string msg = "";

                if (!player->GetGuild())
                {
                    msg = "No tienes hermandad.";
                }
                else if (player->GetGuild()->GetOnlineCount(nLevelMin) < nMinOnline)
                {
                    msg = "No hay ";
                    msg += std::to_string(nMinOnline);
                    msg += " personas online con nivel ";
                    msg += std::to_string(nLevelMin);
                    msg += " o superior.";
                }
                chat.SendSysMessage(msg.c_str());
                AddGossipItemFor(player, 8, "Entiendo...", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
                AddGossipItemFor(player, 8, "Salir", GUILD_PROMO_GOSSIP_SENDER_MAIN, GUILD_PROMO_GOSSIP_ACTION_SALIR);
                SendGossipMenuFor(player, PromoGuildGossip3, source);
                return true;
            }


        }
        else if (player->IsCurrentItemGossip(PLAYER_BOOST))
        {
            SendRoleMenu(player, source);          
        }
    }
    return true;
}


bool Promocion::GossipHello(Player *player, ObjectGuid source, uint32 nIdItemEmpleado)
{
    if (player->IsCurrentItemGossip(PLAYER_BOOST))
        nItemPromoIndividual = nIdItemEmpleado;
    else if (player->IsCurrentItemGossip(GUILD_PROMO_EXTEND))
        nItemExtenderPromo = nIdItemEmpleado;
    return GossipHello(player, source);
}




