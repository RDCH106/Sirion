#include "WorldQueue.h"
#include "SpellMgr.h"
#include "SpellInfo.h"


void World_Queue::Encolar(AccionesWorldQueueScript x)
{
    m_lst_AccionesWorldQueue.push_back(x);

}

AccionesWorldQueueScript World_Queue::Desencolar()
{
    AccionesWorldQueueScript n;
    //Por defecto, doy Type = TYPE_QUEUE_NULL para detectarlo despues si falla
    n.nActionType = TYPE_QUEUE_NULL;
    if (!m_lst_AccionesWorldQueue.empty())
    {
        n = m_lst_AccionesWorldQueue.front();
        m_lst_AccionesWorldQueue.pop_front();
    }
    return n;
}

World_Queue::World_Queue()
{
    //Lo anulo para no llamar al thread, vuelta al modo con script
   /* m_bSeguir = true;
    m_threadLearn = std::thread(([&]() {DoWork(); }));*/
}



World_Queue::~World_Queue()
{
    //Lo anulo para no llamar al thread, vuelta al modo con script
   /* m_bSeguir = false;
    m_threadLearn.join();*/
}


bool World_Queue::IsGoingToLearn(Player *player, uint32 nIdCharacter, uint32 nIdSpell)
{    
    AccionesWorldQueueScript LearnPrevio = from(m_lst_AccionesWorldQueue)
        >> where([&, nIdCharacter, nIdSpell](AccionesWorldQueueScript const & g) {	return g.nIdCharacter == nIdCharacter && g.nIdSpell == nIdSpell && g.nActionType == TYPE_AUTOLEARN_SPELL; })
        >> select([](AccionesWorldQueueScript const & g) {return g; })
        >> first_or_default();

    if (LearnPrevio.nActionType == 0 && LearnPrevio.nIdCharacter == 0 && LearnPrevio.nIdSpell == 0)
        return false;

    SpellInfo const* sSpellPrevio = sSpellMgr->GetSpellInfo(LearnPrevio.nIdSpell)->GetPrevRankSpell();
    if (sSpellPrevio)
    {
        if (player->HasSpell(sSpellPrevio->Id) || IsGoingToLearn(player,nIdCharacter, sSpellPrevio->Id))
            return true;
        else
            return false;
    }
    return true;
}

bool World_Queue::CanLearnSpell(Player *player, uint32 nIdCharacter, uint32 nIdSpell)
{
    //Si ya lo tiene, retornamos false
    if (player->HasSpell(nIdSpell))
        return false;

    SpellInfo const* sSpellPrevio = sSpellMgr->GetSpellInfo(nIdSpell)->GetPrevRankSpell();
    if (sSpellPrevio)
    {
        //Turrado, pendiente de revisar esta parte
        if (player->HasSpell(sSpellPrevio->Id) || IsGoingToLearn(player, nIdCharacter, sSpellPrevio->Id))
            return true;
        else
            return false;
    }

    //Si llega hasta aqui, es que ha cumplido requisitos
    return true;
    
}

World_Queue* World_Queue::instance()
{
    static World_Queue instance;
    return &instance;
}
