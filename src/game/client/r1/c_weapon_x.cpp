#include "c_weapon_x.h"

float C_WeaponX::GetZoomFOVInterpAmount(const float curTime) const
{
	return m_playerData.GetZoomFOVInterpAmount(curTime);
}
