/*
Listo para puesta en produccion
*/

#include "..\Auxiliares\Loyalty\Loyalty_menu.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "ScriptPCH.h"
#include "WorldSession.h"
#include "World.h"
#include "StringFormat.h"



class npc_loyalty : public CreatureScript
{
public:
	npc_loyalty() : CreatureScript("npc_loyalty") { };

	struct npc_loyalty_AI : public ScriptedAI
	{
		npc_loyalty_AI(Creature* creature) : ScriptedAI(creature) { }
			

		bool GossipHello(Player* player) override
		{			
			return Loyalty_Gossip::GossipHello(player, me);			
		}

		bool GossipSelect(Player *player, uint32 menuId, uint32 gossipListId) override
		{
			return Loyalty_Gossip::GossipSelect(player, me,menuId,gossipListId);
		}

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_loyalty_AI(creature);
	}
};


class npc_loyalty_load : public WorldScript
{
public:
	npc_loyalty_load() : WorldScript("npc_loyalty_load") {}

	void OnStartup() override
	{
		LoadNpcLoyalty();
	}
};

void AddSC_npc_vote_rewarder()
{
	new npc_loyalty();
	new npc_loyalty_load();
}