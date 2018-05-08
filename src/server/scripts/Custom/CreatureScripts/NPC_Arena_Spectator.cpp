/*
* Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
Name: Arena Spectator
Complete: 80%
Comment: This script allow to spectate games
Category: Custom Script
Future Developing: 5v5, Specific Player
Credits: Flameshot http://flameshot.tk
		 LilleCarl
EndScriptData */

#include "..\Auxiliares\ArenaSpectator\Arena_Spectator_menu.h"

class npc_arena_spectator : public CreatureScript
{
public:
	npc_arena_spectator() : CreatureScript("npc_arena_spectator") { }

	

	struct npc_arena_spectatorAI : public ScriptedAI
	{
		npc_arena_spectatorAI(Creature* creature) : ScriptedAI(creature) { }

		bool GossipHello(Player* player) override
		{
			return ArenaSpectator_Gossip::GossipHello(player, me);
		}

		bool GossipSelect(Player *player, uint32 menuId, uint32 gossipListId) override
		{
			return ArenaSpectator_Gossip::GossipSelect(player, me, menuId, gossipListId);
		}

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_arena_spectatorAI(creature);
	}
};


void AddSC_arena_spectator_script()
{
	new npc_arena_spectator();
}