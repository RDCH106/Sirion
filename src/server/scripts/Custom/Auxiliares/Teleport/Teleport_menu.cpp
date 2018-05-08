/*
 * Copyright (C) 20??-2008 Wilibald09
 * Copyright (C) 2011-2015 ArkCORE <http://www.arkania.net/>
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */



#include "Teleport_menu.h"
#include "LinQ.hpp"

using namespace LinQ;


#define GOSSIP_SHOW_DESTINATION        1000
#define GOSSIP_TELEPORT         1001
#define GOSSIP_NEXT_PAGEC       1002
#define GOSSIP_PREV_PAGEC       1003
#define GOSSIP_NEXT_PAGED       1004
#define GOSSIP_PREV_PAGED       1005
#define GOSSIP_MAIN_MENU        1006

#define SPELL_ID_PASSIVE_RESURRECTION_SICKNESS  15007
#define SPELL_VISUAL_TELEPORT   35517

#define NB_ITEM_PAGE            100
#define MSG_CAT                 100000
#define MSG_DEST                100001

#define NEXT_PAGE               "-> [Siguiente]"
#define PREV_PAGE               "<- [Anterior]"
#define MAIN_MENU               "<= [Pagina Principal]"



namespace nsNpcTel
{
    Page PageC, PageD;
    Page Cat;

    // Conversion function int->string
    std::string ConvertStr(const int64 &val)
    {
        std::ostringstream ostr;
        ostr << val;
        return ostr.str();
    }

    // Conversion function intMoney->stringMoney
    std::string ConvertMoney(const uint32 &Money)
    {
        std::string Str(ConvertStr(Money));
        uint32 SizeStr = Str.length();

        if (SizeStr > 4)
            Str = Str.insert(Str.length() - 4, "g");
        if (SizeStr > 2)
            Str = Str.insert(Str.length() - 2, "s");
        Str += "c";

        return Str;
    }

    //Casteio de spells y auras
    void CastSpell_ApplyAuras(Player* player, WorldObject* source, std::string const& data, Accion accion)
    {
        uint32 spellid = 0;
        Tokenizer tokens(data, ' ');

        for (Tokenizer::const_iterator iter = tokens.begin(); iter != tokens.end(); ++iter)
        {
            uint32 spellid = atoi(*iter);
            if (sSpellMgr->GetSpellInfo(spellid))
            {
                switch (accion)
                {
                case ApplyAura:
                    player->AddAura(spellid, player);
                    break;
                case CastSpell:
                    player->CastSpell(player, spellid, true);
                    break;
                default:
                    break;
                }
            }
        }

    }

    // Teleport Player
    void Teleport(Player * const player, const uint16 &map,
        const float &X, const float &Y, const float &Z, const float &orient)
    {
        player->CastSpell(player, SPELL_VISUAL_TELEPORT, true);
        player->TeleportTo(map, X, Y, Z, orient);
    }

    // Display categories
    void AffichCat(Player * const player, WorldObject* const source)
    {
        uint8 loc = player->GetSession()->GetSessionDbcLocale();

        if (PageC[player] > 0)
            AddGossipItemFor(player, 7, PREV_PAGE, GOSSIP_PREV_PAGEC, 0);

        VCatDest_t i(PageC[player] * NB_ITEM_PAGE);
        for (; i < TabCatDest.size() && i < (NB_ITEM_PAGE * (PageC[player] + 1)); ++i)
        {
            if (TabCatDest[i].IsAllowedCategory(player))
                AddGossipItemFor(player, 3, TabCatDest[i].GetName(loc, player->IsGameMaster()).c_str(), GOSSIP_SHOW_DESTINATION, i); //book categorie	

        }

        if (i < TabCatDest.size())
            AddGossipItemFor(player, 7, NEXT_PAGE, GOSSIP_NEXT_PAGEC, 0);

        SendGossipMenuFor(player, MSG_CAT, source->GetGUID());
    }

    // Display destination categories
    void AffichDest(Player * const player, WorldObject* const source)
    {
        uint32 nMenus = 0;
        uint8 loc = player->GetSession()->GetSessionDbcLocale();

        if (PageD[player] > 0)
            AddGossipItemFor(player, 7, PREV_PAGE, GOSSIP_PREV_PAGED, 0);

        CatDest::VDest_t i(PageD[player] * NB_ITEM_PAGE);
        for (; i < TabCatDest[Cat[player]].size() && i < (NB_ITEM_PAGE * (PageD[player] + 1)); ++i)
        {
            if (!TabCatDest[Cat[player]].IsAllowedDestine(player, Cat[player], i)) continue;

            if (nMenus > 28)
                continue;
            nMenus++;


            uint32 costeOro = 0;
            std::string icon = TabCatDest[Cat[player]].GetDest(i).m_icon;
            std::string size_v = TabCatDest[Cat[player]].GetDest(i).m_size_v;
            std::string size_h = TabCatDest[Cat[player]].GetDest(i).m_size_h;
            std::string colour = TabCatDest[Cat[player]].GetDest(i).m_colour;
            std::string name = TabCatDest[Cat[player]].GetDest(i).m_name[loc];
            if (name.length() == 0)
                name = TabCatDest[Cat[player]].GetDest(i).m_name[0];

            std::string msgConfirmacion = "";
            if (TabCatDest[Cat[player]].GetDest(i).m_cost == 0)
            {
                if (TabCatDest[Cat[player]].GetDest(i).m_popup_text == "")	msgConfirmacion = "Seras teleportado a:\n" + name;
                else msgConfirmacion = TabCatDest[Cat[player]].GetDest(i).m_popup_text;
            }
            else
            {
                if (TabCatDest[Cat[player]].GetDest(i).m_coin == 0)
                {

                    //si tiene la racial de goblin, aplica descuento, sino coje el precio exacto
                    if (player->HasSpell(69044))
                    {
                        costeOro = (TabCatDest[Cat[player]].GetDest(i).m_cost * (100 - TabCatDest[Cat[player]].GetDest(i).m_descuento)) / 100;
                    }
                    else
                    {
                        costeOro = TabCatDest[Cat[player]].GetDest(i).m_cost;
                    }

                    if (TabCatDest[Cat[player]].GetDest(i).m_popup_text == "")	msgConfirmacion = "Seras teleportado a:\n" + name;
                    else msgConfirmacion = TabCatDest[Cat[player]].GetDest(i).m_popup_text;
                    msgConfirmacion += "\n\nCoste: ";
                }
                else
                {
                    const ItemTemplate* moneda = sObjectMgr->GetItemTemplate(TabCatDest[Cat[player]].GetDest(i).m_coin);
                    if (!moneda) continue;
                    if (TabCatDest[Cat[player]].GetDest(i).m_popup_text == "")	msgConfirmacion = "Seras teleportado a:\n" + name;
                    else msgConfirmacion = TabCatDest[Cat[player]].GetDest(i).m_popup_text;
                    msgConfirmacion += "\n\nCoste: \n" + std::to_string(TabCatDest[Cat[player]].GetDest(i).m_cost) + " " + moneda->Name1;
                    //msgConfirmacion = "Seguro que quieres teleportarte a\n\n" + name + "\n\nEl coste es: \n" + std::to_string(TabCatDest[Cat[player]].GetDest(i).m_cost) + " " + moneda->Name1;
                }
            }

            /*name = "|TInterface/ICONS/"+icon+":"+size+":"+size+"|t "+name;*/
            name = "|T" + icon + ":" + size_v + ":" + size_h + "|t|c" + colour + " " + name;



            AddGossipItemFor(player, 2, name.c_str(), GOSSIP_TELEPORT, i, msgConfirmacion, ((TabCatDest[Cat[player]].GetDest(i).m_coin == 0) ? costeOro : 0), false); //taxi destination
        }

        if (i < TabCatDest[Cat[player]].size())
            AddGossipItemFor(player, 7, NEXT_PAGE, GOSSIP_NEXT_PAGED, 0);

        AddGossipItemFor(player, 7, MAIN_MENU, GOSSIP_MAIN_MENU, 0);

        SendGossipMenuFor(player, MSG_DEST, source->GetGUID());
    }

    // Verification before teleportation
    void ActionTeleport(Player * const player, WorldObject* const source, const uint32 &id)
    {
        Dest dest(TabCatDest[Cat[player]].GetDest(id));

        //Compruebo el tipo de grupo
        //tipo 0-> no necesita grupo; tipe 1-> necesita grupo; tipo 2->necesita banda
        const Group*  grupo = player->GetGroup();
        if (dest.m_tipogrupo > 0 && !player->IsGameMaster()) //si es 0 no necesita grupo
        {
            //Como aqui el destino es mayor que 0, compruebo si hay grupo
            if (grupo)
            {
                //tengo grupo, por lo tanto el tipo 1 lo pasa, compruebo si el destino es tipo 2 y si esta en banda
                if (dest.m_tipogrupo > 1 && !grupo->isRaidGroup())
                {
                    std::string msg("No formas parte de ninguna banda.");
                    ObjectGuid guid = source->GetGUID();
                    if (guid.IsCreature())
                        source->ToCreature()->Whisper(msg.c_str(), LANG_UNIVERSAL, player);
                    else if (guid.IsPlayer())
                    {
                        ChatHandler handler = ChatHandler(player->GetSession());
                        handler.PSendSysMessage(msg.c_str());
                    }
                    return;
                }
            }
            else
            {
                std::string msg("No formas parte de ningun grupo.");
                ObjectGuid guid = source->GetGUID();
                if (guid.IsCreature())
                    source->ToCreature()->Whisper(msg.c_str(), LANG_UNIVERSAL, player);
                else if (guid.IsPlayer())
                {
                    ChatHandler handler = ChatHandler(player->GetSession());
                    handler.PSendSysMessage(msg.c_str());
                }
                return;
            }
        }


        if (dest.m_coin == 0)
        {
            uint32 costeOro = 0;
            //si tiene la racial de goblin, aplica descuento, sino coje el precio exacto
            if (player->HasSpell(69044))
            {
                costeOro = (dest.m_cost * (100 - dest.m_descuento)) / 100;
            }
            else
            {
                costeOro = dest.m_cost;
            }


            //Si la moneda es 0, es en Oro
            if (player->GetMoney() < costeOro && !player->IsGameMaster())
            {
                LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
                char const* text = sObjectMgr->GetTrinityString(8001, loc_idx);
                std::string msg(text + (" " + ConvertMoney(dest.m_cost) + "."));

                ObjectGuid guid = source->GetGUID();
                if (guid.IsCreature())
                    source->ToCreature()->Whisper(msg.c_str(), LANG_UNIVERSAL, player);
                else if (guid.IsPlayer())
                    source->ToPlayer()->Whisper(msg.c_str(), LANG_UNIVERSAL, player);
                return;
            }

            if (!player->IsGameMaster() && costeOro)
                player->ModifyMoney(-1 * costeOro);
        }
        else
        {
            //Si la moneda no es 0, el coste es en tokkens
            if (!player->HasItemCount(dest.m_coin, dest.m_cost, false) && !player->IsGameMaster())
            {
                LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
                /*char const* text = sObjectMgr->GetTrinityString(8001, loc_idx);*/
                const ItemTemplate* moneda = sObjectMgr->GetItemTemplate(dest.m_coin);
                if (moneda)
                {
                    std::string msg("No tienes suficientes " + moneda->Name1 + ", el precio del destino es de " + std::to_string(dest.m_cost) + ".");
                    ObjectGuid guid = source->GetGUID();
                    if (guid.IsCreature())
                        source->ToCreature()->Whisper(msg.c_str(), LANG_UNIVERSAL, player);
                    else if (guid.IsPlayer())
                    {
                        ChatHandler handler = ChatHandler(player->GetSession());
                        handler.PSendSysMessage(msg.c_str());
                    }

                }
                return;
            }

            if (!player->IsGameMaster() && dest.m_cost)
                player->DestroyItemCount(dest.m_coin, -1 * (dest.m_cost), true, false);

        }




        //Si hay algo en el campo auras, los troceo, compruebo y aplico
        if (dest.m_auras != "") CastSpell_ApplyAuras(player, source, dest.m_auras, ApplyAura);
        //Si hay algo en el campo spell, los troceo, compruebo y casteo
        if (dest.m_spells != "") CastSpell_ApplyAuras(player, source, dest.m_spells, CastSpell);

        //Compruebo si esta montado y lo desmonoto
        if (player->IsMounted())
        {
            player->Dismount();
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);
        }

        //Teleporto al player
        Teleport(player, dest.m_map, dest.m_X, dest.m_Y, dest.m_Z, dest.m_orient);
    }

    void _TeleportRecomendacion(Player* player)
    {
        uint32 idCategoria = 8;
        //Busco si alguna cumple
        TeleportMenuItem menu = from(m_lstMenuTeleport)
            >> where([&, player](TeleportMenuItem const & g) {	return g.m_Categoria == idCategoria && IsAllowedTele(player, g); })
            >> select([](TeleportMenuItem const & g) {return g; })
            >> take(1)
            >> first_or_default();

        if (menu.m_Categoria == 0 && menu.m_menuID == 0)
            return;

        //Compruebo si esta montado y lo desmonoto
        if (player->IsMounted())
        {
            player->Dismount();
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);
        }

        //Teleporto al player
        Teleport(player, menu.m_map, menu.m_X, menu.m_Y, menu.m_Z, menu.m_orient);
    }


    void _ShowRecomendacion(Player * const player)
    {
        uint32 idCategoria = 8;
        //Busco si alguna cumple
        TeleportMenuItem menu = from(m_lstMenuTeleport)
            >> where([&, player](TeleportMenuItem const & g) {	return g.m_Categoria == idCategoria && g.m_levelMin == player->getLevel(); })
            >> select([](TeleportMenuItem const & g) {return g; })
            >> take(1)
            >> first_or_default();

        if (menu.m_Categoria == 0 && menu.m_menuID == 0)
            return;

        //Si cumple, cierro menus y seteo que estamos en el gossip de Guia del Aventurero
        player->SetCurrentNPCGossip(TRAVELER_GUIDE_HELLO_REQUEST);
        ClearGossipMenuFor(player);
        CloseGossipMenuFor(player);
        AddGossipItemFor(player, 3, "Guia del aventurero", GOSSIP_SHOW_DESTINATION, 0, menu.m_popup, 0, false); //Muestro la opcion
        SendGossipMenuFor(player, MSG_DEST, player->GetGUID());
    }

}




bool Teleport_Gossip::GossipHello(Player *player, WorldObject* source)
{
    CloseGossipMenuFor(player);
    ClearGossipMenuFor(player);
    PageC(player) = PageD(player) = Cat(player) = 0;

    if (player->IsInCombat())
    {
        CloseGossipMenuFor(player);

        LocaleConstant loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
        char const* text = sObjectMgr->GetTrinityString(8002, loc_idx);
        player->GetSession()->SendNotification("ESTAS EN COMBATE");
        ObjectGuid guid = source->GetGUID();
        if (guid.IsCreature())
            source->ToCreature()->Whisper("ESTAS EN COMBATE", LANG_UNIVERSAL, player);
        else if (guid.IsPlayer())
        {
            ChatHandler handler = ChatHandler(player->GetSession());
            handler.PSendSysMessage("ESTAS EN COMBATE");
        }
        /*me->Whisper(text, LANG_UNIVERSAL, player);*/

        return true;
    }
    AffichCat(player, source);
    return true;
}

bool Teleport_Gossip::GossipSelect(Player *player, WorldObject* source, uint32 /*menuId*/, uint32 gossipListId)
{
    uint32 sender = player->PlayerTalkClass->GetGossipOptionSender(gossipListId);
    uint32 param = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);


    player->PlayerTalkClass->ClearMenus();
    switch (sender)
    {
        // Display destinations
    case GOSSIP_SHOW_DESTINATION:
        Cat(player) = param;
        AffichDest(player, source);
        break;

        // Previous categories page
    case GOSSIP_PREV_PAGEC:
        --PageC(player);
        AffichCat(player, source);
        break;

        // Next page categories
    case GOSSIP_NEXT_PAGEC:
        ++PageC(player);
        AffichCat(player, source);
        break;

        // Previous destinations page
    case GOSSIP_PREV_PAGED:
        --PageD(player);
        AffichDest(player, source);
        break;

        // Next destination page
    case GOSSIP_NEXT_PAGED:
        ++PageD(player);
        AffichDest(player, source);
        break;

        // Display main menu
    case GOSSIP_MAIN_MENU:
        GossipHello(player, source);
        break;

        // Teleportation
    case GOSSIP_TELEPORT:
        CloseGossipMenuFor(player);
        if (player->HasAura(SPELL_ID_PASSIVE_RESURRECTION_SICKNESS))
        {
            ObjectGuid guid = source->GetGUID();
            if (guid.IsCreature())
                source->ToCreature()->CastSpell(player, 38588, false); // Healing effect
            else if (guid.IsPlayer())
                source->ToPlayer()->CastSpell(player, 38588, false); // Healing effect
            player->RemoveAurasDueToSpell(SPELL_ID_PASSIVE_RESURRECTION_SICKNESS);
        }

        ActionTeleport(player, source, param);
        break;
    }
    return true;
}

void Teleport_Gossip::ShowRecomendacion(Player *player)
{
    _ShowRecomendacion(player);
}

void Teleport_Gossip::TeleportRecomendacion(Player *player)
{
    _TeleportRecomendacion(player);
}

