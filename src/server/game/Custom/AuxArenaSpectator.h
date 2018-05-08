
#ifndef ARENASPECTATOR_H
#define ARENASPECTATOR_H

#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "ObjectAccessor.h"
#include "ArenaTeamMgr.h"
#include "WorldSession.h"


class TC_GAME_API ArenaSpectator
{
public:	

	static bool EspectarJugador(ChatHandler* handler, char const* args);	
	static bool EspectarJugador(Player* player, Player* target) { return _espectarJugador(player, target); };
	
	static bool Salir(ChatHandler* handler) { return _salir(handler->GetSession()->GetPlayer()); };
	static bool Salir(Player* player) { return _salir(player); };
	
	static bool Ver(ChatHandler* handler, const char *args);
	static bool Ver(Player* player, Player* target) { return _ver(player, target); };
	
	static bool Reset(ChatHandler* handler) { return _reset(handler->GetSession()->GetPlayer()); };
	static bool Reset(Player* player) { return _reset(player); };;
	

private:
	static bool _espectarJugador(Player* player, Player* target);
	static bool _ver(Player* player, Player* target);
	static bool _salir(Player* player);
	static bool _reset(Player* player);
};
#endif