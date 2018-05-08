#include "AuxAccountMount.h"


void FiltroMonturas::Load_Account_Mount_Data()
{
	m_MonturasAccount.clear();
	QueryResult resultado = WorldDatabase.Query("Select idSpell,raceMask,classMask,learn from account_mount_data");
	if (resultado)
	{
		int count = 0;
		do
		{
			count++;
			Field* fields = resultado->Fetch();
			Account_mount_data_Row row
			{
				fields[1].GetUInt32(),
				fields[2].GetUInt32(),
				fields[3].GetBool()
			};
			m_MonturasAccount.insert(Account_mount_data_Table::value_type(fields[0].GetUInt32(), row));
		} while (resultado->NextRow());

		TC_LOG_INFO("server.loading", "Loaded %u AccountMountData Rows.", count);
	}
	else
		TC_LOG_ERROR("misc", "WARNING >> Loaded 0 AccountMountData Rows.");
}

Account_mount_data_Row FiltroMonturas::GetRow(uint32 SpellID)
{
	Account_mount_data_Row ret;
	Account_mount_data_Table::const_iterator itr = m_MonturasAccount.find(SpellID);
	if (itr != m_MonturasAccount.end())
	{
		ret.m_maskRace = itr->second.m_maskRace;
		ret.m_maskClass = itr->second.m_maskClass;
		ret.m_learn = itr->second.m_learn;
	}
	else
	{
		ret.m_learn = 0;
		ret.m_maskClass = -1;
		ret.m_maskRace = -1;		
	}
	return ret;	
	
}

FiltroMonturas* FiltroMonturas::instance()
{
	static FiltroMonturas instance;
	return &instance;
}