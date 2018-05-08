/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

 /** \file
     \ingroup world
 */

#include "World.h"
#include "AccountMgr.h"
#include "AchievementMgr.h"
#include "AddonMgr.h"
#include "ArenaTeamMgr.h"
#include "AuctionHouseBot.h"
#include "AuctionHouseMgr.h"
#include "AuxGuildRank.h"
#include "BattlefieldMgr.h"
#include "BattlegroundMgr.h"
#include "AnticheatMgr.h"
#include "CalendarMgr.h"
#include "Channel.h"
#include "CharacterCache.h"
#include "CharacterDatabaseCleaner.h"
#include "Chat.h"
#include "Config.h"
#include "CreatureAIRegistry.h"
#include "CreatureGroups.h"
#include "CreatureTextMgr.h"
#include "DatabaseEnv.h"
#include "DisableMgr.h"
#include "GameEventMgr.h"
#include "GameObjectModel.h"
#include "GameTime.h"
#include "GitRevision.h"
#include "GridNotifiersImpl.h"
#include "GroupMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "InstanceSaveMgr.h"
#include "IRCClient.h"
#include "Language.h"
#include "LFGMgr.h"
#include "Log.h"
#include "LootItemStorage.h"
#include "LootMgr.h"
#include "M2Stores.h"
#include "MapManager.h"
#include "ServerNews.h"
#include "Memory.h"
#include "Metric.h"
#include "MMapFactory.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "OutdoorPvPMgr.h"
#include "PetitionMgr.h"
#include "Player.h"
#include "PlayerDump.h"
#include "PoolMgr.h"
#include "QueryCallback.h"
#include "Realm.h"
#include "ScriptMgr.h"
#include "ScriptReloadMgr.h"
#include "ServerMotd.h"
#include "SkillDiscovery.h"
#include "SkillExtraItems.h"
#include "SmartScriptMgr.h"
#include "SpellMgr.h"
#include "TicketMgr.h"
#include "TransportMgr.h"
#include "Unit.h"
#include "UpdateTime.h"
#include "VMapFactory.h"
#include "VMapManager2.h"
#include "WardenCheckMgr.h"
#include "WaypointManager.h"
#include "WeatherMgr.h"
#include "WhoListStorage.h"
#include "WorldSession.h"

#include <boost/asio/ip/address.hpp>

TC_GAME_API std::atomic<bool> World::m_stopEvent(false);
TC_GAME_API uint8 World::m_ExitCode = SHUTDOWN_EXIT_CODE;

TC_GAME_API std::atomic<uint32> World::m_worldLoopCounter(0);

TC_GAME_API float World::m_MaxVisibleDistanceOnContinents = DEFAULT_VISIBILITY_DISTANCE;
TC_GAME_API float World::m_MaxVisibleDistanceInInstances = DEFAULT_VISIBILITY_INSTANCE;
TC_GAME_API float World::m_MaxVisibleDistanceInBGArenas = DEFAULT_VISIBILITY_BGARENAS;

TC_GAME_API int32 World::m_visibility_notify_periodOnContinents = DEFAULT_VISIBILITY_NOTIFY_PERIOD;
TC_GAME_API int32 World::m_visibility_notify_periodInInstances = DEFAULT_VISIBILITY_NOTIFY_PERIOD;
TC_GAME_API int32 World::m_visibility_notify_periodInBGArenas = DEFAULT_VISIBILITY_NOTIFY_PERIOD;

/// World constructor
World::World()
{
    m_playerLimit = 0;
    m_allowedSecurityLevel = SEC_PLAYER;
    m_allowMovement = true;
    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;

    m_maxActiveSessionCount = 0;
    m_maxQueuedSessionCount = 0;
    m_PlayerCount = 0;
    m_MaxPlayerCount = 0;
    m_NextDailyQuestReset = 0;
    m_NextWeeklyQuestReset = 0;
    m_NextMonthlyQuestReset = 0;
    m_NextRandomBGReset = 0;
    m_NextGuildReset = 0;

    m_defaultDbcLocale = LOCALE_enUS;
    m_availableDbcLocaleMask = 0;

    mail_timer = 0;
    mail_timer_expires = 0;

    m_isClosed = false;

    m_CleaningFlags = 0;

    memset(rate_values, 0, sizeof(rate_values));
    memset(m_int_configs, 0, sizeof(m_int_configs));
    memset(m_bool_configs, 0, sizeof(m_bool_configs));
    memset(m_float_configs, 0, sizeof(m_float_configs));

    _guidWarn = false;
    _guidAlert = false;
    _warnDiff = 0;
    _warnShutdownTime = time(nullptr);
}

/// World destructor
World::~World()
{
    ///- Empty the kicked session set
    while (!m_sessions.empty())
    {
        // not remove from queue, prevent loading new sessions
        delete m_sessions.begin()->second;
        m_sessions.erase(m_sessions.begin());
    }

    CliCommandHolder* command = nullptr;
    while (cliCmdQueue.next(command))
        delete command;

    VMAP::VMapFactory::clear();
    MMAP::MMapFactory::clear();

    /// @todo free addSessQueue
}

World* World::instance()
{
    static World instance;
    return &instance;
}

/// Find a player in a specified zone
Player* World::FindPlayerInZone(uint32 zone)
{
    ///- circle through active sessions and return the first player found in the zone
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (!itr->second)
            continue;

        Player* player = itr->second->GetPlayer();
        if (!player)
            continue;

        if (player->IsInWorld() && player->GetZoneId() == zone)
            return player;
    }
    return nullptr;
}

bool World::IsClosed() const
{
    return m_isClosed;
}

void World::SetClosed(bool val)
{
    m_isClosed = val;

    // Invert the value, for simplicity for scripters.
    sScriptMgr->OnOpenStateChange(!val);
}

void World::TriggerGuidWarning()
{
    // Lock this only to prevent multiple maps triggering at the same time
    std::lock_guard<std::mutex> lock(_guidAlertLock);

    time_t gameTime = GameTime::GetGameTime();
    time_t today = (gameTime / DAY) * DAY;

    // Check if our window to restart today has passed. 5 mins until quiet time
    while (gameTime >= (today + (getIntConfig(CONFIG_RESPAWN_RESTARTQUIETTIME) * HOUR) - 1810))
        today += DAY;

    // Schedule restart for 30 minutes before quiet time, or as long as we have
    _warnShutdownTime = today + (getIntConfig(CONFIG_RESPAWN_RESTARTQUIETTIME) * HOUR) - 1800;

    _guidWarn = true;
    SendGuidWarning();
}

void World::TriggerGuidAlert()
{
    // Lock this only to prevent multiple maps triggering at the same time
    std::lock_guard<std::mutex> lock(_guidAlertLock);

    DoGuidAlertRestart();
    _guidAlert = true;
    _guidWarn = false;
}

void World::DoGuidWarningRestart()
{
    if (m_ShutdownTimer)
        return;

    ShutdownServ(1800, SHUTDOWN_MASK_RESTART, RESTART_EXIT_CODE);
    _warnShutdownTime += HOUR;
}

void World::DoGuidAlertRestart()
{
    if (m_ShutdownTimer)
        return;

    ShutdownServ(300, SHUTDOWN_MASK_RESTART, RESTART_EXIT_CODE, _alertRestartReason);
}

void World::SendGuidWarning()
{
    if (!m_ShutdownTimer && _guidWarn && getIntConfig(CONFIG_RESPAWN_GUIDWARNING_FREQUENCY) > 0)
        SendServerMessage(SERVER_MSG_STRING, _guidWarningMsg.c_str());
    _warnDiff = 0;
}

/// Find a session by its id
WorldSession* World::FindSession(uint32 id) const
{
    SessionMap::const_iterator itr = m_sessions.find(id);

    if (itr != m_sessions.end())
        return itr->second;                                 // also can return nullptr for kicked session
    else
        return nullptr;
}

/// Remove a given session
bool World::RemoveSession(uint32 id)
{
    ///- Find the session, kick the user, but we can't delete session at this moment to prevent iterator invalidation
    SessionMap::const_iterator itr = m_sessions.find(id);

    if (itr != m_sessions.end() && itr->second)
    {
        if (itr->second->PlayerLoading())
            return false;

        itr->second->KickPlayer();
    }

    return true;
}

void World::AddSession(WorldSession* s)
{
    addSessQueue.add(s);
}

void World::AddSession_(WorldSession* s)
{
    ASSERT(s);

    //NOTE - Still there is race condition in WorldSession* being used in the Sockets

    ///- kick already loaded player with same account (if any) and remove session
    ///- if player is in loading and want to load again, return
    if (!RemoveSession(s->GetAccountId()))
    {
        s->KickPlayer();
        delete s;                                           // session not added yet in session list, so not listed in queue
        return;
    }

    // decrease session counts only at not reconnection case
    bool decrease_session = true;

    // if session already exist, prepare to it deleting at next world update
    // NOTE - KickPlayer() should be called on "old" in RemoveSession()
    {
        SessionMap::const_iterator old = m_sessions.find(s->GetAccountId());

        if (old != m_sessions.end())
        {
            // prevent decrease sessions count if session queued
            if (RemoveQueuedPlayer(old->second))
                decrease_session = false;
            // not remove replaced session form queue if listed
            delete old->second;
        }
    }

    m_sessions[s->GetAccountId()] = s;

    uint32 Sessions = GetActiveAndQueuedSessionCount();
    uint32 pLimit = GetPlayerAmountLimit();
    uint32 QueueSize = GetQueuedSessionCount(); //number of players in the queue

    //so we don't count the user trying to
    //login as a session and queue the socket that we are using
    if (decrease_session)
        --Sessions;

    if (pLimit > 0 && Sessions >= pLimit && !s->HasPermission(rbac::RBAC_PERM_SKIP_QUEUE) && !HasRecentlyDisconnected(s))
    {
        AddQueuedPlayer(s);
        UpdateMaxSessionCounters();
        TC_LOG_INFO("misc", "PlayerQueue: Account id %u is in Queue Position (%u).", s->GetAccountId(), ++QueueSize);
        return;
    }

    s->InitializeSession();

    UpdateMaxSessionCounters();

    // Updates the population
    if (pLimit > 0)
    {
        float popu = (float)GetActiveSessionCount();              // updated number of users on the server
        popu /= pLimit;
        popu *= 2;
        TC_LOG_INFO("misc", "Server Population (%f).", popu);
    }
}

bool World::HasRecentlyDisconnected(WorldSession* session)
{
    if (!session)
        return false;

    if (uint32 tolerance = getIntConfig(CONFIG_INTERVAL_DISCONNECT_TOLERANCE))
    {
        for (DisconnectMap::iterator i = m_disconnects.begin(); i != m_disconnects.end();)
        {
            if (difftime(i->second, time(nullptr)) < tolerance)
            {
                if (i->first == session->GetAccountId())
                    return true;
                ++i;
            }
            else
                m_disconnects.erase(i++);
        }
    }
    return false;
}

int32 World::GetQueuePos(WorldSession* sess)
{
    uint32 position = 1;

    for (Queue::const_iterator iter = m_QueuedPlayer.begin(); iter != m_QueuedPlayer.end(); ++iter, ++position)
        if ((*iter) == sess)
            return position;

    return 0;
}

void World::AddQueuedPlayer(WorldSession* sess)
{
    sess->SetInQueue(true);
    m_QueuedPlayer.push_back(sess);

    // The 1st SMSG_AUTH_RESPONSE needs to contain other info too.
    sess->SendAuthResponse(AUTH_WAIT_QUEUE, false, GetQueuePos(sess));
}

bool World::RemoveQueuedPlayer(WorldSession* sess)
{
    // sessions count including queued to remove (if removed_session set)
    uint32 sessions = GetActiveSessionCount();

    uint32 position = 1;
    Queue::iterator iter = m_QueuedPlayer.begin();

    // search to remove and count skipped positions
    bool found = false;

    for (; iter != m_QueuedPlayer.end(); ++iter, ++position)
    {
        if (*iter == sess)
        {
            sess->SetInQueue(false);
            sess->ResetTimeOutTime(false);
            iter = m_QueuedPlayer.erase(iter);
            found = true;                                   // removing queued session
            break;
        }
    }

    // iter point to next socked after removed or end()
    // position store position of removed socket and then new position next socket after removed

    // if session not queued then we need decrease sessions count
    if (!found && sessions)
        --sessions;

    // accept first in queue
    if ((!m_playerLimit || sessions < m_playerLimit) && !m_QueuedPlayer.empty())
    {
        WorldSession* pop_sess = m_QueuedPlayer.front();
        pop_sess->InitializeSession();
        m_QueuedPlayer.pop_front();

        // update iter to point first queued socket or end() if queue is empty now
        iter = m_QueuedPlayer.begin();
        position = 1;
    }

    // update position from iter to end()
    // iter point to first not updated socket, position store new position
    for (; iter != m_QueuedPlayer.end(); ++iter, ++position)
        (*iter)->SendAuthWaitQue(position);

    return found;
}

/// Initialize config values
void World::LoadConfigSettings(bool reload)
{
    if (reload)
    {
        std::string configError;
        if (!sConfigMgr->Reload(configError))
        {
            TC_LOG_ERROR("misc", "World settings reload fail: %s.", configError.c_str());
            return;
        }
        sLog->LoadFromConfig();
        sMetric->LoadFromConfigs();
    }

    // load update time related configs
    sWorldUpdateTime.LoadFromConfig();

    ///- Read the player limit and the Message of the day from the config file
    SetPlayerAmountLimit(sConfigMgr->GetIntDefault("PlayerLimit", 100));
    Motd::SetMotd(sConfigMgr->GetStringDefault("Motd", "Welcome to a Trinity Core Server."));

    ///- Read ticket system setting from the config file
    m_bool_configs[CONFIG_ALLOW_TICKETS] = sConfigMgr->GetBoolDefault("AllowTickets", true);
    m_bool_configs[CONFIG_DELETE_CHARACTER_TICKET_TRACE] = sConfigMgr->GetBoolDefault("DeletedCharacterTicketTrace", false);

    ///- Get string for new logins (newly created characters)
    SetNewCharString(sConfigMgr->GetStringDefault("PlayerStart.String", ""));

    ///- Send server info on login?
    m_int_configs[CONFIG_ENABLE_SINFO_LOGIN] = sConfigMgr->GetIntDefault("Server.LoginInfo", 0);

    ///- Read all rates from the config file
    rate_values[RATE_HEALTH] = sConfigMgr->GetFloatDefault("Rate.Health", 1.0f);
    if (rate_values[RATE_HEALTH] < 0)
    {
        TC_LOG_ERROR("server.loading", "Rate.Health (%f) must be > 0. Using 1 instead.", rate_values[RATE_HEALTH]);
        rate_values[RATE_HEALTH] = 1;
    }
    rate_values[RATE_POWER_MANA] = sConfigMgr->GetFloatDefault("Rate.Mana", 1.0f);
    if (rate_values[RATE_POWER_MANA] < 0)
    {
        TC_LOG_ERROR("server.loading", "Rate.Mana (%f) must be > 0. Using 1 instead.", rate_values[RATE_POWER_MANA]);
        rate_values[RATE_POWER_MANA] = 1;
    }
    rate_values[RATE_POWER_RAGE_INCOME] = sConfigMgr->GetFloatDefault("Rate.Rage.Income", 1.0f);
    rate_values[RATE_POWER_RAGE_LOSS] = sConfigMgr->GetFloatDefault("Rate.Rage.Loss", 1.0f);
    if (rate_values[RATE_POWER_RAGE_LOSS] < 0)
    {
        TC_LOG_ERROR("server.loading", "Rate.Rage.Loss (%f) must be > 0. Using 1 instead.", rate_values[RATE_POWER_RAGE_LOSS]);
        rate_values[RATE_POWER_RAGE_LOSS] = 1;
    }
    rate_values[RATE_POWER_RUNICPOWER_INCOME] = sConfigMgr->GetFloatDefault("Rate.RunicPower.Income", 1.0f);
    rate_values[RATE_POWER_RUNICPOWER_LOSS] = sConfigMgr->GetFloatDefault("Rate.RunicPower.Loss", 1.0f);
    if (rate_values[RATE_POWER_RUNICPOWER_LOSS] < 0)
    {
        TC_LOG_ERROR("server.loading", "Rate.RunicPower.Loss (%f) must be > 0. Using 1 instead.", rate_values[RATE_POWER_RUNICPOWER_LOSS]);
        rate_values[RATE_POWER_RUNICPOWER_LOSS] = 1;
    }
    rate_values[RATE_POWER_FOCUS] = sConfigMgr->GetFloatDefault("Rate.Focus", 1.0f);
    rate_values[RATE_POWER_ENERGY] = sConfigMgr->GetFloatDefault("Rate.Energy", 1.0f);

    rate_values[RATE_SKILL_DISCOVERY] = sConfigMgr->GetFloatDefault("Rate.Skill.Discovery", 1.0f);

    rate_values[RATE_DROP_ITEM_CRAFT] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Craft", 1.0f);
    rate_values[RATE_DROP_ITEM_POOR] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Poor", 1.0f);
    rate_values[RATE_DROP_ITEM_NORMAL] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Normal", 1.0f);
    rate_values[RATE_DROP_ITEM_UNCOMMON] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Uncommon", 1.0f);
    rate_values[RATE_DROP_ITEM_RARE] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Rare", 1.0f);
    rate_values[RATE_DROP_ITEM_EPIC] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Epic", 1.0f);
    rate_values[RATE_DROP_ITEM_LEGENDARY] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Legendary", 1.0f);
    rate_values[RATE_DROP_ITEM_ARTIFACT] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Artifact", 1.0f);
    rate_values[RATE_DROP_ITEM_REFERENCED] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.Referenced", 1.0f);
    rate_values[RATE_DROP_ITEM_REFERENCED_AMOUNT] = sConfigMgr->GetFloatDefault("Rate.Drop.Item.ReferencedAmount", 1.0f);
    rate_values[RATE_DROP_MONEY] = sConfigMgr->GetFloatDefault("Rate.Drop.Money", 1.0f);
    rate_values[RATE_XP_KILL] = sConfigMgr->GetFloatDefault("Rate.XP.Kill", 1.0f);
    rate_values[RATE_XP_BG_KILL] = sConfigMgr->GetFloatDefault("Rate.XP.BattlegroundKill", 1.0f);
    rate_values[RATE_XP_QUEST] = sConfigMgr->GetFloatDefault("Rate.XP.Quest", 1.0f);
    rate_values[RATE_XP_EXPLORE] = sConfigMgr->GetFloatDefault("Rate.XP.Explore", 1.0f);
    rate_values[RATE_REPAIRCOST] = sConfigMgr->GetFloatDefault("Rate.RepairCost", 1.0f);
    if (rate_values[RATE_REPAIRCOST] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "Rate.RepairCost (%f) must be >=0. Using 0.0 instead.", rate_values[RATE_REPAIRCOST]);
        rate_values[RATE_REPAIRCOST] = 0.0f;
    }
    rate_values[RATE_REPUTATION_GAIN] = sConfigMgr->GetFloatDefault("Rate.Reputation.Gain", 1.0f);
    rate_values[RATE_REPUTATION_LOWLEVEL_KILL] = sConfigMgr->GetFloatDefault("Rate.Reputation.LowLevel.Kill", 1.0f);
    rate_values[RATE_REPUTATION_LOWLEVEL_QUEST] = sConfigMgr->GetFloatDefault("Rate.Reputation.LowLevel.Quest", 1.0f);
    rate_values[RATE_REPUTATION_RECRUIT_A_FRIEND_BONUS] = sConfigMgr->GetFloatDefault("Rate.Reputation.RecruitAFriendBonus", 0.1f);
    rate_values[RATE_CREATURE_NORMAL_DAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Normal.Damage", 1.0f);
    rate_values[RATE_CREATURE_ELITE_ELITE_DAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.Elite.Damage", 1.0f);
    rate_values[RATE_CREATURE_ELITE_RAREELITE_DAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.RAREELITE.Damage", 1.0f);
    rate_values[RATE_CREATURE_ELITE_WORLDBOSS_DAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.WORLDBOSS.Damage", 1.0f);
    rate_values[RATE_CREATURE_ELITE_RARE_DAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.RARE.Damage", 1.0f);
    rate_values[RATE_CREATURE_NORMAL_HP] = sConfigMgr->GetFloatDefault("Rate.Creature.Normal.HP", 1.0f);
    rate_values[RATE_CREATURE_ELITE_ELITE_HP] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.Elite.HP", 1.0f);
    rate_values[RATE_CREATURE_ELITE_RAREELITE_HP] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.RAREELITE.HP", 1.0f);
    rate_values[RATE_CREATURE_ELITE_WORLDBOSS_HP] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.WORLDBOSS.HP", 1.0f);
    rate_values[RATE_CREATURE_ELITE_RARE_HP] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.RARE.HP", 1.0f);
    rate_values[RATE_CREATURE_NORMAL_SPELLDAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Normal.SpellDamage", 1.0f);
    rate_values[RATE_CREATURE_ELITE_ELITE_SPELLDAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.Elite.SpellDamage", 1.0f);
    rate_values[RATE_CREATURE_ELITE_RAREELITE_SPELLDAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.RAREELITE.SpellDamage", 1.0f);
    rate_values[RATE_CREATURE_ELITE_WORLDBOSS_SPELLDAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.WORLDBOSS.SpellDamage", 1.0f);
    rate_values[RATE_CREATURE_ELITE_RARE_SPELLDAMAGE] = sConfigMgr->GetFloatDefault("Rate.Creature.Elite.RARE.SpellDamage", 1.0f);
    rate_values[RATE_CREATURE_AGGRO] = sConfigMgr->GetFloatDefault("Rate.Creature.Aggro", 1.0f);
    rate_values[RATE_REST_INGAME] = sConfigMgr->GetFloatDefault("Rate.Rest.InGame", 1.0f);
    rate_values[RATE_REST_OFFLINE_IN_TAVERN_OR_CITY] = sConfigMgr->GetFloatDefault("Rate.Rest.Offline.InTavernOrCity", 1.0f);
    rate_values[RATE_REST_OFFLINE_IN_WILDERNESS] = sConfigMgr->GetFloatDefault("Rate.Rest.Offline.InWilderness", 1.0f);
    rate_values[RATE_DAMAGE_FALL] = sConfigMgr->GetFloatDefault("Rate.Damage.Fall", 1.0f);
    rate_values[RATE_AUCTION_TIME] = sConfigMgr->GetFloatDefault("Rate.Auction.Time", 1.0f);
    rate_values[RATE_AUCTION_DEPOSIT] = sConfigMgr->GetFloatDefault("Rate.Auction.Deposit", 1.0f);
    rate_values[RATE_AUCTION_CUT] = sConfigMgr->GetFloatDefault("Rate.Auction.Cut", 1.0f);
    rate_values[RATE_HONOR] = sConfigMgr->GetFloatDefault("Rate.Honor", 1.0f);
    rate_values[RATE_ARENA_POINTS] = sConfigMgr->GetFloatDefault("Rate.ArenaPoints", 1.0f);
    rate_values[RATE_INSTANCE_RESET_TIME] = sConfigMgr->GetFloatDefault("Rate.InstanceResetTime", 1.0f);
    rate_values[RATE_TALENT] = sConfigMgr->GetFloatDefault("Rate.Talent", 1.0f);
    if (rate_values[RATE_TALENT] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "Rate.Talent (%f) must be > 0. Using 1 instead.", rate_values[RATE_TALENT]);
        rate_values[RATE_TALENT] = 1.0f;
    }
    rate_values[RATE_MOVESPEED] = sConfigMgr->GetFloatDefault("Rate.MoveSpeed", 1.0f);
    if (rate_values[RATE_MOVESPEED] < 0)
    {
        TC_LOG_ERROR("server.loading", "Rate.MoveSpeed (%f) must be > 0. Using 1 instead.", rate_values[RATE_MOVESPEED]);
        rate_values[RATE_MOVESPEED] = 1.0f;
    }
    for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i) playerBaseMoveSpeed[i] = baseMoveSpeed[i] * rate_values[RATE_MOVESPEED];
    rate_values[RATE_CORPSE_DECAY_LOOTED] = sConfigMgr->GetFloatDefault("Rate.Corpse.Decay.Looted", 0.5f);

    rate_values[RATE_TARGET_POS_RECALCULATION_RANGE] = sConfigMgr->GetFloatDefault("TargetPosRecalculateRange", 1.5f);
    if (rate_values[RATE_TARGET_POS_RECALCULATION_RANGE] < CONTACT_DISTANCE)
    {
        TC_LOG_ERROR("server.loading", "TargetPosRecalculateRange (%f) must be >= %f. Using %f instead.", rate_values[RATE_TARGET_POS_RECALCULATION_RANGE], CONTACT_DISTANCE, CONTACT_DISTANCE);
        rate_values[RATE_TARGET_POS_RECALCULATION_RANGE] = CONTACT_DISTANCE;
    }
    else if (rate_values[RATE_TARGET_POS_RECALCULATION_RANGE] > NOMINAL_MELEE_RANGE)
    {
        TC_LOG_ERROR("server.loading", "TargetPosRecalculateRange (%f) must be <= %f. Using %f instead.",
            rate_values[RATE_TARGET_POS_RECALCULATION_RANGE], NOMINAL_MELEE_RANGE, NOMINAL_MELEE_RANGE);
        rate_values[RATE_TARGET_POS_RECALCULATION_RANGE] = NOMINAL_MELEE_RANGE;
    }

    rate_values[RATE_DURABILITY_LOSS_ON_DEATH] = sConfigMgr->GetFloatDefault("DurabilityLoss.OnDeath", 10.0f);
    if (rate_values[RATE_DURABILITY_LOSS_ON_DEATH] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "DurabilityLoss.OnDeath (%f) must be >=0. Using 0.0 instead.", rate_values[RATE_DURABILITY_LOSS_ON_DEATH]);
        rate_values[RATE_DURABILITY_LOSS_ON_DEATH] = 0.0f;
    }
    if (rate_values[RATE_DURABILITY_LOSS_ON_DEATH] > 100.0f)
    {
        TC_LOG_ERROR("server.loading", "DurabilityLoss.OnDeath (%f) must be <= 100. Using 100.0 instead.", rate_values[RATE_DURABILITY_LOSS_ON_DEATH]);
        rate_values[RATE_DURABILITY_LOSS_ON_DEATH] = 0.0f;
    }
    rate_values[RATE_DURABILITY_LOSS_ON_DEATH] = rate_values[RATE_DURABILITY_LOSS_ON_DEATH] / 100.0f;

    rate_values[RATE_DURABILITY_LOSS_DAMAGE] = sConfigMgr->GetFloatDefault("DurabilityLossChance.Damage", 0.5f);
    if (rate_values[RATE_DURABILITY_LOSS_DAMAGE] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "DurabilityLossChance.Damage (%f) must be >=0. Using 0.0 instead.", rate_values[RATE_DURABILITY_LOSS_DAMAGE]);
        rate_values[RATE_DURABILITY_LOSS_DAMAGE] = 0.0f;
    }
    rate_values[RATE_DURABILITY_LOSS_ABSORB] = sConfigMgr->GetFloatDefault("DurabilityLossChance.Absorb", 0.5f);
    if (rate_values[RATE_DURABILITY_LOSS_ABSORB] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "DurabilityLossChance.Absorb (%f) must be >=0. Using 0.0 instead.", rate_values[RATE_DURABILITY_LOSS_ABSORB]);
        rate_values[RATE_DURABILITY_LOSS_ABSORB] = 0.0f;
    }
    rate_values[RATE_DURABILITY_LOSS_PARRY] = sConfigMgr->GetFloatDefault("DurabilityLossChance.Parry", 0.05f);
    if (rate_values[RATE_DURABILITY_LOSS_PARRY] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "DurabilityLossChance.Parry (%f) must be >=0. Using 0.0 instead.", rate_values[RATE_DURABILITY_LOSS_PARRY]);
        rate_values[RATE_DURABILITY_LOSS_PARRY] = 0.0f;
    }
    rate_values[RATE_DURABILITY_LOSS_BLOCK] = sConfigMgr->GetFloatDefault("DurabilityLossChance.Block", 0.05f);
    if (rate_values[RATE_DURABILITY_LOSS_BLOCK] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "DurabilityLossChance.Block (%f) must be >=0. Using 0.0 instead.", rate_values[RATE_DURABILITY_LOSS_BLOCK]);
        rate_values[RATE_DURABILITY_LOSS_BLOCK] = 0.0f;
    }
    rate_values[RATE_MONEY_QUEST] = sConfigMgr->GetFloatDefault("Rate.Quest.Money.Reward", 1.0f);
    if (rate_values[RATE_MONEY_QUEST] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "Rate.Quest.Money.Reward (%f) must be >=0. Using 0 instead.", rate_values[RATE_MONEY_QUEST]);
        rate_values[RATE_MONEY_QUEST] = 0.0f;
    }
    rate_values[RATE_MONEY_MAX_LEVEL_QUEST] = sConfigMgr->GetFloatDefault("Rate.Quest.Money.Max.Level.Reward", 1.0f);
    if (rate_values[RATE_MONEY_MAX_LEVEL_QUEST] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "Rate.Quest.Money.Max.Level.Reward (%f) must be >=0. Using 0 instead.", rate_values[RATE_MONEY_MAX_LEVEL_QUEST]);
        rate_values[RATE_MONEY_MAX_LEVEL_QUEST] = 0.0f;
    }
    ///- Read other configuration items from the config file

    m_bool_configs[CONFIG_DURABILITY_LOSS_IN_PVP] = sConfigMgr->GetBoolDefault("DurabilityLoss.InPvP", false);

    m_int_configs[CONFIG_COMPRESSION] = sConfigMgr->GetIntDefault("Compression", 1);
    if (m_int_configs[CONFIG_COMPRESSION] < 1 || m_int_configs[CONFIG_COMPRESSION] > 9)
    {
        TC_LOG_ERROR("server.loading", "Compression level (%i) must be in range 1..9. Using default compression level (1).", m_int_configs[CONFIG_COMPRESSION]);
        m_int_configs[CONFIG_COMPRESSION] = 1;
    }
    m_bool_configs[CONFIG_ADDON_CHANNEL] = sConfigMgr->GetBoolDefault("AddonChannel", true);
    m_bool_configs[CONFIG_CLEAN_CHARACTER_DB] = sConfigMgr->GetBoolDefault("CleanCharacterDB", false);
    m_int_configs[CONFIG_PERSISTENT_CHARACTER_CLEAN_FLAGS] = sConfigMgr->GetIntDefault("PersistentCharacterCleanFlags", 0);
    m_int_configs[CONFIG_AUCTION_GETALL_DELAY] = sConfigMgr->GetIntDefault("Auction.GetAllScanDelay", 900);
    m_int_configs[CONFIG_AUCTION_SEARCH_DELAY] = sConfigMgr->GetIntDefault("Auction.SearchDelay", 300);
    if (m_int_configs[CONFIG_AUCTION_SEARCH_DELAY] < 100 || m_int_configs[CONFIG_AUCTION_SEARCH_DELAY] > 10000)
    {
        TC_LOG_ERROR("server.loading", "Auction.SearchDelay (%i) must be between 100 and 10000. Using default of 300ms", m_int_configs[CONFIG_AUCTION_SEARCH_DELAY]);
        m_int_configs[CONFIG_AUCTION_SEARCH_DELAY] = 300;
    }
    m_int_configs[CONFIG_CHAT_CHANNEL_LEVEL_REQ] = sConfigMgr->GetIntDefault("ChatLevelReq.Channel", 1);
    m_int_configs[CONFIG_CHAT_WHISPER_LEVEL_REQ] = sConfigMgr->GetIntDefault("ChatLevelReq.Whisper", 1);
    m_int_configs[CONFIG_CHAT_EMOTE_LEVEL_REQ] = sConfigMgr->GetIntDefault("ChatLevelReq.Emote", 1);
    m_int_configs[CONFIG_CHAT_SAY_LEVEL_REQ] = sConfigMgr->GetIntDefault("ChatLevelReq.Say", 1);
    m_int_configs[CONFIG_CHAT_YELL_LEVEL_REQ] = sConfigMgr->GetIntDefault("ChatLevelReq.Yell", 1);
    m_int_configs[CONFIG_PARTY_LEVEL_REQ] = sConfigMgr->GetIntDefault("PartyLevelReq", 1);
    m_int_configs[CONFIG_TRADE_LEVEL_REQ] = sConfigMgr->GetIntDefault("LevelReq.Trade", 1);
    m_int_configs[CONFIG_TICKET_LEVEL_REQ] = sConfigMgr->GetIntDefault("LevelReq.Ticket", 1);
    m_int_configs[CONFIG_AUCTION_LEVEL_REQ] = sConfigMgr->GetIntDefault("LevelReq.Auction", 1);
    m_int_configs[CONFIG_MAIL_LEVEL_REQ] = sConfigMgr->GetIntDefault("LevelReq.Mail", 1);
    m_bool_configs[CONFIG_PRESERVE_CUSTOM_CHANNELS] = sConfigMgr->GetBoolDefault("PreserveCustomChannels", false);
    m_int_configs[CONFIG_PRESERVE_CUSTOM_CHANNEL_DURATION] = sConfigMgr->GetIntDefault("PreserveCustomChannelDuration", 14);
    m_bool_configs[CONFIG_GRID_UNLOAD] = sConfigMgr->GetBoolDefault("GridUnload", true);
    m_bool_configs[CONFIG_BASEMAP_LOAD_GRIDS] = sConfigMgr->GetBoolDefault("BaseMapLoadAllGrids", false);
    if (m_bool_configs[CONFIG_BASEMAP_LOAD_GRIDS] && m_bool_configs[CONFIG_GRID_UNLOAD])
    {
        TC_LOG_ERROR("server.loading", "BaseMapLoadAllGrids enabled, but GridUnload also enabled. GridUnload must be disabled to enable base map pre-loading. Base map pre-loading disabled");
        m_bool_configs[CONFIG_BASEMAP_LOAD_GRIDS] = false;
    }
    m_bool_configs[CONFIG_INSTANCEMAP_LOAD_GRIDS] = sConfigMgr->GetBoolDefault("InstanceMapLoadAllGrids", false);
    if (m_bool_configs[CONFIG_INSTANCEMAP_LOAD_GRIDS] && m_bool_configs[CONFIG_GRID_UNLOAD])
    {
        TC_LOG_ERROR("server.loading", "InstanceMapLoadAllGrids enabled, but GridUnload also enabled. GridUnload must be disabled to enable instance map pre-loading. Instance map pre-loading disabled");
        m_bool_configs[CONFIG_INSTANCEMAP_LOAD_GRIDS] = false;
    }
    m_int_configs[CONFIG_INTERVAL_SAVE] = sConfigMgr->GetIntDefault("PlayerSaveInterval", 15 * MINUTE * IN_MILLISECONDS);
    m_int_configs[CONFIG_INTERVAL_DISCONNECT_TOLERANCE] = sConfigMgr->GetIntDefault("DisconnectToleranceInterval", 0);
    m_bool_configs[CONFIG_STATS_SAVE_ONLY_ON_LOGOUT] = sConfigMgr->GetBoolDefault("PlayerSave.Stats.SaveOnlyOnLogout", true);

    m_int_configs[CONFIG_MIN_LEVEL_STAT_SAVE] = sConfigMgr->GetIntDefault("PlayerSave.Stats.MinLevel", 0);
    if (m_int_configs[CONFIG_MIN_LEVEL_STAT_SAVE] > MAX_LEVEL)
    {
        TC_LOG_ERROR("server.loading", "PlayerSave.Stats.MinLevel (%i) must be in range 0..80. Using default, do not save character stats (0).", m_int_configs[CONFIG_MIN_LEVEL_STAT_SAVE]);
        m_int_configs[CONFIG_MIN_LEVEL_STAT_SAVE] = 0;
    }

    m_int_configs[CONFIG_INTERVAL_GRIDCLEAN] = sConfigMgr->GetIntDefault("GridCleanUpDelay", 5 * MINUTE * IN_MILLISECONDS);
    if (m_int_configs[CONFIG_INTERVAL_GRIDCLEAN] < MIN_GRID_DELAY)
    {
        TC_LOG_ERROR("server.loading", "GridCleanUpDelay (%i) must be greater %u. Use this minimal value.", m_int_configs[CONFIG_INTERVAL_GRIDCLEAN], MIN_GRID_DELAY);
        m_int_configs[CONFIG_INTERVAL_GRIDCLEAN] = MIN_GRID_DELAY;
    }
    if (reload)
        sMapMgr->SetGridCleanUpDelay(m_int_configs[CONFIG_INTERVAL_GRIDCLEAN]);

    m_int_configs[CONFIG_INTERVAL_MAPUPDATE] = sConfigMgr->GetIntDefault("MapUpdateInterval", 100);
    if (m_int_configs[CONFIG_INTERVAL_MAPUPDATE] < MIN_MAP_UPDATE_DELAY)
    {
        TC_LOG_ERROR("server.loading", "MapUpdateInterval (%i) must be greater %u. Use this minimal value.", m_int_configs[CONFIG_INTERVAL_MAPUPDATE], MIN_MAP_UPDATE_DELAY);
        m_int_configs[CONFIG_INTERVAL_MAPUPDATE] = MIN_MAP_UPDATE_DELAY;
    }
    if (reload)
        sMapMgr->SetMapUpdateInterval(m_int_configs[CONFIG_INTERVAL_MAPUPDATE]);

    m_int_configs[CONFIG_INTERVAL_CHANGEWEATHER] = sConfigMgr->GetIntDefault("ChangeWeatherInterval", 10 * MINUTE * IN_MILLISECONDS);

    if (reload)
    {
        uint32 val = sConfigMgr->GetIntDefault("WorldServerPort", 8085);
        if (val != m_int_configs[CONFIG_PORT_WORLD])
            TC_LOG_ERROR("server.loading", "WorldServerPort option can't be changed at worldserver.conf reload, using current value (%u).", m_int_configs[CONFIG_PORT_WORLD]);
    }
    else
        m_int_configs[CONFIG_PORT_WORLD] = sConfigMgr->GetIntDefault("WorldServerPort", 8085);

    // Config values are in "milliseconds" but we handle SocketTimeOut only as "seconds" so divide by 1000
    m_int_configs[CONFIG_SOCKET_TIMEOUTTIME] = sConfigMgr->GetIntDefault("SocketTimeOutTime", 900000) / 1000;
    m_int_configs[CONFIG_SOCKET_TIMEOUTTIME_ACTIVE] = sConfigMgr->GetIntDefault("SocketTimeOutTimeActive", 60000) / 1000;

    m_int_configs[CONFIG_SESSION_ADD_DELAY] = sConfigMgr->GetIntDefault("SessionAddDelay", 10000);

    m_float_configs[CONFIG_GROUP_XP_DISTANCE] = sConfigMgr->GetFloatDefault("MaxGroupXPDistance", 74.0f);
    m_float_configs[CONFIG_MAX_RECRUIT_A_FRIEND_DISTANCE] = sConfigMgr->GetFloatDefault("MaxRecruitAFriendBonusDistance", 100.0f);

    m_int_configs[CONFIG_MIN_QUEST_SCALED_XP_RATIO] = sConfigMgr->GetIntDefault("MinQuestScaledXPRatio", 0);
    if (m_int_configs[CONFIG_MIN_QUEST_SCALED_XP_RATIO] > 100)
    {
        TC_LOG_ERROR("server.loading", "MinQuestScaledXPRatio (%i) must be in range 0..100. Set to 0.", m_int_configs[CONFIG_MIN_QUEST_SCALED_XP_RATIO]);
        m_int_configs[CONFIG_MIN_QUEST_SCALED_XP_RATIO] = 0;
    }

    m_int_configs[CONFIG_MIN_CREATURE_SCALED_XP_RATIO] = sConfigMgr->GetIntDefault("MinCreatureScaledXPRatio", 0);
    if (m_int_configs[CONFIG_MIN_CREATURE_SCALED_XP_RATIO] > 100)
    {
        TC_LOG_ERROR("server.loading", "MinCreatureScaledXPRatio (%i) must be in range 0..100. Set to 0.", m_int_configs[CONFIG_MIN_CREATURE_SCALED_XP_RATIO]);
        m_int_configs[CONFIG_MIN_CREATURE_SCALED_XP_RATIO] = 0;
    }

    m_int_configs[CONFIG_MIN_DISCOVERED_SCALED_XP_RATIO] = sConfigMgr->GetIntDefault("MinDiscoveredScaledXPRatio", 0);
    if (m_int_configs[CONFIG_MIN_DISCOVERED_SCALED_XP_RATIO] > 100)
    {
        TC_LOG_ERROR("server.loading", "MinDiscoveredScaledXPRatio (%i) must be in range 0..100. Set to 0.", m_int_configs[CONFIG_MIN_DISCOVERED_SCALED_XP_RATIO]);
        m_int_configs[CONFIG_MIN_DISCOVERED_SCALED_XP_RATIO] = 0;
    }

    /// @todo Add MonsterSight (with meaning) in worldserver.conf or put them as define
    m_float_configs[CONFIG_SIGHT_MONSTER] = sConfigMgr->GetFloatDefault("MonsterSight", 50.0f);

    if (reload)
    {
        uint32 val = sConfigMgr->GetIntDefault("GameType", 0);
        if (val != m_int_configs[CONFIG_GAME_TYPE])
            TC_LOG_ERROR("server.loading", "GameType option can't be changed at worldserver.conf reload, using current value (%u).", m_int_configs[CONFIG_GAME_TYPE]);
    }
    else
        m_int_configs[CONFIG_GAME_TYPE] = sConfigMgr->GetIntDefault("GameType", 0);

    if (reload)
    {
        uint32 val = sConfigMgr->GetIntDefault("RealmZone", REALM_ZONE_DEVELOPMENT);
        if (val != m_int_configs[CONFIG_REALM_ZONE])
            TC_LOG_ERROR("server.loading", "RealmZone option can't be changed at worldserver.conf reload, using current value (%u).", m_int_configs[CONFIG_REALM_ZONE]);
    }
    else
        m_int_configs[CONFIG_REALM_ZONE] = sConfigMgr->GetIntDefault("RealmZone", REALM_ZONE_DEVELOPMENT);

    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_INTERACTION_CALENDAR] = sConfigMgr->GetBoolDefault("AllowTwoSide.Interaction.Calendar", false);
    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHANNEL] = sConfigMgr->GetBoolDefault("AllowTwoSide.Interaction.Channel", false);
    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP] = sConfigMgr->GetBoolDefault("AllowTwoSide.Interaction.Group", false);
    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD] = sConfigMgr->GetBoolDefault("AllowTwoSide.Interaction.Guild", false);
    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION] = sConfigMgr->GetBoolDefault("AllowTwoSide.Interaction.Auction", false);
    m_bool_configs[CONFIG_ALLOW_TWO_SIDE_TRADE] = sConfigMgr->GetBoolDefault("AllowTwoSide.Trade", false);
    m_int_configs[CONFIG_STRICT_PLAYER_NAMES] = sConfigMgr->GetIntDefault("StrictPlayerNames", 0);
    m_int_configs[CONFIG_STRICT_CHARTER_NAMES] = sConfigMgr->GetIntDefault("StrictCharterNames", 0);
    m_int_configs[CONFIG_STRICT_PET_NAMES] = sConfigMgr->GetIntDefault("StrictPetNames", 0);

    m_int_configs[CONFIG_MIN_PLAYER_NAME] = sConfigMgr->GetIntDefault("MinPlayerName", 2);
    if (m_int_configs[CONFIG_MIN_PLAYER_NAME] < 1 || m_int_configs[CONFIG_MIN_PLAYER_NAME] > MAX_PLAYER_NAME)
    {
        TC_LOG_ERROR("server.loading", "MinPlayerName (%i) must be in range 1..%u. Set to 2.", m_int_configs[CONFIG_MIN_PLAYER_NAME], MAX_PLAYER_NAME);
        m_int_configs[CONFIG_MIN_PLAYER_NAME] = 2;
    }

    m_int_configs[CONFIG_MIN_CHARTER_NAME] = sConfigMgr->GetIntDefault("MinCharterName", 2);
    if (m_int_configs[CONFIG_MIN_CHARTER_NAME] < 1 || m_int_configs[CONFIG_MIN_CHARTER_NAME] > MAX_CHARTER_NAME)
    {
        TC_LOG_ERROR("server.loading", "MinCharterName (%i) must be in range 1..%u. Set to 2.", m_int_configs[CONFIG_MIN_CHARTER_NAME], MAX_CHARTER_NAME);
        m_int_configs[CONFIG_MIN_CHARTER_NAME] = 2;
    }

    m_int_configs[CONFIG_MIN_PET_NAME] = sConfigMgr->GetIntDefault("MinPetName", 2);
    if (m_int_configs[CONFIG_MIN_PET_NAME] < 1 || m_int_configs[CONFIG_MIN_PET_NAME] > MAX_PET_NAME)
    {
        TC_LOG_ERROR("server.loading", "MinPetName (%i) must be in range 1..%u. Set to 2.", m_int_configs[CONFIG_MIN_PET_NAME], MAX_PET_NAME);
        m_int_configs[CONFIG_MIN_PET_NAME] = 2;
    }

    m_int_configs[CONFIG_CHARTER_COST_GUILD] = sConfigMgr->GetIntDefault("Guild.CharterCost", 1000);
    m_int_configs[CONFIG_CHARTER_COST_ARENA_2v2] = sConfigMgr->GetIntDefault("ArenaTeam.CharterCost.2v2", 800000);
    m_int_configs[CONFIG_CHARTER_COST_ARENA_3v3] = sConfigMgr->GetIntDefault("ArenaTeam.CharterCost.3v3", 1200000);
    m_int_configs[CONFIG_CHARTER_COST_ARENA_5v5] = sConfigMgr->GetIntDefault("ArenaTeam.CharterCost.5v5", 2000000);

    m_int_configs[CONFIG_CHARACTER_CREATING_DISABLED] = sConfigMgr->GetIntDefault("CharacterCreating.Disabled", 0);
    m_int_configs[CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK] = sConfigMgr->GetIntDefault("CharacterCreating.Disabled.RaceMask", 0);
    m_int_configs[CONFIG_CHARACTER_CREATING_DISABLED_CLASSMASK] = sConfigMgr->GetIntDefault("CharacterCreating.Disabled.ClassMask", 0);

    m_int_configs[CONFIG_CHARACTERS_PER_REALM] = sConfigMgr->GetIntDefault("CharactersPerRealm", 10);
    if (m_int_configs[CONFIG_CHARACTERS_PER_REALM] < 1 || m_int_configs[CONFIG_CHARACTERS_PER_REALM] > 10)
    {
        TC_LOG_ERROR("server.loading", "CharactersPerRealm (%i) must be in range 1..10. Set to 10.", m_int_configs[CONFIG_CHARACTERS_PER_REALM]);
        m_int_configs[CONFIG_CHARACTERS_PER_REALM] = 10;
    }

    // must be after CONFIG_CHARACTERS_PER_REALM
    m_int_configs[CONFIG_CHARACTERS_PER_ACCOUNT] = sConfigMgr->GetIntDefault("CharactersPerAccount", 50);
    if (m_int_configs[CONFIG_CHARACTERS_PER_ACCOUNT] < m_int_configs[CONFIG_CHARACTERS_PER_REALM])
    {
        TC_LOG_ERROR("server.loading", "CharactersPerAccount (%i) can't be less than CharactersPerRealm (%i).", m_int_configs[CONFIG_CHARACTERS_PER_ACCOUNT], m_int_configs[CONFIG_CHARACTERS_PER_REALM]);
        m_int_configs[CONFIG_CHARACTERS_PER_ACCOUNT] = m_int_configs[CONFIG_CHARACTERS_PER_REALM];
    }

    m_int_configs[CONFIG_DEATH_KNIGHTS_PER_REALM] = sConfigMgr->GetIntDefault("DeathKnightsPerRealm", 1);
    if (int32(m_int_configs[CONFIG_DEATH_KNIGHTS_PER_REALM]) < 0 || m_int_configs[CONFIG_DEATH_KNIGHTS_PER_REALM] > 10)
    {
        TC_LOG_ERROR("server.loading", "DeathKnightsPerRealm (%i) must be in range 0..10. Set to 1.", m_int_configs[CONFIG_DEATH_KNIGHTS_PER_REALM]);
        m_int_configs[CONFIG_DEATH_KNIGHTS_PER_REALM] = 1;
    }

    m_int_configs[CONFIG_CHARACTER_CREATING_MIN_LEVEL_FOR_DEATH_KNIGHT] = sConfigMgr->GetIntDefault("CharacterCreating.MinLevelForDeathKnight", 55);

    m_int_configs[CONFIG_SKIP_CINEMATICS] = sConfigMgr->GetIntDefault("SkipCinematics", 0);
    if (int32(m_int_configs[CONFIG_SKIP_CINEMATICS]) < 0 || m_int_configs[CONFIG_SKIP_CINEMATICS] > 2)
    {
        TC_LOG_ERROR("server.loading", "SkipCinematics (%i) must be in range 0..2. Set to 0.", m_int_configs[CONFIG_SKIP_CINEMATICS]);
        m_int_configs[CONFIG_SKIP_CINEMATICS] = 0;
    }

    if (reload)
    {
        uint32 val = sConfigMgr->GetIntDefault("MaxPlayerLevel", DEFAULT_MAX_LEVEL);
        if (val != m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
            TC_LOG_ERROR("server.loading", "MaxPlayerLevel option can't be changed at config reload, using current value (%u).", m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
    }
    else
        m_int_configs[CONFIG_MAX_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("MaxPlayerLevel", DEFAULT_MAX_LEVEL);

    if (m_int_configs[CONFIG_MAX_PLAYER_LEVEL] > MAX_LEVEL)
    {
        TC_LOG_ERROR("server.loading", "MaxPlayerLevel (%i) must be in range 1..%u. Set to %u.", m_int_configs[CONFIG_MAX_PLAYER_LEVEL], MAX_LEVEL, MAX_LEVEL);
        m_int_configs[CONFIG_MAX_PLAYER_LEVEL] = MAX_LEVEL;
    }

    m_int_configs[CONFIG_MIN_DUALSPEC_LEVEL] = sConfigMgr->GetIntDefault("MinDualSpecLevel", 40);

    m_int_configs[CONFIG_START_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("StartPlayerLevel", 1);
    if (m_int_configs[CONFIG_START_PLAYER_LEVEL] < 1)
    {
        TC_LOG_ERROR("server.loading", "StartPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to 1.", m_int_configs[CONFIG_START_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_PLAYER_LEVEL] = 1;
    }
    else if (m_int_configs[CONFIG_START_PLAYER_LEVEL] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "StartPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to %u.", m_int_configs[CONFIG_START_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_PLAYER_LEVEL] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }

    m_int_configs[CONFIG_START_DEATH_KNIGHT_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("StartDeathKnightPlayerLevel", 55);
    if (m_int_configs[CONFIG_START_DEATH_KNIGHT_PLAYER_LEVEL] < 1)
    {
        TC_LOG_ERROR("server.loading", "StartDeathKnightPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to 55.",
            m_int_configs[CONFIG_START_DEATH_KNIGHT_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_DEATH_KNIGHT_PLAYER_LEVEL] = 55;
    }
    else if (m_int_configs[CONFIG_START_DEATH_KNIGHT_PLAYER_LEVEL] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "StartDeathKnightPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to %u.",
            m_int_configs[CONFIG_START_DEATH_KNIGHT_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_DEATH_KNIGHT_PLAYER_LEVEL] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }

    m_int_configs[CONFIG_START_GOBLIN_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("StartGoblinPlayerLevel", 20);
    if (m_int_configs[CONFIG_START_GOBLIN_PLAYER_LEVEL] < 1)
    {
        TC_LOG_ERROR("server.loading", "StartGoblinPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to 20.",
            m_int_configs[CONFIG_START_GOBLIN_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_GOBLIN_PLAYER_LEVEL] = 20;
    }
    else if (m_int_configs[CONFIG_START_GOBLIN_PLAYER_LEVEL] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "StartGoblinPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to %u.",
            m_int_configs[CONFIG_START_GOBLIN_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_GOBLIN_PLAYER_LEVEL] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }

    m_int_configs[CONFIG_START_HUARGUEN_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("StartHuargenPlayerLevel", 20);
    if (m_int_configs[CONFIG_START_HUARGUEN_PLAYER_LEVEL] < 1)
    {
        TC_LOG_ERROR("server.loading", "StartHuargenPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to 20.",
            m_int_configs[CONFIG_START_HUARGUEN_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_HUARGUEN_PLAYER_LEVEL] = 20;
    }
    else if (m_int_configs[CONFIG_START_HUARGUEN_PLAYER_LEVEL] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "StartHuargenPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to %u.",
            m_int_configs[CONFIG_START_HUARGUEN_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_HUARGUEN_PLAYER_LEVEL] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }

    m_int_configs[CONFIG_START_ALIANCE_PANDAREN_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("StartAliancePandarenPlayerLevel", 20);
    if (m_int_configs[CONFIG_START_ALIANCE_PANDAREN_PLAYER_LEVEL] < 1)
    {
        TC_LOG_ERROR("server.loading", "StartAliancePandarenPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to 20.",
            m_int_configs[CONFIG_START_ALIANCE_PANDAREN_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_ALIANCE_PANDAREN_PLAYER_LEVEL] = 20;
    }
    else if (m_int_configs[CONFIG_START_ALIANCE_PANDAREN_PLAYER_LEVEL] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "StartAliancePandarenPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to %u.",
            m_int_configs[CONFIG_START_ALIANCE_PANDAREN_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_ALIANCE_PANDAREN_PLAYER_LEVEL] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }

    m_int_configs[CONFIG_START_HORDE_PANDAREN_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("StartHordePandarenPlayerLevel", 20);
    if (m_int_configs[CONFIG_START_HORDE_PANDAREN_PLAYER_LEVEL] < 1)
    {
        TC_LOG_ERROR("server.loading", "StartHordePandarenPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to 20.",
            m_int_configs[CONFIG_START_HORDE_PANDAREN_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_HORDE_PANDAREN_PLAYER_LEVEL] = 20;
    }
    else if (m_int_configs[CONFIG_START_HORDE_PANDAREN_PLAYER_LEVEL] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "StartHordePandarenPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to %u.",
            m_int_configs[CONFIG_START_HORDE_PANDAREN_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_HORDE_PANDAREN_PLAYER_LEVEL] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }


    m_int_configs[CONFIG_START_ALIANCE_BLOOD_ELF_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("StartAlianceBloodElfPlayerLevel", 20);
    if (m_int_configs[CONFIG_START_ALIANCE_BLOOD_ELF_PLAYER_LEVEL] < 1)
    {
        TC_LOG_ERROR("server.loading", "StartAlianceBloodElfPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to 20.",
            m_int_configs[CONFIG_START_ALIANCE_BLOOD_ELF_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_ALIANCE_BLOOD_ELF_PLAYER_LEVEL] = 20;
    }
    else if (m_int_configs[CONFIG_START_ALIANCE_BLOOD_ELF_PLAYER_LEVEL] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "StartAlianceBloodElfPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to %u.",
            m_int_configs[CONFIG_START_ALIANCE_BLOOD_ELF_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_ALIANCE_BLOOD_ELF_PLAYER_LEVEL] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }

    m_int_configs[CONFIG_START_HORDE_NIGHT_ELF_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("StartHordeNightElfPlayerLevel", 20);
    if (m_int_configs[CONFIG_START_HORDE_NIGHT_ELF_PLAYER_LEVEL] < 1)
    {
        TC_LOG_ERROR("server.loading", "StartHordeNightElfPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to 20.",
            m_int_configs[CONFIG_START_HORDE_NIGHT_ELF_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_HORDE_NIGHT_ELF_PLAYER_LEVEL] = 20;
    }
    else if (m_int_configs[CONFIG_START_HORDE_NIGHT_ELF_PLAYER_LEVEL] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "StartHordeNightElfPlayerLevel (%i) must be in range 1..MaxPlayerLevel(%u). Set to %u.",
            m_int_configs[CONFIG_START_HORDE_NIGHT_ELF_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_HORDE_NIGHT_ELF_PLAYER_LEVEL] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }




    m_int_configs[CONFIG_START_PLAYER_MONEY] = sConfigMgr->GetIntDefault("StartPlayerMoney", 0);
    if (int32(m_int_configs[CONFIG_START_PLAYER_MONEY]) < 0)
    {
        TC_LOG_ERROR("server.loading", "StartPlayerMoney (%i) must be in range 0..%u. Set to %u.", m_int_configs[CONFIG_START_PLAYER_MONEY], MAX_MONEY_AMOUNT, 0);
        m_int_configs[CONFIG_START_PLAYER_MONEY] = 0;
    }
    else if (m_int_configs[CONFIG_START_PLAYER_MONEY] > MAX_MONEY_AMOUNT)
    {
        TC_LOG_ERROR("server.loading", "StartPlayerMoney (%i) must be in range 0..%u. Set to %u.",
            m_int_configs[CONFIG_START_PLAYER_MONEY], MAX_MONEY_AMOUNT, MAX_MONEY_AMOUNT);
        m_int_configs[CONFIG_START_PLAYER_MONEY] = MAX_MONEY_AMOUNT;
    }

    m_int_configs[CONFIG_MAX_HONOR_POINTS] = sConfigMgr->GetIntDefault("MaxHonorPoints", 75000);
    if (int32(m_int_configs[CONFIG_MAX_HONOR_POINTS]) < 0)
    {
        TC_LOG_ERROR("server.loading", "MaxHonorPoints (%i) can't be negative. Set to 0.", m_int_configs[CONFIG_MAX_HONOR_POINTS]);
        m_int_configs[CONFIG_MAX_HONOR_POINTS] = 0;
    }

    m_int_configs[CONFIG_START_HONOR_POINTS] = sConfigMgr->GetIntDefault("StartHonorPoints", 0);
    if (int32(m_int_configs[CONFIG_START_HONOR_POINTS]) < 0)
    {
        TC_LOG_ERROR("server.loading", "StartHonorPoints (%i) must be in range 0..MaxHonorPoints(%u). Set to %u.",
            m_int_configs[CONFIG_START_HONOR_POINTS], m_int_configs[CONFIG_MAX_HONOR_POINTS], 0);
        m_int_configs[CONFIG_START_HONOR_POINTS] = 0;
    }
    else if (m_int_configs[CONFIG_START_HONOR_POINTS] > m_int_configs[CONFIG_MAX_HONOR_POINTS])
    {
        TC_LOG_ERROR("server.loading", "StartHonorPoints (%i) must be in range 0..MaxHonorPoints(%u). Set to %u.",
            m_int_configs[CONFIG_START_HONOR_POINTS], m_int_configs[CONFIG_MAX_HONOR_POINTS], m_int_configs[CONFIG_MAX_HONOR_POINTS]);
        m_int_configs[CONFIG_START_HONOR_POINTS] = m_int_configs[CONFIG_MAX_HONOR_POINTS];
    }

    m_int_configs[CONFIG_MAX_ARENA_POINTS] = sConfigMgr->GetIntDefault("MaxArenaPoints", 10000);
    if (int32(m_int_configs[CONFIG_MAX_ARENA_POINTS]) < 0)
    {
        TC_LOG_ERROR("server.loading", "MaxArenaPoints (%i) can't be negative. Set to 0.", m_int_configs[CONFIG_MAX_ARENA_POINTS]);
        m_int_configs[CONFIG_MAX_ARENA_POINTS] = 0;
    }

    m_int_configs[CONFIG_START_ARENA_POINTS] = sConfigMgr->GetIntDefault("StartArenaPoints", 0);
    if (int32(m_int_configs[CONFIG_START_ARENA_POINTS]) < 0)
    {
        TC_LOG_ERROR("server.loading", "StartArenaPoints (%i) must be in range 0..MaxArenaPoints(%u). Set to %u.",
            m_int_configs[CONFIG_START_ARENA_POINTS], m_int_configs[CONFIG_MAX_ARENA_POINTS], 0);
        m_int_configs[CONFIG_START_ARENA_POINTS] = 0;
    }
    else if (m_int_configs[CONFIG_START_ARENA_POINTS] > m_int_configs[CONFIG_MAX_ARENA_POINTS])
    {
        TC_LOG_ERROR("server.loading", "StartArenaPoints (%i) must be in range 0..MaxArenaPoints(%u). Set to %u.",
            m_int_configs[CONFIG_START_ARENA_POINTS], m_int_configs[CONFIG_MAX_ARENA_POINTS], m_int_configs[CONFIG_MAX_ARENA_POINTS]);
        m_int_configs[CONFIG_START_ARENA_POINTS] = m_int_configs[CONFIG_MAX_ARENA_POINTS];
    }

    m_int_configs[CONFIG_MAX_RECRUIT_A_FRIEND_BONUS_PLAYER_LEVEL] = sConfigMgr->GetIntDefault("RecruitAFriend.MaxLevel", 60);
    if (m_int_configs[CONFIG_MAX_RECRUIT_A_FRIEND_BONUS_PLAYER_LEVEL] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "RecruitAFriend.MaxLevel (%i) must be in the range 0..MaxLevel(%u). Set to %u.",
            m_int_configs[CONFIG_MAX_RECRUIT_A_FRIEND_BONUS_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], 60);
        m_int_configs[CONFIG_MAX_RECRUIT_A_FRIEND_BONUS_PLAYER_LEVEL] = 60;
    }

    m_int_configs[CONFIG_MAX_RECRUIT_A_FRIEND_BONUS_PLAYER_LEVEL_DIFFERENCE] = sConfigMgr->GetIntDefault("RecruitAFriend.MaxDifference", 4);
    m_bool_configs[CONFIG_ALL_TAXI_PATHS] = sConfigMgr->GetBoolDefault("AllFlightPaths", false);
    m_bool_configs[CONFIG_INSTANT_TAXI] = sConfigMgr->GetBoolDefault("InstantFlightPaths", false);

    m_bool_configs[CONFIG_INSTANCE_IGNORE_LEVEL] = sConfigMgr->GetBoolDefault("Instance.IgnoreLevel", false);
    m_bool_configs[CONFIG_INSTANCE_IGNORE_RAID] = sConfigMgr->GetBoolDefault("Instance.IgnoreRaid", false);

    m_bool_configs[CONFIG_CAST_UNSTUCK] = sConfigMgr->GetBoolDefault("CastUnstuck", true);
    m_int_configs[CONFIG_INSTANCE_RESET_TIME_HOUR] = sConfigMgr->GetIntDefault("Instance.ResetTimeHour", 4);
    m_int_configs[CONFIG_INSTANCE_UNLOAD_DELAY] = sConfigMgr->GetIntDefault("Instance.UnloadDelay", 30 * MINUTE * IN_MILLISECONDS);
    m_int_configs[CONFIG_DAILY_QUEST_RESET_TIME_HOUR] = sConfigMgr->GetIntDefault("Quests.DailyResetTime", 3);

    m_int_configs[CONFIG_MAX_PRIMARY_TRADE_SKILL] = sConfigMgr->GetIntDefault("MaxPrimaryTradeSkill", 2);
    m_int_configs[CONFIG_MIN_PETITION_SIGNS] = sConfigMgr->GetIntDefault("MinPetitionSigns", 9);
    if (m_int_configs[CONFIG_MIN_PETITION_SIGNS] > 9)
    {
        TC_LOG_ERROR("server.loading", "MinPetitionSigns (%i) must be in range 0..9. Set to 9.", m_int_configs[CONFIG_MIN_PETITION_SIGNS]);
        m_int_configs[CONFIG_MIN_PETITION_SIGNS] = 9;
    }
    rate_values[RATE_PVP_RANK_EXTRA_HONOR] = sConfigMgr->GetFloatDefault("PvPRank.Rate.ExtraHonor", 1);
    std::string s_pvp_ranks = sConfigMgr->GetStringDefault("PvPRank.HKPerRank", "10,50,100,200,450,750,1300,2000,3500,6000,9500,15000,21000,30000");
    char *c_pvp_ranks = const_cast<char*>(s_pvp_ranks.c_str());
    for (int i = 0; i != HKRANKMAX; i++)
    {
        if (i == 0)
            pvp_ranks[0] = 0;
        else if (i == 1)
            pvp_ranks[1] = atoi(strtok(c_pvp_ranks, ","));
        else
            pvp_ranks[i] = atoi(strtok(NULL, ","));
    }

    m_int_configs[CONFIG_GM_LOGIN_STATE] = sConfigMgr->GetIntDefault("GM.LoginState", 2);
    m_int_configs[CONFIG_GM_VISIBLE_STATE] = sConfigMgr->GetIntDefault("GM.Visible", 2);
    m_int_configs[CONFIG_GM_CHAT] = sConfigMgr->GetIntDefault("GM.Chat", 2);
    m_int_configs[CONFIG_GM_WHISPERING_TO] = sConfigMgr->GetIntDefault("GM.WhisperingTo", 2);
    m_int_configs[CONFIG_GM_FREEZE_DURATION] = sConfigMgr->GetIntDefault("GM.FreezeAuraDuration", 0);

    m_int_configs[CONFIG_GM_LEVEL_IN_GM_LIST] = sConfigMgr->GetIntDefault("GM.InGMList.Level", SEC_ADMINISTRATOR);
    m_int_configs[CONFIG_GM_LEVEL_IN_WHO_LIST] = sConfigMgr->GetIntDefault("GM.InWhoList.Level", SEC_ADMINISTRATOR);
    m_int_configs[CONFIG_START_GM_LEVEL] = sConfigMgr->GetIntDefault("GM.StartLevel", 1);
    if (m_int_configs[CONFIG_START_GM_LEVEL] < m_int_configs[CONFIG_START_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "GM.StartLevel (%i) must be in range StartPlayerLevel(%u)..%u. Set to %u.",
            m_int_configs[CONFIG_START_GM_LEVEL], m_int_configs[CONFIG_START_PLAYER_LEVEL], MAX_LEVEL, m_int_configs[CONFIG_START_PLAYER_LEVEL]);
        m_int_configs[CONFIG_START_GM_LEVEL] = m_int_configs[CONFIG_START_PLAYER_LEVEL];
    }
    else if (m_int_configs[CONFIG_START_GM_LEVEL] > MAX_LEVEL)
    {
        TC_LOG_ERROR("server.loading", "GM.StartLevel (%i) must be in range 1..%u. Set to %u.", m_int_configs[CONFIG_START_GM_LEVEL], MAX_LEVEL, MAX_LEVEL);
        m_int_configs[CONFIG_START_GM_LEVEL] = MAX_LEVEL;
    }
    m_bool_configs[CONFIG_ALLOW_GM_GROUP] = sConfigMgr->GetBoolDefault("GM.AllowInvite", false);
    m_bool_configs[CONFIG_GM_LOWER_SECURITY] = sConfigMgr->GetBoolDefault("GM.LowerSecurity", false);
    m_float_configs[CONFIG_CHANCE_OF_GM_SURVEY] = sConfigMgr->GetFloatDefault("GM.TicketSystem.ChanceOfGMSurvey", 50.0f);
    m_int_configs[CONFIG_FORCE_SHUTDOWN_THRESHOLD] = sConfigMgr->GetIntDefault("GM.ForceShutdownThreshold", 30);

    m_int_configs[CONFIG_GROUP_VISIBILITY] = sConfigMgr->GetIntDefault("Visibility.GroupMode", 1);

    m_int_configs[CONFIG_MAIL_DELIVERY_DELAY] = sConfigMgr->GetIntDefault("MailDeliveryDelay", HOUR);
    m_int_configs[CONFIG_CLEAN_OLD_MAIL_TIME] = sConfigMgr->GetIntDefault("CleanOldMailTime", 4);
    if (m_int_configs[CONFIG_CLEAN_OLD_MAIL_TIME] > 23)
    {
        TC_LOG_ERROR("server.loading", "CleanOldMailTime (%u) must be an hour, between 0 and 23. Set to 4.", m_int_configs[CONFIG_CLEAN_OLD_MAIL_TIME]);
        m_int_configs[CONFIG_CLEAN_OLD_MAIL_TIME] = 4;
    }

    m_int_configs[CONFIG_UPTIME_UPDATE] = sConfigMgr->GetIntDefault("UpdateUptimeInterval", 10);
    if (int32(m_int_configs[CONFIG_UPTIME_UPDATE]) <= 0)
    {
        TC_LOG_ERROR("server.loading", "UpdateUptimeInterval (%i) must be > 0, set to default 10.", m_int_configs[CONFIG_UPTIME_UPDATE]);
        m_int_configs[CONFIG_UPTIME_UPDATE] = 10;
    }
    if (reload)
    {
        m_timers[WUPDATE_UPTIME].SetInterval(m_int_configs[CONFIG_UPTIME_UPDATE] * MINUTE*IN_MILLISECONDS);
        m_timers[WUPDATE_UPTIME].Reset();
    }

    // log db cleanup interval
    m_int_configs[CONFIG_LOGDB_CLEARINTERVAL] = sConfigMgr->GetIntDefault("LogDB.Opt.ClearInterval", 10);
    if (int32(m_int_configs[CONFIG_LOGDB_CLEARINTERVAL]) <= 0)
    {
        TC_LOG_ERROR("server.loading", "LogDB.Opt.ClearInterval (%i) must be > 0, set to default 10.", m_int_configs[CONFIG_LOGDB_CLEARINTERVAL]);
        m_int_configs[CONFIG_LOGDB_CLEARINTERVAL] = 10;
    }
    if (reload)
    {
        m_timers[WUPDATE_CLEANDB].SetInterval(m_int_configs[CONFIG_LOGDB_CLEARINTERVAL] * MINUTE * IN_MILLISECONDS);
        m_timers[WUPDATE_CLEANDB].Reset();
    }
    m_int_configs[CONFIG_LOGDB_CLEARTIME] = sConfigMgr->GetIntDefault("LogDB.Opt.ClearTime", 1209600); // 14 days default
    TC_LOG_INFO("server.loading", "Will clear `logs` table of entries older than %i seconds every %u minutes.",
        m_int_configs[CONFIG_LOGDB_CLEARTIME], m_int_configs[CONFIG_LOGDB_CLEARINTERVAL]);

    m_int_configs[CONFIG_SKILL_CHANCE_ORANGE] = sConfigMgr->GetIntDefault("SkillChance.Orange", 100);
    m_int_configs[CONFIG_SKILL_CHANCE_YELLOW] = sConfigMgr->GetIntDefault("SkillChance.Yellow", 75);
    m_int_configs[CONFIG_SKILL_CHANCE_GREEN] = sConfigMgr->GetIntDefault("SkillChance.Green", 25);
    m_int_configs[CONFIG_SKILL_CHANCE_GREY] = sConfigMgr->GetIntDefault("SkillChance.Grey", 0);

    m_int_configs[CONFIG_SKILL_CHANCE_MINING_STEPS] = sConfigMgr->GetIntDefault("SkillChance.MiningSteps", 75);
    m_int_configs[CONFIG_SKILL_CHANCE_SKINNING_STEPS] = sConfigMgr->GetIntDefault("SkillChance.SkinningSteps", 75);

    m_bool_configs[CONFIG_SKILL_PROSPECTING] = sConfigMgr->GetBoolDefault("SkillChance.Prospecting", false);
    m_bool_configs[CONFIG_SKILL_MILLING] = sConfigMgr->GetBoolDefault("SkillChance.Milling", false);

    m_int_configs[CONFIG_SKILL_GAIN_CRAFTING] = sConfigMgr->GetIntDefault("SkillGain.Crafting", 1);

    m_int_configs[CONFIG_SKILL_GAIN_DEFENSE] = sConfigMgr->GetIntDefault("SkillGain.Defense", 1);

    m_int_configs[CONFIG_SKILL_GAIN_GATHERING] = sConfigMgr->GetIntDefault("SkillGain.Gathering", 1);

    m_int_configs[CONFIG_SKILL_GAIN_WEAPON] = sConfigMgr->GetIntDefault("SkillGain.Weapon", 1);

    m_int_configs[CONFIG_MAX_OVERSPEED_PINGS] = sConfigMgr->GetIntDefault("MaxOverspeedPings", 2);
    if (m_int_configs[CONFIG_MAX_OVERSPEED_PINGS] != 0 && m_int_configs[CONFIG_MAX_OVERSPEED_PINGS] < 2)
    {
        TC_LOG_ERROR("server.loading", "MaxOverspeedPings (%i) must be in range 2..infinity (or 0 to disable check). Set to 2.", m_int_configs[CONFIG_MAX_OVERSPEED_PINGS]);
        m_int_configs[CONFIG_MAX_OVERSPEED_PINGS] = 2;
    }

    m_bool_configs[CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY] = sConfigMgr->GetBoolDefault("SaveRespawnTimeImmediately", true);
    if (!m_bool_configs[CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY])
    {
        TC_LOG_WARN("server.loading", "SaveRespawnTimeImmediately triggers assertions when disabled, overridden to Enabled");
        m_bool_configs[CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY] = true;
    }

    m_bool_configs[CONFIG_WEATHER] = sConfigMgr->GetBoolDefault("ActivateWeather", true);

    m_int_configs[CONFIG_DISABLE_BREATHING] = sConfigMgr->GetIntDefault("DisableWaterBreath", SEC_CONSOLE);

    m_bool_configs[CONFIG_ALWAYS_MAX_SKILL_FOR_LEVEL] = sConfigMgr->GetBoolDefault("AlwaysMaxSkillForLevel", false);

    if (reload)
    {
        uint32 val = sConfigMgr->GetIntDefault("Expansion", 2);
        if (val != m_int_configs[CONFIG_EXPANSION])
            TC_LOG_ERROR("server.loading", "Expansion option can't be changed at worldserver.conf reload, using current value (%u).", m_int_configs[CONFIG_EXPANSION]);
    }
    else
        m_int_configs[CONFIG_EXPANSION] = sConfigMgr->GetIntDefault("Expansion", 2);

    m_int_configs[CONFIG_CHATFLOOD_MESSAGE_COUNT] = sConfigMgr->GetIntDefault("ChatFlood.MessageCount", 10);
    m_int_configs[CONFIG_CHATFLOOD_MESSAGE_DELAY] = sConfigMgr->GetIntDefault("ChatFlood.MessageDelay", 1);
    m_int_configs[CONFIG_CHATFLOOD_MUTE_TIME] = sConfigMgr->GetIntDefault("ChatFlood.MuteTime", 10);

    m_bool_configs[CONFIG_EVENT_ANNOUNCE] = sConfigMgr->GetBoolDefault("Event.Announce", false);

    m_float_configs[CONFIG_CREATURE_FAMILY_FLEE_ASSISTANCE_RADIUS] = sConfigMgr->GetFloatDefault("CreatureFamilyFleeAssistanceRadius", 30.0f);
    m_float_configs[CONFIG_CREATURE_FAMILY_ASSISTANCE_RADIUS] = sConfigMgr->GetFloatDefault("CreatureFamilyAssistanceRadius", 10.0f);
    m_int_configs[CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY] = sConfigMgr->GetIntDefault("CreatureFamilyAssistanceDelay", 1500);
    m_int_configs[CONFIG_CREATURE_FAMILY_FLEE_DELAY] = sConfigMgr->GetIntDefault("CreatureFamilyFleeDelay", 7000);

    m_int_configs[CONFIG_WORLD_BOSS_LEVEL_DIFF] = sConfigMgr->GetIntDefault("WorldBossLevelDiff", 3);

    m_bool_configs[CONFIG_QUEST_ENABLE_QUEST_TRACKER] = sConfigMgr->GetBoolDefault("Quests.EnableQuestTracker", false);

    // note: disable value (-1) will assigned as 0xFFFFFFF, to prevent overflow at calculations limit it to max possible player level MAX_LEVEL(100)
    m_int_configs[CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF] = sConfigMgr->GetIntDefault("Quests.LowLevelHideDiff", 4);
    if (m_int_configs[CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF] > MAX_LEVEL)
        m_int_configs[CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF] = MAX_LEVEL;
    m_int_configs[CONFIG_QUEST_HIGH_LEVEL_HIDE_DIFF] = sConfigMgr->GetIntDefault("Quests.HighLevelHideDiff", 7);
    if (m_int_configs[CONFIG_QUEST_HIGH_LEVEL_HIDE_DIFF] > MAX_LEVEL)
        m_int_configs[CONFIG_QUEST_HIGH_LEVEL_HIDE_DIFF] = MAX_LEVEL;
    m_bool_configs[CONFIG_QUEST_IGNORE_RAID] = sConfigMgr->GetBoolDefault("Quests.IgnoreRaid", false);
    m_bool_configs[CONFIG_QUEST_IGNORE_AUTO_ACCEPT] = sConfigMgr->GetBoolDefault("Quests.IgnoreAutoAccept", false);
    m_bool_configs[CONFIG_QUEST_IGNORE_AUTO_COMPLETE] = sConfigMgr->GetBoolDefault("Quests.IgnoreAutoComplete", false);

    m_int_configs[CONFIG_RANDOM_BG_RESET_HOUR] = sConfigMgr->GetIntDefault("Battleground.Random.ResetHour", 6);
    if (m_int_configs[CONFIG_RANDOM_BG_RESET_HOUR] > 23)
    {
        TC_LOG_ERROR("server.loading", "Battleground.Random.ResetHour (%i) can't be load. Set to 6.", m_int_configs[CONFIG_RANDOM_BG_RESET_HOUR]);
        m_int_configs[CONFIG_RANDOM_BG_RESET_HOUR] = 6;
    }

    m_int_configs[CONFIG_GUILD_RESET_HOUR] = sConfigMgr->GetIntDefault("Guild.ResetHour", 6);
    if (m_int_configs[CONFIG_GUILD_RESET_HOUR] > 23)
    {
        TC_LOG_ERROR("misc", "Guild.ResetHour (%i) can't be load. Set to 6.", m_int_configs[CONFIG_GUILD_RESET_HOUR]);
        m_int_configs[CONFIG_GUILD_RESET_HOUR] = 6;
    }

    m_bool_configs[CONFIG_DETECT_POS_COLLISION] = sConfigMgr->GetBoolDefault("DetectPosCollision", true);

    m_bool_configs[CONFIG_RESTRICTED_LFG_CHANNEL] = sConfigMgr->GetBoolDefault("Channel.RestrictedLfg", true);
    m_int_configs[CONFIG_TALENTS_INSPECTING] = sConfigMgr->GetIntDefault("TalentsInspecting", 1);
    m_bool_configs[CONFIG_CHAT_FAKE_MESSAGE_PREVENTING] = sConfigMgr->GetBoolDefault("ChatFakeMessagePreventing", false);
    m_int_configs[CONFIG_CHAT_STRICT_LINK_CHECKING_SEVERITY] = sConfigMgr->GetIntDefault("ChatStrictLinkChecking.Severity", 0);
    m_int_configs[CONFIG_CHAT_STRICT_LINK_CHECKING_KICK] = sConfigMgr->GetIntDefault("ChatStrictLinkChecking.Kick", 0);

    m_int_configs[CONFIG_CORPSE_DECAY_NORMAL] = sConfigMgr->GetIntDefault("Corpse.Decay.NORMAL", 60);
    m_int_configs[CONFIG_CORPSE_DECAY_RARE] = sConfigMgr->GetIntDefault("Corpse.Decay.RARE", 300);
    m_int_configs[CONFIG_CORPSE_DECAY_ELITE] = sConfigMgr->GetIntDefault("Corpse.Decay.ELITE", 300);
    m_int_configs[CONFIG_CORPSE_DECAY_RAREELITE] = sConfigMgr->GetIntDefault("Corpse.Decay.RAREELITE", 300);
    m_int_configs[CONFIG_CORPSE_DECAY_WORLDBOSS] = sConfigMgr->GetIntDefault("Corpse.Decay.WORLDBOSS", 3600);

    m_int_configs[CONFIG_DEATH_SICKNESS_LEVEL] = sConfigMgr->GetIntDefault("Death.SicknessLevel", 11);
    m_bool_configs[CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVP] = sConfigMgr->GetBoolDefault("Death.CorpseReclaimDelay.PvP", true);
    m_bool_configs[CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVE] = sConfigMgr->GetBoolDefault("Death.CorpseReclaimDelay.PvE", true);
    m_bool_configs[CONFIG_DEATH_BONES_WORLD] = sConfigMgr->GetBoolDefault("Death.Bones.World", true);
    m_bool_configs[CONFIG_DEATH_BONES_BG_OR_ARENA] = sConfigMgr->GetBoolDefault("Death.Bones.BattlegroundOrArena", true);

    m_bool_configs[CONFIG_DIE_COMMAND_MODE] = sConfigMgr->GetBoolDefault("Die.Command.Mode", true);

    m_float_configs[CONFIG_THREAT_RADIUS] = sConfigMgr->GetFloatDefault("ThreatRadius", 60.0f);

    // always use declined names in the russian client
    m_bool_configs[CONFIG_DECLINED_NAMES_USED] =
        (m_int_configs[CONFIG_REALM_ZONE] == REALM_ZONE_RUSSIAN) ? true : sConfigMgr->GetBoolDefault("DeclinedNames", false);

    m_float_configs[CONFIG_LISTEN_RANGE_SAY] = sConfigMgr->GetFloatDefault("ListenRange.Say", 25.0f);
    m_float_configs[CONFIG_LISTEN_RANGE_TEXTEMOTE] = sConfigMgr->GetFloatDefault("ListenRange.TextEmote", 25.0f);
    m_float_configs[CONFIG_LISTEN_RANGE_YELL] = sConfigMgr->GetFloatDefault("ListenRange.Yell", 300.0f);

    m_bool_configs[CONFIG_BATTLEGROUND_CAST_DESERTER] = sConfigMgr->GetBoolDefault("Battleground.CastDeserter", true);
    m_bool_configs[CONFIG_BATTLEGROUND_QUEUE_ANNOUNCER_ENABLE] = sConfigMgr->GetBoolDefault("Battleground.QueueAnnouncer.Enable", false);
    m_bool_configs[CONFIG_BATTLEGROUND_QUEUE_ANNOUNCER_PLAYERONLY] = sConfigMgr->GetBoolDefault("Battleground.QueueAnnouncer.PlayerOnly", false);
    m_bool_configs[CONFIG_BATTLEGROUND_STORE_STATISTICS_ENABLE] = sConfigMgr->GetBoolDefault("Battleground.StoreStatistics.Enable", false);
    m_bool_configs[CONFIG_BATTLEGROUND_TRACK_DESERTERS] = sConfigMgr->GetBoolDefault("Battleground.TrackDeserters.Enable", false);
    m_int_configs[CONFIG_BATTLEGROUND_REPORT_AFK] = sConfigMgr->GetIntDefault("Battleground.ReportAFK", 3);
    if (m_int_configs[CONFIG_BATTLEGROUND_REPORT_AFK] < 1)
    {
        TC_LOG_ERROR("server.loading", "Battleground.ReportAFK (%d) must be >0. Using 3 instead.", m_int_configs[CONFIG_BATTLEGROUND_REPORT_AFK]);
        m_int_configs[CONFIG_BATTLEGROUND_REPORT_AFK] = 3;
    }
    if (m_int_configs[CONFIG_BATTLEGROUND_REPORT_AFK] > 9)
    {
        TC_LOG_ERROR("server.loading", "Battleground.ReportAFK (%d) must be <10. Using 3 instead.", m_int_configs[CONFIG_BATTLEGROUND_REPORT_AFK]);
        m_int_configs[CONFIG_BATTLEGROUND_REPORT_AFK] = 3;
    }
    m_int_configs[CONFIG_BATTLEGROUND_INVITATION_TYPE] = sConfigMgr->GetIntDefault("Battleground.InvitationType", 0);
    m_int_configs[CONFIG_BATTLEGROUND_PREMATURE_FINISH_TIMER] = sConfigMgr->GetIntDefault("Battleground.PrematureFinishTimer", 5 * MINUTE * IN_MILLISECONDS);
    m_int_configs[CONFIG_BATTLEGROUND_PREMADE_GROUP_WAIT_FOR_MATCH] = sConfigMgr->GetIntDefault("Battleground.PremadeGroupWaitForMatch", 30 * MINUTE * IN_MILLISECONDS);
    m_bool_configs[CONFIG_BG_XP_FOR_KILL] = sConfigMgr->GetBoolDefault("Battleground.GiveXPForKills", false);
    m_int_configs[CONFIG_ARENA_MAX_RATING_DIFFERENCE] = sConfigMgr->GetIntDefault("Arena.MaxRatingDifference", 150);
    m_int_configs[CONFIG_ARENA_RATING_DISCARD_TIMER] = sConfigMgr->GetIntDefault("Arena.RatingDiscardTimer", 10 * MINUTE * IN_MILLISECONDS);
    m_int_configs[CONFIG_ARENA_RATED_UPDATE_TIMER] = sConfigMgr->GetIntDefault("Arena.RatedUpdateTimer", 5 * IN_MILLISECONDS);
    m_bool_configs[CONFIG_ARENA_AUTO_DISTRIBUTE_POINTS] = sConfigMgr->GetBoolDefault("Arena.AutoDistributePoints", false);
    m_int_configs[CONFIG_ARENA_AUTO_DISTRIBUTE_INTERVAL_DAYS] = sConfigMgr->GetIntDefault("Arena.AutoDistributeInterval", 7);
    m_bool_configs[CONFIG_ARENA_QUEUE_ANNOUNCER_ENABLE] = sConfigMgr->GetBoolDefault("Arena.QueueAnnouncer.Enable", false);
    m_int_configs[CONFIG_ARENA_SEASON_ID] = sConfigMgr->GetIntDefault("Arena.ArenaSeason.ID", 1);
    m_int_configs[CONFIG_ARENA_START_RATING] = sConfigMgr->GetIntDefault("Arena.ArenaStartRating", 0);
    m_int_configs[CONFIG_ARENA_START_PERSONAL_RATING] = sConfigMgr->GetIntDefault("Arena.ArenaStartPersonalRating", 1000);
    m_int_configs[CONFIG_ARENA_START_MATCHMAKER_RATING] = sConfigMgr->GetIntDefault("Arena.ArenaStartMatchmakerRating", 1500);
    m_bool_configs[CONFIG_ARENA_SEASON_IN_PROGRESS] = sConfigMgr->GetBoolDefault("Arena.ArenaSeason.InProgress", true);
    m_bool_configs[CONFIG_ARENA_LOG_EXTENDED_INFO] = sConfigMgr->GetBoolDefault("ArenaLog.ExtendedInfo", false);
    m_float_configs[CONFIG_ARENA_WIN_RATING_MODIFIER_1] = sConfigMgr->GetFloatDefault("Arena.ArenaWinRatingModifier1", 48.0f);
    m_float_configs[CONFIG_ARENA_WIN_RATING_MODIFIER_2] = sConfigMgr->GetFloatDefault("Arena.ArenaWinRatingModifier2", 24.0f);
    m_float_configs[CONFIG_ARENA_LOSE_RATING_MODIFIER] = sConfigMgr->GetFloatDefault("Arena.ArenaLoseRatingModifier", 24.0f);
    m_float_configs[CONFIG_ARENA_MATCHMAKER_RATING_MODIFIER] = sConfigMgr->GetFloatDefault("Arena.ArenaMatchmakerRatingModifier", 24.0f);

    m_bool_configs[CONFIG_ARENA_1V1_ENABLE] = sConfigMgr->GetBoolDefault("Arena.1v1.Enable", true);
    m_bool_configs[CONFIG_ARENA_1V1_ANNOUNCER] = sConfigMgr->GetBoolDefault("Arena.1v1.Announcer", false);
    m_int_configs[CONFIG_ARENA_1V1_MIN_LEVEL] = sConfigMgr->GetIntDefault("Arena.1v1.MinLevel", 80);
    m_int_configs[CONFIG_ARENA_1V1_COSTS] = sConfigMgr->GetIntDefault("Arena.1v1.Costs", 400000);
    m_bool_configs[CONFIG_ARENA_1V1_VENDOR_RATING] = sConfigMgr->GetBoolDefault("Arena.1v1.VendorRating", false);
    m_float_configs[CONFIG_ARENA_1V1_ARENAPOINTS_MULTI] = sConfigMgr->GetFloatDefault("Arena.1v1.ArenaPointsMulti", 0.64f);
    m_bool_configs[CONFIG_ARENA_1V1_BLOCK_FORBIDDEN_TALENTS] = sConfigMgr->GetBoolDefault("Arena.1v1.BlockForbiddenTalents", true);

    m_bool_configs[CONFIG_OFFHAND_CHECK_AT_SPELL_UNLEARN] = sConfigMgr->GetBoolDefault("OffhandCheckAtSpellUnlearn", true);

    m_bool_configs[BATTLEGROUND_CROSSFACTION_ENABLED] = sConfigMgr->GetBoolDefault("CrossFactionSystem.Battlegrounds.enable", true);

    m_int_configs[CONFIG_CREATURE_PICKPOCKET_REFILL] = sConfigMgr->GetIntDefault("Creature.PickPocketRefillDelay", 10 * MINUTE);
    m_int_configs[CONFIG_CREATURE_STOP_FOR_PLAYER] = sConfigMgr->GetIntDefault("Creature.MovingStopTimeForPlayer", 3 * MINUTE * IN_MILLISECONDS);

    if (int32 clientCacheId = sConfigMgr->GetIntDefault("ClientCacheVersion", 0))
    {
        // overwrite DB/old value
        if (clientCacheId > 0)
            m_int_configs[CONFIG_CLIENTCACHE_VERSION] = clientCacheId;
        else
            TC_LOG_ERROR("server.loading", "ClientCacheVersion can't be negative %d, ignored.", clientCacheId);
    }
    TC_LOG_INFO("server.loading", "Client cache version set to: %u", m_int_configs[CONFIG_CLIENTCACHE_VERSION]);

    m_int_configs[CONFIG_GUILD_EVENT_LOG_COUNT] = sConfigMgr->GetIntDefault("Guild.EventLogRecordsCount", GUILD_EVENTLOG_MAX_RECORDS);
    if (m_int_configs[CONFIG_GUILD_EVENT_LOG_COUNT] > GUILD_EVENTLOG_MAX_RECORDS)
        m_int_configs[CONFIG_GUILD_EVENT_LOG_COUNT] = GUILD_EVENTLOG_MAX_RECORDS;
    m_int_configs[CONFIG_GUILD_BANK_EVENT_LOG_COUNT] = sConfigMgr->GetIntDefault("Guild.BankEventLogRecordsCount", GUILD_BANKLOG_MAX_RECORDS);
    if (m_int_configs[CONFIG_GUILD_BANK_EVENT_LOG_COUNT] > GUILD_BANKLOG_MAX_RECORDS)
        m_int_configs[CONFIG_GUILD_BANK_EVENT_LOG_COUNT] = GUILD_BANKLOG_MAX_RECORDS;

    //visibility on continents
    m_MaxVisibleDistanceOnContinents = sConfigMgr->GetFloatDefault("Visibility.Distance.Continents", DEFAULT_VISIBILITY_DISTANCE);
    if (m_MaxVisibleDistanceOnContinents < 45 * sWorld->getRate(RATE_CREATURE_AGGRO))
    {
        TC_LOG_ERROR("server.loading", "Visibility.Distance.Continents can't be less max aggro radius %f", 45 * sWorld->getRate(RATE_CREATURE_AGGRO));
        m_MaxVisibleDistanceOnContinents = 45 * sWorld->getRate(RATE_CREATURE_AGGRO);
    }
    else if (m_MaxVisibleDistanceOnContinents > MAX_VISIBILITY_DISTANCE)
    {
        TC_LOG_ERROR("server.loading", "Visibility.Distance.Continents can't be greater %f", MAX_VISIBILITY_DISTANCE);
        m_MaxVisibleDistanceOnContinents = MAX_VISIBILITY_DISTANCE;
    }

    //visibility in instances
    m_MaxVisibleDistanceInInstances = sConfigMgr->GetFloatDefault("Visibility.Distance.Instances", DEFAULT_VISIBILITY_INSTANCE);
    if (m_MaxVisibleDistanceInInstances < 45 * sWorld->getRate(RATE_CREATURE_AGGRO))
    {
        TC_LOG_ERROR("server.loading", "Visibility.Distance.Instances can't be less max aggro radius %f", 45 * sWorld->getRate(RATE_CREATURE_AGGRO));
        m_MaxVisibleDistanceInInstances = 45 * sWorld->getRate(RATE_CREATURE_AGGRO);
    }
    else if (m_MaxVisibleDistanceInInstances > MAX_VISIBILITY_DISTANCE)
    {
        TC_LOG_ERROR("server.loading", "Visibility.Distance.Instances can't be greater %f", MAX_VISIBILITY_DISTANCE);
        m_MaxVisibleDistanceInInstances = MAX_VISIBILITY_DISTANCE;
    }

    //visibility in BG/Arenas
    m_MaxVisibleDistanceInBGArenas = sConfigMgr->GetFloatDefault("Visibility.Distance.BGArenas", DEFAULT_VISIBILITY_BGARENAS);
    if (m_MaxVisibleDistanceInBGArenas < 45 * sWorld->getRate(RATE_CREATURE_AGGRO))
    {
        TC_LOG_ERROR("server.loading", "Visibility.Distance.BGArenas can't be less max aggro radius %f", 45 * sWorld->getRate(RATE_CREATURE_AGGRO));
        m_MaxVisibleDistanceInBGArenas = 45 * sWorld->getRate(RATE_CREATURE_AGGRO);
    }
    else if (m_MaxVisibleDistanceInBGArenas > MAX_VISIBILITY_DISTANCE)
    {
        TC_LOG_ERROR("server.loading", "Visibility.Distance.BGArenas can't be greater %f", MAX_VISIBILITY_DISTANCE);
        m_MaxVisibleDistanceInBGArenas = MAX_VISIBILITY_DISTANCE;
    }

    m_visibility_notify_periodOnContinents = sConfigMgr->GetIntDefault("Visibility.Notify.Period.OnContinents", DEFAULT_VISIBILITY_NOTIFY_PERIOD);
    m_visibility_notify_periodInInstances = sConfigMgr->GetIntDefault("Visibility.Notify.Period.InInstances", DEFAULT_VISIBILITY_NOTIFY_PERIOD);
    m_visibility_notify_periodInBGArenas = sConfigMgr->GetIntDefault("Visibility.Notify.Period.InBGArenas", DEFAULT_VISIBILITY_NOTIFY_PERIOD);

    ///- Load the CharDelete related config options
    m_int_configs[CONFIG_CHARDELETE_METHOD] = sConfigMgr->GetIntDefault("CharDelete.Method", 0);
    m_int_configs[CONFIG_CHARDELETE_MIN_LEVEL] = sConfigMgr->GetIntDefault("CharDelete.MinLevel", 0);
    m_int_configs[CONFIG_CHARDELETE_DEATH_KNIGHT_MIN_LEVEL] = sConfigMgr->GetIntDefault("CharDelete.DeathKnight.MinLevel", 0);
    m_int_configs[CONFIG_CHARDELETE_KEEP_DAYS] = sConfigMgr->GetIntDefault("CharDelete.KeepDays", 30);

    // No aggro from gray mobs
    m_int_configs[CONFIG_NO_GRAY_AGGRO_ABOVE] = sConfigMgr->GetIntDefault("NoGrayAggro.Above", 0);
    m_int_configs[CONFIG_NO_GRAY_AGGRO_BELOW] = sConfigMgr->GetIntDefault("NoGrayAggro.Below", 0);
    if (m_int_configs[CONFIG_NO_GRAY_AGGRO_ABOVE] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "NoGrayAggro.Above (%i) must be in range 0..%u. Set to %u.", m_int_configs[CONFIG_NO_GRAY_AGGRO_ABOVE], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_NO_GRAY_AGGRO_ABOVE] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }
    if (m_int_configs[CONFIG_NO_GRAY_AGGRO_BELOW] > m_int_configs[CONFIG_MAX_PLAYER_LEVEL])
    {
        TC_LOG_ERROR("server.loading", "NoGrayAggro.Below (%i) must be in range 0..%u. Set to %u.", m_int_configs[CONFIG_NO_GRAY_AGGRO_BELOW], m_int_configs[CONFIG_MAX_PLAYER_LEVEL], m_int_configs[CONFIG_MAX_PLAYER_LEVEL]);
        m_int_configs[CONFIG_NO_GRAY_AGGRO_BELOW] = m_int_configs[CONFIG_MAX_PLAYER_LEVEL];
    }
    if (m_int_configs[CONFIG_NO_GRAY_AGGRO_ABOVE] > 0 && m_int_configs[CONFIG_NO_GRAY_AGGRO_ABOVE] < m_int_configs[CONFIG_NO_GRAY_AGGRO_BELOW])
    {
        TC_LOG_ERROR("server.loading", "NoGrayAggro.Below (%i) cannot be greater than NoGrayAggro.Above (%i). Set to %i.", m_int_configs[CONFIG_NO_GRAY_AGGRO_BELOW], m_int_configs[CONFIG_NO_GRAY_AGGRO_ABOVE], m_int_configs[CONFIG_NO_GRAY_AGGRO_ABOVE]);
        m_int_configs[CONFIG_NO_GRAY_AGGRO_BELOW] = m_int_configs[CONFIG_NO_GRAY_AGGRO_ABOVE];
    }

    // Respawn Settings
    m_int_configs[CONFIG_RESPAWN_MINCHECKINTERVALMS] = sConfigMgr->GetIntDefault("Respawn.MinCheckIntervalMS", 5000);
    m_int_configs[CONFIG_RESPAWN_DYNAMICMODE] = sConfigMgr->GetIntDefault("Respawn.DynamicMode", 0);
    if (m_int_configs[CONFIG_RESPAWN_DYNAMICMODE] > 1)
    {
        TC_LOG_ERROR("server.loading", "Invalid value for Respawn.DynamicMode (%u). Set to 0.", m_int_configs[CONFIG_RESPAWN_DYNAMICMODE]);
        m_int_configs[CONFIG_RESPAWN_DYNAMICMODE] = 0;
    }
    m_bool_configs[CONFIG_RESPAWN_DYNAMIC_ESCORTNPC] = sConfigMgr->GetBoolDefault("Respawn.DynamicEscortNPC", false);
    m_int_configs[CONFIG_RESPAWN_GUIDWARNLEVEL] = sConfigMgr->GetIntDefault("Respawn.GuidWarnLevel", 12000000);
    if (m_int_configs[CONFIG_RESPAWN_GUIDWARNLEVEL] > 16777215)
    {
        TC_LOG_ERROR("server.loading", "Respawn.GuidWarnLevel (%u) cannot be greater than maximum GUID (16777215). Set to 12000000.", m_int_configs[CONFIG_RESPAWN_GUIDWARNLEVEL]);
        m_int_configs[CONFIG_RESPAWN_GUIDWARNLEVEL] = 12000000;
    }
    m_int_configs[CONFIG_RESPAWN_GUIDALERTLEVEL] = sConfigMgr->GetIntDefault("Respawn.GuidAlertLevel", 16000000);
    if (m_int_configs[CONFIG_RESPAWN_GUIDALERTLEVEL] > 16777215)
    {
        TC_LOG_ERROR("server.loading", "Respawn.GuidWarnLevel (%u) cannot be greater than maximum GUID (16777215). Set to 16000000.", m_int_configs[CONFIG_RESPAWN_GUIDALERTLEVEL]);
        m_int_configs[CONFIG_RESPAWN_GUIDALERTLEVEL] = 16000000;
    }
    m_int_configs[CONFIG_RESPAWN_RESTARTQUIETTIME] = sConfigMgr->GetIntDefault("Respawn.RestartQuietTime", 3);
    if (m_int_configs[CONFIG_RESPAWN_RESTARTQUIETTIME] > 23)
    {
        TC_LOG_ERROR("server.loading", "Respawn.RestartQuietTime (%u) must be an hour, between 0 and 23. Set to 3.", m_int_configs[CONFIG_RESPAWN_RESTARTQUIETTIME]);
        m_int_configs[CONFIG_RESPAWN_RESTARTQUIETTIME] = 3;
    }
    m_float_configs[CONFIG_RESPAWN_DYNAMICRATE_CREATURE] = sConfigMgr->GetFloatDefault("Respawn.DynamicRateCreature", 10.0f);
    if (m_float_configs[CONFIG_RESPAWN_DYNAMICRATE_CREATURE] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "Respawn.DynamicRateCreature (%f) must be positive. Set to 10.", m_float_configs[CONFIG_RESPAWN_DYNAMICRATE_CREATURE]);
        m_float_configs[CONFIG_RESPAWN_DYNAMICRATE_CREATURE] = 10.0f;
    }
    m_int_configs[CONFIG_RESPAWN_DYNAMICMINIMUM_CREATURE] = sConfigMgr->GetIntDefault("Respawn.DynamicMinimumCreature", 10);
    m_float_configs[CONFIG_RESPAWN_DYNAMICRATE_GAMEOBJECT] = sConfigMgr->GetFloatDefault("Respawn.DynamicRateGameObject", 10.0f);
    if (m_float_configs[CONFIG_RESPAWN_DYNAMICRATE_GAMEOBJECT] < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "Respawn.DynamicRateGameObject (%f) must be positive. Set to 10.", m_float_configs[CONFIG_RESPAWN_DYNAMICRATE_GAMEOBJECT]);
        m_float_configs[CONFIG_RESPAWN_DYNAMICRATE_GAMEOBJECT] = 10.0f;
    }
    m_int_configs[CONFIG_RESPAWN_DYNAMICMINIMUM_GAMEOBJECT] = sConfigMgr->GetIntDefault("Respawn.DynamicMinimumGameObject", 10);
    _guidWarningMsg = sConfigMgr->GetStringDefault("Respawn.WarningMessage", "There will be an unscheduled server restart at 03:00. The server will be available again shortly after.");
    _alertRestartReason = sConfigMgr->GetStringDefault("Respawn.AlertRestartReason", "Urgent Maintenance");
    m_int_configs[CONFIG_RESPAWN_GUIDWARNING_FREQUENCY] = sConfigMgr->GetIntDefault("Respawn.WarningFrequency", 1800);
    ///- Read the "Data" directory from the config file
    std::string dataPath = sConfigMgr->GetStringDefault("DataDir", "./");
    if (dataPath.empty() || (dataPath.at(dataPath.length() - 1) != '/' && dataPath.at(dataPath.length() - 1) != '\\'))
        dataPath.push_back('/');

#if TRINITY_PLATFORM == TRINITY_PLATFORM_UNIX || TRINITY_PLATFORM == TRINITY_PLATFORM_APPLE
    if (dataPath[0] == '~')
    {
        char const* home = getenv("HOME");
        if (home)
            dataPath.replace(0, 1, home);
    }
#endif

    if (reload)
    {
        if (dataPath != m_dataPath)
            TC_LOG_ERROR("server.loading", "DataDir option can't be changed at worldserver.conf reload, using current value (%s).", m_dataPath.c_str());
    }
    else
    {
        m_dataPath = dataPath;
        TC_LOG_INFO("server.loading", "Using DataDir %s", m_dataPath.c_str());
    }

    m_bool_configs[CONFIG_ENABLE_MMAPS] = sConfigMgr->GetBoolDefault("mmap.enablePathFinding", false);
    TC_LOG_INFO("server.loading", "WORLD: MMap data directory is: %smmaps", m_dataPath.c_str());

    m_bool_configs[CONFIG_VMAP_INDOOR_CHECK] = sConfigMgr->GetBoolDefault("vmap.enableIndoorCheck", 0);
    bool enableIndoor = sConfigMgr->GetBoolDefault("vmap.enableIndoorCheck", true);
    bool enableLOS = sConfigMgr->GetBoolDefault("vmap.enableLOS", true);
    bool enableHeight = sConfigMgr->GetBoolDefault("vmap.enableHeight", true);

    if (!enableHeight)
        TC_LOG_ERROR("server.loading", "VMap height checking disabled! Creatures movements and other various things WILL be broken! Expect no support.");

    VMAP::VMapFactory::createOrGetVMapManager()->setEnableLineOfSightCalc(enableLOS);
    VMAP::VMapFactory::createOrGetVMapManager()->setEnableHeightCalc(enableHeight);
    TC_LOG_INFO("server.loading", "VMap support included. LineOfSight: %i, getHeight: %i, indoorCheck: %i", enableLOS, enableHeight, enableIndoor);
    TC_LOG_INFO("server.loading", "VMap data directory is: %svmaps", m_dataPath.c_str());

    m_int_configs[CONFIG_MAX_WHO] = sConfigMgr->GetIntDefault("MaxWhoListReturns", 49);
    m_bool_configs[CONFIG_START_ALL_SPELLS] = sConfigMgr->GetBoolDefault("PlayerStart.AllSpells", false);
    m_int_configs[CONFIG_HONOR_AFTER_DUEL] = sConfigMgr->GetIntDefault("HonorPointsAfterDuel", 0);
    m_bool_configs[CONFIG_RESET_DUEL_COOLDOWNS] = sConfigMgr->GetBoolDefault("ResetDuelCooldowns", false);
    m_bool_configs[CONFIG_RESET_DUEL_HEALTH_MANA] = sConfigMgr->GetBoolDefault("ResetDuelHealthMana", false);
    m_bool_configs[CONFIG_START_ALL_EXPLORED] = sConfigMgr->GetBoolDefault("PlayerStart.MapsExplored", false);
    m_bool_configs[CONFIG_START_ALL_REP] = sConfigMgr->GetBoolDefault("PlayerStart.AllReputation", false);
    m_bool_configs[CONFIG_ALWAYS_MAXSKILL] = sConfigMgr->GetBoolDefault("AlwaysMaxWeaponSkill", false);
    m_bool_configs[CONFIG_PVP_TOKEN_ENABLE] = sConfigMgr->GetBoolDefault("PvPToken.Enable", false);
    m_int_configs[CONFIG_PVP_TOKEN_MAP_TYPE] = sConfigMgr->GetIntDefault("PvPToken.MapAllowType", 4);
    m_int_configs[CONFIG_PVP_TOKEN_ID] = sConfigMgr->GetIntDefault("PvPToken.ItemID", 29434);
    m_int_configs[CONFIG_PVP_TOKEN_COUNT] = sConfigMgr->GetIntDefault("PvPToken.ItemCount", 1);
    if (m_int_configs[CONFIG_PVP_TOKEN_COUNT] < 1)
        m_int_configs[CONFIG_PVP_TOKEN_COUNT] = 1;

    m_bool_configs[CONFIG_ALLOW_TRACK_BOTH_RESOURCES] = sConfigMgr->GetBoolDefault("AllowTrackBothResources", false);
    m_bool_configs[CONFIG_NO_RESET_TALENT_COST] = sConfigMgr->GetBoolDefault("NoResetTalentsCost", false);
    m_bool_configs[CONFIG_SHOW_KICK_IN_WORLD] = sConfigMgr->GetBoolDefault("ShowKickInWorld", false);
    m_bool_configs[CONFIG_SHOW_MUTE_IN_WORLD] = sConfigMgr->GetBoolDefault("ShowMuteInWorld", false);
    m_bool_configs[CONFIG_SHOW_BAN_IN_WORLD] = sConfigMgr->GetBoolDefault("ShowBanInWorld", false);
    m_int_configs[CONFIG_NUMTHREADS] = sConfigMgr->GetIntDefault("MapUpdate.Threads", 1);
    m_int_configs[CONFIG_MAX_RESULTS_LOOKUP_COMMANDS] = sConfigMgr->GetIntDefault("Command.LookupMaxResults", 0);

    // Warden
    m_bool_configs[CONFIG_WARDEN_ENABLED] = sConfigMgr->GetBoolDefault("Warden.Enabled", false);
    m_int_configs[CONFIG_WARDEN_NUM_MEM_CHECKS] = sConfigMgr->GetIntDefault("Warden.NumMemChecks", 3);
    m_int_configs[CONFIG_WARDEN_NUM_OTHER_CHECKS] = sConfigMgr->GetIntDefault("Warden.NumOtherChecks", 7);
    m_int_configs[CONFIG_WARDEN_CLIENT_BAN_DURATION] = sConfigMgr->GetIntDefault("Warden.BanDuration", 86400);
    m_int_configs[CONFIG_WARDEN_CLIENT_CHECK_HOLDOFF] = sConfigMgr->GetIntDefault("Warden.ClientCheckHoldOff", 30);
    m_int_configs[CONFIG_WARDEN_CLIENT_FAIL_ACTION] = sConfigMgr->GetIntDefault("Warden.ClientCheckFailAction", 0);
    m_int_configs[CONFIG_WARDEN_CLIENT_RESPONSE_DELAY] = sConfigMgr->GetIntDefault("Warden.ClientResponseDelay", 600);

    // Dungeon finder
    m_int_configs[CONFIG_LFG_OPTIONSMASK] = sConfigMgr->GetIntDefault("DungeonFinder.OptionsMask", 1);

    // DBC_ItemAttributes
    m_bool_configs[CONFIG_DBC_ENFORCE_ITEM_ATTRIBUTES] = sConfigMgr->GetBoolDefault("DBC.EnforceItemAttributes", true);

    // Accountpassword Secruity
    m_int_configs[CONFIG_ACC_PASSCHANGESEC] = sConfigMgr->GetIntDefault("Account.PasswordChangeSecurity", 0);

    // Random Battleground Rewards
    m_int_configs[CONFIG_BG_REWARD_WINNER_HONOR_FIRST] = sConfigMgr->GetIntDefault("Battleground.RewardWinnerHonorFirst", 30);
    m_int_configs[CONFIG_BG_REWARD_WINNER_ARENA_FIRST] = sConfigMgr->GetIntDefault("Battleground.RewardWinnerArenaFirst", 25);
    m_int_configs[CONFIG_BG_REWARD_WINNER_HONOR_LAST] = sConfigMgr->GetIntDefault("Battleground.RewardWinnerHonorLast", 15);
    m_int_configs[CONFIG_BG_REWARD_WINNER_ARENA_LAST] = sConfigMgr->GetIntDefault("Battleground.RewardWinnerArenaLast", 0);
    m_int_configs[CONFIG_BG_REWARD_LOSER_HONOR_FIRST] = sConfigMgr->GetIntDefault("Battleground.RewardLoserHonorFirst", 5);
    m_int_configs[CONFIG_BG_REWARD_LOSER_HONOR_LAST] = sConfigMgr->GetIntDefault("Battleground.RewardLoserHonorLast", 5);

    // Max instances per hour
    m_int_configs[CONFIG_MAX_INSTANCES_PER_HOUR] = sConfigMgr->GetIntDefault("AccountInstancesPerHour", 5);

    // Anounce reset of instance to whole party
    m_bool_configs[CONFIG_INSTANCES_RESET_ANNOUNCE] = sConfigMgr->GetBoolDefault("InstancesResetAnnounce", false);

    // AutoBroadcast
    m_bool_configs[CONFIG_AUTOBROADCAST] = sConfigMgr->GetBoolDefault("AutoBroadcast.On", false);
    m_int_configs[CONFIG_AUTOBROADCAST_CENTER] = sConfigMgr->GetIntDefault("AutoBroadcast.Center", 0);
    m_int_configs[CONFIG_AUTOBROADCAST_INTERVAL] = sConfigMgr->GetIntDefault("AutoBroadcast.Timer", 60000);
    if (reload)
    {
        m_timers[WUPDATE_AUTOBROADCAST].SetInterval(m_int_configs[CONFIG_AUTOBROADCAST_INTERVAL]);
        m_timers[WUPDATE_AUTOBROADCAST].Reset();
    }

    // MySQL ping time interval
    m_int_configs[CONFIG_DB_PING_INTERVAL] = sConfigMgr->GetIntDefault("MaxPingTime", 30);

    // misc
    m_bool_configs[CONFIG_PDUMP_NO_PATHS] = sConfigMgr->GetBoolDefault("PlayerDump.DisallowPaths", true);
    m_bool_configs[CONFIG_PDUMP_NO_OVERWRITE] = sConfigMgr->GetBoolDefault("PlayerDump.DisallowOverwrite", true);
    m_bool_configs[CONFIG_UI_QUESTLEVELS_IN_DIALOGS] = sConfigMgr->GetBoolDefault("UI.ShowQuestLevelsInDialogs", false);

    m_bool_configs[CONFIG_ANTICHEAT_ENABLE] = sConfigMgr->GetBoolDefault("Anticheat.Enable", true);
    m_int_configs[CONFIG_ANTICHEAT_REPORTS_INGAME_NOTIFICATION] = sConfigMgr->GetIntDefault("Anticheat.ReportsForIngameWarnings", 70);
    m_int_configs[CONFIG_ANTICHEAT_DETECTIONS_ENABLED] = sConfigMgr->GetIntDefault("Anticheat.DetectionsEnabled", 31);
    m_int_configs[CONFIG_ANTICHEAT_MAX_REPORTS_FOR_DAILY_REPORT] = sConfigMgr->GetIntDefault("Anticheat.MaxReportsForDailyReport", 70);
    m_float_configs[CONFIG_ANTICHEAT_SPEED_fACTOR] = sConfigMgr->GetFloatDefault("Anticheat.SpeedDetecionFactor", 1.25f);

    // Wintergrasp battlefield
    m_bool_configs[CONFIG_WINTERGRASP_ENABLE] = sConfigMgr->GetBoolDefault("Wintergrasp.Enable", false);
    m_int_configs[CONFIG_WINTERGRASP_PLR_MAX] = sConfigMgr->GetIntDefault("Wintergrasp.PlayerMax", 100);
    m_int_configs[CONFIG_WINTERGRASP_PLR_MIN] = sConfigMgr->GetIntDefault("Wintergrasp.PlayerMin", 0);
    m_int_configs[CONFIG_WINTERGRASP_PLR_MIN_LVL] = sConfigMgr->GetIntDefault("Wintergrasp.PlayerMinLvl", 77);
    m_int_configs[CONFIG_WINTERGRASP_BATTLETIME] = sConfigMgr->GetIntDefault("Wintergrasp.BattleTimer", 30);
    m_int_configs[CONFIG_WINTERGRASP_NOBATTLETIME] = sConfigMgr->GetIntDefault("Wintergrasp.NoBattleTimer", 150);
    m_int_configs[CONFIG_WINTERGRASP_RESTART_AFTER_CRASH] = sConfigMgr->GetIntDefault("Wintergrasp.CrashRestartTimer", 10);

    // Stats limits
    m_bool_configs[CONFIG_STATS_LIMITS_ENABLE] = sConfigMgr->GetBoolDefault("Stats.Limits.Enable", false);
    m_float_configs[CONFIG_STATS_LIMITS_DODGE] = sConfigMgr->GetFloatDefault("Stats.Limits.Dodge", 95.0f);
    m_float_configs[CONFIG_STATS_LIMITS_PARRY] = sConfigMgr->GetFloatDefault("Stats.Limits.Parry", 95.0f);
    m_float_configs[CONFIG_STATS_LIMITS_BLOCK] = sConfigMgr->GetFloatDefault("Stats.Limits.Block", 95.0f);
    m_float_configs[CONFIG_STATS_LIMITS_CRIT] = sConfigMgr->GetFloatDefault("Stats.Limits.Crit", 95.0f);

    //packet spoof punishment
    m_int_configs[CONFIG_PACKET_SPOOF_POLICY] = sConfigMgr->GetIntDefault("PacketSpoof.Policy", (uint32)WorldSession::DosProtection::POLICY_KICK);
    m_int_configs[CONFIG_PACKET_SPOOF_BANMODE] = sConfigMgr->GetIntDefault("PacketSpoof.BanMode", (uint32)BAN_ACCOUNT);
    if (m_int_configs[CONFIG_PACKET_SPOOF_BANMODE] == BAN_CHARACTER || m_int_configs[CONFIG_PACKET_SPOOF_BANMODE] > BAN_IP)
        m_int_configs[CONFIG_PACKET_SPOOF_BANMODE] = BAN_ACCOUNT;

    m_int_configs[CONFIG_PACKET_SPOOF_BANDURATION] = sConfigMgr->GetIntDefault("PacketSpoof.BanDuration", 86400);

    m_int_configs[CONFIG_BIRTHDAY_TIME] = sConfigMgr->GetIntDefault("BirthdayTime", 1222964635);

    m_bool_configs[CONFIG_IP_BASED_ACTION_LOGGING] = sConfigMgr->GetBoolDefault("Allow.IP.Based.Action.Logging", false);

    // AHBot
    m_int_configs[CONFIG_AHBOT_UPDATE_INTERVAL] = sConfigMgr->GetIntDefault("AuctionHouseBot.Update.Interval", 20);

    m_bool_configs[CONFIG_CALCULATE_CREATURE_ZONE_AREA_DATA] = sConfigMgr->GetBoolDefault("Calculate.Creature.Zone.Area.Data", false);
    m_bool_configs[CONFIG_CALCULATE_GAMEOBJECT_ZONE_AREA_DATA] = sConfigMgr->GetBoolDefault("Calculate.Gameoject.Zone.Area.Data", false);

    // HotSwap
    m_bool_configs[CONFIG_HOTSWAP_ENABLED] = sConfigMgr->GetBoolDefault("HotSwap.Enabled", true);
    m_bool_configs[CONFIG_HOTSWAP_RECOMPILER_ENABLED] = sConfigMgr->GetBoolDefault("HotSwap.EnableReCompiler", true);
    m_bool_configs[CONFIG_HOTSWAP_EARLY_TERMINATION_ENABLED] = sConfigMgr->GetBoolDefault("HotSwap.EnableEarlyTermination", true);
    m_bool_configs[CONFIG_HOTSWAP_BUILD_FILE_RECREATION_ENABLED] = sConfigMgr->GetBoolDefault("HotSwap.EnableBuildFileRecreation", true);
    m_bool_configs[CONFIG_HOTSWAP_INSTALL_ENABLED] = sConfigMgr->GetBoolDefault("HotSwap.EnableInstall", true);
    m_bool_configs[CONFIG_HOTSWAP_PREFIX_CORRECTION_ENABLED] = sConfigMgr->GetBoolDefault("HotSwap.EnablePrefixCorrection", true);

    // prevent character rename on character customization
    m_bool_configs[CONFIG_PREVENT_RENAME_CUSTOMIZATION] = sConfigMgr->GetBoolDefault("PreventRenameCharacterOnCustomization", false);

    // Allow 5-man parties to use raid warnings
    m_bool_configs[CONFIG_CHAT_PARTY_RAID_WARNINGS] = sConfigMgr->GetBoolDefault("PartyRaidWarnings", false);

    // Allow to cache data queries
    m_bool_configs[CONFIG_CACHE_DATA_QUERIES] = sConfigMgr->GetBoolDefault("CacheDataQueries", true);


    // LoyaltyNPC Points
    m_int_configs[CONFIG_LOYALTY_NPC_POINTS] = sConfigMgr->GetIntDefault("LoyaltyPoints.NPC", 20);
    m_int_configs[CONFIG_LOYALTY_1ST_BG] = sConfigMgr->GetIntDefault("LoyaltyPoints.1st_bg", 10);
    m_int_configs[CONFIG_LOYALTY_REST_BG] = sConfigMgr->GetIntDefault("LoyaltyPoints.rest_bg", 5);
    m_int_configs[CONFIG_LOYALTY_1ST_ALEH] = sConfigMgr->GetIntDefault("LoyaltyPoints.1st_aleh", 10);
    m_int_configs[CONFIG_LOYALTY_REST_ALEH] = sConfigMgr->GetIntDefault("LoyaltyPoints.rest_aleh", 5);
    // Loyalty Shop
    m_int_configs[CONFIG_LOYALTY_SHOP_WORLDSTATE] = sConfigMgr->GetIntDefault("LoyaltyPoints.shop_mask", 0);

    // Tmorg 
    m_int_configs[CONFIG_TMORG_COST] = sConfigMgr->GetIntDefault("Tmorg.Cost", 300000);

    // Guild House
    m_int_configs[CONFIG_GUILD_HOUSE_TOKEN_OR_GOLD] = sConfigMgr->GetIntDefault("Guildhouse.TokenOrGold", 0);
    m_int_configs[CONFIG_GUILD_HOUSE_BUY_COST] = sConfigMgr->GetIntDefault("Guildhouse.BuyCost", 1000);
    m_int_configs[CONFIG_GUILD_HOUSE_SELL_COST] = sConfigMgr->GetIntDefault("Guildhouse.SellCost", 500);

    //PlayedTimedRewards
    m_int_configs[CONFIG_PLAYER_TIMED_REWARD_GLOBAL_INTERVAL] = sConfigMgr->GetIntDefault("PlayedTimeReward.Global.Interval", 0) * IN_MILLISECONDS;
    m_int_configs[CONFIG_PLAYER_TIMED_REWARD_MONEY_INTERVAL] = sConfigMgr->GetIntDefault("PlayedTimeReward.Money.Interval", 0) * IN_MILLISECONDS;
    m_int_configs[CONFIG_PLAYER_TIMED_REWARD_HONOR_INTERVAL] = sConfigMgr->GetIntDefault("PlayedTimeReward.Honor.Interval", 0) * IN_MILLISECONDS;
    m_int_configs[CONFIG_PLAYER_TIMED_REWARD_ARENA_INTERVAL] = sConfigMgr->GetIntDefault("PlayedTimeReward.Arena.Interval", 0) * IN_MILLISECONDS;
    m_int_configs[CONFIG_PLAYER_TIMED_REWARD_LOYALTY_POINTS_INTERVAL] = sConfigMgr->GetIntDefault("PlayedTimeReward.LoyaltyPoints.Interval", 0) * IN_MILLISECONDS;

    m_int_configs[CONFIG_PLAYER_TIMED_REWARD_MONEY_AMOUNT] = sConfigMgr->GetIntDefault("PlayedTimeReward.Money.Amount", 0);
    m_int_configs[CONFIG_PLAYER_TIMED_REWARD_HONOR_AMOUNT] = sConfigMgr->GetIntDefault("PlayedTimeReward.Honor.Amount", 0);
    m_int_configs[CONFIG_PLAYER_TIMED_REWARD_ARENA_AMOUNT] = sConfigMgr->GetIntDefault("PlayedTimeReward.Arena.Amount", 0);
    m_int_configs[CONFIG_PLAYER_TIMED_REWARD_LOYALTY_POINTS_AMOUNT] = sConfigMgr->GetIntDefault("PlayedTimeReward.LoyaltyPoints.Amount", 0);


    //Guild-Level-System
    LoadGuildBonusInfo();

    // Whether to use LoS from game objects
    m_bool_configs[CONFIG_CHECK_GOBJECT_LOS] = sConfigMgr->GetBoolDefault("CheckGameObjectLoS", true);

    // call ScriptMgr if we're reloading the configuration
    if (reload)
        sScriptMgr->OnConfigLoad(reload);

    // IRC Configurations.
    int ConfCnt = 0;
    sIRC->_chan_count = 0;
    if (sConfigMgr->GetIntDefault("irc.active", 1) == 1)
        sIRC->Active = true;
    else
        sIRC->Active = false;

    sIRC->_Host = sConfigMgr->GetStringDefault("irc.host", "192.160.1.11");
    if (sIRC->_Host.size() > 0)
        ConfCnt++;
    sIRC->_Mver = "Version 4.1";
    sIRC->_Port = sConfigMgr->GetIntDefault("irc.port", 4000);
    sIRC->_User = sConfigMgr->GetStringDefault("irc.user", "TriniChat");
    sIRC->_Pass = sConfigMgr->GetStringDefault("irc.pass", "Services Password");
    sIRC->_Nick = sConfigMgr->GetStringDefault("irc.nick", "TriniChat");
    sIRC->_Auth = sConfigMgr->GetIntDefault("irc.auth", 0);
    sIRC->_Auth_Nick = sConfigMgr->GetStringDefault("irc.auth.nick", "AuthNick");
    sIRC->_ICC = sConfigMgr->GetStringDefault("irc.icc", "001");
    sIRC->_defchan = sConfigMgr->GetStringDefault("irc.defchan", "lobby");
    sIRC->_ldefc = sConfigMgr->GetIntDefault("irc.ldef", 0);
    sIRC->_wct = sConfigMgr->GetIntDefault("irc.wct", 30000);
    sIRC->ajoin = sConfigMgr->GetIntDefault("irc.ajoin", 0);
    sIRC->_staffLink = sConfigMgr->GetIntDefault("irc.staff_link", 1);
    sIRC->_staffChan = sConfigMgr->GetStringDefault("irc.staff_chan", "staff");
    sIRC->_bot_names = sConfigMgr->GetStringDefault("irc.ignore_bots", "");
    sIRC->ajchan = sConfigMgr->GetStringDefault("irc.ajchan", "world");
    sIRC->onlrslt = sConfigMgr->GetIntDefault("irc.online.result", 10);
    sIRC->BOTMASK = sConfigMgr->GetIntDefault("Botmask", 0);
    sIRC->TICMASK = sConfigMgr->GetIntDefault("Ticketmask", 0);
    sIRC->logfile = sConfigMgr->GetStringDefault("irc.logfile.prefix", "IRC_");
    sIRC->logmask = sConfigMgr->GetIntDefault("irc.logmask", 0);
    sIRC->logchan = sConfigMgr->GetStringDefault("irc.logchannel", "");
    sIRC->logchanpw = sConfigMgr->GetStringDefault("irc.logchannelpw", "");
    for (int i = 1; i < MAX_CONF_CHANNELS; i++)
    {
        std::ostringstream ss;
        ss << i;
        std::string ci = "irc.chan_" + ss.str();
        std::string pw = "irc.pass_" + ss.str();
        std::string t_chan = sConfigMgr->GetStringDefault(ci.c_str(), "");
        if (t_chan.size() > 0)
        {
            sIRC->_chan_count++;
            sIRC->_irc_chan[sIRC->_chan_count] = t_chan;
            sIRC->_irc_pass[sIRC->_chan_count] = sConfigMgr->GetStringDefault(pw.c_str(), t_chan.c_str());
            ci = "wow.chan_" + ss.str();
            sIRC->_wow_chan[sIRC->_chan_count] = sConfigMgr->GetStringDefault(ci.c_str(), t_chan.c_str());
        }
    }
    sIRC->JoinMsg = sConfigMgr->GetStringDefault("irc.joinmsg", "TriniChat $Ver for Trinitycore 3.3.x");
    sIRC->RstMsg = sConfigMgr->GetStringDefault("irc.rstmsg", "TriniChat Is Restarting, I Will Be Right Back!");
    sIRC->kikmsg = sConfigMgr->GetStringDefault("irc.kickmsg", "Do Not Kick Me Again, Severe Actions Will Be Taken!");

    // IRC LINES
    sIRC->ILINES[WOW_IRC] = sConfigMgr->GetStringDefault("chat.wow_irc", "\003<WoW>[\002$Name($Level)\002\003] $Msg");
    sIRC->ILINES[IRC_WOW] = sConfigMgr->GetStringDefault("chat.irc_wow", "\003<IRC>[$Name]: $Msg");
    sIRC->ILINES[JOIN_WOW] = sConfigMgr->GetStringDefault("chat.join_wow", "\00312>>\00304 $Name \003Joined The Channel!");
    sIRC->ILINES[JOIN_IRC] = sConfigMgr->GetStringDefault("chat.join_irc", "\003[$Name]: Has Joined IRC!");
    sIRC->ILINES[LEAVE_WOW] = sConfigMgr->GetStringDefault("chat.leave_wow", "\00312<<\00304 $Name \003Left The Channel!");
    sIRC->ILINES[LEAVE_IRC] = sConfigMgr->GetStringDefault("chat.leave_irc", "\003[$Name]: Has Left IRC!");
    sIRC->ILINES[CHANGE_NICK] = sConfigMgr->GetStringDefault("chat.change_nick", "\003<> $Name Is Now Known As $NewName!");

    // TriniChat Options
    sIRC->_MCA = sConfigMgr->GetIntDefault("irc.maxattempt", 10);
    sIRC->_autojoinkick = sConfigMgr->GetIntDefault("irc.autojoin_kick", 1);
    sIRC->_cmd_prefx = sConfigMgr->GetStringDefault("irc.command_prefix", ".");

    sIRC->_op_gm = sConfigMgr->GetIntDefault("irc.op_gm_login", 0);
    sIRC->_op_gm_lev = sConfigMgr->GetIntDefault("irc.op_gm_level", 3);

    // Misc Options
    sIRC->games = sConfigMgr->GetIntDefault("irc.fun.games", 0);
    sIRC->gmlog = sConfigMgr->GetIntDefault("irc.gmlog", 1);
    sIRC->Status = sConfigMgr->GetStringDefault("irc.StatusChannel", "");
    sIRC->Statuspw = sConfigMgr->GetStringDefault("irc.StatusChannelPW", "");
    sIRC->anchn = sConfigMgr->GetStringDefault("irc.AnnounceChannel", "");
    sIRC->anchnpw = sConfigMgr->GetStringDefault("irc.AnnounceChannelPW", "");
    sIRC->ticann = sConfigMgr->GetStringDefault("irc.Tickets", "");
    sIRC->ticannpw = sConfigMgr->GetStringDefault("irc.TicketsPW", "");
    sIRC->autoanc = sConfigMgr->GetIntDefault("irc.auto.announce", 30);
    sIRC->ojGM1 = sConfigMgr->GetStringDefault("irc.gm1", "[Moderator]");
    sIRC->ojGM2 = sConfigMgr->GetStringDefault("irc.gm2", "[GameMaster]");
    sIRC->ojGM3 = sConfigMgr->GetStringDefault("irc.gm3", "[Developer]");
    sIRC->ojGM4 = sConfigMgr->GetStringDefault("irc.gm4", "[Owner]");
    // REQUIRED GM LEVEL
    QueryResult result = WorldDatabase.PQuery("SELECT `Command`, `gmlevel` FROM `irc_commands` ORDER BY `Command`");
    if (result)
    {
        Field *fields = result->Fetch();
        for (uint64 i = 0; i < result->GetRowCount(); i++)
        {
            //TODO: ELSEIF? STRCMP?
            std::string command = fields[0].GetCString();
            uint32 gmlvl = fields[1].GetUInt32();
            if (command == "acct") sIRC->CACCT = gmlvl;
            if (command == "ban") sIRC->CBAN = gmlvl;
            if (command == "char") sIRC->CCHAN = gmlvl;
            if (command == "char") sIRC->CCHAR = gmlvl;
            if (command == "fun") sIRC->CFUN = gmlvl;
            if (command == "help") sIRC->CHELP = gmlvl;
            if (command == "inchan") sIRC->CINCHAN = gmlvl;
            if (command == "info") sIRC->CINFO = gmlvl;
            if (command == "item") sIRC->CITEM = gmlvl;
            if (command == "jail") sIRC->CJAIL = gmlvl;
            if (command == "kick") sIRC->CKICK = gmlvl;
            if (command == "kill") sIRC->_KILL = gmlvl;
            if (command == "level") sIRC->CLEVEL = gmlvl;
            if (command == "lookup") sIRC->CLOOKUP = gmlvl;
            if (command == "money") sIRC->CMONEY = gmlvl;
            if (command == "mute") sIRC->CMUTE = gmlvl;
            if (command == "online") sIRC->CONLINE = gmlvl;
            if (command == "pm") sIRC->CPM = gmlvl;
            if (command == "reconnect") sIRC->CRECONNECT = gmlvl;
            if (command == "reload") sIRC->CRELOAD = gmlvl;
            if (command == "restart") sIRC->CSHUTDOWN = gmlvl;
            if (command == "revive") sIRC->CREVIVE = gmlvl;
            if (command == "saveall") sIRC->CSAVEALL = gmlvl;
            if (command == "server") sIRC->CSERVERCMD = gmlvl;
            if (command == "shutdown") sIRC->CSHUTDOWN = gmlvl;
            if (command == "spell") sIRC->CSPELL = gmlvl;
            if (command == "sysmsg") sIRC->CSYSMSG = gmlvl;
            if (command == "tele") sIRC->CTELE = gmlvl;
            if (command == "top") sIRC->CTOP = gmlvl;
            if (command == "who") sIRC->CWHO = gmlvl;
            if (command == "player") sIRC->CPLAYER = gmlvl;
            result->NextRow();
        }
    }
    else
    {
        sIRC->CACCT = 3;
        sIRC->CBAN = 3;
        sIRC->CCHAN = 3;
        sIRC->CCHAR = 3;
        sIRC->CFUN = 3;
        sIRC->CHELP = 3;
        sIRC->CINCHAN = 3;
        sIRC->CINFO = 3;
        sIRC->CITEM = 3;
        sIRC->CJAIL = 3;
        sIRC->CKICK = 3;
        sIRC->_KILL = 3;
        sIRC->CLEVEL = 3;
        sIRC->CLOOKUP = 3;
        sIRC->CMONEY = 3;
        sIRC->CMUTE = 3;
        sIRC->CONLINE = 3;
        sIRC->CPM = 3;
        sIRC->CRECONNECT = 3;
        sIRC->CRELOAD = 3;
        sIRC->CREVIVE = 3;
        sIRC->CSAVEALL = 3;
        sIRC->CSERVERCMD = 3;
        sIRC->CSHUTDOWN = 3;
        sIRC->CSPELL = 3;
        sIRC->CSYSMSG = 3;
        sIRC->CTELE = 3;
        sIRC->CTOP = 3;
        sIRC->CWHO = 3;
    }
    }

/// Initialize the World
void World::SetInitialWorldSettings()
{
    ///- Server startup begin
    uint32 startupBegin = getMSTime();

    ///- Initialize the random number generator
    srand((unsigned int)time(nullptr));

    ///- Initialize detour memory management
    dtAllocSetCustom(dtCustomAlloc, dtCustomFree);

    ///- Initialize VMapManager function pointers (to untangle game/collision circular deps)
    if (VMAP::VMapManager2* vmmgr2 = dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager()))
    {
        vmmgr2->GetLiquidFlagsPtr = &GetLiquidFlags;
        vmmgr2->IsVMAPDisabledForPtr = &DisableMgr::IsVMAPDisabledFor;
    }

    ///- Initialize config settings
    LoadConfigSettings();

	//Initialize the news system
	ServerNews::LoadSettedNew();

    TC_LOG_ERROR("misc" "Loading TrinityCore configuration settings...", "");

    ///- Initialize Allowed Security Level
    LoadDBAllowedSecurityLevel();

    ///- Init highest guids before any table loading to prevent using not initialized guids in some code.
    sObjectMgr->SetHighestGuids();

    ///- Check the existence of the map files for all races' startup areas.
    if (!MapManager::ExistMapAndVMap(0, -6240.32f, 331.033f)
        || !MapManager::ExistMapAndVMap(0, -8949.95f, -132.493f)
        || !MapManager::ExistMapAndVMap(1, -618.518f, -4251.67f)
        || !MapManager::ExistMapAndVMap(0, 1676.35f, 1677.45f)
        || !MapManager::ExistMapAndVMap(1, 10311.3f, 832.463f)
        || !MapManager::ExistMapAndVMap(1, -2917.58f, -257.98f)
        || (m_int_configs[CONFIG_EXPANSION] && (
            !MapManager::ExistMapAndVMap(530, 10349.6f, -6357.29f) ||
            !MapManager::ExistMapAndVMap(530, -3961.64f, -13931.2f))))
    {
        TC_LOG_FATAL("server.loading", "Unable to load critical files - server shutting down !!!");
        exit(1);
    }

    ///- Initialize pool manager
    sPoolMgr->Initialize();

    ///- Initialize game event manager
    sGameEventMgr->Initialize();

    ///- Loading strings. Getting no records means core load has to be canceled because no error message can be output.

    TC_LOG_INFO("server.loading", "Loading Trinity strings...");
    if (!sObjectMgr->LoadTrinityStrings())
        exit(1);                                            // Error message displayed in function already

    ///- Update the realm entry in the database with the realm type from the config file
    //No SQL injection as values are treated as integers

    // not send custom type REALM_FFA_PVP to realm list
    uint32 server_type = IsFFAPvPRealm() ? uint32(REALM_TYPE_PVP) : getIntConfig(CONFIG_GAME_TYPE);
    uint32 realm_zone = getIntConfig(CONFIG_REALM_ZONE);

    LoginDatabase.PExecute("UPDATE realmlist SET icon = %u, timezone = %u WHERE id = '%d'", server_type, realm_zone, realm.Id.Realm);      // One-time query

    ///- Load the DBC files
    TC_LOG_INFO("server.loading", "Initialize data stores...");
    LoadDBCStores(m_dataPath);
    DetectDBCLang();

    // Load cinematic cameras
    LoadM2Cameras(m_dataPath);

    std::vector<uint32> mapIds;
    for (uint32 mapId = 0; mapId < sMapStore.GetNumRows(); mapId++)
        if (sMapStore.LookupEntry(mapId))
            mapIds.push_back(mapId);

    if (VMAP::VMapManager2* vmmgr2 = dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager()))
        vmmgr2->InitializeThreadUnsafe(mapIds);

    MMAP::MMapManager* mmmgr = MMAP::MMapFactory::createOrGetMMapManager();
    mmmgr->InitializeThreadUnsafe(mapIds);

    TC_LOG_INFO("server.loading", "Initializing PlayerDump tables...");
    PlayerDump::InitializeTables();

    ///- Initialize static helper structures
    AIRegistry::Initialize();

    TC_LOG_INFO("server.loading", "Loading SpellInfo store...");
    sSpellMgr->LoadSpellInfoStore();

    TC_LOG_INFO("server.loading", "Loading SpellInfo corrections...");
    sSpellMgr->LoadSpellInfoCorrections();

    TC_LOG_INFO("server.loading", "Loading SkillLineAbilityMultiMap Data...");
    sSpellMgr->LoadSkillLineAbilityMap();

    TC_LOG_INFO("server.loading", "Loading SpellInfo custom attributes...");
    sSpellMgr->LoadSpellInfoCustomAttributes();

    TC_LOG_INFO("server.loading", "Loading SpellInfo diminishing infos...");
    sSpellMgr->LoadSpellInfoDiminishing();

    TC_LOG_INFO("server.loading", "Loading SpellInfo immunity infos...");
    sSpellMgr->LoadSpellInfoImmunities();

    TC_LOG_INFO("server.loading", "Loading GameObject models...");
    LoadGameObjectModelList(m_dataPath);

    TC_LOG_INFO("server.loading", "Loading Script Names...");
    sObjectMgr->LoadScriptNames();

    TC_LOG_INFO("server.loading", "Loading Instance Template...");
    sObjectMgr->LoadInstanceTemplate();

    // Must be called before `creature_respawn`/`gameobject_respawn` tables
    TC_LOG_INFO("server.loading", "Loading instances...");
    sInstanceSaveMgr->LoadInstances();

    // Load before guilds and arena teams
    TC_LOG_INFO("server.loading", "Loading character cache store...");
    sCharacterCache->LoadCharacterCacheStorage();

    TC_LOG_INFO("server.loading", "Loading Broadcast texts...");
    sObjectMgr->LoadBroadcastTexts();
    sObjectMgr->LoadBroadcastTextLocales();

    TC_LOG_INFO("server.loading", "Loading Localization strings...");
    uint32 oldMSTime = getMSTime();
    sObjectMgr->LoadCreatureLocales();
    sObjectMgr->LoadGameObjectLocales();
    sObjectMgr->LoadItemLocales();
    sObjectMgr->LoadItemSetNameLocales();
    sObjectMgr->LoadQuestLocales();
    sObjectMgr->LoadNpcTextLocales();
    sObjectMgr->LoadPageTextLocales();
    sObjectMgr->LoadGossipMenuItemsLocales();
    sObjectMgr->LoadPointOfInterestLocales();
    sObjectMgr->LoadQuestGreetingsLocales();

    sObjectMgr->SetDBCLocaleIndex(GetDefaultDbcLocale());        // Get once for all the locale index of DBC language (console/broadcasts)
    TC_LOG_INFO("server.loading", ">> Localization strings loaded in %u ms", GetMSTimeDiffToNow(oldMSTime));

    TC_LOG_INFO("server.loading", "Loading Account Roles and Permissions...");
    sAccountMgr->LoadRBAC();

    TC_LOG_INFO("server.loading", "Loading Page Texts...");
    sObjectMgr->LoadPageTexts();

    TC_LOG_INFO("server.loading", "Loading Game Object Templates...");         // must be after LoadPageTexts
    sObjectMgr->LoadGameObjectTemplate();

    TC_LOG_INFO("server.loading", "Loading Game Object template addons...");
    sObjectMgr->LoadGameObjectTemplateAddons();

    TC_LOG_INFO("server.loading", "Loading Transport templates...");
    sTransportMgr->LoadTransportTemplates();

    TC_LOG_INFO("server.loading", "Loading Transport animations and rotations...");
    sTransportMgr->LoadTransportAnimationAndRotation();

    TC_LOG_INFO("server.loading", "Loading Spell Rank Data...");
    sSpellMgr->LoadSpellRanks();

    TC_LOG_INFO("server.loading", "Loading Spell Required Data...");
    sSpellMgr->LoadSpellRequired();

    TC_LOG_INFO("server.loading", "Loading Spell Group types...");
    sSpellMgr->LoadSpellGroups();

    TC_LOG_INFO("server.loading", "Loading Spell Learn Skills...");
    sSpellMgr->LoadSpellLearnSkills();                           // must be after LoadSpellRanks

    TC_LOG_INFO("server.loading", "Loading SpellInfo SpellSpecific and AuraState...");
    sSpellMgr->LoadSpellInfoSpellSpecificAndAuraState();         // must be after LoadSpellRanks

    TC_LOG_INFO("server.loading", "Loading Spell Learn Spells...");
    sSpellMgr->LoadSpellLearnSpells();

    TC_LOG_INFO("server.loading", "Loading Spell Proc conditions and data...");
    sSpellMgr->LoadSpellProcs();

    TC_LOG_INFO("server.loading", "Loading Spell Bonus Data...");
    sSpellMgr->LoadSpellBonusess();

    TC_LOG_INFO("server.loading", "Loading Aggro Spells Definitions...");
    sSpellMgr->LoadSpellThreats();

    TC_LOG_INFO("server.loading", "Loading Spell Group Stack Rules...");
    sSpellMgr->LoadSpellGroupStackRules();

    TC_LOG_INFO("server.loading", "Loading NPC Texts...");
    sObjectMgr->LoadGossipText();

    TC_LOG_INFO("server.loading", "Loading Enchant Spells Proc datas...");
    sSpellMgr->LoadSpellEnchantProcData();

    TC_LOG_INFO("server.loading", "Loading Item Random Enchantments Table...");
    LoadRandomEnchantmentsTable();

    TC_LOG_INFO("server.loading", "Loading Disables");                         // must be before loading quests and items
    DisableMgr::LoadDisables();

    TC_LOG_INFO("server.loading", "Loading Items...");                         // must be after LoadRandomEnchantmentsTable and LoadPageTexts
    sObjectMgr->LoadItemTemplates();

    TC_LOG_INFO("server.loading", "Loading Item set names...");                // must be after LoadItemPrototypes
    sObjectMgr->LoadItemSetNames();

    TC_LOG_INFO("server.loading", "Loading Creature Model Based Info Data...");
    sObjectMgr->LoadCreatureModelInfo();

    TC_LOG_INFO("server.loading", "Loading Creature templates...");
    sObjectMgr->LoadCreatureTemplates();

    TC_LOG_INFO("server.loading", "Loading Equipment templates...");           // must be after LoadCreatureTemplates
    sObjectMgr->LoadEquipmentTemplates();

    TC_LOG_INFO("server.loading", "Loading Creature template addons...");
    sObjectMgr->LoadCreatureTemplateAddons();

    TC_LOG_INFO("server.loading", "Loading Reputation Reward Rates...");
    sObjectMgr->LoadReputationRewardRate();

    TC_LOG_INFO("server.loading", "Loading Creature Reputation OnKill Data...");
    sObjectMgr->LoadReputationOnKill();

    TC_LOG_INFO("server.loading", "Loading Reputation Spillover Data...");
    sObjectMgr->LoadReputationSpilloverTemplate();

    TC_LOG_INFO("server.loading", "Loading Points Of Interest Data...");
    sObjectMgr->LoadPointsOfInterest();

    TC_LOG_INFO("server.loading", "Loading Creature Base Stats...");
    sObjectMgr->LoadCreatureClassLevelStats();

    TC_LOG_INFO("server.loading", "Loading Spawn Group Templates...");
    sObjectMgr->LoadSpawnGroupTemplates();

    TC_LOG_INFO("server.loading", "Loading instance spawn groups...");
    sObjectMgr->LoadInstanceSpawnGroups();

    TC_LOG_INFO("server.loading", "Loading Creature Data...");
    sObjectMgr->LoadCreatures();

    TC_LOG_INFO("server.loading", "Loading Temporary Summon Data...");
    sObjectMgr->LoadTempSummons();                               // must be after LoadCreatureTemplates() and LoadGameObjectTemplates()

    TC_LOG_INFO("server.loading", "Loading pet levelup spells...");
    sSpellMgr->LoadPetLevelupSpellMap();

    TC_LOG_INFO("server.loading", "Loading pet default spells additional to levelup spells...");
    sSpellMgr->LoadPetDefaultSpells();

    TC_LOG_INFO("server.loading", "Loading Creature Addon Data...");
    sObjectMgr->LoadCreatureAddons();                            // must be after LoadCreatureTemplates() and LoadCreatures()

    TC_LOG_INFO("server.loading", "Loading Gameobject Data...");
    sObjectMgr->LoadGameObjects();

    TC_LOG_INFO("server.loading", "Loading Spawn Group Data...");
    sObjectMgr->LoadSpawnGroups();

    TC_LOG_INFO("server.loading", "Loading GameObject Addon Data...");
    sObjectMgr->LoadGameObjectAddons();                          // must be after LoadGameObjectTemplate() and LoadGameObjects()

    TC_LOG_INFO("server.loading", "Loading GameObject Quest Items...");
    sObjectMgr->LoadGameObjectQuestItems();

    TC_LOG_INFO("server.loading", "Loading Creature Quest Items...");
    sObjectMgr->LoadCreatureQuestItems();

    TC_LOG_INFO("server.loading", "Loading Creature Linked Respawn...");
    sObjectMgr->LoadLinkedRespawn();                             // must be after LoadCreatures(), LoadGameObjects()

    TC_LOG_INFO("server.loading", "Loading Weather Data...");
    WeatherMgr::LoadWeatherData();

    TC_LOG_INFO("server.loading", "Loading Quests...");
    sObjectMgr->LoadQuests();                                    // must be loaded after DBCs, creature_template, item_template, gameobject tables

    TC_LOG_INFO("server.loading", "Checking Quest Disables");
    DisableMgr::CheckQuestDisables();                           // must be after loading quests

    TC_LOG_INFO("server.loading", "Loading Quest POI");
    sObjectMgr->LoadQuestPOI();

    TC_LOG_INFO("server.loading", "Loading Quests Starters and Enders...");
    sObjectMgr->LoadQuestStartersAndEnders();                    // must be after quest load

    TC_LOG_INFO("server.loading", "Loading Quests Greetings...");
    sObjectMgr->LoadQuestGreetings();                           // must be loaded after creature_template, gameobject_template tables

    TC_LOG_INFO("server.loading", "Loading Objects Pooling Data...");
    sPoolMgr->LoadFromDB();

    TC_LOG_INFO("server.loading", "Loading Game Event Data...");               // must be after loading pools fully
    sGameEventMgr->LoadHolidayDates();                           // Must be after loading DBC
    sGameEventMgr->LoadFromDB();                                 // Must be after loading holiday dates

    TC_LOG_INFO("server.loading", "Loading UNIT_NPC_FLAG_SPELLCLICK Data..."); // must be after LoadQuests
    sObjectMgr->LoadNPCSpellClickSpells();

    TC_LOG_INFO("server.loading", "Loading Vehicle Template Accessories...");
    sObjectMgr->LoadVehicleTemplateAccessories();                // must be after LoadCreatureTemplates() and LoadNPCSpellClickSpells()

    TC_LOG_INFO("server.loading", "Loading Vehicle Accessories...");
    sObjectMgr->LoadVehicleAccessories();                       // must be after LoadCreatureTemplates() and LoadNPCSpellClickSpells()

    TC_LOG_INFO("server.loading", "Loading SpellArea Data...");                // must be after quest load
    sSpellMgr->LoadSpellAreas();

    TC_LOG_INFO("server.loading", "Loading AreaTrigger definitions...");
    sObjectMgr->LoadAreaTriggerTeleports();

    TC_LOG_INFO("server.loading", "Loading Access Requirements...");
    sObjectMgr->LoadAccessRequirements();                        // must be after item template load

    TC_LOG_INFO("server.loading", "Loading Quest Area Triggers...");
    sObjectMgr->LoadQuestAreaTriggers();                         // must be after LoadQuests

    TC_LOG_INFO("server.loading", "Loading Tavern Area Triggers...");
    sObjectMgr->LoadTavernAreaTriggers();

    TC_LOG_INFO("server.loading", "Loading AreaTrigger script names...");
    sObjectMgr->LoadAreaTriggerScripts();

    TC_LOG_INFO("server.loading", "Loading LFG entrance positions..."); // Must be after areatriggers
    sLFGMgr->LoadLFGDungeons();

    TC_LOG_INFO("server.loading", "Loading Dungeon boss data...");
    sObjectMgr->LoadInstanceEncounters();

    TC_LOG_INFO("server.loading", "Loading LFG rewards...");
    sLFGMgr->LoadRewards();

    TC_LOG_INFO("server.loading", "Loading Graveyard-zone links...");
    sObjectMgr->LoadGraveyardZones();

    TC_LOG_INFO("server.loading", "Loading spell pet auras...");
    sSpellMgr->LoadSpellPetAuras();

    TC_LOG_INFO("server.loading", "Loading Spell target coordinates...");
    sSpellMgr->LoadSpellTargetPositions();

    TC_LOG_INFO("server.loading", "Loading enchant custom attributes...");
    sSpellMgr->LoadEnchantCustomAttr();

    TC_LOG_INFO("server.loading", "Loading linked spells...");
    sSpellMgr->LoadSpellLinked();

    TC_LOG_INFO("server.loading", "Loading Player Create Data...");
    sObjectMgr->LoadPlayerInfo();

    TC_LOG_INFO("server.loading", "Loading Exploration BaseXP Data...");
    sObjectMgr->LoadExplorationBaseXP();

    TC_LOG_INFO("server.loading", "Loading Pet Name Parts...");
    sObjectMgr->LoadPetNames();

    CharacterDatabaseCleaner::CleanDatabase();

    TC_LOG_INFO("server.loading", "Loading the max pet number...");
    sObjectMgr->LoadPetNumber();

    TC_LOG_INFO("server.loading", "Loading pet level stats...");
    sObjectMgr->LoadPetLevelInfo();

    TC_LOG_INFO("server.loading", "Loading Player level dependent mail rewards...");
    sObjectMgr->LoadMailLevelRewards();

    // Loot tables
    LoadLootTables();

    TC_LOG_INFO("server.loading", "Loading Skill Discovery Table...");
    LoadSkillDiscoveryTable();

    TC_LOG_INFO("server.loading", "Loading Skill Extra Item Table...");
    LoadSkillExtraItemTable();

    TC_LOG_INFO("server.loading", "Loading Skill Perfection Data Table...");
    LoadSkillPerfectItemTable();

    TC_LOG_INFO("server.loading", "Loading Skill Fishing base level requirements...");
    sObjectMgr->LoadFishingBaseSkillLevel();

    TC_LOG_INFO("server.loading", "Loading Achievements...");
    sAchievementMgr->LoadAchievementReferenceList();
    TC_LOG_INFO("server.loading", "Loading Achievement Criteria Lists...");
    sAchievementMgr->LoadAchievementCriteriaList();
    TC_LOG_INFO("server.loading", "Loading Achievement Criteria Data...");
    sAchievementMgr->LoadAchievementCriteriaData();
    TC_LOG_INFO("server.loading", "Loading Achievement Rewards...");
    sAchievementMgr->LoadRewards();
    TC_LOG_INFO("server.loading", "Loading Achievement Reward Locales...");
    sAchievementMgr->LoadRewardLocales();
    TC_LOG_INFO("server.loading", "Loading Completed Achievements...");
    sAchievementMgr->LoadCompletedAchievements();

    ///- Load dynamic data tables from the database
    TC_LOG_INFO("server.loading", "Loading Item Auctions...");
    sAuctionMgr->LoadAuctionItems();

    TC_LOG_INFO("server.loading", "Loading Auctions...");
    sAuctionMgr->LoadAuctions();

    TC_LOG_INFO("server.loading", "Loading Guilds...");
    sGuildMgr->LoadGuilds();

    TC_LOG_INFO("server.loading", "Loading ArenaTeams...");
    sArenaTeamMgr->LoadArenaTeams();

    TC_LOG_INFO("server.loading", "Loading Groups...");
    sGroupMgr->LoadGroups();

    TC_LOG_INFO("server.loading", "Loading ReservedNames...");
    sObjectMgr->LoadReservedPlayersNames();

    TC_LOG_INFO("server.loading", "Loading GameObjects for quests...");
    sObjectMgr->LoadGameObjectForQuests();

    TC_LOG_INFO("server.loading", "Loading BattleMasters...");
    sBattlegroundMgr->LoadBattleMastersEntry();                 // must be after load CreatureTemplate

    TC_LOG_INFO("server.loading", "Loading GameTeleports...");
    sObjectMgr->LoadGameTele();

    TC_LOG_INFO("server.loading", "Loading Gossip menu...");
    sObjectMgr->LoadGossipMenu();

    TC_LOG_INFO("server.loading", "Loading Gossip menu options...");
    sObjectMgr->LoadGossipMenuItems();

    TC_LOG_INFO("server.loading", "Loading Vendors...");
    sObjectMgr->LoadVendors();                                   // must be after load CreatureTemplate and ItemTemplate

    TC_LOG_INFO("server.loading", "Loading Trainers...");
    sObjectMgr->LoadTrainerSpell();                              // must be after load CreatureTemplate

    TC_LOG_INFO("server.loading", "Loading Waypoints...");
    sWaypointMgr->Load();

    TC_LOG_INFO("server.loading", "Loading SmartAI Waypoints...");
    sSmartWaypointMgr->LoadFromDB();

    TC_LOG_INFO("server.loading", "Loading Creature Formations...");
    sFormationMgr->LoadCreatureFormations();

    TC_LOG_INFO("server.loading", "Loading World States...");              // must be loaded before battleground, outdoor PvP and conditions
    LoadWorldStates();

    TC_LOG_INFO("server.loading", "Loading Conditions...");
    sConditionMgr->LoadConditions();

    TC_LOG_INFO("server.loading", "Loading faction change achievement pairs...");
    sObjectMgr->LoadFactionChangeAchievements();

    TC_LOG_INFO("server.loading", "Loading faction change spell pairs...");
    sObjectMgr->LoadFactionChangeSpells();

    TC_LOG_INFO("server.loading", "Loading faction change quest pairs...");
    sObjectMgr->LoadFactionChangeQuests();

    TC_LOG_INFO("server.loading", "Loading faction change item pairs...");
    sObjectMgr->LoadFactionChangeItems();

    TC_LOG_INFO("server.loading", "Loading faction change reputation pairs...");
    sObjectMgr->LoadFactionChangeReputations();

    TC_LOG_INFO("server.loading", "Loading faction change title pairs...");
    sObjectMgr->LoadFactionChangeTitles();

    TC_LOG_INFO("server.loading", "Loading GM tickets...");
    sTicketMgr->LoadTickets();

    TC_LOG_INFO("server.loading", "Loading GM surveys...");
    sTicketMgr->LoadSurveys();

    TC_LOG_INFO("server.loading", "Loading client addons...");
    AddonMgr::LoadFromDB();

    ///- Handle outdated emails (delete/return)
    TC_LOG_INFO("server.loading", "Returning old mails...");
    sObjectMgr->ReturnOrDeleteOldMails(false);

    TC_LOG_INFO("server.loading", "Loading Autobroadcasts...");
    LoadAutobroadcasts();

    ///- Load and initialize scripts
    sObjectMgr->LoadSpellScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sObjectMgr->LoadEventScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sObjectMgr->LoadWaypointScripts();

    TC_LOG_INFO("server.loading", "Loading spell script names...");
    sObjectMgr->LoadSpellScriptNames();

    TC_LOG_INFO("server.loading", "Loading Creature Texts...");
    sCreatureTextMgr->LoadCreatureTexts();

    TC_LOG_INFO("server.loading", "Loading Creature Text Locales...");
    sCreatureTextMgr->LoadCreatureTextLocales();

    TC_LOG_INFO("server.loading", "Initializing Scripts...");
    sScriptMgr->Initialize();
    sScriptMgr->OnConfigLoad(false);                                // must be done after the ScriptMgr has been properly initialized

    TC_LOG_INFO("server.loading", "Validating spell scripts...");
    sObjectMgr->ValidateSpellScripts();

    TC_LOG_INFO("server.loading", "Loading SmartAI scripts...");
    sSmartScriptMgr->LoadSmartAIFromDB();

    TC_LOG_INFO("server.loading", "Loading Calendar data...");
    sCalendarMgr->LoadFromDB();

    TC_LOG_INFO("server.loading", "Loading Petitions...");
    sPetitionMgr->LoadPetitions();

    TC_LOG_INFO("server.loading", "Loading Signatures...");
    sPetitionMgr->LoadSignatures();

    TC_LOG_INFO("server.loading", "Loading Item loot...");
    sLootItemStorage->LoadStorageFromDB();

    TC_LOG_INFO("server.loading", "Initialize query data...");
    sObjectMgr->InitializeQueriesData(QUERY_DATA_ALL);

    ///- Initialize game time and timers
    TC_LOG_INFO("server.loading", "Initialize game time and timers");
    GameTime::UpdateGameTimers();

    LoginDatabase.PExecute("INSERT INTO uptime (realmid, starttime, uptime, revision) VALUES(%u, %u, 0, '%s')",
        realm.Id.Realm, uint32(GameTime::GetStartTime()), GitRevision::GetFullVersion());       // One-time query

    static uint32 autoanc = 1;
    autoanc = sIRC->autoanc;

    m_timers[WUPDATE_WEATHERS].SetInterval(1 * IN_MILLISECONDS);
    m_timers[WUPDATE_AUCTIONS].SetInterval(MINUTE*IN_MILLISECONDS);
    m_timers[WUPDATE_AUCTIONS_PENDING].SetInterval(250);
    m_timers[WUPDATE_UPTIME].SetInterval(m_int_configs[CONFIG_UPTIME_UPDATE] * MINUTE*IN_MILLISECONDS);
    //Update "uptime" table based on configuration entry in minutes.
    m_timers[WUPDATE_CORPSES].SetInterval(20 * MINUTE * IN_MILLISECONDS);
    //erase corpses every 20 minutes
    m_timers[WUPDATE_CLEANDB].SetInterval(m_int_configs[CONFIG_LOGDB_CLEARINTERVAL] * MINUTE*IN_MILLISECONDS);
    // clean logs table every 14 days by default
    m_timers[WUPDATE_AUTOBROADCAST].SetInterval(getIntConfig(CONFIG_AUTOBROADCAST_INTERVAL));
    m_timers[WUPDATE_DELETECHARS].SetInterval(DAY*IN_MILLISECONDS); // check for chars to delete every day

    // for AhBot
    m_timers[WUPDATE_AHBOT].SetInterval(getIntConfig(CONFIG_AHBOT_UPDATE_INTERVAL) * IN_MILLISECONDS); // every 20 sec

    m_timers[WUPDATE_PINGDB].SetInterval(getIntConfig(CONFIG_DB_PING_INTERVAL)*MINUTE*IN_MILLISECONDS);    // Mysql ping time in minutes

    m_timers[WUPDATE_CHECK_FILECHANGES].SetInterval(500);

    m_timers[WUPDATE_WHO_LIST].SetInterval(5 * IN_MILLISECONDS); // update who list cache every 5 seconds

    m_timers[WUPDATE_AUTOANC].SetInterval(autoanc*MINUTE * 1000);

    //to set mailtimer to return mails every day between 4 and 5 am
    //mailtimer is increased when updating auctions
    //one second is 1000 -(tested on win system)
    /// @todo Get rid of magic numbers
    tm localTm;
    time_t gameTime = GameTime::GetGameTime();
    localtime_r(&gameTime, &localTm);
    uint8 CleanOldMailsTime = getIntConfig(CONFIG_CLEAN_OLD_MAIL_TIME);
    mail_timer = ((((localTm.tm_hour + (24 - CleanOldMailsTime)) % 24)* HOUR * IN_MILLISECONDS) / m_timers[WUPDATE_AUCTIONS].GetInterval());
    //1440
    mail_timer_expires = ((DAY * IN_MILLISECONDS) / (m_timers[WUPDATE_AUCTIONS].GetInterval()));
    TC_LOG_INFO("server.loading", "Mail timer set to: " UI64FMTD ", mail return is called every " UI64FMTD " minutes", uint64(mail_timer), uint64(mail_timer_expires));

    ///- Initialize MapManager
    TC_LOG_INFO("server.loading", "Starting Map System");
    sMapMgr->Initialize();

    TC_LOG_INFO("server.loading", "Starting Game Event system...");
    uint32 nextGameEvent = sGameEventMgr->StartSystem();
    m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);    //depend on next event

    // Delete all characters which have been deleted X days before
    Player::DeleteOldCharacters();

    TC_LOG_INFO("server.loading", "Initialize AuctionHouseBot...");
    sAuctionBot->Initialize();

    // Delete all custom channels which haven't been used for PreserveCustomChannelDuration days.
    Channel::CleanOldChannelsInDB();

    TC_LOG_INFO("server.loading", "Initializing Opcodes...");
    opcodeTable.Initialize();

    TC_LOG_INFO("server.loading", "Starting Arena Season...");
    sGameEventMgr->StartArenaSeason();

    sTicketMgr->Initialize();

    ///- Initialize Battlegrounds
    TC_LOG_INFO("server.loading", "Starting Battleground System");
    sBattlegroundMgr->LoadBattlegroundTemplates();
    sBattlegroundMgr->InitAutomaticArenaPointDistribution();

    ///- Initialize outdoor pvp
    TC_LOG_INFO("server.loading", "Starting Outdoor PvP System");
    sOutdoorPvPMgr->InitOutdoorPvP();

    ///- Initialize Battlefield
    TC_LOG_INFO("server.loading", "Starting Battlefield System");
    sBattlefieldMgr->InitBattlefield();

    TC_LOG_INFO("server.loading", "Loading Transports...");
    sTransportMgr->SpawnContinentTransports();

    ///- Initialize Warden
    TC_LOG_INFO("server.loading", "Loading Warden Checks...");
    sWardenCheckMgr->LoadWardenChecks();

    TC_LOG_INFO("server.loading", "Loading Warden Action Overrides...");
    sWardenCheckMgr->LoadWardenOverrides();

    TC_LOG_INFO("server.loading", "Deleting expired bans...");
    LoginDatabase.Execute("DELETE FROM ip_banned WHERE unbandate <= UNIX_TIMESTAMP() AND unbandate<>bandate");      // One-time query

    TC_LOG_INFO("server.loading", "Calculate next daily quest reset time...");
    InitDailyQuestResetTime();

    TC_LOG_INFO("server.loading", "Calculate next weekly quest reset time...");
    InitWeeklyQuestResetTime();

    TC_LOG_INFO("server.loading", "Calculate next monthly quest reset time...");
    InitMonthlyQuestResetTime();

    TC_LOG_INFO("server.loading", "Calculate random battleground reset time...");
    InitRandomBGResetTime();

    TC_LOG_INFO("server.loading", "Calculate guild limitation(s) reset time...");
    InitGuildResetTime();

    // Preload all cells, if required for the base maps
    if (sWorld->getBoolConfig(CONFIG_BASEMAP_LOAD_GRIDS))
    {
        sMapMgr->DoForAllMaps([](Map* map)
        {
            if (!map->Instanceable())
            {
                TC_LOG_INFO("server.loading", "Pre-loading base map data for map %u", map->GetId());
                map->LoadAllCells();
            }
        });
    }

    uint32 startupDuration = GetMSTimeDiffToNow(startupBegin);

    TC_LOG_INFO("server.worldserver", "World initialized in %u minutes %u seconds", (startupDuration / 60000), ((startupDuration % 60000) / 1000));

    TC_METRIC_EVENT("events", "World initialized", "World initialized in " + std::to_string(startupDuration / 60000) + " minutes " + std::to_string((startupDuration % 60000) / 1000) + " seconds");

    if (uint32 realmId = sConfigMgr->GetIntDefault("RealmID", 0)) // 0 reserved for auth
        sLog->SetRealmId(realmId);
}

void World::DetectDBCLang()
{
    uint8 m_lang_confid = sConfigMgr->GetIntDefault("DBC.Locale", 255);

    if (m_lang_confid != 255 && m_lang_confid >= TOTAL_LOCALES)
    {
        TC_LOG_ERROR("server.loading", "Incorrect DBC.Locale! Must be >= 0 and < %d (set to 0)", TOTAL_LOCALES);
        m_lang_confid = LOCALE_enUS;
    }

    ChrRacesEntry const* race = sChrRacesStore.LookupEntry(1);

    std::string availableLocalsStr;

    uint8 default_locale = TOTAL_LOCALES;
    for (uint8 i = default_locale - 1; i < TOTAL_LOCALES; --i)  // -1 will be 255 due to uint8
    {
        if (race->name[i][0] != '\0')                     // check by race names
        {
            default_locale = i;
            m_availableDbcLocaleMask |= (1 << i);
            availableLocalsStr += localeNames[i];
            availableLocalsStr += " ";
        }
    }

    if (default_locale != m_lang_confid && m_lang_confid < TOTAL_LOCALES &&
        (m_availableDbcLocaleMask & (1 << m_lang_confid)))
    {
        default_locale = m_lang_confid;
    }

    if (default_locale >= TOTAL_LOCALES)
    {
        TC_LOG_ERROR("server.loading", "Unable to determine your DBC Locale! (corrupt DBC?)");
        exit(1);
    }

    m_defaultDbcLocale = LocaleConstant(default_locale);

    TC_LOG_INFO("server.loading", "Using %s DBC Locale as default. All available DBC locales: %s", localeNames[m_defaultDbcLocale], availableLocalsStr.empty() ? "<none>" : availableLocalsStr.c_str());
}

void World::LoadAutobroadcasts()
{
    uint32 oldMSTime = getMSTime();

    m_Autobroadcasts.clear();
    m_AutobroadcastsWeights.clear();

    uint32 realmId = sConfigMgr->GetIntDefault("RealmID", 0);
    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_AUTOBROADCAST);
    stmt->setInt32(0, realmId);
    PreparedQueryResult result = LoginDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 autobroadcasts definitions. DB table `autobroadcast` is empty for this realm!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint8 id = fields[0].GetUInt8();

        m_Autobroadcasts[id] = fields[2].GetString();
        m_AutobroadcastsWeights[id] = fields[1].GetUInt8();

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u autobroadcast definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

/// Update the World !
void World::Update(uint32 diff)
{
    ///- Update the game time and check for shutdown time
    _UpdateGameTime();
    time_t currentGameTime = GameTime::GetGameTime();

    sWorldUpdateTime.UpdateWithDiff(diff);

    // Record update if recording set in log and diff is greater then minimum set in log
    sWorldUpdateTime.RecordUpdateTime(GameTime::GetGameTimeMS(), diff, GetActiveSessionCount());

    ///- Update the different timers
    for (int i = 0; i < WUPDATE_COUNT; ++i)
    {
        if (m_timers[i].GetCurrent() >= 0)
            m_timers[i].Update(diff);
        else
            m_timers[i].SetCurrent(0);
    }

    ///- Update Who List Storage
    if (m_timers[WUPDATE_WHO_LIST].Passed())
    {
        m_timers[WUPDATE_WHO_LIST].Reset();
        sWhoListStorageMgr->Update();
    }

    /// Handle daily quests reset time
    if (currentGameTime > m_NextDailyQuestReset)
    {
        ResetDailyQuests();
        InitDailyQuestResetTime(false);
    }

    /// Handle weekly quests reset time
    if (currentGameTime > m_NextWeeklyQuestReset)
        ResetWeeklyQuests();

    /// Handle monthly quests reset time
    if (currentGameTime > m_NextMonthlyQuestReset)
        ResetMonthlyQuests();

    if (currentGameTime > m_NextRandomBGReset)
        ResetRandomBG();

    if (currentGameTime > m_NextGuildReset)
        ResetGuildCap();

    /// <ul><li> Handle auctions when the timer has passed
    if (m_timers[WUPDATE_AUCTIONS].Passed())
    {
        m_timers[WUPDATE_AUCTIONS].Reset();

        ///- Update mails (return old mails with item, or delete them)
        //(tested... works on win)
        if (++mail_timer > mail_timer_expires)
        {
            mail_timer = 0;
            sObjectMgr->ReturnOrDeleteOldMails(true);
        }

        ///- Handle expired auctions
        sAuctionMgr->Update();
    }

    if (m_timers[WUPDATE_AUCTIONS_PENDING].Passed())
    {
        m_timers[WUPDATE_AUCTIONS_PENDING].Reset();

        sAuctionMgr->UpdatePendingAuctions();
    }

    /// <li> Handle AHBot operations
    if (m_timers[WUPDATE_AHBOT].Passed())
    {
        sAuctionBot->Update();
        m_timers[WUPDATE_AHBOT].Reset();
    }

    /// <li> Handle file changes
    if (m_timers[WUPDATE_CHECK_FILECHANGES].Passed())
    {
        sScriptReloadMgr->Update();
        m_timers[WUPDATE_CHECK_FILECHANGES].Reset();
    }

    /// <li> Handle session updates when the timer has passed
    sWorldUpdateTime.RecordUpdateTimeReset();
    UpdateSessions(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateSessions");

    /// <li> Handle weather updates when the timer has passed
    if (m_timers[WUPDATE_WEATHERS].Passed())
    {
        m_timers[WUPDATE_WEATHERS].Reset();
        WeatherMgr::Update(uint32(m_timers[WUPDATE_WEATHERS].GetInterval()));
    }

    /// <li> Update uptime table
    if (m_timers[WUPDATE_UPTIME].Passed())
    {
        uint32 tmpDiff = GameTime::GetUptime();
        uint32 maxOnlinePlayers = GetMaxPlayerCount();

        m_timers[WUPDATE_UPTIME].Reset();

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_UPTIME_PLAYERS);

        stmt->setUInt32(0, tmpDiff);
        stmt->setUInt16(1, uint16(maxOnlinePlayers));
        stmt->setUInt32(2, realm.Id.Realm);
        stmt->setUInt32(3, uint32(GameTime::GetStartTime()));

        LoginDatabase.Execute(stmt);
    }

    /// <li> Clean logs table
    if (sWorld->getIntConfig(CONFIG_LOGDB_CLEARTIME) > 0) // if not enabled, ignore the timer
    {
        if (m_timers[WUPDATE_CLEANDB].Passed())
        {
            m_timers[WUPDATE_CLEANDB].Reset();

            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_OLD_LOGS);

            stmt->setUInt32(0, sWorld->getIntConfig(CONFIG_LOGDB_CLEARTIME));
            stmt->setUInt32(1, uint32(time(0)));
            stmt->setUInt32(2, realm.Id.Realm);

            LoginDatabase.Execute(stmt);
        }
    }

    /// <li> Handle all other objects
    ///- Update objects when the timer has passed (maps, transport, creatures, ...)
    sWorldUpdateTime.RecordUpdateTimeReset();
    sMapMgr->Update(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateMapMgr");

    if (sWorld->getBoolConfig(CONFIG_AUTOBROADCAST))
    {
        if (m_timers[WUPDATE_AUTOBROADCAST].Passed())
        {
            m_timers[WUPDATE_AUTOBROADCAST].Reset();
            SendAutoBroadcast();
        }
    }

    sBattlegroundMgr->Update(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateBattlegroundMgr");

    sOutdoorPvPMgr->Update(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateOutdoorPvPMgr");

    sBattlefieldMgr->Update(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("BattlefieldMgr");

    ///- Delete all characters which have been deleted X days before
    if (m_timers[WUPDATE_DELETECHARS].Passed())
    {
        m_timers[WUPDATE_DELETECHARS].Reset();
        Player::DeleteOldCharacters();
    }

    sLFGMgr->Update(diff);
    sWorldUpdateTime.RecordUpdateTimeDuration("UpdateLFGMgr");

    // execute callbacks from sql queries that were queued recently
    ProcessQueryCallbacks();
    sWorldUpdateTime.RecordUpdateTimeDuration("ProcessQueryCallbacks");

    ///- Erase corpses once every 20 minutes
    if (m_timers[WUPDATE_CORPSES].Passed())
    {
        m_timers[WUPDATE_CORPSES].Reset();
        sMapMgr->DoForAllMaps([](Map* map)
        {
            map->RemoveOldCorpses();
        });
    }

    ///- Process Game events when necessary
    if (m_timers[WUPDATE_EVENTS].Passed())
    {
        m_timers[WUPDATE_EVENTS].Reset();                   // to give time for Update() to be processed
        uint32 nextGameEvent = sGameEventMgr->Update();
        m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);
        m_timers[WUPDATE_EVENTS].Reset();
    }

    ///- Ping to keep MySQL connections alive
    if (m_timers[WUPDATE_PINGDB].Passed())
    {
        m_timers[WUPDATE_PINGDB].Reset();
        TC_LOG_DEBUG("misc", "Ping MySQL to keep connection alive");
        CharacterDatabase.KeepAlive();
        LoginDatabase.KeepAlive();
        WorldDatabase.KeepAlive();
    }

    if (m_timers[WUPDATE_AUTOANC].Passed())
    {
        m_timers[WUPDATE_AUTOANC].Reset();
        SendRNDBroadcastIRC();
    }

    // update the instance reset times
    sInstanceSaveMgr->Update();

    // Check for shutdown warning
    if (_guidWarn && !_guidAlert)
    {
        _warnDiff += diff;
        if (GameTime::GetGameTime() >= _warnShutdownTime)
            DoGuidWarningRestart();
        else if (_warnDiff > getIntConfig(CONFIG_RESPAWN_GUIDWARNING_FREQUENCY) * IN_MILLISECONDS)
            SendGuidWarning();
    }

    // And last, but not least handle the issued cli commands
    ProcessCliCommands();

    sScriptMgr->OnWorldUpdate(diff);

    // Stats logger update
    sMetric->Update();
    TC_METRIC_VALUE("update_time_diff", diff);
}

void World::ForceGameEventUpdate()
{
    m_timers[WUPDATE_EVENTS].Reset();                   // to give time for Update() to be processed
    uint32 nextGameEvent = sGameEventMgr->Update();
    m_timers[WUPDATE_EVENTS].SetInterval(nextGameEvent);
    m_timers[WUPDATE_EVENTS].Reset();
}

/// Send a packet to all players (except self if mentioned)
void World::SendGlobalMessage(WorldPacket* packet, WorldSession* self, uint32 team)
{
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second &&
            itr->second->GetPlayer() &&
            itr->second->GetPlayer()->IsInWorld() &&
            itr->second != self &&
            (team == 0 || itr->second->GetPlayer()->GetTeam() == team))
        {
            itr->second->SendPacket(packet);
        }
    }
}

/// Send a packet to all GMs (except self if mentioned)
void World::SendGlobalGMMessage(WorldPacket* packet, WorldSession* self, uint32 team)
{
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        // check if session and can receive global GM Messages and its not self
        WorldSession* session = itr->second;
        if (!session || session == self || !session->HasPermission(rbac::RBAC_PERM_RECEIVE_GLOBAL_GM_TEXTMESSAGE))
            continue;

        // Player should be in world
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;

        // Send only to same team, if team is given
        if (!team || player->GetTeam() == team)
            session->SendPacket(packet);
    }
}

namespace Trinity
{
    class WorldWorldTextBuilder
    {
    public:
        typedef std::vector<WorldPacket*> WorldPacketList;
        explicit WorldWorldTextBuilder(uint32 textId, va_list* args = nullptr) : i_textId(textId), i_args(args) { }
        void operator()(WorldPacketList& data_list, LocaleConstant loc_idx)
        {
            char const* text = sObjectMgr->GetTrinityString(i_textId, loc_idx);

            if (i_args)
            {
                // we need copy va_list before use or original va_list will corrupted
                va_list ap;
                va_copy(ap, *i_args);

                char str[2048];
                vsnprintf(str, 2048, text, ap);
                va_end(ap);

                do_helper(data_list, &str[0]);
            }
            else
                do_helper(data_list, (char*)text);
        }
    private:
        char* lineFromMessage(char*& pos) { char* start = strtok(pos, "\n"); pos = nullptr; return start; }
        void do_helper(WorldPacketList& data_list, char* text)
        {
            char* pos = text;
            while (char* line = lineFromMessage(pos))
            {
                WorldPacket* data = new WorldPacket();
                ChatHandler::BuildChatPacket(*data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, line);
                data_list.push_back(data);
            }
        }

        uint32 i_textId;
        va_list* i_args;
    };
}                                                           // namespace Trinity

/// Send a System Message to all players (except self if mentioned)
void World::SendWorldText(uint32 string_id, ...)
{
    va_list ap;
    va_start(ap, string_id);

    Trinity::WorldWorldTextBuilder wt_builder(string_id, &ap);
    Trinity::LocalizedPacketListDo<Trinity::WorldWorldTextBuilder> wt_do(wt_builder);
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (!itr->second || !itr->second->GetPlayer() || !itr->second->GetPlayer()->IsInWorld())
            continue;

        wt_do(itr->second->GetPlayer());
    }

    va_end(ap);
}

/// Send a System Message to all GMs (except self if mentioned)
void World::SendGMText(uint32 string_id, ...)
{
    va_list ap;
    va_start(ap, string_id);

    Trinity::WorldWorldTextBuilder wt_builder(string_id, &ap);
    Trinity::LocalizedPacketListDo<Trinity::WorldWorldTextBuilder> wt_do(wt_builder);
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        // Session should have permissions to receive global gm messages
        WorldSession* session = itr->second;
        if (!session || !session->HasPermission(rbac::RBAC_PERM_RECEIVE_GLOBAL_GM_TEXTMESSAGE))
            continue;

        // Player should be in world
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;

        wt_do(player);
    }

    va_end(ap);
}

/// DEPRECATED, only for debug purpose. Send a System Message to all players (except self if mentioned)
void World::SendGlobalText(char const* text, WorldSession* self)
{
    WorldPacket data;

    // need copy to prevent corruption by strtok call in LineFromMessage original string
    char* buf = strdup(text);
    char* pos = buf;

    while (char* line = ChatHandler::LineFromMessage(pos))
    {
        ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, line);
        SendGlobalMessage(&data, self);
    }

    free(buf);
}

/// Send a packet to all players (or players selected team) in the zone (except self if mentioned)
bool World::SendZoneMessage(uint32 zone, WorldPacket* packet, WorldSession* self, uint32 team)
{
    bool foundPlayerToSend = false;
    SessionMap::const_iterator itr;

    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
    {
        if (itr->second &&
            itr->second->GetPlayer() &&
            itr->second->GetPlayer()->IsInWorld() &&
            itr->second->GetPlayer()->GetZoneId() == zone &&
            itr->second != self &&
            (team == 0 || itr->second->GetPlayer()->GetTeam() == team))
        {
            itr->second->SendPacket(packet);
            foundPlayerToSend = true;
        }
    }

    return foundPlayerToSend;
}

/// Send a System Message to all players in the zone (except self if mentioned)
void World::SendZoneText(uint32 zone, char const* text, WorldSession* self, uint32 team)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, text);
    SendZoneMessage(zone, &data, self, team);
}

/// Kick (and save) all players
void World::KickAll()
{
    m_QueuedPlayer.clear();                                 // prevent send queue update packet and login queued sessions

    // session not removed at kick and will removed in next update tick
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        itr->second->KickPlayer();
}

/// Kick (and save) all players with security level less `sec`
void World::KickAllLess(AccountTypes sec)
{
    // session not removed at kick and will removed in next update tick
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetSecurity() < sec)
            itr->second->KickPlayer();
}

/// Ban an account or ban an IP address, duration will be parsed using TimeStringToSecs if it is positive, otherwise permban
BanReturn World::BanAccount(BanMode mode, std::string const& nameOrIP, std::string const& duration, std::string const& reason, std::string const& author)
{
    uint32 duration_secs = TimeStringToSecs(duration);
    return BanAccount(mode, nameOrIP, duration_secs, reason, author);
}

/// Ban an account or ban an IP address, duration is in seconds if positive, otherwise permban
BanReturn World::BanAccount(BanMode mode, std::string const& nameOrIP, uint32 duration_secs, std::string const& reason, std::string const& author)
{
    PreparedQueryResult resultAccounts = PreparedQueryResult(nullptr); //used for kicking
    PreparedStatement* stmt = nullptr;

    ///- Update the database with ban information
    switch (mode)
    {
    case BAN_IP:
        // No SQL injection with prepared statements
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_BY_IP);
        stmt->setString(0, nameOrIP);
        resultAccounts = LoginDatabase.Query(stmt);
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_IP_BANNED);
        stmt->setString(0, nameOrIP);
        stmt->setUInt32(1, duration_secs);
        stmt->setString(2, author);
        stmt->setString(3, reason);
        LoginDatabase.Execute(stmt);
        break;
    case BAN_ACCOUNT:
        // No SQL injection with prepared statements
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_ID_BY_NAME);
        stmt->setString(0, nameOrIP);
        resultAccounts = LoginDatabase.Query(stmt);
        break;
    case BAN_CHARACTER:
        // No SQL injection with prepared statements
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_ACCOUNT_BY_NAME);
        stmt->setString(0, nameOrIP);
        resultAccounts = CharacterDatabase.Query(stmt);
        break;
    default:
        return BAN_SYNTAX_ERROR;
    }

    if (!resultAccounts)
    {
        if (mode == BAN_IP)
            return BAN_SUCCESS;                             // ip correctly banned but nobody affected (yet)
        else
            return BAN_NOTFOUND;                            // Nobody to ban
    }

    ///- Disconnect all affected players (for IP it can be several)
    SQLTransaction trans = LoginDatabase.BeginTransaction();
    do
    {
        Field* fieldsAccount = resultAccounts->Fetch();
        uint32 account = fieldsAccount[0].GetUInt32();

        if (mode != BAN_IP)
        {
            // make sure there is only one active ban
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED);
            stmt->setUInt32(0, account);
            trans->Append(stmt);
            // No SQL injection with prepared statements
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_ACCOUNT_BANNED);
            stmt->setUInt32(0, account);
            stmt->setUInt32(1, duration_secs);
            stmt->setString(2, author);
            stmt->setString(3, reason);
            trans->Append(stmt);
        }

        if (WorldSession* sess = FindSession(account))
            if (std::string(sess->GetPlayerName()) != author)
                sess->KickPlayer();
    } while (resultAccounts->NextRow());

    LoginDatabase.CommitTransaction(trans);

    return BAN_SUCCESS;
}

/// Remove a ban from an account or IP address
bool World::RemoveBanAccount(BanMode mode, std::string const& nameOrIP)
{
    PreparedStatement* stmt = nullptr;
    if (mode == BAN_IP)
    {
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_IP_NOT_BANNED);
        stmt->setString(0, nameOrIP);
        LoginDatabase.Execute(stmt);
    }
    else
    {
        uint32 account = 0;
        if (mode == BAN_ACCOUNT)
            account = AccountMgr::GetId(nameOrIP);
        else if (mode == BAN_CHARACTER)
            account = sCharacterCache->GetCharacterAccountIdByName(nameOrIP);

        if (!account)
            return false;

        //NO SQL injection as account is uint32
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_NOT_BANNED);
        stmt->setUInt32(0, account);
        LoginDatabase.Execute(stmt);
    }
    return true;
}

/// Ban an account or ban an IP address, duration will be parsed using TimeStringToSecs if it is positive, otherwise permban
BanReturn World::BanCharacter(std::string const& name, std::string const& duration, std::string const& reason, std::string const& author)
{
    Player* banned = ObjectAccessor::FindConnectedPlayerByName(name);
    ObjectGuid::LowType guid = 0;

    uint32 duration_secs = TimeStringToSecs(duration);

    /// Pick a player to ban if not online
    if (!banned)
    {
        ObjectGuid fullGuid = sCharacterCache->GetCharacterGuidByName(name);
        if (fullGuid.IsEmpty())
            return BAN_NOTFOUND;                                    // Nobody to ban

        guid = fullGuid.GetCounter();
    }
    else
        guid = banned->GetGUID().GetCounter();

    //A�ado una transaccion al ban, para evitar que se pueda insertar primero el ban, y desbanear despues
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    // make sure there is only one active ban
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_BAN);
    stmt->setUInt32(0, guid);
    /*CharacterDatabase.Execute(stmt);*/
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_BAN);
    stmt->setUInt32(0, guid);
    stmt->setUInt32(1, duration_secs);
    stmt->setString(2, author);
    stmt->setString(3, reason);
    /* CharacterDatabase.Execute(stmt);*/
    trans->Append(stmt);

    //Comiteo la transaccion
    CharacterDatabase.CommitTransaction(trans);
    if (banned)
        banned->GetSession()->KickPlayer();

    return BAN_SUCCESS;
}

/// Remove a ban from a character
bool World::RemoveBanCharacter(std::string const& name)
{
    Player* banned = ObjectAccessor::FindConnectedPlayerByName(name);
    ObjectGuid::LowType guid = 0;

    /// Pick a player to ban if not online
    if (!banned)
    {
        ObjectGuid fullGuid = sCharacterCache->GetCharacterGuidByName(name);
        if (fullGuid.IsEmpty())
            return false;

        guid = fullGuid.GetCounter();
    }
    else
        guid = banned->GetGUID().GetCounter();

    if (!guid)
        return false;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHARACTER_BAN);
    stmt->setUInt32(0, guid);
    CharacterDatabase.Execute(stmt);
    return true;
}

/// Update the game time
void World::_UpdateGameTime()
{
    ///- update the time
    time_t lastGameTime = GameTime::GetGameTime();
    GameTime::UpdateGameTimers();

    uint32 elapsed = uint32(GameTime::GetGameTime() - lastGameTime);

    ///- if there is a shutdown timer
    if (!IsStopped() && m_ShutdownTimer > 0 && elapsed > 0)
    {
        ///- ... and it is overdue, stop the world (set m_stopEvent)
        if (m_ShutdownTimer <= elapsed)
        {
            if (!(m_ShutdownMask & SHUTDOWN_MASK_IDLE) || GetActiveAndQueuedSessionCount() == 0)
                m_stopEvent = true;                         // exist code already set
            else
                m_ShutdownTimer = 1;                        // minimum timer value to wait idle state
        }
        ///- ... else decrease it and if necessary display a shutdown countdown to the users
        else
        {
            m_ShutdownTimer -= elapsed;

            ShutdownMsg();
        }
    }
}

/// Shutdown the server
void World::ShutdownServ(uint32 time, uint32 options, uint8 exitcode, const std::string& reason)
{
    // ignore if server shutdown at next tick
    if (IsStopped())
        return;

    m_ShutdownMask = options;
    m_ExitCode = exitcode;

    ///- If the shutdown time is 0, set m_stopEvent (except if shutdown is 'idle' with remaining sessions)
    if (time == 0)
    {
        if (!(options & SHUTDOWN_MASK_IDLE) || GetActiveAndQueuedSessionCount() == 0)
            m_stopEvent = true;                             // exist code already set
        else
            m_ShutdownTimer = 1;                            //So that the session count is re-evaluated at next world tick
    }
    ///- Else set the shutdown timer and warn users
    else
    {
        m_ShutdownTimer = time;
        ShutdownMsg(true, nullptr, reason);
    }

    sScriptMgr->OnShutdownInitiate(ShutdownExitCode(exitcode), ShutdownMask(options));
}

/// Display a shutdown message to the user(s)
void World::ShutdownMsg(bool show, Player* player, const std::string& reason)
{
    // not show messages for idle shutdown mode
    if (m_ShutdownMask & SHUTDOWN_MASK_IDLE)
        return;

    ///- Display a message every 12 hours, hours, 5 minutes, minute, 5 seconds and finally seconds
    if (show ||
        (m_ShutdownTimer < 5 * MINUTE && (m_ShutdownTimer % 15) == 0) || // < 5 min; every 15 sec
        (m_ShutdownTimer < 15 * MINUTE && (m_ShutdownTimer % MINUTE) == 0) || // < 15 min ; every 1 min
        (m_ShutdownTimer < 30 * MINUTE && (m_ShutdownTimer % (5 * MINUTE)) == 0) || // < 30 min ; every 5 min
        (m_ShutdownTimer < 12 * HOUR && (m_ShutdownTimer % HOUR) == 0) || // < 12 h ; every 1 h
        (m_ShutdownTimer > 12 * HOUR && (m_ShutdownTimer % (12 * HOUR)) == 0)) // > 12 h ; every 12 h
    {
        std::string str = secsToTimeString(m_ShutdownTimer);
        if (!reason.empty())
            str += " - " + reason;

        ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_TIME : SERVER_MSG_SHUTDOWN_TIME;

        SendServerMessage(msgid, str.c_str(), player);
        TC_LOG_DEBUG("misc", "Server is %s in %s", (m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shuttingdown"), str.c_str());
    }
}

/// Cancel a planned server shutdown
uint32 World::ShutdownCancel()
{
    // nothing cancel or too later
    if (!m_ShutdownTimer || m_stopEvent)
        return 0;

    ServerMessageType msgid = (m_ShutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_CANCELLED : SERVER_MSG_SHUTDOWN_CANCELLED;

    uint32 oldTimer = m_ShutdownTimer;
    m_ShutdownMask = 0;
    m_ShutdownTimer = 0;
    m_ExitCode = SHUTDOWN_EXIT_CODE;                       // to default value
    SendServerMessage(msgid);

    TC_LOG_DEBUG("misc", "Server %s cancelled.", (m_ShutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shutdown"));

    sScriptMgr->OnShutdownCancel();
    return oldTimer;
}

/// Send a server message to the user(s)
void World::SendServerMessage(ServerMessageType type, const char *text, Player* player)
{
    WorldPacket data(SMSG_SERVER_MESSAGE, 50);              // guess size
    data << uint32(type);
    if (type <= SERVER_MSG_STRING)
        data << text;

    if (player)
        player->SendDirectMessage(&data);
    else
        SendGlobalMessage(&data);
}

void World::UpdateSessions(uint32 diff)
{
    ///- Add new sessions
    WorldSession* sess = nullptr;
    while (addSessQueue.next(sess))
        AddSession_(sess);

    ///- Then send an update signal to remaining ones
    for (SessionMap::iterator itr = m_sessions.begin(), next; itr != m_sessions.end(); itr = next)
    {
        next = itr;
        ++next;

        ///- and remove not active sessions from the list
        WorldSession* pSession = itr->second;
        WorldSessionFilter updater(pSession);

        if (!pSession->Update(diff, updater))    // As interval = 0
        {
            if (!RemoveQueuedPlayer(itr->second) && itr->second && getIntConfig(CONFIG_INTERVAL_DISCONNECT_TOLERANCE))
                m_disconnects[itr->second->GetAccountId()] = time(nullptr);
            RemoveQueuedPlayer(pSession);
            m_sessions.erase(itr);
            delete pSession;

        }
    }
}

// This handles the issued and queued CLI commands
void World::ProcessCliCommands()
{
    CliCommandHolder::Print zprint = nullptr;
    void* callbackArg = nullptr;
    CliCommandHolder* command = nullptr;
    while (cliCmdQueue.next(command))
    {
        TC_LOG_INFO("misc", "CLI command under processing...");
        zprint = command->m_print;
        callbackArg = command->m_callbackArg;
        CliHandler handler(callbackArg, zprint);
        handler.ParseCommands(command->m_command);
        if (command->m_commandFinished)
            command->m_commandFinished(callbackArg, !handler.HasSentErrorMessage());
        delete command;
    }
}

void World::SendAutoBroadcast()
{
    if (m_Autobroadcasts.empty())
        return;

    uint32 weight = 0;
    AutobroadcastsWeightMap selectionWeights;
    std::string msg;

    for (AutobroadcastsWeightMap::const_iterator it = m_AutobroadcastsWeights.begin(); it != m_AutobroadcastsWeights.end(); ++it)
    {
        if (it->second)
        {
            weight += it->second;
            selectionWeights[it->first] = it->second;
        }
    }

    if (weight)
    {
        uint32 selectedWeight = urand(0, weight - 1);
        weight = 0;
        for (AutobroadcastsWeightMap::const_iterator it = selectionWeights.begin(); it != selectionWeights.end(); ++it)
        {
            weight += it->second;
            if (selectedWeight < weight)
            {
                msg = m_Autobroadcasts[it->first];
                break;
            }
        }
    }
    else
        msg = m_Autobroadcasts[urand(0, m_Autobroadcasts.size())];


    if (msg.find("GuildTop") != std::string::npos)
    {
        char* GuildTopText = strtok((char*)msg.c_str(), " ");
        char* strTop = strtok(NULL, " ");
        uint32 nTop = atoi(strTop);
        if (nTop && nTop > 0) msg = GetGuildTop(nTop);
        else msg = "Fallo con la difusion del guild Rank";
    }

    uint32 abcenter = sWorld->getIntConfig(CONFIG_AUTOBROADCAST_CENTER);

    if (abcenter == 0)
        sWorld->SendWorldText(LANG_AUTO_BROADCAST, msg.c_str());
    else if (abcenter == 1)
    {
        WorldPacket data(SMSG_NOTIFICATION, (msg.size() + 1));
        data << msg;
        sWorld->SendGlobalMessage(&data);
    }
    else if (abcenter == 2)
    {
        sWorld->SendWorldText(LANG_AUTO_BROADCAST, msg.c_str());

        WorldPacket data(SMSG_NOTIFICATION, (msg.size() + 1));
        data << msg;
        sWorld->SendGlobalMessage(&data);
    }

    TC_LOG_DEBUG("misc", "AutoBroadcast: '%s'", msg.c_str());
}

void World::SendRNDBroadcastIRC()
{
    std::string msg;
    QueryResult result = WorldDatabase.PQuery("SELECT `message` FROM `irc_autoannounce` ORDER BY RAND() LIMIT 1");
    if (!result)
        return;
    msg = result->Fetch()[0].GetString();

    sWorld->SendWorldText(6612, msg.c_str());
    std::string ircchan = "#";
    ircchan += sIRC->anchn;
    sIRC->Send_IRC_Channel(ircchan, sIRC->MakeMsg("\00304,08\037/!\\\037\017\00304 Automatic System Message \00304,08\037/!\\\037\017 %s", "%s", msg.c_str()), true);
}


void World::UpdateRealmCharCount(uint32 accountId)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_COUNT);
    stmt->setUInt32(0, accountId);
    _queryProcessor.AddQuery(CharacterDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&World::_UpdateRealmCharCount, this, std::placeholders::_1)));
}

void World::_UpdateRealmCharCount(PreparedQueryResult resultCharCount)
{
    if (resultCharCount)
    {
        Field* fields = resultCharCount->Fetch();
        uint32 accountId = fields[0].GetUInt32();
        uint8 charCount = uint8(fields[1].GetUInt64());

        SQLTransaction trans = LoginDatabase.BeginTransaction();

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_REALM_CHARACTERS_BY_REALM);
        stmt->setUInt32(0, accountId);
        stmt->setUInt32(1, realm.Id.Realm);
        trans->Append(stmt);

        stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_REALM_CHARACTERS);
        stmt->setUInt8(0, charCount);
        stmt->setUInt32(1, accountId);
        stmt->setUInt32(2, realm.Id.Realm);
        trans->Append(stmt);

        LoginDatabase.CommitTransaction(trans);
    }
}

void World::InitWeeklyQuestResetTime()
{
    time_t wstime = uint64(sWorld->getWorldState(WS_WEEKLY_QUEST_RESET_TIME));
    time_t curtime = time(nullptr);
    m_NextWeeklyQuestReset = wstime < curtime ? curtime : time_t(wstime);
}

void World::InitDailyQuestResetTime(bool loading)
{
    time_t mostRecentQuestTime = 0;

    if (loading)
    {
        QueryResult result = CharacterDatabase.Query("SELECT MAX(time) FROM character_queststatus_daily");
        if (result)
        {
            Field* fields = result->Fetch();
            mostRecentQuestTime = time_t(fields[0].GetUInt32());
        }
    }

    // FIX ME: client not show day start time
    time_t curTime = time(nullptr);
    tm localTm;
    localtime_r(&curTime, &localTm);
    localTm.tm_hour = getIntConfig(CONFIG_DAILY_QUEST_RESET_TIME_HOUR);
    localTm.tm_min = 0;
    localTm.tm_sec = 0;

    // current day reset time
    time_t curDayResetTime = mktime(&localTm);

    // last reset time before current moment
    time_t resetTime = (curTime < curDayResetTime) ? curDayResetTime - DAY : curDayResetTime;

    // need reset (if we have quest time before last reset time (not processed by some reason)
    if (mostRecentQuestTime && mostRecentQuestTime <= resetTime)
        m_NextDailyQuestReset = mostRecentQuestTime;
    else // plan next reset time
        m_NextDailyQuestReset = (curTime >= curDayResetTime) ? curDayResetTime + DAY : curDayResetTime;
}

void World::InitMonthlyQuestResetTime()
{
    time_t wstime = uint64(sWorld->getWorldState(WS_MONTHLY_QUEST_RESET_TIME));
    time_t curtime = time(nullptr);
    m_NextMonthlyQuestReset = wstime < curtime ? curtime : time_t(wstime);
}

void World::InitRandomBGResetTime()
{
    time_t bgtime = uint64(sWorld->getWorldState(WS_BG_DAILY_RESET_TIME));
    if (!bgtime)
        m_NextRandomBGReset = time_t(time(nullptr));         // game time not yet init

    // generate time by config
    time_t curTime = time(nullptr);
    tm localTm;
    localtime_r(&curTime, &localTm);
    localTm.tm_hour = getIntConfig(CONFIG_RANDOM_BG_RESET_HOUR);
    localTm.tm_min = 0;
    localTm.tm_sec = 0;

    // current day reset time
    time_t nextDayResetTime = mktime(&localTm);

    // next reset time before current moment
    if (curTime >= nextDayResetTime)
        nextDayResetTime += DAY;

    // normalize reset time
    m_NextRandomBGReset = bgtime < curTime ? nextDayResetTime - DAY : nextDayResetTime;

    if (!bgtime)
        sWorld->setWorldState(WS_BG_DAILY_RESET_TIME, uint64(m_NextRandomBGReset));
}

void World::InitGuildResetTime()
{
    time_t gtime = uint64(getWorldState(WS_GUILD_DAILY_RESET_TIME));
    if (!gtime)
        m_NextGuildReset = time_t(time(nullptr));         // game time not yet init

    // generate time by config
    time_t curTime = time(nullptr);
    tm localTm;
    localtime_r(&curTime, &localTm);
    localTm.tm_hour = getIntConfig(CONFIG_GUILD_RESET_HOUR);
    localTm.tm_min = 0;
    localTm.tm_sec = 0;

    // current day reset time
    time_t nextDayResetTime = mktime(&localTm);

    // next reset time before current moment
    if (curTime >= nextDayResetTime)
        nextDayResetTime += DAY;

    // normalize reset time
    m_NextGuildReset = gtime < curTime ? nextDayResetTime - DAY : nextDayResetTime;

    if (!gtime)
        sWorld->setWorldState(WS_GUILD_DAILY_RESET_TIME, uint64(m_NextGuildReset));
}

void World::ResetDailyQuests()
{
    TC_LOG_INFO("misc", "Daily quests reset for all characters.");

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_RESET_CHARACTER_QUESTSTATUS_DAILY);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetDailyQuestStatus();

    // change available dailies
    sPoolMgr->ChangeDailyQuests();

    sAnticheatMgr->ResetDailyReportStates();
}

void World::LoadDBAllowedSecurityLevel()
{
    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_REALMLIST_SECURITY_LEVEL);
    stmt->setInt32(0, int32(realm.Id.Realm));
    PreparedQueryResult result = LoginDatabase.Query(stmt);

    if (result)
        SetPlayerSecurityLimit(AccountTypes(result->Fetch()->GetUInt8()));
}

void World::SetPlayerSecurityLimit(AccountTypes _sec)
{
    AccountTypes sec = _sec < SEC_CONSOLE ? _sec : SEC_PLAYER;
    bool update = sec > m_allowedSecurityLevel;
    m_allowedSecurityLevel = sec;
    if (update)
        KickAllLess(m_allowedSecurityLevel);
}

void World::ResetWeeklyQuests()
{
    TC_LOG_INFO("misc", "Weekly quests reset for all characters.");

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_RESET_CHARACTER_QUESTSTATUS_WEEKLY);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetWeeklyQuestStatus();

    m_NextWeeklyQuestReset = time_t(m_NextWeeklyQuestReset + WEEK);
    sWorld->setWorldState(WS_WEEKLY_QUEST_RESET_TIME, uint64(m_NextWeeklyQuestReset));

    // change available weeklies
    sPoolMgr->ChangeWeeklyQuests();
}

void World::ResetMonthlyQuests()
{
    TC_LOG_INFO("misc", "Monthly quests reset for all characters.");

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_RESET_CHARACTER_QUESTSTATUS_MONTHLY);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetMonthlyQuestStatus();

    // generate time
    time_t curTime = time(nullptr);
    tm localTm;
    localtime_r(&curTime, &localTm);

    int month = localTm.tm_mon;
    int year = localTm.tm_year;

    ++month;

    // month 11 is december, next is january (0)
    if (month > 11)
    {
        month = 0;
        year += 1;
    }

    // reset time for next month
    localTm.tm_year = year;
    localTm.tm_mon = month;
    localTm.tm_mday = 1;        // don't know if we really need config option for day / hour
    localTm.tm_hour = 0;
    localTm.tm_min = 0;
    localTm.tm_sec = 0;

    time_t nextMonthResetTime = mktime(&localTm);

    // plan next reset time
    m_NextMonthlyQuestReset = (curTime >= nextMonthResetTime) ? nextMonthResetTime + MONTH : nextMonthResetTime;

    sWorld->setWorldState(WS_MONTHLY_QUEST_RESET_TIME, uint64(m_NextMonthlyQuestReset));
}

void World::ResetEventSeasonalQuests(uint16 event_id)
{
    TC_LOG_INFO("misc", "Seasonal quests reset for all characters.");

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_RESET_CHARACTER_QUESTSTATUS_SEASONAL_BY_EVENT);
    stmt->setUInt16(0, event_id);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetSeasonalQuestStatus(event_id);
}

void World::ResetRandomBG()
{
    TC_LOG_INFO("misc", "Random BG status reset for all characters.");

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_BATTLEGROUND_RANDOM_ALL);
    CharacterDatabase.Execute(stmt);

    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->SetRandomWinner(false);

    m_NextRandomBGReset = time_t(m_NextRandomBGReset + DAY);
    sWorld->setWorldState(WS_BG_DAILY_RESET_TIME, uint64(m_NextRandomBGReset));
}

void World::ResetGuildCap()
{
    TC_LOG_INFO("misc", "Guild Daily Cap reset.");

    m_NextGuildReset = time_t(m_NextGuildReset + DAY);
    sWorld->setWorldState(WS_GUILD_DAILY_RESET_TIME, uint64(m_NextGuildReset));
    sGuildMgr->ResetTimes();
}

void World::UpdateMaxSessionCounters()
{
    m_maxActiveSessionCount = std::max(m_maxActiveSessionCount, uint32(m_sessions.size() - m_QueuedPlayer.size()));
    m_maxQueuedSessionCount = std::max(m_maxQueuedSessionCount, uint32(m_QueuedPlayer.size()));
}

void World::LoadDBVersion()
{
    QueryResult result = WorldDatabase.Query("SELECT db_version, cache_id FROM version LIMIT 1");
    if (result)
    {
        Field* fields = result->Fetch();

        m_DBVersion = fields[0].GetString();
        // will be overwrite by config values if different and non-0
        m_int_configs[CONFIG_CLIENTCACHE_VERSION] = fields[1].GetUInt32();
    }

    if (m_DBVersion.empty())
        m_DBVersion = "Unknown world database.";
}

void World::UpdateAreaDependentAuras()
{
    SessionMap::const_iterator itr;
    for (itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (itr->second && itr->second->GetPlayer() && itr->second->GetPlayer()->IsInWorld())
        {
            itr->second->GetPlayer()->UpdateAreaDependentAuras(itr->second->GetPlayer()->GetAreaId());
            itr->second->GetPlayer()->UpdateZoneDependentAuras(itr->second->GetPlayer()->GetZoneId());
        }
}

void World::LoadWorldStates()
{
    uint32 oldMSTime = getMSTime();

    QueryResult result = CharacterDatabase.Query("SELECT entry, value FROM worldstates");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 world states. DB table `worldstates` is empty!");

        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        m_worldstates[fields[0].GetUInt32()] = fields[1].GetUInt32();
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u world states in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

bool World::IsPvPRealm() const
{
    return (getIntConfig(CONFIG_GAME_TYPE) == REALM_TYPE_PVP || getIntConfig(CONFIG_GAME_TYPE) == REALM_TYPE_RPPVP || getIntConfig(CONFIG_GAME_TYPE) == REALM_TYPE_FFA_PVP);
}

bool World::IsFFAPvPRealm() const
{
    return getIntConfig(CONFIG_GAME_TYPE) == REALM_TYPE_FFA_PVP;
}

// Setting a worldstate will save it to DB
void World::setWorldState(uint32 index, uint64 value)
{
    WorldStatesMap::const_iterator it = m_worldstates.find(index);
    if (it != m_worldstates.end())
    {
        if (it->second == value)
            return;

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_WORLDSTATE);

        stmt->setUInt32(0, uint32(value));
        stmt->setUInt32(1, index);

        CharacterDatabase.Execute(stmt);
    }
    else
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_WORLDSTATE);

        stmt->setUInt32(0, index);
        stmt->setUInt32(1, uint32(value));

        CharacterDatabase.Execute(stmt);
    }
    m_worldstates[index] = value;
}

uint64 World::getWorldState(uint32 index) const
{
    WorldStatesMap::const_iterator it = m_worldstates.find(index);
    return it != m_worldstates.end() ? it->second : 0;
}

void World::ProcessQueryCallbacks()
{
    _queryProcessor.ProcessReadyQueries();
}

void World::ReloadRBAC()
{
    // Passive reload, we mark the data as invalidated and next time a permission is checked it will be reloaded
    TC_LOG_INFO("rbac", "World::ReloadRBAC()");
    for (SessionMap::const_iterator itr = m_sessions.begin(); itr != m_sessions.end(); ++itr)
        if (WorldSession* session = itr->second)
            session->InvalidateRBACData();
}

//Guild-Level-System [Start]
void World::LoadGuildBonusInfo()
{
    //Moechte keinen riesigen Abschnitt in die Worldconf hinzufuegen, deswegen
    //soll das ganze ueber eine Tabelle in der CharDB geregelt werden.
    //Hier werden die benoetigten Daten geladen.
    m_req_guildLevel_gold_1 = SelectReqGuildLevelForBonus(GUILD_BONUS_GOLD_1);
    m_req_guildLevel_xp_1 = SelectReqGuildLevelForBonus(GUILD_BONUS_XP_1);
    m_req_guildLevel_schneller_geist = SelectReqGuildLevelForBonus(GUILD_BONUS_SCHNELLER_GEIST);
    m_req_guildLevel_reperatur_1 = SelectReqGuildLevelForBonus(GUILD_BONUS_REPERATUR_1);
    m_req_guildLevel_gold_2 = SelectReqGuildLevelForBonus(GUILD_BONUS_GOLD_2);
    m_req_guildLevel_reittempo_1 = SelectReqGuildLevelForBonus(GUILD_BONUS_REITTEMPO_1);
    m_req_guildLevel_reputation_1 = SelectReqGuildLevelForBonus(GUILD_BONUS_RUF_1);
    m_req_guildLevel_xp_2 = SelectReqGuildLevelForBonus(GUILD_BONUS_XP_2);
    m_req_guildLevel_reperatur_2 = SelectReqGuildLevelForBonus(GUILD_BONUS_REPERATUR_2);
    m_req_guildLevel_reittempo_2 = SelectReqGuildLevelForBonus(GUILD_BONUS_REITTEMPO_2);
    m_req_guildLevel_reputation_2 = SelectReqGuildLevelForBonus(GUILD_BONUS_RUF_2);
    m_req_guildLevel_honor_1 = SelectReqGuildLevelForBonus(GUILD_BONUS_EHRE_1);
    m_req_guildLevel_honor_2 = SelectReqGuildLevelForBonus(GUILD_BONUS_EHRE_2);
}

uint8 World::SelectReqGuildLevelForBonus(uint8 guildBonus)
{
    PreparedStatement* stmt;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_GUILD_BONUS_INFO);
    stmt->setUInt8(0, guildBonus);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (result)
        return (*result)[0].GetUInt8();
    else
        return 0;
}

uint8 World::GetReqGuildLevelForBonus(uint8 guildBonus)
{
    switch (guildBonus)
    {
    case GUILD_BONUS_GOLD_1:
        return m_req_guildLevel_gold_1;
        break;
    case GUILD_BONUS_GOLD_2:
        return m_req_guildLevel_gold_2;
        break;
    case GUILD_BONUS_EHRE_1:
        return m_req_guildLevel_honor_1;
        break;
    case GUILD_BONUS_EHRE_2:
        return m_req_guildLevel_honor_2;
        break;
    case GUILD_BONUS_REITTEMPO_1:
        return m_req_guildLevel_reittempo_1;
        break;
    case GUILD_BONUS_REITTEMPO_2:
        return m_req_guildLevel_reittempo_2;
        break;
    case GUILD_BONUS_REPERATUR_1:
        return m_req_guildLevel_reperatur_1;
        break;
    case GUILD_BONUS_REPERATUR_2:
        return m_req_guildLevel_reperatur_2;
        break;
    case GUILD_BONUS_RUF_1:
        return m_req_guildLevel_reputation_1;
        break;
    case GUILD_BONUS_RUF_2:
        return m_req_guildLevel_reputation_2;
        break;
    case GUILD_BONUS_SCHNELLER_GEIST:
        return m_req_guildLevel_schneller_geist;
        break;
    case GUILD_BONUS_XP_1:
        return m_req_guildLevel_xp_1;
        break;
    case GUILD_BONUS_XP_2:
        return m_req_guildLevel_xp_2;
        break;
    default:
        return 0;
        break;
    }
}

void World::RemoveOldCorpses()
{
    m_timers[WUPDATE_CORPSES].SetCurrent(m_timers[WUPDATE_CORPSES].GetInterval());
}

Realm realm;

CliCommandHolder::CliCommandHolder(void* callbackArg, char const* command, Print zprint, CommandFinished commandFinished)
    : m_callbackArg(callbackArg), m_command(strdup(command)), m_print(zprint), m_commandFinished(commandFinished)
{
}

CliCommandHolder::~CliCommandHolder()
{
    free(m_command);
}