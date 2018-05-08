#include "..\Auxiliares\TransmorgVendor\TransmogVendor_menu.h"

class ps_tmorg : public PlayerScript
{
public:
	ps_tmorg() : PlayerScript("ps_tmorg") { }

	void GossipHelloRequest(Player* player) override
	{
		if (player->IsCurrentNPCGossip(TRANSMORG_GOSSIP_HELLO_REQUEST))
			TransmorgVendor_Gossip::GossipHello(player, player);
	}

	void GossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
	{
		if (player->IsCurrentNPCGossip(TRANSMORG_GOSSIP_HELLO_REQUEST))
			TransmorgVendor_Gossip::GossipSelect(player, player, menuId, gossipListId);
	}
};

void AddSC_ps_tmorg()
{
	new ps_tmorg;
}