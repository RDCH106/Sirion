/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ServerNews.h"
#include "DatabaseEnv.h"
std::string strNew = "";


void ServerNews::SetNew(int nNew)
{
	PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_NEW);
	stmt->setInt32(0, nNew);
	PreparedQueryResult result = LoginDatabase.Query(stmt);
	if (result)
	{
		strNew = result->Fetch()[0].GetString();

		SQLTransaction trans = LoginDatabase.BeginTransaction();

		stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_RESET_NEWS);
		trans->Append(stmt);
		stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_SET_NEW);
		stmt->setInt32(0, nNew);
		trans->Append(stmt);
		LoginDatabase.CommitTransaction(trans);
	}
}

std::string ServerNews::GetNew()
{
	return strNew;
}


void ServerNews::LoadSettedNew()
{
	PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACTIVE_NEW);
	PreparedQueryResult result = LoginDatabase.Query(stmt);
	if (result)
	{
		strNew = result->Fetch()[0].GetString();
	}
}


