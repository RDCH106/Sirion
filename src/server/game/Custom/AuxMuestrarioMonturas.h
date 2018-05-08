#include "ScriptMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "sc_npc_loyalty.h"
#include "LinQ.hpp"
#include <Map.h>
#include <regex>
#include <string>

using namespace LinQ;
using namespace nsNpcLoyalty;


class TC_GAME_API MuestrarioMonturas
{
private:
	std::vector<uint32> m_Monturas;
	uint32 posicion = 0;

public:
	static MuestrarioMonturas* instance();
	void LoadMuestrario(std::list<RewardsMenuItem, std::allocator<RewardsMenuItem>> lstMenuRewards);
	uint32 GetMontura();
	void AddPosition();

};
#define sMuestrarioMonturas MuestrarioMonturas::instance()
