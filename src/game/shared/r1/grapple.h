#ifndef GAME_GRAPPLE_H
#define GAME_GRAPPLE_H

#include "mathlib/vector.h"

struct GrappleData
{
	char gap_0[8];
	Vector3D m_grappleVel;
	Vector3D m_grapplePoints[4];
	int m_grapplePointCount;
	bool m_grappleAttached;
	bool m_grapplePulling;
	bool m_grappleSwinging;
	bool m_grappleRetracting;
	bool m_grappleForcedRetracting;
	bool m_grappleGracePeriodFinished;
	char gap_4e[2];
	float m_grappleUsedPower;
	float m_grappleActivateTime;
	float m_grapplePullTime;
	float m_grappleAttachTime;
	float m_grappleDetachTime;
	int m_grappleMeleeTarget;
	int m_grappleAutoAimTarget;
	bool m_grappleHasGoodVelocity;
	char gap_6d[3];
	float m_grappleLastGoodVelocityTime;
	float m_grappleSwingDetachLowSpeed;
	float m_grappleSwingHoldTime;
	char gap_7c[4];
};

#endif // GAME_GRAPPLE_H
