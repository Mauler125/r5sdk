#ifndef VALUEHISTORY_H
#define VALUEHISTORY_H

class ValueHistory
{
	static const int MAX_HISTORY = 256;
	ImVec2 m_samples[MAX_HISTORY];
	int m_hsamples;
public:
	ValueHistory();

	inline void addSample(const float x, const float y)
	{
		m_hsamples = (m_hsamples+MAX_HISTORY-1) % MAX_HISTORY;
		m_samples[m_hsamples] = ImVec2(x, y);
	}
	
	inline int getSampleCount() const
	{
		return MAX_HISTORY;
	}

	inline int getSampleOffset() const
	{
		return m_hsamples;
	}
	
	inline ImVec2 getSample(const int i) const
	{
		return m_samples[(m_hsamples+i) % MAX_HISTORY];
	}

	inline ImVec2* getSampleBuffer()
	{
		return m_samples;
	}
	
	float getSampleMin() const;
	float getSampleMax() const;
	float getAverage() const;
};

#endif // VALUEHISTORY_H