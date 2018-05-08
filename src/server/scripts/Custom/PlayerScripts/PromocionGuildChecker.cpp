#include "World.h"
#include "Player.h"
#include "WorldSession.h"
#include "AccountMgr.h"


void BanPlayer(Player* player)
{
    sWorld->BanCharacter(player->GetName(), "-1", "NO SE PUEDE DESBANEAR! Razon: Incumplir requisitos de la promocion", "Sirion Server");
}


class PromocionGuildCheck : public PlayerScript
{
public:
    PromocionGuildCheck() : PlayerScript("PromocionGuildCheck") { }

    void OnLogin(Player* player, bool bFirstLogin)
    {
        //Si tiene activada la promocion de guild, y ya no tiene el debuff, han pasado 7 dias
        if (player->HasPromoGuild() && !player->HasPromoDebuff())
        {
            uint32 nTotalTime = player->GetTotalPlayedTime();
            //Se contabiliza el tiempo en segundos, por eso, 7 horas = 3600s * 7
            if(nTotalTime < (3600*7))
            {                
                BanPlayer(player);
            }
        }
    }
    
};

void AddSC_PromocionGuildCheck()
{
    new PromocionGuildCheck;
}
