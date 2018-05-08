#include "World.h"
#include "Channel.h"
#include "Player.h"
#include "WorldSession.h"
class announce_login : public PlayerScript
{
public:
    announce_login() : PlayerScript("announce_login") { }

    void OnLogin(Player* player, bool bFirstLogin)
    {
		if (player->GetSession()->GetSecurity() > 0)
			return;
        if (player->GetTeam() == ALLIANCE)
        {
            std::ostringstream ss;
            if (bFirstLogin)  ss << "|cff3DAEFF[Aviso]:|r |cff0026FF" << player->GetName() << "|r acaba de ingresar al reino.";
            else            ss << "|cff3DAEFF[Aviso]|cffFFD800 : El jugador |cff4CFF00" << player->GetName() << "|r |cffFFD800se ha conectado.|r";
            sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
        }
        else
        {
            std::ostringstream ss;
            if (bFirstLogin) ss << "|cff3DAEFF[Aviso]:|r |cffFF0000 " << player->GetName() << "|r acaba de ingresar al reino.";
            else            ss << "|cff3DAEFF[Aviso]|cffFFD800 : El jugador |cffFF0000" << player->GetName() << "|r |cffFFD800se ha conectado.|r";
            sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
        }
    }
};

void AddSC_announce_login()
{
    new announce_login;
}
