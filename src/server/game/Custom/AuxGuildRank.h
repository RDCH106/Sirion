
#include "Common.h"
#include "DatabaseEnv.h"

std::string GetGuildTop(uint32 limit)
{
	uint32 cuenta = 1;
	std::string msg = "Top" + std::to_string(limit) + " Hermandades";
	PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_TOP);
	stmt->setInt32(0, limit);
	PreparedQueryResult result = CharacterDatabase.Query(stmt);
	if (result)
	{		
		do
		{
			Field* fields = result->Fetch();
			msg += "\n" + std::to_string(cuenta)+"->" + fields[1].GetString();
			cuenta++;
		} while (result->NextRow());
	}
	else
		msg += "\nNo hay Hermandades que cumplan los requisitos";

	return msg;
}

