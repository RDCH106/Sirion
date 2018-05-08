#include "AuxArenaSpectator.h"
#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "ObjectAccessor.h"
#include "ArenaTeamMgr.h"
#include "RBAC.h"

class arena_spectator_commands : public CommandScript
{
public:
	arena_spectator_commands() : CommandScript("arena_spectator_commands") { }

	static bool HandleSpectateCommand(ChatHandler* handler, char const* args)
	{		
		return ArenaSpectator::EspectarJugador(handler,args);
	}

	static bool HandleSpectateCancelCommand(ChatHandler* handler, const char* /*args*/)
	{	
		return  ArenaSpectator::Salir(handler);
	}

	static bool HandleSpectateFromCommand(ChatHandler* handler, const char *args)
	{
		return ArenaSpectator::Ver(handler, args);
	}

	static bool HandleSpectateResetCommand(ChatHandler* handler, const char *args)
	{
		return  ArenaSpectator::Reset(handler);
	}

	std::vector<ChatCommand> GetCommands() const override
	{
		static std::vector<ChatCommand> spectateCommandTable =
		{
			{ "player",	 rbac::RBAC_PERM_COMMAND_SPECTATE_PLAYER, true, &HandleSpectateCommand,       "" },
			{ "view",	 rbac::RBAC_PERM_COMMAND_SPECTATE_VIEW,   true, &HandleSpectateFromCommand,   "" },
			{ "reset",   rbac::RBAC_PERM_COMMAND_SPECTATE_RESET,  true, &HandleSpectateResetCommand,  "" },
			{ "leave",   rbac::RBAC_PERM_COMMAND_SPECTATE_LEAVE,  true, &HandleSpectateCancelCommand, "" },
			//{ NULL,					0,						 false,				NULL,			  "" }
		};

		static std::vector<ChatCommand> commandTable =
		{
			{ "spectate", rbac::RBAC_PERM_COMMAND_SPECTATE, false, NULL, "", spectateCommandTable },
			//{ NULL, 0, false, NULL, "", NULL }
		};

		return commandTable;
	}
};

void AddSC_spectate_commandscript()
{
	new arena_spectator_commands();
} 