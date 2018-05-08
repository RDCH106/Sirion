#include "..\Auxiliares\Teleport\Teleport_menu.h"

class ps_teleport : public PlayerScript
{
public:
	ps_teleport() : PlayerScript("ps_teleport") { }

	void GossipHelloRequest(Player* player) override
	{
		if (player->IsCurrentNPCGossip(TELEPORT_GOSSIP_HELLO_REQUEST))
			Teleport_Gossip::GossipHello(player, player);
	}

	void GossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
	{
        if (player->IsCurrentNPCGossip(TELEPORT_GOSSIP_HELLO_REQUEST))
            Teleport_Gossip::GossipSelect(player, player, menuId, gossipListId);
        else if (player->IsCurrentNPCGossip(TRAVELER_GUIDE_HELLO_REQUEST))
            Teleport_Gossip::TeleportRecomendacion(player);

	}

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (player->getLevel() < 5 || player->getLevel() == 80)
            return;
        Teleport_Gossip::ShowRecomendacion(player);
    }
};

void AddSC_ps_teleport()
{
	new ps_teleport;
}
