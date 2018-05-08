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

#ifndef SC_NPC_TELEPORT_H
#define SC_NPC_TELEPORT_H

#include "DatabaseEnvFwd.h"
#include <vector>
#include "Log.h"

namespace nsNpcTel
{
    //----------------PARTE PARA LA GUIA DEL VIAJERO
    struct TeleportMenuItem
    {
        uint32      m_Categoria;
        uint32		m_menuID;  
        uint8       m_levelMin;
        uint8       m_levelMax;      
        uint32		m_maskRace;      
        uint32      m_cost;
        uint32      m_map;
        float       m_X, m_Y, m_Z, m_orient;
        std::string m_popup;

    };
    extern TC_GAME_API std::list<TeleportMenuItem, std::allocator<TeleportMenuItem>> m_lstMenuTeleport;
    extern TC_GAME_API bool IsAllowedTele(Player * const player, TeleportMenuItem recompensa);
    

    //--------------FIN



	// Structure representing the destinations
	struct Dest
	{
		std::string m_icon;
		std::string m_size_v;
		std::string m_size_h;
		std::string m_colour;
		std::string m_name[9];
		float       m_X, m_Y, m_Z, m_orient;
		uint16      m_map;
		uint8       m_levelMin;
		uint8       m_levelMax;
		uint32		m_itemLevel;
		uint32		m_item1;
		uint32		m_item2;
		uint32		m_questA;
		uint32		m_questH;
		uint32		m_archievemnt;
		uint32		m_maskRace;
		uint32		m_guild;
		uint32      m_cost;
		uint32		m_coin;
		std::string	m_spells;
		std::string	m_auras;
		std::string m_popup_text;
		uint32		m_descuento;
		uint8		m_tipogrupo;
		uint32		m_evento;
		bool		m_parches;
	};

	struct CatValue
	{
		uint32 catid;
	};

	struct CatName
	{
		std::string icon;
		std::string m_size_v;
		std::string m_size_h;
		std::string colour;
		std::string name[9];
	};

	// Class representing the categories of destinations
	class TC_GAME_API CatDest
	{
	public:

		typedef std::vector<Dest> VDest;
		typedef VDest::size_type  VDest_t;

		CatDest(const CatValue cat, const CatName catname);

		void   AddDest(const Dest &item) { m_TabDest.push_back(item); }
		Dest   GetDest(const uint32 &id) const { return m_TabDest[id]; }
		uint32 GetCatID(void)             const { return m_catvalue.catid; }
		uint32 size(void)             const { return m_TabDest.size(); }

		std::string GetName(const uint8 loc /* = 0 */, const bool IsGM = false)    const;
		bool IsAllowedCategory(Player * const player) const;
		bool IsAllowedDestine(Player * const player, uint32 categoria, uint32 posicion)const;
		static uint32 CountOfCategoryAllowedBy(Player * const player);

	private:

		CatValue m_catvalue;
		CatName m_catname;
		VDest       m_TabDest;
	};

	// Class page for current player
	class TC_GAME_API Page
	{
	protected:

		// Class instance for current player
		class Instance
		{
		public:

			Instance(Player * const player, const uint32 &PageId = 0)
				: m_player(player), m_PageId(PageId) {}

			Instance TC_GAME_API & operator =  (const uint32 &id);
			Instance TC_GAME_API & operator ++ (void);
			Instance   operator ++ (int32);
			Instance TC_GAME_API & operator -- (void);
			Instance   operator -- (int32);

			uint32   GetPageId(void) const { return m_PageId; }
			Player * GetPlayer(void) const { return m_player; }

		private:

			Player *m_player;
			uint32  m_PageId;
		};

	public:

		typedef std::vector<Instance> VInst;
		typedef VInst::size_type      VInst_t;

		Page(void) { m_TabInstance.clear(); }

		Instance &   operator () (Player * const player);
		uint32 operator [] (Player * const player) const;

	private:

		VInst m_TabInstance;
	};

	typedef std::vector <CatDest> VCatDest;
	typedef VCatDest::size_type   VCatDest_t;

	// Verification of data integrity
	bool IsValidData(const CatValue cat);
    extern TC_GAME_API VCatDest TabCatDest;

    


	
}

// Loading contents of database
void TC_GAME_API LoadNpcTele(void);


#endif
