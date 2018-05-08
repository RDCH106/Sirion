#include "Config.h"
#include "World.h"
#include "DatabaseEnv.h"
#include "ScriptedGossip.h"

#define MSG_GOSSIP_TELE          "Teleportar a la Guild House"
#define MSG_GOSSIP_BUY           "Comprar Guild House"
#define MSG_GOSSIP_SELL          "Vender Guild House"
#define MSG_GOSSIP_NEXTPAGE      "Siguiente -->"
#define MSG_INCOMBAT             "Estas en combate"
#define MSG_NOGUILDHOUSE         "Tu hermandad no tiene Guild House"
#define MSG_NOFREEGH             "Lo sentimos, todas las Guild House estan en uso"
#define MSG_ALREADYHAVEGH        "Lo sentimos, ya tienes otra Guild House (%s)."
#define MSG_NOTENOUGHMONEY       "No tienes %u de oro para comprar la Guild House"
#define MSG_GHOCCUPIED           "Lo sentimos, esta Guild House ya esta en uso"
#define MSG_CONGRATULATIONS      "Felicidades! Ya tienes una Guild House"
#define MSG_SOLD                 "Has vendido tu Guild House por %u de oro"
#define MSG_NOTINGUILD           "Necesitas estar en una hermandad para usar una Guild House"
#define MSG_SELL_CONFIRM         "Estas seguro de que quieres vender?"

#define OFFSET_GH_ID_TO_ACTION 1500
#define OFFSET_SHOWBUY_FROM 10000

#define ACTION_TELE 1001
#define ACTION_SHOW_BUYLIST 1002 //deprecated. Use (OFFSET_SHOWBUY_FROM  0) instead
#define ACTION_SELL_GUILDHOUSE 1003

#define ICON_GOSSIP_BALOON 0
#define ICON_GOSSIP_WING 2
#define ICON_GOSSIP_BOOK 3
#define ICON_GOSSIP_WHEEL1 4
#define ICON_GOSSIP_WHEEL2 5
#define ICON_GOSSIP_GOLD 6
#define ICON_GOSSIP_BALOONDOTS 7
#define ICON_GOSSIP_TABARD 8
#define ICON_GOSSIP_XSWORDS 9

#define COST_GH_BUY 10000000 //1000 g.
#define COST_GH_SELL 5000000 //500 g.

#define GOSSIP_COUNT_MAX 10

class GuildHouseMaster : public CreatureScript
{
public:
	GuildHouseMaster() : CreatureScript("GuildHouseMaster") { }

	static bool isPlayerGuildLeader(Player *player)
	{
		return (player->GetRank() == 0) && (player->GetGuildId() != 0);
	}

	static bool getGuildHouseCoords(uint32 guildId, float &x, float &y, float &z, uint32 &map)
	{
		if (guildId == 0)
		{
			//if player has no guild
			return false;
		}

		QueryResult result;
		result = WorldDatabase.PQuery("SELECT `x`, `y`, `z`, `map` FROM `guildhouses` WHERE `guildId` = %u", guildId);
		if (result)
		{
			Field *fields = result->Fetch();
			x = fields[0].GetFloat();
			y = fields[1].GetFloat();
			z = fields[2].GetFloat();
			map = fields[3].GetUInt32();
			return true;
		}

		return false;
	}

	static void teleportPlayerToGuildHouse(Player *player, Creature *_creature)
	{
		if (player->GetGuildId() == 0)
		{
			//if player has no guild
			_creature->Whisper(MSG_NOTINGUILD, LANG_UNIVERSAL, player);
			return;
		}

		if (!player->getAttackers().empty())
		{
			//if player in combat
			_creature->Say(MSG_INCOMBAT, LANG_UNIVERSAL, player);
			return;
		}

		float x, y, z;
		uint32 map;

		if (getGuildHouseCoords(player->GetGuildId(), x, y, z, map))
		{
			//teleport player to the specified location
			player->TeleportTo(map, x, y, z, 0.0f);
		}
		else
			_creature->Whisper(MSG_NOGUILDHOUSE, LANG_UNIVERSAL, player);
	}

	static bool showBuyList(Player *player, Creature *_creature, uint32 showFromId = 0)
	{
		//show not occupied guildhouses

		QueryResult result;
		result = WorldDatabase.PQuery("SELECT `id`, `comment` FROM `guildhouses` WHERE `guildId` = 0 AND `id` > %u ORDER BY `id` ASC LIMIT %u",
			showFromId, GOSSIP_COUNT_MAX);

		if (result)
		{
			uint32 guildhouseId = 0;
			std::string comment = "";
			do
			{
				Field *fields = result->Fetch();
				guildhouseId = fields[0].GetInt32();
				comment = fields[1].GetString();

				//send comment as a gossip item
				//transmit guildhouseId in Action variable
				AddGossipItemFor(player, ICON_GOSSIP_TABARD, comment, GOSSIP_SENDER_MAIN, guildhouseId + OFFSET_GH_ID_TO_ACTION);
			} while (result->NextRow());

			if (result->GetRowCount() == GOSSIP_COUNT_MAX)
			{
				//assume that we have additional page
				//add link to next GOSSIP_COUNT_MAX items
				AddGossipItemFor(player, ICON_GOSSIP_BALOONDOTS, MSG_GOSSIP_NEXTPAGE, GOSSIP_SENDER_MAIN,
					guildhouseId + OFFSET_SHOWBUY_FROM);
			}

			SendGossipMenuFor(player,DEFAULT_GOSSIP_MESSAGE, _creature->GetGUID());

			return true;
		}
		else if (!result)
		{
			//all guildhouses are occupied
			_creature->Whisper(MSG_NOFREEGH, LANG_UNIVERSAL, player);
			CloseGossipMenuFor(player);
		}
		else
		{
			//this condition occurs when COUNT(guildhouses) % GOSSIP_COUNT_MAX == 0
			//just show GHs from beginning
			showBuyList(player, _creature, 0);
		}

		return false;
	}

	static bool isPlayerHasGuildhouse(Player *player, Creature *_creature, bool whisper = false)
	{
		QueryResult result;
		result = WorldDatabase.PQuery("SELECT `comment` FROM `guildhouses` WHERE `guildId` = %u",
			player->GetGuildId());

		if (result)
		{
			if (whisper)
			{
				//whisper to player "already have etc..."
				Field *fields = result->Fetch();
				char msg[100];
				sprintf(msg, MSG_ALREADYHAVEGH, fields[0].GetCString());
				_creature->Whisper(msg, LANG_UNIVERSAL, player);
			}

			return true;
		}

		return false;
	}

	static void buyGuildhouse(Player *player, Creature *_creature, uint32 guildhouseId)
	{

		bool token = sWorld->getIntConfig(CONFIG_GUILD_HOUSE_TOKEN_OR_GOLD);
		int cost = sWorld->getIntConfig(CONFIG_GUILD_HOUSE_SELL_COST);

		if (player->GetMoney() < COST_GH_BUY)
		{
			//show how much money player need to buy GH (in gold)
			char msg[100];
			sprintf(msg, MSG_NOTENOUGHMONEY, COST_GH_BUY - sWorld->getIntConfig(CONFIG_GUILD_HOUSE_SELL_COST));
			_creature->Whisper(msg, LANG_UNIVERSAL, player);
			return;
		}

		if (isPlayerHasGuildhouse(player, _creature, true))
		{
			//player already have GH
			return;
		}

		QueryResult result;

		//check if somebody already occupied this GH
		result = WorldDatabase.PQuery("SELECT `id` FROM `guildhouses` WHERE `id` = %u AND `guildId` <> 0", guildhouseId);

		if (result)
		{
			_creature->Whisper(MSG_GHOCCUPIED, LANG_UNIVERSAL, player);
			return;
		}

		//update DB
		result = WorldDatabase.PQuery("UPDATE `guildhouses` SET `guildId` = %u WHERE `id` = %u",
			player->GetGuildId(), guildhouseId);

        #pragma warning(suppress: 4146)
		player->ModifyMoney(-sWorld->getIntConfig(CONFIG_GUILD_HOUSE_BUY_COST) * 10000);

		//player->DestroyItemCount(token, cost, true);
		_creature->Say(MSG_CONGRATULATIONS, LANG_UNIVERSAL, player);

	}

	static void sellGuildhouse(Player *player, Creature *_creature)
	{
		if (isPlayerHasGuildhouse(player, _creature))
		{
			QueryResult result;
			result = WorldDatabase.PQuery("UPDATE `guildhouses` SET `guildId` = 0 WHERE `guildId` = %u",
				player->GetGuildId());

			player->ModifyMoney(sWorld->getIntConfig(CONFIG_GUILD_HOUSE_SELL_COST) * 10000);

			//display message e.g. "here your money etc."
			char msg[100];
			sprintf(msg, MSG_SOLD, sWorld->getIntConfig(CONFIG_GUILD_HOUSE_SELL_COST));
			_creature->Whisper(msg, LANG_UNIVERSAL, player);
		}
	}

	struct GuildHouseMasterAI : public ScriptedAI
	{
		GuildHouseMasterAI(Creature* creature) : ScriptedAI(creature) { }

		bool GossipSelect(Player *player, uint32 menuId, uint32 gossipListId) override
		{
			uint32 sender = player->PlayerTalkClass->GetGossipOptionSender(gossipListId);
			uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);

			player->PlayerTalkClass->ClearMenus();
			if (sender != GOSSIP_SENDER_MAIN)
				return false;

			switch (action)
			{
			case ACTION_TELE:
				//teleport player to GH
				CloseGossipMenuFor(player);
				teleportPlayerToGuildHouse(player, me);
				break;
			case ACTION_SHOW_BUYLIST:
				//show list of GHs which currently not occupied
				showBuyList(player, me);
				break;
			case ACTION_SELL_GUILDHOUSE:
				sellGuildhouse(player, me);
				CloseGossipMenuFor(player);
				break;
			default:
				if (action > OFFSET_SHOWBUY_FROM)
				{
					showBuyList(player, me, action - OFFSET_SHOWBUY_FROM);
				}
				else if (action > OFFSET_GH_ID_TO_ACTION)
				{
					//player clicked on buy list
					CloseGossipMenuFor(player);

					//get guildhouseId from action
					//guildhouseId = action - OFFSET_GH_ID_TO_ACTION
					buyGuildhouse(player, me, action - OFFSET_GH_ID_TO_ACTION);
				}
				break;
			}

			return true;
		}


		bool GossipHello(Player *player)
		{
			ClearGossipMenuFor(player);
			AddGossipItemFor(player, ICON_GOSSIP_BALOON, MSG_GOSSIP_TELE, GOSSIP_SENDER_MAIN, ACTION_TELE);

			if (isPlayerGuildLeader(player))
			{
				if (isPlayerHasGuildhouse(player, me))
				{
					//and additional for guildhouse owner (Removed :
					AddGossipItemFor(player,ICON_GOSSIP_GOLD, MSG_GOSSIP_SELL, GOSSIP_SENDER_MAIN, ACTION_SELL_GUILDHOUSE, MSG_SELL_CONFIRM, 0, false);

				}
				else
				{
					//show additional menu for guild leader
					AddGossipItemFor(player, ICON_GOSSIP_GOLD, MSG_GOSSIP_BUY, GOSSIP_SENDER_MAIN, ACTION_SHOW_BUYLIST);
				}
			}

			SendGossipMenuFor(player,DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
			return true;
		}

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new GuildHouseMasterAI(creature);
	}

};
void AddSC_guildmaster()
{
	new GuildHouseMaster();
}
