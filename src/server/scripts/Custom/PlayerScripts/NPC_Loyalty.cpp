#include "..\Auxiliares\Loyalty\Loyalty_menu.h"
class ps_loyalty : public PlayerScript
{
public:
	ps_loyalty() : PlayerScript("ps_loyalty") { }

	void GossipHelloRequest(Player* player) override
	{
		if (player->IsCurrentNPCGossip(LOYALTY_GOSSIP_HELLO_REQUEST))
			Loyalty_Gossip::GossipHello(player, player);
		else if (player->IsCurrentNPCGossip(LOYALTY_SHOP_HELLO_REQUEST))
			Loyalty_Gossip::ShowCategory(player, player);
	}

	void GossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
	{
		if (player->IsCurrentNPCGossip(LOYALTY_GOSSIP_HELLO_REQUEST) || player->IsCurrentNPCGossip(LOYALTY_SHOP_HELLO_REQUEST))
			Loyalty_Gossip::GossipSelect(player, player, menuId, gossipListId);
	}
};

void AddSC_ps_loyalty()
{
	new ps_loyalty;
}