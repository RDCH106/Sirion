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

#ifndef SC_NPC_LOYALTY_H
#define SC_NPC_LOYALTY_H

#include <vector>
#include "DatabaseEnv.h"
#include "Log.h"

namespace nsNpcLoyalty
{

	// Structure representing the rewards
	struct RewardsMenuItem
	{
		uint32		m_menuID;
		uint32		m_optionID;
		uint32		m_actionID;
		std::string m_name;
		std::string m_name_sin_parches;
        uint32			m_Recompensa;
		uint32		m_Cantidad;
		uint8       m_levelMin;
		uint8       m_levelMax;
		uint32		m_itemLevel;
		uint32		m_item1;
		uint32		m_item2;
		uint32		m_questA;
		uint32		m_questH;
		uint32		m_archievemnt;
		uint32		m_maskRace;
		bool		m_guild;
		uint32      m_cost;
		std::string m_popup_text;
		uint32		m_evento;
		bool		m_parches;
		uint32		m_maskClass;
		uint32      m_categoria;
		std::string m_codigo;
	};

	struct RewardsMenuTemplate
	{
		uint32 templateID;
		uint32 itemID;
		uint32 cantidad;
	};

	bool TC_GAME_API IsAllowedCategory(Player * const player, uint32 menuID);
	bool TC_GAME_API IsAllowedItem(Player * const player, RewardsMenuItem recompensa,bool skipPatchCheck = false);
	extern TC_GAME_API std::list<RewardsMenuItem, std::allocator<RewardsMenuItem>> m_lstMenuRewards;
	extern TC_GAME_API std::list<RewardsMenuTemplate, std::allocator<RewardsMenuTemplate>> m_lstMenuRewardsTemplate;

}

//Mapa de Menu


// Loading contents of database
void TC_GAME_API LoadNpcLoyalty(void);

#endif
