//Falta probar
#include "ScriptPCH.h"
#include "ScriptMgr.h"
#include "WorldQueue.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include "DBCStores.h"
#include "AuxAccountMount.h"

class AccountMounts : public PlayerScript
{
public:
	AccountMounts() : PlayerScript("AccountMounts") { }

	void OnLogin(Player* player, bool /*firstLogin*/) override
	{
		//Si no equitacion nada
		if (!player->HasSkill(762) || player->GetSkillValue(762) < 70) return;

		if (player->getRace() == 16) { player->CastSpell(player, 68996, true, NULL, NULL, player->GetGUID()); }
		AccionesWorldQueueScript tmp;
		tmp.nActionType = TYPE_ACCOUNT_MOUNT;

		std::vector<uint32> Spells;

		QueryResult result2 = CharacterDatabase.PQuery("SELECT spell FROM character_spell WHERE guid in (select guid from characters where account = %u) and guid <> %u", player->GetSession()->GetAccountId(), player->GetGUID());
		if (!result2)
			return;

		do
		{
			Spells.push_back(result2->Fetch()[0].GetUInt32());
		} while (result2->NextRow());

		for (auto& i : Spells)
		{	
			SpellEntry const* sSpell = sSpellStore.LookupEntry(i);
			if (!sSpell)
				continue;
			if (sSpell->Effect[0] == SPELL_EFFECT_APPLY_AURA && sSpell->EffectApplyAuraName[0] == SPELL_AURA_MOUNTED) //Indica que es montura
			{
				Account_mount_data_Row Mount_Row = sFiltroMonturas->GetRow(i);
				if (Mount_Row.m_maskClass == -1 || Mount_Row.m_maskRace == -1 || !Mount_Row.m_learn) continue;
				if (player->HasSpell(i))continue;

				//Compruebo la raza
				if (Mount_Row.m_maskRace > 0)
				{
					if ((Mount_Row.m_maskRace & player->getRaceMask()) == 0)
						continue;
				}

				//Compruebo la clase
				if (Mount_Row.m_maskClass > 0)
				{
					if ((Mount_Row.m_maskClass & player->getClassMask()) == 0)
						continue;
				}

				if (player->IsSpellFitByClassAndRace(sSpell->Id))
				{
					tmp.nIdSpell = sSpell->Id;
					tmp.nIdCharacter = player->GetGUID();
					sWorldQueueScript->Encolar(tmp);
				}
			}
		}
	}
};

class WS_Account_mount_data_Table_Load : public WorldScript
{
public:
	WS_Account_mount_data_Table_Load() : WorldScript("World_Load_Account_mount_data_Table") {}

	void OnStartup() override
	{
		sFiltroMonturas->Load_Account_Mount_Data();
	}
};

void AddSC_accontmounts()
{
	new AccountMounts;
	new WS_Account_mount_data_Table_Load();
}
