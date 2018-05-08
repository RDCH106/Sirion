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
#include "..\Auxiliares\Teleport\Teleport_menu.h"


class npc_teleport_gossip : public CreatureScript
{
public:
	npc_teleport_gossip() : CreatureScript("npc_teleport") {}

	struct npc_teleportAI : public ScriptedAI
	{
		npc_teleportAI(Creature* creature) : ScriptedAI(creature) { }


		bool GossipHello(Player *player) override 
		{
			return Teleport_Gossip::GossipHello(player, me);
		}

		bool GossipSelect(Player *player, uint32 menuId, uint32 gossipListId) override
		{			
			return Teleport_Gossip::GossipSelect(player,me,menuId,gossipListId);
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_teleportAI(creature);
	}
};

class npc_teleport_load : public WorldScript
{
public:
	npc_teleport_load() : WorldScript("npc_teleport_load") {}

	void OnStartup() override
	{
		LoadNpcTele();
	}
};

void AddSC_npc_teleport()
{
	new npc_teleport_gossip;
	new npc_teleport_load;
}
// End of TeleNPC2
