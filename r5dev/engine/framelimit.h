#ifndef FRAMELIMIT_H
#define FRAMELIMIT_H

//-----------------------------------------------------------------------------
// RenderThread frame limiter
//-----------------------------------------------------------------------------
class CFrameLimit
{
public:
	CFrameLimit(void);

	void Reset(double target);
	void Run(void);

private:
	double m_MilliSeconds;
	double m_FramesPerSecond;
	//double m_EffectiveMilliSeconds;

	//LARGE_INTEGER m_Last;
	LARGE_INTEGER m_Start;
	LARGE_INTEGER m_Next;
	LARGE_INTEGER m_Time;
	uint32_t m_Frames;
	bool m_bRestart;
};

extern CFrameLimit g_FrameLimiter;

#endif // FRAMELIMIT_H
