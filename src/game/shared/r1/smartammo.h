#ifndef SHARED_SMARTAMMO_H
#define SHARED_SMARTAMMO_H

class SmartAmmo_WeaponData
{
	void* __vftable;
	int numTargetEntities;
	EHANDLE targetEntities[8];
	float fractions[8];
	float previousFractions[8];
	float currentFractions[8];
	int visiblePoints[8];
	float lastVisibleTimes[8];
	float lastFullLockTimes[8];
	EHANDLE storedTargets[8];
	float lastNewTargetTime;
	int trackerCount;
	EHANDLE trackerEntities[8];
	int trackerLocks[8];
	float trackerTimes[8];
	char gap_174[4];
	EHANDLE previousTargetEntities[10];
	EHANDLE previousTrackerEntities[10];
	int previousTrackerLocks[10];
};

// Client's class is identical.
typedef SmartAmmo_WeaponData SmartAmmo_WeaponData_Client;

#endif // SHARED_SMARTAMMO_H
