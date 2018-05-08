/*
Listo para puesta en produccion
*/
#include "Chat.h"
#include "sc_npc_loyalty.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptPCH.h"
#include "WorldSession.h"
#include "World.h"
#include "StringFormat.h"
#include "LinQ.hpp"

using namespace nsNpcLoyalty;
using namespace LinQ;


TC_GAME_API class Loyalty_Gossip
{
public:

	static bool GossipSelect(Player *player, WorldObject* source, uint32 menuId, uint32 gossipListId);
	
	static bool GossipHello(Player* player, WorldObject* source);

	static bool ShowCategory(Player * const player, WorldObject* source);

};