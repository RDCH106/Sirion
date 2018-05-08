#ifndef AUXAUTOLEARN_H
#define AUXAUTOLEARN_H

#include "ScriptMgr.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "Log.h"

#define Warrior_Low 200001
#define Warrior_Highlevel 200002
#define Paladin_Lowlevel 200003
#define Paladin_Highlevel 200004
#define Paladin_Allyonly 200020
#define Paladin_Hordeonly 200021
#define Druid_Lowlevel 200005
#define Druid_Highlevel 200006
#define Mage_Lowlevel 200007
#define Mage_Highlevel 200008
#define Warlock_Lowlevel 200009
#define Warlock_Highlevel 200010
#define Priest_Lowlevel 200011
#define Priest_Highlevel 200012
#define Hunter_Lowlevel 200013
#define Hunter_Highlevel 200014
#define Rogue_Lowlevel 200015
#define Rogue_Highlevel 200016
#define Shaman_Lowlevel 200017
#define Shaman_Highlevel 200018
#define Shaman_HighlevelAlly 200030
#define DeathKnights_Alllevel 200019


struct AutoLearnSpeels
{
	uint32 nClass;
	uint32 nSpellId;
	uint32 nMinLevel;
	uint32 nFaction;
};



class TC_GAME_API AutoLearnItems
{
private:
	uint32 m_AutoLearnSize = 0;
	AutoLearnSpeels* m_arrAutoLearn;
public:
	static AutoLearnItems* instance();
	void LoadAutoLearn();	
	uint32 GetSize() {return m_AutoLearnSize; }
	AutoLearnSpeels GetItem(uint32 item) { return m_arrAutoLearn[item]; }
};
#define sAutoLearnItems AutoLearnItems::instance()
#endif