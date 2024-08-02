#ifndef PLAYER_VIEWOFFSET_H
#define PLAYER_VIEWOFFSET_H

struct Player_ViewOffsetEntityData
{
	void* __vftable;
	EHANDLE viewOffsetEntityHandle;
	float lerpInDuration;
	float lerpOutDuration;
	bool stabilizePlayerEyeAngles;
};

#endif // PLAYER_VIEWOFFSET_H
