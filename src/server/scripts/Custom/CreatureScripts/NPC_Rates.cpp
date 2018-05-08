#include "ScriptMgr.h"
#include "AchievementMgr.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"
#include <sstream>

class npc_rates_gossip : public CreatureScript
{
public:
	npc_rates_gossip() : CreatureScript("npc_rates") {}

	struct npc_ratesAI : public ScriptedAI
	{
		npc_ratesAI(Creature* creature) : ScriptedAI(creature) { }



		bool GossipHello(Player *player) override 
		{
			player->PlayerTalkClass->ClearMenus();
			AddGossipItemFor(player, 0, "Quiero EXP x 1", GOSSIP_SENDER_MAIN, 1);
			AddGossipItemFor(player, 0, "Quiero EXP x 5", GOSSIP_SENDER_MAIN, 5);
			AddGossipItemFor(player, 0, "Quiero EXP x 10", GOSSIP_SENDER_MAIN, 10);
			AddGossipItemFor(player, 3, "Salir", GOSSIP_SENDER_MAIN, 0);
			SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
			return true;
		}


		bool GossipSelect(Player *player, uint32 /*menuId*/, uint32 gossipListId) override
		{
			uint32 sender = player->PlayerTalkClass->GetGossipOptionSender(gossipListId);
			uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);

			if (!player) return false;
			player->PlayerTalkClass->ClearMenus();
			switch (sender)
			{
			case GOSSIP_SENDER_MAIN:
				if (action > 0)
				{
					player->SetExpRate(action);
					if (action == 1)	player->GetSession()->SendAreaTriggerMessage("Se ha cambiado tu ratio de experiencia a Blizkie!");
					if (action == 5)	player->GetSession()->SendAreaTriggerMessage("Se ha cambiado tu ratio de experiencia x5!");
					if (action == 10)	player->GetSession()->SendAreaTriggerMessage("Se ha cambiado tu ratio de experiencia x10!");
					GossipHello(player);
				}
				else CloseGossipMenuFor(player);
				break;
			default:

				break;
			}



			return true;
		}

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_ratesAI(creature);
	}

};


void AddSC_npc_rates()
{
	new npc_rates_gossip;
}
