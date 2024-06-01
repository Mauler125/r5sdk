//=============================================================================//
// 
// Purpose: simple class that could be used to manage depths of nested elements
// 
//=============================================================================//
#ifndef TIER1_DEPTHCOUNTER_H
#define TIER1_DEPTHCOUNTER_H

template<class T>
class CDepthCounter
{
public:
	CDepthCounter(T& counter) : ref(counter)
	{
		ref++;
	}
	~CDepthCounter()
	{
		ref--;
	}

	T Get()
	{
		return ref;
	}

private:
	T& ref;
};

#endif // TIER1_DEPTHCOUNTER_H
