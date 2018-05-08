#include "Player.h"
#include "Guild.h"
#include "World.h"
#include "WorldSession.h"
#include "DatabaseEnv.h"
#include "ScriptedGossip.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "WorldQueue.h"
#include "Item.h"
#include "Chat.h"

TC_GAME_API class Promocion
{
public:


    bool static GossipSelect(Player *player, uint32 menuId, uint32 gossipListId, ObjectGuid source);

    bool static GossipHello(Player *player, ObjectGuid source);

    bool static GossipHello(Player *player, ObjectGuid source, uint32 nIdItemPromoIndividual);

};


