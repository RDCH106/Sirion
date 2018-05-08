#include "Channel.h"
#include "ScriptMgr.h"
#include "Chat.h"


class System_Taberna : public PlayerScript
{
public:
	System_Taberna() : PlayerScript("System_Taberna") { }

    void AddIcon(Player* player, std::string& msg, uint32 lang)
    {
		if (player->isGMChat()) 
			return;
		if (player->GetTeam() == ALLIANCE)
			msg = "|TInterface/FriendsFrame/PlusManz-Alliance:20:20|t" + msg;
		else if (player->GetTeam() == HORDE)
			msg = "|TInterface/FriendsFrame/PlusManz-Horde:20:20|t" + msg;

    };
   
    void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Channel* channel) override
    {
		std::string canal = channel->GetName();
		std::transform(canal.begin(), canal.end(), canal.begin(), ::tolower);
		if(canal == "taberna")  AddIcon(player, msg, lang);
    }
};

void AddSC_System_Taberna()
{
    new System_Taberna();
}
