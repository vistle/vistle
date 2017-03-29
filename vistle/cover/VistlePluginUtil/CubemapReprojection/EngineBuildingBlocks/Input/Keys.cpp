// EngineBuildingBlocks/Input/Keys.cpp

#include <EngineBuildingBlocks/Input/Keys.h>

using namespace EngineBuildingBlocks::Input;

Keys KeyMap::GetGeneralKey(int specificKey) const
{
	auto it = m_SpecificToGeneral.find(specificKey);
	if (it == m_SpecificToGeneral.end())
	{
		return Keys::Unknown;
	}
	return it->second;
}

int KeyMap::GetSpecificKey(Keys generalKey) const
{
	return m_GeneralToSpecific[static_cast<unsigned>(generalKey)];
}