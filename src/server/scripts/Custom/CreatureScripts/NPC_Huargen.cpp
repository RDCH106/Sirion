#include "ScriptMgr.h"
#include "AchievementMgr.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"
#include <sstream>

class npc_quest_huargen : public CreatureScript
{
public:
	npc_quest_huargen() : CreatureScript("quest_huargen") { }

	struct npc_ratesAI : public ScriptedAI
	{
		npc_ratesAI(Creature* creature) : ScriptedAI(creature) { }

		void QuestAccept(Player* player, Quest const* quest) override
		{
			if (quest->GetQuestId() == 800001)
			{
				player->LearnSpell(6000200, false); //CAMBIO DE FASE ( CUANDO RELOGUEES ) CAMBIO A 10
				me->Say("Ellos, hay vienen.... Ugh...", LANG_UNIVERSAL, 0);
				player->SetPhaseMask(10, false);
			}

		}

		void QuestReward(Player* player, Quest const* quest,uint32 /*opt*/) override
		{
			if (quest->GetQuestId() == 800001)
			{
				player->LearnSpell(6000200, false); //CAMBIO DE FASE ( CUANDO RELOGUEES ) CAMBIO A 10
				me->Say("Ellos, hay vienen.... Ugh...", LANG_UNIVERSAL, 0);
				player->SetPhaseMask(10, false);
			}

		}

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_ratesAI(creature);
	}


};


void AddSC_quest_huargen()
{
	new npc_quest_huargen();
}