#include "ScriptPCH.h"
#include "World.h"
#include "WorldSession.h"

class ZoneProtector : public PlayerScript
{
public:
    ZoneProtector() : PlayerScript("ZoneProtector") {}

    void OnUpdateZone(Player* player, uint32 newZone, uint32 /*newArea*/) override
    {
        //Si estamos en la zona de testeo 
        if (newZone == 3817)
        {
            //Sin ser GM
            if (player->GetSession()->GetSecurity() == 0)
                //kick
                player->GetSession()->KickPlayer();
        }
    }
};

void AddSC_ZoneProtector()
{
    new ZoneProtector;
}
