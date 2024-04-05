#ifndef KVLEAKTRACE_H
#define KVLEAKTRACE_H
#include "tier1/utlvector.h"

#ifdef LEAKTRACK
class CLeakTrack
{
public:
	CLeakTrack()
	{
	}
	~CLeakTrack()
	{
		if (keys.Count() != 0)
		{
			Assert(0);
		}
	}

	struct kve
	{
		KeyValues* kv;
		char		name[256];
	};

	void AddKv(KeyValues* kv, char const* name)
	{
		kve k;
		strncpy(k.name, name ? name : "NULL", sizeof(k.name));
		k.kv = kv;

		keys.AddToTail(k);
	}

	void RemoveKv(KeyValues* kv)
	{
		int c = keys.Count();
		for (int i = 0; i < c; i++)
		{
			if (keys[i].kv == kv)
			{
				keys.Remove(i);
				break;
			}
		}
	}

	CUtlVector< kve > keys;
};

static CLeakTrack track;

#define TRACK_KV_ADD( ptr, name )	track.AddKv( ptr, name )
#define TRACK_KV_REMOVE( ptr )		track.RemoveKv( ptr )

#else

#define TRACK_KV_ADD( ptr, name ) 
#define TRACK_KV_REMOVE( ptr )	

#endif
#endif // KVLEAKTRACE_H