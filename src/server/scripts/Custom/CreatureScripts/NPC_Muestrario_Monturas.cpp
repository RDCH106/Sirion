#include "AuxMuestrarioMonturas.h"
#include "Config.h"
#include "ScriptPCH.h"
#include "Language.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include "Player.h"
#include "ObjectMgr.h"
#include <vector>


uint32 posicion = 0;

class npcMuestrarioMonturas : public CreatureScript
{
public:
	npcMuestrarioMonturas() : CreatureScript("MuestrarioMonturas") { }

	struct npcMuestrarioMonturas_AI : public ScriptedAI
	{
		npcMuestrarioMonturas_AI(Creature* creature) : ScriptedAI(creature) { }

		uint32 TemporizadorInvocacion = 1000;


		void UpdateAI(uint32 diff) override
		{
			if (TemporizadorInvocacion <= diff)
			{
				TemporizadorInvocacion = 1000; // seteo el contador a 7s para que no spamee en la pantalla

				uint32 idmontura = sMuestrarioMonturas->GetMontura();
				if (idmontura > 0)
				{					
					if (sObjectMgr->GetCreatureTemplate(idmontura))
					{
						Position pos = me->GetPosition();
						pos.m_positionX -= 4;
						me->SummonCreature(idmontura, pos, TEMPSUMMON_TIMED_DESPAWN, 1200);						
					}
				}
			}
			else TemporizadorInvocacion -= diff;
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npcMuestrarioMonturas_AI(creature);
	}
};


class npcMuestrarioMonturas_load : public WorldScript
{
public:
	npcMuestrarioMonturas_load() : WorldScript("MuestrarioMonturas_load") {}

	uint32 TemporizadorAvance = 1100;


	void OnUpdate(uint32 diff) override
	{
		if (TemporizadorAvance <= diff)
		{
			TemporizadorAvance = 1100;
			sMuestrarioMonturas->AddPosition();
		}
		else
			TemporizadorAvance -= diff;
	}
};

void AddSC_MuestrarioMonturas()
{
	new npcMuestrarioMonturas_load();
	new npcMuestrarioMonturas();
}