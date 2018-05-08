
#include "..\Auxiliares\Promo\Promo.h"

class npc_GuildPromotion : public CreatureScript
{
public:
    npc_GuildPromotion() : CreatureScript("GuildPromotion") { }

    struct npc_GuildPromotionAI : public ScriptedAI
    {
        npc_GuildPromotionAI(Creature* creature) : ScriptedAI(creature) { }

        bool GossipSelect(Player *player, uint32 menuId, uint32 gossipListId) override
        {
            return Promocion::GossipSelect(player, menuId, gossipListId, me->GetGUID());
        }

        bool GossipHello(Player *player) override
        {
            return Promocion::GossipHello(player, me->GetGUID());
        }

    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_GuildPromotionAI(creature);
    }

};

void AddSC_npc_GuildPromotion()
{
    new npc_GuildPromotion();
}
