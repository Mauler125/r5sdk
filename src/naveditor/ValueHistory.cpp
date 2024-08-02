#include "NavEditor/Include/ValueHistory.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif

ValueHistory::ValueHistory() :
	m_hsamples(0)
{
	for (int i = 0; i < MAX_HISTORY; ++i)
		m_samples[i] = ImVec2(0,0);
}

float ValueHistory::getSampleMin() const
{
	float val = m_samples[0].y;
	for (int i = 1; i < MAX_HISTORY; ++i)
		if (m_samples[i].y < val)
			val = m_samples[i].y;
	return val;
} 

float ValueHistory::getSampleMax() const
{
	float val = m_samples[0].y;
	for (int i = 1; i < MAX_HISTORY; ++i)
		if (m_samples[i].y > val)
			val = m_samples[i].y;
	return val;
}

float ValueHistory::getAverage() const
{
	float val = 0;
	for (int i = 0; i < MAX_HISTORY; ++i)
		val += m_samples[i].y;
	return val/(float)MAX_HISTORY;
}
