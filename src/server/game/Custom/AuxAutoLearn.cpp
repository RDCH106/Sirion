#include "AuxAutoLearn.h"


AutoLearnItems* AutoLearnItems::instance()
{
	static AutoLearnItems instance;
	return &instance;
}

void AutoLearnItems::LoadAutoLearn()
{
	QueryResult result = WorldDatabase.PQuery("SELECT ID,spellID,ReqLevel FROM npc_trainer where ID >= 200001 and ID <= 200021");
	if (result)
	{
		m_AutoLearnSize = result->GetRowCount();
		delete[] m_arrAutoLearn;
		m_arrAutoLearn = new AutoLearnSpeels[m_AutoLearnSize];
		int nIteracion = 0;
		do
		{
			Field* fields = result->Fetch();
			switch (fields[0].GetInt32())
			{
			case Warrior_Low:
				m_arrAutoLearn[nIteracion].nClass = CLASS_WARRIOR;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Warrior_Highlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_WARRIOR;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Paladin_Lowlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_PALADIN;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Paladin_Highlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_PALADIN;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Paladin_Allyonly:
				m_arrAutoLearn[nIteracion].nClass = CLASS_PALADIN;
				m_arrAutoLearn[nIteracion].nFaction = ALLIANCE;
				break;
			case Paladin_Hordeonly:
				m_arrAutoLearn[nIteracion].nClass = CLASS_PALADIN;
				m_arrAutoLearn[nIteracion].nFaction = HORDE;
				break;
			case Druid_Lowlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_DRUID;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Druid_Highlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_DRUID;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Mage_Lowlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_MAGE;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Mage_Highlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_MAGE;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Warlock_Lowlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_WARLOCK;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Warlock_Highlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_WARLOCK;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Priest_Lowlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_PRIEST;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Priest_Highlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_PRIEST;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Hunter_Lowlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_HUNTER;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Hunter_Highlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_HUNTER;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Rogue_Lowlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_ROGUE;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Rogue_Highlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_ROGUE;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case Shaman_Lowlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_SHAMAN;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
				/*case Shaman_HighlevelAlly:
				m_arrAutoLearn[nIteracion].nClass = CLASS_SHAMAN;
				m_arrAutoLearn[nIteracion].nFaction = ALLIANCE;
				break;*/
			case Shaman_Highlevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_SHAMAN;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			case DeathKnights_Alllevel:
				m_arrAutoLearn[nIteracion].nClass = CLASS_DEATH_KNIGHT;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			default:
				m_arrAutoLearn[nIteracion].nClass = 0;
				m_arrAutoLearn[nIteracion].nFaction = 0;
				break;
			}

			m_arrAutoLearn[nIteracion].nSpellId = fields[1].GetInt32();
			m_arrAutoLearn[nIteracion].nMinLevel = fields[2].GetInt8();
			nIteracion++;

		} while (result->NextRow());
	}

	TC_LOG_INFO("server.loading", "%u Spells Cargados en Autolearn.", m_AutoLearnSize);
}