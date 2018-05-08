#include "WorldQueue.h"
#include "SpellMgr.h"
#include "World.h"

uint32 nNextCycle = 5000;



class World_LearnSpells : public WorldScript
{
public:
    World_LearnSpells() : WorldScript("World_LearnSpells") {}

    void OnUpdate(uint32 diff) override
    {
        //Lo anulo para probar con un thread en game, de modo que no tenga que esperar, y sea instantaneo
       /* return;*/
        if (nNextCycle <= diff)
        {
            int nAccionesCiclo = 5;
            bool bSeguir = true;
            for (int i = 0; i < nAccionesCiclo && bSeguir; i++)
            {
                if (sWorldQueueScript->Size() > 0)
                {
                    nNextCycle = 0;
                    AccionesWorldQueueScript accion = sWorldQueueScript->Desencolar();
                    {
                        //Añado la comprobacion de si es Type == TYPE_QUEUE_NULL
                        if (accion.nActionType == TYPE_QUEUE_NULL)
                        {
                            std::string str = "";
                            str = "DebugInfo->Accion null en LearnQueue";
                            WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
                            data << str;
                            sWorld->SendGlobalGMMessage(&data);
                            continue;
                        }
                        Player* player = ObjectAccessor::FindPlayerByLowGUID(accion.nIdCharacter);
                        if (!player)
                        {
                            std::string str = "";
                            str = "DebugInfo->Player ";
                            str += std::to_string(accion.nIdCharacter);
                            str += " no encontrado cola LearnQueue";
                            WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
                            data << str;
                            sWorld->SendGlobalGMMessage(&data);
                            continue;
                        }

                        switch (accion.nActionType)
                        {
                        case TYPE_AUTOLEARN_SPELL:
                            //Si no cumple los requisitos, saltamos al siguiente
                            if (!sWorldQueueScript->CanLearnSpell(player, accion.nIdCharacter, accion.nIdSpell))
                                continue;
                            //Si los cumple, lo enseñamos
                            player->LearnSpell(accion.nIdSpell, false);
                            break;
                        case TYPE_ACCOUNT_MOUNT:
                            player->LearnSpell(accion.nIdSpell, false);
                            break;
                        case TYPE_TELEPORT:
                            player->TeleportTo(accion.nMap, accion.lfX, accion.lfY, accion.lfZ, accion.lfO);
                            break;
                        }
                    }
                }
                else
                {
                    bSeguir = false;
                    nNextCycle = 5000;
                }
            }

        }
        else nNextCycle -= diff;
    }

};
void AddSC_wslearn()
{
    new World_LearnSpells();
}
