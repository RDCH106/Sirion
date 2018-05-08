#ifndef SPELLLEARNQUEUE_H
#define SPELLLEARNQUEUE_H

#include "ScriptMgr.h"
#include "LinQ.hpp"
#include "World.h"
#include "Player.h"
#include "ObjectAccessor.h"

#include <thread>

using namespace LinQ;

enum ActionType
{
	TYPE_QUEUE_NULL = 0,
	TYPE_AUTOLEARN_SPELL,
	TYPE_ACCOUNT_MOUNT,
    TYPE_TELEPORT,
};

struct AccionesWorldQueueScript
{
	uint32 nIdCharacter;
	uint32 nIdSpell;
	uint32 nActionType;
    uint32 nMap;
    float lfX;
    float lfY;
    float lfZ;
    float lfO;
};


class TC_GAME_API World_Queue
{
private:
    World_Queue();
    ~World_Queue();
    bool IsGoingToLearn(Player *player, uint32 nIdCharacter, uint32 nIdSpell);


    bool m_bSeguir;
    std::thread m_threadLearn;
	std::list<AccionesWorldQueueScript, std::allocator<AccionesWorldQueueScript>> m_lst_AccionesWorldQueue; 
public:
	static World_Queue* instance();
	AccionesWorldQueueScript Desencolar();
	void Encolar(AccionesWorldQueueScript);
    bool CanLearnSpell(Player *player, uint32 nIdCharacter, uint32 nIdSpell);
	uint32 Size() { return m_lst_AccionesWorldQueue.size(); }
};
#define sWorldQueueScript World_Queue::instance()
#endif
