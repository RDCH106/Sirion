#include "World.h"
#include "Channel.h"
#include "Player.h"
#include "Guild.h"
class ApplyGuildDebuff : public GuildScript
{
public:
    ApplyGuildDebuff() : GuildScript("GuildDisbandDebuff") { }

    void OnDisband(Guild* guild) override
    {
        guild->ApplyDisbandDebuff();
    }
    
};

void AddSC_ApplyGuildDebuff()
{
    new ApplyGuildDebuff;
}
