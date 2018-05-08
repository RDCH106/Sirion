#include "AuxArenaSpectator.h"
#include "Chat.h"
#include "Player.h"
#include "Map.h"
#include "MotionMaster.h"


bool ArenaSpectator::_espectarJugador(Player* player, Player* target)
{
	
	ChatHandler handler(player->GetSession());
	if (target == player || target->GetGUID() == player->GetGUID())
	{
		handler.PSendSysMessage("No puedes espectarte a ti mismo.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (player->IsInCombat())
	{
		handler.PSendSysMessage("Estas en combate.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (!target)
	{
		handler.PSendSysMessage("El objetivo no existe o no esta online.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (player->GetPet())
	{
		handler.PSendSysMessage("Debes esconder tu mascota.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (player->GetMap()->IsBattlegroundOrArena() && !player->IsSpectator())
	{
		handler.PSendSysMessage("Ya estas en una arena o campo de batalla.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	Map* cMap = target->GetMap();
	if (!cMap->IsBattleArena())
	{
		handler.PSendSysMessage("El jugador no esta en arena.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (player->GetMap()->IsBattleground())
	{
		handler.PSendSysMessage("No puedes hacer esto en un campo de batalla.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (target->HasAura(32728) || target->HasAura(32727))
	{
		handler.PSendSysMessage("La arena aun no ha empezado.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (target->IsSpectator())
	{
		handler.PSendSysMessage("No es posible, el objetivo es un espectador.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (player->IsMounted())
	{
		handler.PSendSysMessage("No puedes espectar montado.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	// all's well, set bg id
	// when porting out from the bg, it will be reset to 0
	player->SetBattlegroundId(target->GetBattlegroundId(), target->GetBattlegroundTypeId());
	// remember current position as entry point for return at bg end teleportation
	if (!player->GetMap()->IsBattlegroundOrArena())
		player->SetBattlegroundEntryPoint();

	// stop flight if need
	if (player->IsInFlight())
	{
		player->GetMotionMaster()->MovementExpired();
		player->CleanupAfterTaxiFlight();
	}
	// save only in non-flight case
	else
		player->SaveRecallPosition();

	// search for two teams
	Battleground *bGround = target->GetBattleground();
	if (bGround->isRated())
	{
		uint32 slot = bGround->GetArenaType() - 2;
		if (bGround->GetArenaType() == 1 || bGround->GetArenaType() > 3)
			slot = 2;
		uint32 firstTeamID = target->GetArenaTeamId(slot);
		uint32 secondTeamID = 0;
		Player *firstTeamMember = target;
		Player *secondTeamMember = NULL;
		for (Battleground::BattlegroundPlayerMap::const_iterator itr = bGround->GetPlayers().begin(); itr != bGround->GetPlayers().end(); ++itr)
			if (Player* tmpPlayer = ObjectAccessor::FindPlayer(itr->first))
			{
				if (tmpPlayer->IsSpectator())
					continue;

				uint32 tmpID = tmpPlayer->GetArenaTeamId(slot);
				if (tmpID != firstTeamID && tmpID > 0)
				{
					secondTeamID = tmpID;
					secondTeamMember = tmpPlayer;
					break;
				}
			}

		if (firstTeamID > 0 && secondTeamID > 0 && secondTeamMember)
		{
			ArenaTeam *firstTeam = sArenaTeamMgr->GetArenaTeamById(firstTeamID);
			ArenaTeam *secondTeam = sArenaTeamMgr->GetArenaTeamById(secondTeamID);
			if (firstTeam && secondTeam)
			{
				handler.PSendSysMessage("Has entrado en una arena clasificada.");
				handler.PSendSysMessage("Equipos:");
				handler.PSendSysMessage("|cFFffffff%s|r vs |cFFffffff%s|r", firstTeam->GetName().c_str(), secondTeam->GetName().c_str());
				handler.PSendSysMessage("|cFFffffff%u(%u)|r -- |cFFffffff%u(%u)|r", firstTeam->GetRating(), firstTeam->GetAverageMMR(firstTeamMember->GetGroup()),
					secondTeam->GetRating(), secondTeam->GetAverageMMR(secondTeamMember->GetGroup()));
			}
		}
	}

	// to point to see at target with same orientation
	float x, y, z;
	target->GetContactPoint(player, x, y, z);

	player->TeleportTo(target->GetMapId(), x, y, z, player->GetAngle(target), TELE_TO_GM_MODE);
	player->SetPhaseMask(target->GetPhaseMask(), true);
	player->SetSpectate(true);
	target->GetBattleground()->AddSpectator(player->GetGUID());

	return true;
}
bool ArenaSpectator::_salir(Player* player)
{

	if (!player->IsSpectator())
	{
		ChatHandler handler(player->GetSession());
		handler.PSendSysMessage("No eres un espectador.");
		handler.SetSentErrorMessage(true);		
		return false;
	}

	player->GetBattleground()->RemoveSpectator(player->GetGUID());
	player->CancelSpectate();
	player->TeleportToBGEntryPoint();

	return true;
}
bool ArenaSpectator::_ver(Player* player, Player* target)
{
	ChatHandler handler(player->GetSession());
	if (target->HasAuraType(SPELL_AURA_MOD_STEALTH))
	{
		handler.PSendSysMessage("You can't target stealthed players.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (!target)
	{
		handler.PSendSysMessage("El jugador no existe o no esta online.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (!player->IsSpectator())
	{
		handler.PSendSysMessage("No eres un espectador.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (target->IsSpectator() && target != player)
	{
		handler.PSendSysMessage("Tu objetivo es un espectador.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (player->GetMap() != target->GetMap())
	{
		handler.PSendSysMessage("No puedes hacerlo, tu objetivo debe estar en otra arena.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	// check for arena preperation
	// if exists than battle didn`t begin
	if (target->HasAura(32728) || target->HasAura(32727))
	{
		handler.PSendSysMessage("La arena aun no ha empezado.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	(target == player && player->GetSpectateFrom()) ? player->SetViewpoint(player->GetSpectateFrom(), false) :
		player->SetViewpoint(target, true);
	return true;
}
bool ArenaSpectator::_reset(Player* player)
{
	ChatHandler handler(player->GetSession());
	if (!player)
	{
		handler.PSendSysMessage("No se ha podido encontrar al jugador.");
		handler.SetSentErrorMessage(true);
		return false;
	}

	if (!player->IsSpectator())
	{
		handler.PSendSysMessage("No eres un espectador!");
		handler.SetSentErrorMessage(true);
		return false;
	}

	Battleground *bGround = player->GetBattleground();
	if (!bGround)
		return false;

	if (bGround->GetStatus() != STATUS_IN_PROGRESS)
		return true;

	for (Battleground::BattlegroundPlayerMap::const_iterator itr = bGround->GetPlayers().begin(); itr != bGround->GetPlayers().end(); ++itr)
		if (Player* tmpPlayer = ObjectAccessor::FindPlayer(itr->first))
		{
			if (tmpPlayer->IsSpectator())
				continue;

			uint32 tmpID = bGround->GetPlayerTeam(tmpPlayer->GetGUID());

			// generate addon massage
			std::string pName = tmpPlayer->GetName();
			std::string tName = "";

			if (Player *target = tmpPlayer->GetSelectedPlayer())
				tName = target->GetName();

			SpectatorAddonMsg msg; // Travis
			msg.SetPlayer(pName);
			if (tName != "")
				msg.SetTarget(tName);
			msg.SetStatus(tmpPlayer->IsAlive());
			msg.SetClass(tmpPlayer->getClass());
			msg.SetCurrentHP(tmpPlayer->GetHealth());
			msg.SetMaxHP(tmpPlayer->GetMaxHealth());
			Powers powerType = tmpPlayer->getPowerType();
			msg.SetMaxPower(tmpPlayer->GetMaxPower(powerType));
			msg.SetCurrentPower(tmpPlayer->GetPower(powerType));
			msg.SetPowerType(powerType);
			msg.SetTeam(tmpID);
			msg.SendPacket(player->GetGUID());
		}

	return true;
}


bool ArenaSpectator::EspectarJugador(ChatHandler* handler, char const* args)
{
	Player* target;
	ObjectGuid target_guid;
	std::string target_name;
	if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
		return false;
	return _espectarJugador(handler->GetSession()->GetPlayer(), target);
	
}
bool ArenaSpectator::Ver(ChatHandler* handler, const char *args)
{
	Player* target;
	ObjectGuid target_guid;
	std::string target_name;
	if (!handler->extractPlayerTarget((char*)args, &target, &target_guid, &target_name))
		return false;

	return _ver(handler->GetSession()->GetPlayer(), target);
}
