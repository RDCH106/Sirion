#include "Config.h"
#include "ScriptPCH.h"
#include "Language.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "WorldSession.h"

#pragma region Horda
class npcTeleportOrgrimmar : public CreatureScript
{
public:
	npcTeleportOrgrimmar() : CreatureScript("PortalOrgrimmar"){}

	struct npcTeleportOrgrimmarAI : public ScriptedAI
	{
		npcTeleportOrgrimmarAI(Creature* creature) : ScriptedAI(creature){}

		void UpdateAI(uint32 diff)
		{
			uint32 mapa = 1;
			float x = 1469.420532f;
			float y = -4221.385742f;
			float z = 60.522894f;
			float o = 5.980000f;

			if (Player* player = me->SelectNearestPlayer(0.7f))
			{
				if (!player || player->IsBeingTeleported() || !player->IsAlive()) return;

				if (player->IsInCombat())
				{
					player->GetSession()->SendNotification("ESTAS EN COMBATE");
					return;
				}

				player->TeleportTo(mapa, x, y, z, o);
			}

		}
	};

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npcTeleportOrgrimmarAI(creature);
	};

};

class npcTeleportLunaArgenta : public CreatureScript
{
public:
	npcTeleportLunaArgenta() : CreatureScript("PortalLunaArgenta"){}

	struct npcTeleportLunaArgentaAI : public ScriptedAI
	{
		npcTeleportLunaArgentaAI(Creature* creature) : ScriptedAI(creature){}

		void UpdateAI(uint32 diff)
		{
			uint32 mapa = 530;
			float x = 9998.490234f;
			float y = -7106.779785f;
			float z = 48.293503f;
			float o = 2.463562f;

			if (Player* player = me->SelectNearestPlayer(0.7f))
			{
				if (!player || player->IsBeingTeleported() || !player->IsAlive()) return;

				if (player->IsInCombat())
				{
					player->GetSession()->SendNotification("ESTAS EN COMBATE");
					return;
				}

				player->TeleportTo(mapa, x, y, z, o);
			}

		}
	};

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npcTeleportLunaArgentaAI(creature);
	};

};

class npcTeleportCimaTrueno : public CreatureScript
{
public:
	npcTeleportCimaTrueno() : CreatureScript("PortalCimaTrueno"){}

	struct npcTeleportCimaTruenoAI : public ScriptedAI
	{
		npcTeleportCimaTruenoAI(Creature* creature) : ScriptedAI(creature){}

		void UpdateAI(uint32 diff)
		{
			uint32 mapa = 1;
			float x = -964.979980f;
			float y = 283.433014f;
			float z = 112.230003f;
			float o = 3.020000f;

			if (Player* player = me->SelectNearestPlayer(0.7f))
			{
				if (!player || player->IsBeingTeleported() || !player->IsAlive()) return;

				if (player->IsInCombat())
				{
					player->GetSession()->SendNotification("ESTAS EN COMBATE");
					return;
				}

				player->TeleportTo(mapa, x, y, z, o);
			}

		}
	};

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npcTeleportCimaTruenoAI(creature);
	};

};

class npcTeleportEntrañas : public CreatureScript
{
public:
	npcTeleportEntrañas() : CreatureScript("PortalEntranas"){}

	struct npcTeleportEntrañasAI : public ScriptedAI
	{
		npcTeleportEntrañasAI(Creature* creature) : ScriptedAI(creature){}

		void UpdateAI(uint32 diff)
		{
			uint32 mapa = 0;
			float x = 1773.469971f;
			float y = 61.120998f;
			float z = -45.271339f;
			float o = 2.315001f;

			if (Player* player = me->SelectNearestPlayer(0.7f))
			{
				if (!player || player->IsBeingTeleported() || !player->IsAlive()) return;

				if (player->IsInCombat())
				{
					player->GetSession()->SendNotification("ESTAS EN COMBATE");
					return;
				}

				player->TeleportTo(mapa, x, y, z, o);
			}

		}
	};

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npcTeleportEntrañasAI(creature);
	};

};

#pragma endregion

#pragma region Alianza
class npcTeleportExodar : public CreatureScript
{
public:
	npcTeleportExodar() : CreatureScript("PortalExodar"){}

	struct npcTeleportExodarAI : public ScriptedAI
	{
		npcTeleportExodarAI(Creature* creature) : ScriptedAI(creature){}

		void UpdateAI(uint32 diff)
		{
			uint32 mapa = 530;
			float x = -4029.929932f;
			float y = -11572.200195f;
			float z = -136.665090f;
			float o = 2.280828f;
			
			
			if (Player* player = me->SelectNearestPlayer(0.7f))
			{
				if (!player || player->IsBeingTeleported() || !player->IsAlive()) return;
				if (player->IsInCombat())
				{
					player->GetSession()->SendNotification("ESTAS EN COMBATE");
					return;
				}

				player->TeleportTo(mapa, x, y, z, o);
			}

		}
	};

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npcTeleportExodarAI(creature);
	};

};

class npcTeleportDarnassus : public CreatureScript
{
public:
	npcTeleportDarnassus() : CreatureScript("PortalDarnassus"){}

	struct npcTeleportDarnassusAI : public ScriptedAI
	{
		npcTeleportDarnassusAI(Creature* creature) : ScriptedAI(creature){}

		void UpdateAI(uint32 diff)
		{
			uint32 mapa = 1;
			float x = 9660.809570f;
			float y = 2513.639893f;
			float z = 1332.350464f;
			float o = 2.953186f;

			if (Player* player = me->SelectNearestPlayer(0.7f))
			{
				if (!player || player->IsBeingTeleported() || !player->IsAlive()) return;

				if (player->IsInCombat())
				{
					player->GetSession()->SendNotification("ESTAS EN COMBATE");
					return;
				}

				player->TeleportTo(mapa, x, y, z, o);
			}

		}
	};

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npcTeleportDarnassusAI(creature);
	};

};

class npcTeleportForjaz : public CreatureScript
{
public:
	npcTeleportForjaz() : CreatureScript("PortalForjaz"){}

	struct npcTeleportForjazAI : public ScriptedAI
	{
		npcTeleportForjazAI(Creature* creature) : ScriptedAI(creature){}

		void UpdateAI(uint32 diff)
		{
			uint32 mapa = 0;
			float x = -4613.620117f;
			float y = -915.380005f;
			float z = 502.224304f;
			float o = 3.880000f;

			if (Player* player = me->SelectNearestPlayer(0.7f))
			{
				if (!player || player->IsBeingTeleported() || !player->IsAlive()) return;

				if (player->IsInCombat())
				{
					player->GetSession()->SendNotification("ESTAS EN COMBATE");
					return;
				}

				player->TeleportTo(mapa, x, y, z, o);
			}

		}
	};

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npcTeleportForjazAI(creature);
	};

};

class npcTeleportVentormenta : public CreatureScript
{
public:
	npcTeleportVentormenta() : CreatureScript("PortalVentormenta"){}

	struct npcTeleportVentormentaAI : public ScriptedAI
	{
		npcTeleportVentormentaAI(Creature* creature) : ScriptedAI(creature){}

		void UpdateAI(uint32 diff)
		{
			uint32 mapa = 0;
			float x = -9003.459961f;
			float y = 870.031006f;
			float z = 30.313601f;
			float o = 2.280000f;

			if (Player* player = me->SelectNearestPlayer(0.7f))
			{
				if (!player || player->IsBeingTeleported() || !player->IsAlive()) return;

				if (player->IsInCombat())
				{
					player->GetSession()->SendNotification("ESTAS EN COMBATE");
					return;
				}

				player->TeleportTo(mapa, x, y, z, o);
			}

		}
	};

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npcTeleportVentormentaAI(creature);
	};

};

#pragma endregion

#pragma region Neutrales
class npcTeleportShattrath : public CreatureScript
{
public:
	npcTeleportShattrath() : CreatureScript("PortalShattrath"){}

	struct npcTeleportShattrathAI : public ScriptedAI
	{
		npcTeleportShattrathAI(Creature* creature) : ScriptedAI(creature){}

		void UpdateAI(uint32 diff)
		{
			uint32 mapa = 530;
			float x = -1904.760010f;
			float y = 5443.430176f;
			float z = -11.601200f;
			float o = 5.960000f;

			if (Player* player = me->SelectNearestPlayer(0.7f))
			{
				if (!player || player->IsBeingTeleported() || !player->IsAlive()) return;

				if (player->IsInCombat())
				{
					player->GetSession()->SendNotification("ESTAS EN COMBATE");
					return;
				}

				player->TeleportTo(mapa, x, y, z, o);
			}

		}
	};

	CreatureAI* GetAI(Creature* creature) const
	{
		return new npcTeleportShattrathAI(creature);
	};

};

#pragma endregion

void AddSC_Portal()
{
	new npcTeleportOrgrimmar();	
	new npcTeleportLunaArgenta();
	new npcTeleportCimaTrueno();
	new npcTeleportEntrañas();
	new npcTeleportExodar();
	new npcTeleportDarnassus();
	new npcTeleportForjaz();
	new npcTeleportVentormenta();
	new npcTeleportShattrath();
}