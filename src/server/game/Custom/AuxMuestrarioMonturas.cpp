#include "AuxMuestrarioMonturas.h"


void MuestrarioMonturas::LoadMuestrario(std::list<RewardsMenuItem, std::allocator<RewardsMenuItem>> lstMenuRewards)
{
	m_Monturas.clear();
	posicion = 0;

	m_Monturas = from(lstMenuRewards)
		>> where([](RewardsMenuItem const & g) 
	    {
		//Si no tiene I-, no nos vale
		if (g.m_codigo.find("I-") == std::string::npos)
			return false;
		//Si tiene I-, buscamos que el regex vaya a devolver un valor valido
        #pragma warning(suppress: 4129)
		std::regex re("M-([0-9]*)\d*");
		std::smatch match;
		uint32 numero = 0;
		if (std::regex_search(g.m_codigo, match, re) && match.size() > 1)
		{
			numero = stoi(match.str(1));
		}
		return numero > 0; })
		//En el select, obtenemos el numero ya
		>> select([](RewardsMenuItem const & g)
		{
        #pragma warning(suppress: 4129)
		std::regex re("M-([0-9]*)\d*");
		std::smatch match;
		uint32 numero = 0;
		if (std::regex_search(g.m_codigo, match, re) && match.size() > 1)
		{
			numero = stoi(match.str(1));
		}
		return numero; })
		>> to_vector();



}
uint32 MuestrarioMonturas::GetMontura()
{
	if (m_Monturas.size() > 0)
	{
		uint32 id = m_Monturas.at(posicion);		
		return id;
	}
	else
		return 0;
}

void MuestrarioMonturas::AddPosition()
{
	posicion++;
	if (posicion == m_Monturas.size())
		posicion = 0;
}

MuestrarioMonturas* MuestrarioMonturas::instance()
{
	static MuestrarioMonturas instance;
	return &instance;
}
