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

#include "AuxArenaSpectator.h"
#include "Chat.h"
#include "ArenaTeamMgr.h"
#include "BattlegroundMgr.h"
#include "WorldSession.h"
#include "Player.h"
#include "ArenaTeam.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "CreatureTextMgr.h"
#include "Config.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"


TC_GAME_API class ArenaSpectator_Gossip
{
public:

	static bool GossipHello(Player* player, WorldObject* source);

	static bool GossipSelect(Player *player, WorldObject* source, uint32 /*menuId*/, uint32 gossipListId);

};

