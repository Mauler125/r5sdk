#ifndef CLOCKDRIFTMGR_H
#define CLOCKDRIFTMGR_H

struct CClockDriftMgr
{
	void Clear();
	float GetCurrentClockDifference() const;

	enum
	{
		// This controls how much it smooths out the samples from the server.
		NUM_CLOCKDRIFT_SAMPLES = 24
	};

	float field_0[4];
	int field_10;
	float m_ClockOffsets[NUM_CLOCKDRIFT_SAMPLES];
	int m_iCurClockOffset;
	float field_78;
	float field_7C;
	int m_nSimulationTick;
	float m_flClientTickTime;
	float m_flServerTickTime;
	int m_nClientTick;
	int m_nServerTick;
};
static_assert(sizeof(CClockDriftMgr) == 0x94);

#endif // CLOCKDRIFTMGR_H