#include "Config.h"
#include "ScriptPCH.h"
#include "Language.h"
#include "ScriptMgr.h"
#include <thread>
#include <chrono>
#include "AnticheatMgr.h"

#include "DatabaseEnv.h"
#include "ObjectMgr.h" 


class AnticheatExterno : public WorldScript
{
public:
	AnticheatExterno() : WorldScript("Anticheat") { }

	uint32 m_nCycleTimer = 30000;
	uint32 m_nCycleReport = 1000;

	std::string* m_arstrCheaters;
	uint32 m_nArrayLengh = 0;

	void Reset() 
	{

	}

	void OnUpdate(uint32 diff)
	{
		//Leo de la DB
		if (m_nCycleTimer <= diff)
		{
			m_nCycleTimer = 30000; // seteo el contador a 30 s para que no saturar la DB
			int nIteracion = 0;
			QueryResult result = CharacterDatabase.PQuery("select name from characters where online = 1 and guid in (select guid from external_program_player_reports where programa != '')");
			if (result)
			{
				m_nArrayLengh = result->GetRowCount();
				m_arstrCheaters = new std::string[m_nArrayLengh];
				do
				{
					Field* fields = result->Fetch();
					m_arstrCheaters[nIteracion] = fields[0].GetString();
					nIteracion++;


					/*std::string str = "";
					str = "|cFFFFFC00[AC]|cFF00FFFF[|cFF60FF00" + name + "|cffff0000] ESTA USANDO PROGRAMAS EXTERNOS!";
					WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
					data << str;
					sWorld->SendGlobalGMMessage(&data);*/

				} while (result->NextRow());
			}
			else
			{
				m_nArrayLengh = 0;
				m_arstrCheaters = new std::string[m_nArrayLengh];
			}
		}
		else m_nCycleTimer -= diff;

		//Registro los cheat en el anticheat
		if (m_nCycleReport <= diff)
		{
			m_nCycleReport = 1000; 

			Player* player = nullptr;
			
			for (uint32 i = 0; i < m_nArrayLengh; i++)
			{
				normalizePlayerName(m_arstrCheaters[i]);
				player = ObjectAccessor::FindPlayerByName(m_arstrCheaters[i].c_str());
				if (player)
				{
					
					 sAnticheatMgr->ExternalProgramDetection(player);
				}
			}			

		}
		else m_nCycleReport -= diff;


	}

};

void AddSC_Anticheat()
{
	new AnticheatExterno();
}