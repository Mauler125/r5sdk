#ifndef FRAMELIMIT_H
#define FRAMELIMIT_H

//-----------------------------------------------------------------------------
// RenderThread frame limiter
//-----------------------------------------------------------------------------
class CFrameLimit
{
public:
	CFrameLimit(void);

	void Reset(const double target);
	void Run(const double targetFps, const double sleepThreshold, const double maxTolerance);

private:
	double m_MilliSeconds;
	double m_FramesPerSecond;

	LARGE_INTEGER m_Start;
	LARGE_INTEGER m_Next;
	LARGE_INTEGER m_Time;
	uint32_t m_Frames;
	bool m_bRestart;
};

#endif // FRAMELIMIT_H
