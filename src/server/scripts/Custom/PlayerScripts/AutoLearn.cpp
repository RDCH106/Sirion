#include "Player.h"
#include "WorldSession.h"
#include "WorldQueue.h"
#include "AuxAutoLearn.h"






class LearnSpellsOnLevelUp : public PlayerScript
{
public:
	LearnSpellsOnLevelUp() : PlayerScript("LearnSpellsOnLevelUp") {}

	void OnLevelChanged(Player* player, uint8 oldLevel)
	{
		AccionesWorldQueueScript tmp;
		tmp.nActionType = TYPE_AUTOLEARN_SPELL;
		for (uint32 i = 0; i < sAutoLearnItems->GetSize(); i++)
		{
			if (player->getClass() == sAutoLearnItems->GetItem(i).nClass)
			{
				if (player->getLevel() >= sAutoLearnItems->GetItem(i).nMinLevel)
				{
					if (player->IsSpellFitByClassAndRace(sAutoLearnItems->GetItem(i).nSpellId))
					{
						tmp.nIdCharacter = player->GetGUID();
						tmp.nIdSpell = sAutoLearnItems->GetItem(i).nSpellId;
						sWorldQueueScript->Encolar(tmp);
					}					
				}
			}

		}
	}
	
};



class LearnSpellsOnLevelUp_load : public WorldScript
{
public:
	LearnSpellsOnLevelUp_load() : WorldScript("LearnSpellsOnLevelUp_load") {}
	
	void OnStartup() override
	{
		sAutoLearnItems->LoadAutoLearn();
	} //Cargo la tabla de Spells	
};


void AddSC_LearnSpellsOnLevelUp()
{
	new LearnSpellsOnLevelUp();
	new LearnSpellsOnLevelUp_load();
}
