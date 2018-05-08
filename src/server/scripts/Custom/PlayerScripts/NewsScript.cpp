#include "ScriptPCH.h"
#include "World.h"
#include "WorldSession.h"
#include "ServerNews.h"
#include "ScriptedGossip.h"
#include "GossipDef.h"

class NewsSystemPlayer : public PlayerScript
{
public:
	NewsSystemPlayer() : PlayerScript("NewsSystem") {}

	void OnLogin(Player* player, bool bFirstLogin)
    {
		if (!bFirstLogin)
		{
			if (!player->ShowedPopUp())
			{
				AddGossipItemFor(player, 3, "PopUp", 0, 0, ServerNews::GetNew(), 0, false);
				SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, player->GetGUID());
				player->SetPopUp();
			}
		}
    }
};

void AddSC_NewsSystemPlayer()
{
    new NewsSystemPlayer;
}
