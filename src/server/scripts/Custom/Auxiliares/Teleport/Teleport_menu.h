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



#include "ScriptMgr.h"
#include "sc_npc_teleport.h"
#include "Group.h"
#include "ScriptedGossip.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "Chat.h"
#include "WorldSession.h"
#include "SpellMgr.h"
#include "ObjectMgr.h"
#include <sstream>



enum Accion
{
	ApplyAura = 0,
	CastSpell = 1
};

using namespace nsNpcTel;



TC_GAME_API class Teleport_Gossip
{
public:

	static bool GossipHello(Player *player, WorldObject* source);

	static bool GossipSelect(Player *player, WorldObject* source, uint32 /*menuId*/, uint32 gossipListId);

    static void ShowRecomendacion(Player *player);

    static void TeleportRecomendacion(Player *player);

};

