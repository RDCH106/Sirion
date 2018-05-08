//Falta probar
#include "ScriptPCH.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include "SpellMgr.h"
#include "DBCStores.h"

class MountSpeedUpdate : public PlayerScript
{
public:
    MountSpeedUpdate() : PlayerScript("MountSpeedUpdate") { }

    void UpdateSpeed(Player* player)
    {
        //Revisar porque deja volando a la gente

        if (player->IsInTaxi())
            return;


        uint32 nMontura = player->GetMountSpellID();
        uint32 nDisplay = player->GetMountID();
        if (!nMontura || nMontura == 0)
            return;

        if (player->IsMounted())
        {

            bool setFly = false;
            double lfSpeedFly = 1;
            double lfSpeedRun = 1;
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(nMontura);

            //// Prevent stacking of mounts and client crashes upon dismounting
            //player->RemoveAurasByType(SPELL_AURA_MOUNTED, ObjectGuid::Empty, GetHitAura());

            // Triggered spell id dependent on riding skill and zone
            uint32 zoneid, areaid;

            player->GetZoneAndAreaId(zoneid, areaid);
            bool canFly = spellInfo && (spellInfo->CheckLocation(player->GetMapId(), zoneid, areaid, player) == SPELL_CAST_OK);


            uint32 mount = 0;
            AreaTableEntry const* pArea = sAreaTableStore.LookupEntry(player->GetAreaId());

            if (!pArea || !spellInfo || !pArea->IsFlyable() || !player->CanFlyInZone(player->GetMapId(), zoneid, spellInfo))
                canFly = false;

            ////Como dalaran no coge bien los flags de primeras, añado los ids
            //if(zoneid == 4395 && areaid != 4564)
            //	canFly = false;

            if (spellInfo->Effects[0].TriggerSpell == 6000312 || spellInfo->Effects[1].TriggerSpell == 6000312 || spellInfo->Effects[2].TriggerSpell == 6000312) //Voladora
            {

                switch (player->GetBaseSkillValue(SKILL_RIDING))
                {
                case 0:
                    mount = 0;
                    break;
                case 75:
                    mount = 6000314;
                    lfSpeedRun += 0.6;
                    break;
                case 150:
                    mount = 6000315;
                    lfSpeedRun += 1;
                    break;
                case 225:
                    if (canFly)
                    {
                        mount = 6000318;
                        lfSpeedFly += 1.5;
                        lfSpeedRun += 1;
                        setFly = true;
                    }
                    else
                    {
                        mount = 6000315;
                        lfSpeedRun += 1;
                    }
                    break;
                case 300:
                    if (canFly)
                    {
                        if (player->HasSpell(90265))
                        {
                            mount = 6000320;
                            lfSpeedFly += 3.1;
                            lfSpeedRun += 1;
                        }
                        else
                        {
                            mount = 6000319;
                            lfSpeedFly += 2.8;
                            lfSpeedRun += 1;
                        }
                        setFly = true;

                    }
                    else
                    {
                        mount = 6000315;
                        lfSpeedRun += 1;
                    }
                    break;
                default:
                    break;
                }


            }

            else if (spellInfo->Effects[0].TriggerSpell == 6000313 || spellInfo->Effects[1].TriggerSpell == 6000313 || spellInfo->Effects[2].TriggerSpell == 6000313) //Terrestre
            {
                switch (player->GetBaseSkillValue(SKILL_RIDING))
                {
                case 0:
                    mount = 0;
                    break;
                case 75:
                    mount = 6000314;
                    lfSpeedRun += 0.6;
                    break;
                default:
                    mount = 6000315;
                    lfSpeedRun += 1;
                    break;
                }

            }

            if (player->IsGameMaster())			player->GetSession()->SendAreaTriggerMessage(pArea && pArea->IsFlyable() ? "Has cambiado de Area(volar)" : "Has cambiado de Area(tierra)");


            player->AddAura(mount, player);
            player->Mount(nDisplay);
            //player->AddAura(nMontura,player);
            player->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, nDisplay);

            if (!setFly)
            {
                //WorldPacket data(12);
                //data.SetOpcode(SMSG_MOVE_UNSET_CAN_FLY);
                //data << player->GetPackGUID();
                //data << uint32(0);                                      // unknown
                //player->SendMessageToSet(&data, true);				
            }
            else
            {
                WorldPacket data(12);
                data.SetOpcode(SMSG_MOVE_SET_CAN_FLY);
                data << player->GetPackGUID();
                data << uint32(0);                                      // unknown
                player->SendMessageToSet(&data, true);
            }

        }
    }


    void OnUpdateArea(Player* player, uint32 /*newArea*/) override
    {
        UpdateSpeed(player);
    }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        //UpdateSpeed(player);
    }
};

void AddSC_MountSpeedUpdate()
{
    new MountSpeedUpdate();
}
