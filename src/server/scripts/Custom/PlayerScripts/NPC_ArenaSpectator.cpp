#include "..\Auxiliares\ArenaSpectator\Arena_Spectator_menu.h"

class ps_arena_spectator : public PlayerScript
{
public:
	ps_arena_spectator() : PlayerScript("ps_arena_spectator") { }

	void GossipHelloRequest(Player* player) override
	{
		if (player->IsCurrentNPCGossip(ARENA_SPECTATOR_GOSSIP_HELLO_REQUEST))
			ArenaSpectator_Gossip::GossipHello(player, player);
	}

	void GossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
	{
		if (player->IsCurrentNPCGossip(ARENA_SPECTATOR_GOSSIP_HELLO_REQUEST))
			ArenaSpectator_Gossip::GossipSelect(player, player, menuId, gossipListId);
	}
};

void AddSC_ps_arena_spectator()
{
	new ps_arena_spectator;
}