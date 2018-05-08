#include "Config.h"
#include "ScriptPCH.h"
#include "Language.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include "Player.h"


enum GuardianSpells
{
	SPELL_DEATHTOUCH = 5
};

class npcGuardianBlizzard : public CreatureScript
{
public:
	npcGuardianBlizzard() : CreatureScript("GuardiandeBlizzard") { }

	struct npc_guardianAI : public ScriptedAI
	{
		npc_guardianAI(Creature* creature) : ScriptedAI(creature) { }

		uint32 TemporizadorAviso = 0;
		uint32 TemporizadorRetardo = 1000;
		Position PosicionOriginal;

		void Reset() override
		{
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
			TemporizadorRetardo = 1000; // reinicio el temporizador de retardo en el ataque
			PosicionOriginal = me->GetHomePosition(); // Obtiene la posicion inicial del NPC
			me->UpdatePosition(PosicionOriginal); // Lleva al NPC a su posicion inicial
		}

		void EnterCombat(Unit* /*who*/) override
		{
		}

		void UpdateAI(uint32 diff)
		{
			Player* target;
			target = me->SelectNearestPlayer(40.0f); //busca a un jugador al que atacar			
			if (!target) //si no lo encuentra
			{
				target = me->SelectNearestPlayer(80.0f); //busca a un jugador al que avisar	
				
				if (target && !target->IsGameMaster())
				{
					if (TemporizadorAviso <= diff)
					{
						TemporizadorAviso = 7000; // seteo el contador a 7s para que no spamee en la pantalla
						target->GetSession()->SendNotification("TE ESTAS ACERCANDO AL GUARDIAN DE BLIZZARD...¡ HUYE !");
					}
					else TemporizadorAviso -= diff;
				}
				else Reset();

			}
			else if (target->GetSession()->GetSecurity() == 0)
			{
				TemporizadorAviso = 0; // reinicio el temporizador de retardo en el aviso
				if (TemporizadorRetardo <= diff)
				{
					TemporizadorRetardo = 0;
					if (target->IsAlive()) DoCast(target, 31984, true); //si esta vivo, ataca
					if (!target->IsAlive()) target->GetSession()->SendNotification("JA JA JA, DEBERIAS HABER HUIDO..."); //si muere en este update, muestra el mensaje
				}
				else TemporizadorRetardo -= diff;
			}
			else Reset();
		}
	};

	

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_guardianAI(creature);
	}
};

void AddSC_Guardian()
{
	new npcGuardianBlizzard();
}