#ifndef PLAYER_VIEWOFFSET_H
#define PLAYER_VIEWOFFSET_H

struct Player_ViewOffsetEntityData
{
	char gap_0[8];
	int viewOffsetEntityHandle;
	float lerpInDuration;
	float lerpOutDuration;
	bool stabilizePlayerEyeAngles;
};

#endif // PLAYER_VIEWOFFSET_H
