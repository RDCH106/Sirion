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
#include "..\ArenaSpectator\Arena_Spectator_menu.h"

int8 UsingGossip;

enum NpcSpectatorAtions {

	NPC_SPECTATOR_ACTION_LEAVE = 1,
	NPC_SPECTATOR_ACTION_BACK,
	// will be used for scrolling
	NPC_SPECTATOR_ACTION_2V2_GAMES = 1000000,  //NPC_SPECTATOR_ACTION_LIST_GAMES = 1000,
	NPC_SPECTATOR_ACTION_3V3_GAMES = 2000000, // NPC_SPECTATOR_ACTION_LIST_TOP_GAMES = 2000,

	// Disabled till I fix 2v2 , 3v3
	NPC_SPECTATOR_ACTION_5V5_GAMES = 3000000,
	NPC_SPECTATOR_ACTION_SPECIFIC = 4000000,

	// NPC_SPECTATOR_ACTION_SELECTED_PLAYER + player.Guid()
	NPC_SPECTATOR_ACTION_SELECTED_PLAYER = 5000000
};

const uint8  GamesOnPage = 15;

std::string GetClassNameById(uint8 id)
{
	std::string sClass = "";
	switch (id)
	{
	case CLASS_WARRIOR:         sClass = "Guerrero";					break;
	case CLASS_PALADIN:         sClass = "Paladin";						break;
	case CLASS_HUNTER:          sClass = "Cazador";						break;
	case CLASS_ROGUE:           sClass = "Picaro";						break;
	case CLASS_PRIEST:          sClass = "Sacerdote";					break;
	case CLASS_DEATH_KNIGHT:    sClass = "Caballero de la Muerte";      break;
	case CLASS_SHAMAN:          sClass = "Chaman";						break;
	case CLASS_MAGE:            sClass = "Mago";						break;
	case CLASS_WARLOCK:         sClass = "Brujo";						break;
	case CLASS_DRUID:           sClass = "Druida";						break;
	default:					sClass = "SIN ASIGNAR";					break;
	}
	return sClass;
}

std::string GetGamesStringData(Battleground* team, uint16 mmr, uint16 mmrTwo)
{
	std::string teamsMember[BG_TEAMS_COUNT];
	uint32 firstTeamId = 0;

	for (Battleground::BattlegroundPlayerMap::const_iterator itr = team->GetPlayers().begin(); itr != team->GetPlayers().end(); ++itr)
		if (Player* player = ObjectAccessor::FindPlayer(itr->first))
		{
			if (player->IsSpectator())
				continue;

			if (player->IsGameMaster())
				continue;

			uint32 team = itr->second.Team;
			if (!firstTeamId)
				firstTeamId = team;

			// Try to get ArenaTeamName
			std::string name;
			ArenaTeam* arena = sArenaTeamMgr->GetArenaTeamByName(name);
			//

			teamsMember[firstTeamId == team] += GetClassNameById(player->getClass());
		}

	std::string data = teamsMember[0] + "(";
	std::stringstream ss;
	std::stringstream sstwo;
	ss << mmr;
	sstwo << mmrTwo;
	data += ss.str();
	data += ") - ";
	data += teamsMember[1] + "(" + sstwo.str();
	data += ")";
	return data;
}

ObjectGuid GetFirstPlayerGuid(Battleground* team)
{
	for (Battleground::BattlegroundPlayerMap::const_iterator itr = team->GetPlayers().begin(); itr != team->GetPlayers().end(); ++itr)
		if (Player* player = ObjectAccessor::FindPlayer(itr->first))
			return itr->first;
	return ObjectGuid::Empty;
}

void GetAllPlayersName(Battleground* team, std::vector<std::string>* vstrPlayers)
{
	for (Battleground::BattlegroundPlayerMap::const_iterator itr = team->GetPlayers().begin(); itr != team->GetPlayers().end(); ++itr)
		if (Player* player = ObjectAccessor::FindPlayer(itr->first))
			vstrPlayers->push_back(player->GetName());
}

void ShowPage(Player* player, uint16 page, uint32 IsTop,uint32 type)
//void ShowPage(Player* player, uint16 page, bool IsTop)
{
	uint32 firstTeamId = 0;
	uint16 TypeOne = 0;
	uint16 TypeTwo = 0;
	uint16 TypeThree = 0;
	uint16 TypePlayer = 0;
	uint16 mmr = 0;
	uint16 mmrTwo = 0;
	//Para buscar player a player, añado un vector de strings
	std::vector<std::string> vstrPlayers;



	bool haveNextPage = false;
	for (uint8 i = 0; i <= MAX_BATTLEGROUND_TYPE_ID; ++i)
	{
		if (!sBattlegroundMgr->IsArenaType(BattlegroundTypeId(i)))
			continue;

		//BattlegroundContainer arenas = sBattlegroundMgr->GetBattlegroundsByType((BattlegroundTypeId)i);
		BattlegroundData* arenas = sBattlegroundMgr->GetAllBattlegroundsWithTypeId(BattlegroundTypeId(i));

		if (!arenas || arenas->m_Battlegrounds.empty())
			continue;

		for (BattlegroundContainer::const_iterator itr = arenas->m_Battlegrounds.begin(); itr != arenas->m_Battlegrounds.end(); ++itr)
		{
			Battleground* arena = itr->second;
			Player* target = ObjectAccessor::FindPlayer(GetFirstPlayerGuid(arena));
			if (target && (target->HasAura(32728) || target->HasAura(32727)))
				continue;

			if (!arena->GetPlayersSize())
				continue;

			//uint16 mmrTwo = arena->GetArenaMatchmakerRating(0);
			//uint16 mmrThree = arena->GetArenaMatchmakerRating(1);

			// This shows the mmr of both teams on ARENA_TYPE_2v2
			if (arena->GetArenaType() == ARENA_TYPE_2v2)
			{
				mmr = arena->GetArenaMatchmakerRating(0);
				firstTeamId = target->GetArenaTeamId(0);
				Battleground::BattlegroundPlayerMap::const_iterator citr = arena->GetPlayers().begin();
				for (; citr != arena->GetPlayers().end(); ++citr)
					if (Player* plrs = ObjectAccessor::FindPlayer(citr->first))
						if (plrs->GetArenaTeamId(0) != firstTeamId)
							mmrTwo = arena->GetArenaMatchmakerRating(citr->second.Team);
			}
			// This shows the mmr of both teams on ARENA_TYPE_3v3
			else if (arena->GetArenaType() == ARENA_TYPE_3v3)
			{
				mmr = arena->GetArenaMatchmakerRating(1);
				firstTeamId = target->GetArenaTeamId(1);
				Battleground::BattlegroundPlayerMap::const_iterator citr = arena->GetPlayers().begin();
				for (; citr != arena->GetPlayers().end(); ++citr)
					if (Player* plrs = ObjectAccessor::FindPlayer(citr->first))
						if (plrs->GetArenaTeamId(1) != firstTeamId)
							mmrTwo = arena->GetArenaMatchmakerRating(citr->second.Team);
			}
			else if (arena->GetArenaType() == ARENA_TYPE_5v5)
			{
				mmr = arena->GetArenaMatchmakerRating(2);
				firstTeamId = target->GetArenaTeamId(2);
				Battleground::BattlegroundPlayerMap::const_iterator citr = arena->GetPlayers().begin();
				for (; citr != arena->GetPlayers().end(); ++citr)
					if (Player* plrs = ObjectAccessor::FindPlayer(citr->first))
						if (plrs->GetArenaTeamId(2) != firstTeamId)
							mmrTwo = arena->GetArenaMatchmakerRating(citr->second.Team);
			}

			if (IsTop == 0) //esto va a ser el buscar players
			{
				GetAllPlayersName(arena, &vstrPlayers);
			}
			else if (IsTop == ARENA_TYPE_2v2 && arena->GetArenaType() == ARENA_TYPE_2v2)
			{
				TypeOne++;
				if (TypeOne > (page + 1) * GamesOnPage)
				{
					haveNextPage = true;
					break;
				}

				if (TypeOne >= page * GamesOnPage)
					AddGossipItemFor(player, GOSSIP_ICON_BATTLE, GetGamesStringData(arena, mmr, mmrTwo), GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + GetFirstPlayerGuid(arena));
			}
			else if (IsTop == ARENA_TYPE_3v3 && arena->GetArenaType() == ARENA_TYPE_3v3)
			{
				TypeTwo++;
				if (TypeTwo > (page + 1) * GamesOnPage)
				{
					haveNextPage = true;
					break;
				}

				if (TypeTwo >= page * GamesOnPage)
					AddGossipItemFor(player, GOSSIP_ICON_BATTLE, GetGamesStringData(arena, mmr, mmrTwo), GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + GetFirstPlayerGuid(arena));
			}
			else if (IsTop == ARENA_TYPE_5v5 && arena->GetArenaType() == ARENA_TYPE_5v5)
			{
				TypeThree++;
				if (TypeThree > (page + 1) * GamesOnPage)
				{
					haveNextPage = true;
					break;
				}
				if (TypeThree >= page * GamesOnPage)
					AddGossipItemFor(player, GOSSIP_ICON_BATTLE, GetGamesStringData(arena, mmr, mmrTwo), GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + GetFirstPlayerGuid(arena));
			}
		}
	}

	if (IsTop == 0) //esto va a ser el buscar players
	{
		// Sort names using std::sort
		std::sort(vstrPlayers.begin(), vstrPlayers.end());

		for (std::vector<std::string>::const_iterator itr = vstrPlayers.begin(); itr != vstrPlayers.end(); ++itr)
		{
			if (Player* target = ObjectAccessor::FindPlayerByName(itr->c_str()))
			{
				TypePlayer++;
				if (TypePlayer > (page + 1) * GamesOnPage)
				{
					haveNextPage = true;

				}
				uint32 testGuid = target->GetGUID();
				if (TypePlayer >= page * GamesOnPage)
					AddGossipItemFor(player, GOSSIP_ICON_BATTLE, target->GetName() + "(" + GetClassNameById(target->getClass()) + ")", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + target->GetGUID());
			}
		}



	}

	if (page > 0)
	{
		if (type == NPC_SPECTATOR_ACTION_2V2_GAMES)
			AddGossipItemFor(player, GOSSIP_ICON_DOT, "Prev..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2V2_GAMES + page - 1);
		else if (type == NPC_SPECTATOR_ACTION_3V3_GAMES)
			AddGossipItemFor(player, GOSSIP_ICON_DOT, "Prev..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_3V3_GAMES + page - 1);
		else if (type == NPC_SPECTATOR_ACTION_5V5_GAMES)
			AddGossipItemFor(player, GOSSIP_ICON_DOT, "Prev..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_5V5_GAMES + page - 1);
		else if (type == NPC_SPECTATOR_ACTION_SPECIFIC)
			AddGossipItemFor(player, GOSSIP_ICON_DOT, "Prev..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SPECIFIC + page - 1);
	}
	if (haveNextPage)
	{
		if (type == NPC_SPECTATOR_ACTION_2V2_GAMES)
			AddGossipItemFor(player, GOSSIP_ICON_DOT, "Siguiente..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2V2_GAMES + page + 1);
		else if (type == NPC_SPECTATOR_ACTION_3V3_GAMES)
			AddGossipItemFor(player, GOSSIP_ICON_DOT, "Siguiente..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_3V3_GAMES + page + 1);
		else if (type == NPC_SPECTATOR_ACTION_5V5_GAMES)
			AddGossipItemFor(player, GOSSIP_ICON_DOT, "Siguiente..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_5V5_GAMES + page + 1);
		else if (type == NPC_SPECTATOR_ACTION_SPECIFIC)
			AddGossipItemFor(player, GOSSIP_ICON_DOT, "Siguiente..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SPECIFIC + page + 1);
	}
	AddGossipItemFor(player, GOSSIP_ICON_DOT, "<-Atras", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_BACK);

}

bool ArenaSpectator_Gossip::GossipHello(Player* player, WorldObject* source)
{
	CloseGossipMenuFor(player);
	ClearGossipMenuFor(player);
	if (player->IsSpectator())
	{
		AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Salir de la arena", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_LEAVE);
	}
	else 
	{
		AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|TInterface\\icons\\Achievement_Arena_5v5_7:35:35:-30:0|tArenas 1v1", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_5V5_GAMES);
		AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|TInterface\\icons\\Achievement_Arena_2v2_7:35:35:-30:0|tArenas 2v2", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2V2_GAMES);
		AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|TInterface\\icons\\Achievement_Arena_3v3_7:35:35:-30:0|tArenas 3v3", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_3V3_GAMES);
		AddGossipItemFor(player, GOSSIP_ICON_CHAT, "|TInterface\\icons\\Spell_Holy_DevineAegis:35:35:-30:0|tJugador", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SPECIFIC);
	}
	SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());
	return true;
}

bool ArenaSpectator_Gossip::GossipSelect(Player *player, WorldObject* source, uint32 menuId, uint32 gossipListId)
{
	uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);


	player->PlayerTalkClass->ClearMenus();

	/*if (action == NPC_SPECTATOR_ACTION_SPECIFIC)
	{

	}*/

	if (action == NPC_SPECTATOR_ACTION_LEAVE)
	{
		CloseGossipMenuFor(player);
		if (player->IsSpectator())	ArenaSpectator::Salir(player);
	}
	if (action == NPC_SPECTATOR_ACTION_BACK)
	{
		GossipHello(player, source);
	}
	else if (action >= NPC_SPECTATOR_ACTION_2V2_GAMES && action < NPC_SPECTATOR_ACTION_3V3_GAMES)
	{
		AddGossipItemFor(player, GOSSIP_ICON_DOT, " Actualizar", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2V2_GAMES);
		ShowPage(player, action - NPC_SPECTATOR_ACTION_2V2_GAMES, /*false*/ ARENA_TYPE_2v2, NPC_SPECTATOR_ACTION_2V2_GAMES);
		SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());
	}
	else if (action >= NPC_SPECTATOR_ACTION_3V3_GAMES && action < NPC_SPECTATOR_ACTION_5V5_GAMES)
		//else if (action >= NPC_SPECTATOR_ACTION_3V3_GAMES && action < NPC_SPECTATOR_ACTION_2V2_GAMES)
	{
		AddGossipItemFor(player, GOSSIP_ICON_DOT, "Actualizar", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_3V3_GAMES);
		ShowPage(player, action - NPC_SPECTATOR_ACTION_3V3_GAMES, /*true*/ARENA_TYPE_3v3, NPC_SPECTATOR_ACTION_3V3_GAMES);
		SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());
	}
	else if (action >= NPC_SPECTATOR_ACTION_5V5_GAMES && action < NPC_SPECTATOR_ACTION_SPECIFIC)
	{
		AddGossipItemFor(player, GOSSIP_ICON_DOT, "Actualizar", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_5V5_GAMES);
		ShowPage(player, action - NPC_SPECTATOR_ACTION_5V5_GAMES, ARENA_TYPE_5v5, NPC_SPECTATOR_ACTION_5V5_GAMES);
		SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());
	}
	else if (action >= NPC_SPECTATOR_ACTION_SPECIFIC && action < NPC_SPECTATOR_ACTION_SELECTED_PLAYER)
	{
		AddGossipItemFor(player, GOSSIP_ICON_DOT, "Actualizar", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SPECIFIC);
		ShowPage(player, action - NPC_SPECTATOR_ACTION_SPECIFIC, 0, NPC_SPECTATOR_ACTION_SPECIFIC);
		SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, source->GetGUID());
	}
	else
	{
		uint32 testGuid = action - NPC_SPECTATOR_ACTION_SELECTED_PLAYER;
		ObjectGuid guid = ObjectGuid(HighGuid::Player, action - NPC_SPECTATOR_ACTION_SELECTED_PLAYER);
		if (Player* target = ObjectAccessor::FindPlayer(guid))
		{
			ArenaSpectator::EspectarJugador(player, target);
		}
	}
	return true;
}

/*
bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code)
{
if (!player)
return true;

player->PlayerTalkClass->ClearMenus();
player->CLOSE_GOSSIP_MENU();
if (sender == GOSSIP_SENDER_MAIN)
{
switch (action)
{
case NPC_SPECTATOR_ACTION_SPECIFIC: // choosing a player

const char* plrName = code;

char playerName[50];
strcpy(playerName, plrName);

for (int i = 0; i < 13; i++)
{
if (playerName[i] == NULL)
break;
if (i == 0 && playerName[i] > 96)
playerName[0] -= 32;
else if (playerName[i] < 97)
playerName[i] += 32;
}

if (Player* target = ObjectAccessor::FindPlayerByName(playerName))
{
ChatHandler handler(player->GetSession());
char const* pTarget = target->GetName().c_str();
arena_spectator_commands::HandleSpectateCommand(&handler, pTarget);
}
ChatHandler(player->GetSession()).PSendSysMessage("Player is not online or does not exist.");
return true;
}
}

return false;
}

if (IsTop && arena->GetArenaType() == ARENA_TYPE_3v3)
{
TypeThree++;
if (TypeThree > (page + 1) * GamesOnPage)
{
haveNextPage = true;
break;
}

if (TypeThree >= page * GamesOnPage)
AddGossipItemFor(player,GOSSIP_ICON_BATTLE, GetGamesStringData(arena, mmr, mmrTwo/*mmrThree*//*), GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + GetFirstPlayerGuid(arena));

}
else if (!IsTop && arena->GetArenaType() == ARENA_TYPE_2v2)
{
TypeTwo++;
if (TypeTwo > (page + 1) * GamesOnPage)
{
haveNextPage = true;
break;
}

if (TypeTwo >= page * GamesOnPage)
AddGossipItemFor(player, GOSSIP_ICON_BATTLE, GetGamesStringData(arena, mmr, mmrTwo), GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + GetFirstPlayerGuid(arena));
}
}
}

if (page > 0)
AddGossipItemFor(player, GOSSIP_ICON_DOT, "Prev..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2V2_GAMES + page - 1);

if (haveNextPage)
AddGossipItemFor(player, GOSSIP_ICON_DOT, "Next..", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2V2_GAMES + page + 1);
}*/


