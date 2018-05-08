#include "World.h"
#include "Channel.h"
#include "Player.h"
#include "WorldSession.h"
class AutoJoinTaberna : public PlayerScript
{
public:
    AutoJoinTaberna() : PlayerScript("AutoJoinTaberna") { }

    void OnLogin(Player* player, bool bFirstLogin)
    {     
        //TODO Falta hacerlo, nada de lo que hay me gusta, porque si el player ya esta en taberna se lo vuelve a ofrecer
    }
};

void AddSC_AutoJoinTaberna()
{
    new AutoJoinTaberna;
}
