#include "World.h"
#include "WorldSession.h"
#include "Player.h"
#include "Guild.h"

class announce_level : public PlayerScript
{
public:
    announce_level() : PlayerScript("announce_level") { }

    void OnLevelChanged(Player* player, uint8 oldLevel)
    {
        //Si la cuenta es GM
        if (player->GetSession()->GetSecurity() > 0)
            return;
        //Si es una subida por promocion
        if (!player->IsLevelAnnounceActiveFor())
            return;

        if (oldLevel < player->getLevel())
        {
            if (player->getLevel() % 5 == 0)
            {
                std::ostringstream ss;
                ss << "|cff3DAEFF[Level UP]|cffFFD800: |cff3DAEFF " << player->GetName() << "|cff4CFF00 subio a nivel |cff3DAEFF" + std::to_string(player->getLevel()) + ".";
                sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
            }
        }
    }
};

void AddSC_announce_level()
{
    new announce_level;
}
