#include "World.h"
#include "Player.h"
#include "WorldSession.h"
#include "AccountMgr.h"
#include "CharacterDatabase.h"
#include "LoginDatabase.h"

class PromocionLogin : public PlayerScript
{
public:
	PromocionLogin() : PlayerScript("PromocionLogin") { }

	void OnLogin(Player* player, bool bFirstLogin)
	{
		if (bFirstLogin)
		{
			//Obtengo el numero de cuentas
			std::string strIP = player->GetSession()->GetRemoteAddress();
			QueryResult resultado = LoginDatabase.PQuery("SELECT count(*) FROM account WHERE last_ip = '%s'", strIP);
			int nCuentas = resultado->Fetch()[0].GetInt32();

			//Lanzo la consulta sobre la cuenta actual
			resultado = CharacterDatabase.PQuery("Select count(*) from characters where totaltime > 30 and account =%d", player->GetSession()->GetAccountId());
			//Si retorna 0, es porque no hay ningun PJ con tiempo de juego mayor que 30 segundos
			if (resultado->Fetch()[0].GetInt32() == 0)
			{
				player->AddItem(3165, 1); //Aumento de nivel +1
				player->AddItem(5116, 1); //Reseteador
				player->AddItem(34645, 1); //Elixir de conocimiento
				if (nCuentas == 1)
					player->AddItem(200015, 1); //item de mineira para cuando solo hay una cuenta en la IP
			}
		}
	}

};

void AddSC_PromocionLogin()
{
	new PromocionLogin;
}
