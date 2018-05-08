/*
GOMove By Rochet2
Original idea by Mordred

http://rochet2.github.io/
*/

#include "GOMove.h"
#include <math.h>
#include <set>
#include <sstream>
#include <string>
#include "Chat.h"
#include "RBAC.h"
#include "ServerNews.h"
#include "DatabaseEnv.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"
#include "Chat\Chat.h"



class News_commandscript : public CommandScript
{
public:
    News_commandscript() : CommandScript("News_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> newsCommandTable =
        {
            { "set",		rbac::RBAC_PERM_COMMAND_NEWS_SET, false,  &NewsSet_Command, "" },
            { "preview",    rbac::RBAC_PERM_COMMAND_NEWS_PREVIEW,       false,  &NewsPreview_Command, "" },
            { "flush",      rbac::RBAC_PERM_COMMAND_NEWS_FLUSH,       false,  &NewsFlush_Command, "" },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "news",		rbac::RBAC_PERM_COMMAND_NEWS,		   false, nullptr, "", newsCommandTable },
        };
        return commandTable;
    }

    static bool NewsSet_Command(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;
        uint32 nNew = atoi((char*)args);

        if (!nNew)
            return false;
        ServerNews::SetNew(nNew);

        std::string msg = "Noticia cambiada correctamente, recuerda activar el popup";
        handler->SendSysMessage(msg.c_str());

        return true;
    }

    static bool NewsPreview_Command(ChatHandler* handler, const char* args)
    {
        if (!*args)
            return false;
        uint32 nNew = atoi((char*)args);

        if (!nNew)
            return false;

        std::string strNew = "";
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_NEW);
        stmt->setInt32(0, nNew);
        PreparedQueryResult result = LoginDatabase.Query(stmt);
        if (result)
        {
            strNew = result->Fetch()[0].GetString();

            Player* player = handler->GetSession()->GetPlayer();
            ClearGossipMenuFor(player);
            CloseGossipMenuFor(player);
            AddGossipItemFor(player, 3, "PopUp", 0, 0,strNew,0,false);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, player->GetGUID());
        }
        else
        {
            std::string msg = "No se ha encontrado la noticia con el ID seleccionado";
            handler->SendSysMessage(msg.c_str());
            return false;
        }

    }

    static bool NewsFlush_Command(ChatHandler* handler, const char* args)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_RESET_POPUP);
        CharacterDatabase.Execute(stmt);
        std::string msg = "PopUp Activado";
        handler->SendSysMessage(msg.c_str());
        return true;
    }
};




void AddSC_News_commandscript()
{
    new News_commandscript();
}
