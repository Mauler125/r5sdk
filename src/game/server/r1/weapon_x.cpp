#include "weapon_x.h"

float CWeaponX::GetZoomFOVInterpAmount(const float curTime) const
{
	return m_playerData.GetZoomFOVInterpAmount(curTime);
}
