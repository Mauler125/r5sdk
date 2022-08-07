#ifndef CLOCKDRIFTMGR_H
#define CLOCKDRIFTMGR_H

struct __declspec(align(4)) CClockDriftMgr
{
	void Clear();
	float GetCurrentClockDifference() const;

	enum
	{
		// This controls how much it smoothes out the samples from the server.
		NUM_CLOCKDRIFT_SAMPLES = 24
	};

	float field_0[4];
	int field_10;
	float m_ClockOffsets[NUM_CLOCKDRIFT_SAMPLES];
	int m_iCurClockOffset;
	float field_78;
	float field_7C;
	float m_flClientTickTime;
	int m_nClientTick;
	float m_flServerTickTime;
	int m_nServerTick;
	int m_nSimulationTick;
};

#endif // CLOCKDRIFTMGR_H