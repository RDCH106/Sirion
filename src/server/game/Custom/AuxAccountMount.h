#ifndef AUXACCOUNTMOUNT_H
#define AUXACCOUNTMOUNT_H

#include "ScriptMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"

#include <Map.h>

struct Account_mount_data_Row
{
	uint32	m_maskRace;
	uint32	m_maskClass;
	bool	m_learn;
};

//Defino el mapa de items de cada categoria
typedef std::multimap<uint32, Account_mount_data_Row> Account_mount_data_Table;


class TC_GAME_API FiltroMonturas
{
private:
	Account_mount_data_Table m_MonturasAccount;
public:
	static FiltroMonturas* instance();
	void Load_Account_Mount_Data();
	Account_mount_data_Row GetRow(uint32 SpellID);

};
#define sFiltroMonturas FiltroMonturas::instance()
#endif