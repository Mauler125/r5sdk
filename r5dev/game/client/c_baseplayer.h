#ifndef C_BASEPLAYER_H
#define C_BASEPLAYER_H

#include "public/include/icliententity.h"
#include "public/include/icliententitylist.h"
#include "public/include/iclientnetworkable.h"
#include "public/include/iclientrenderable.h"
#include "public/include/iclientthinkable.h"
#include "public/include/iclientunknown.h"
#include "public/include/ihandleentity.h"
#include "public/include/ivscript.h"


class C_BaseCombatCharacter
{
	int m_nPredictionData; // Unk
	//int unk; // Padding?
};

class C_BaseEntity : public IClientEntity
{
	const char* m_pszModelName;
	int unk0;
	char pad[4]; // unk;
	HSCRIPT m_hScriptInstance;
	const char* m_iszScriptId;
};


class C_BaseAnimating : public C_BaseEntity
{

};

class C_BaseAnimatingOverlay : public C_BaseAnimating
{

};

class C_Player : public C_BaseCombatCharacter, public C_BaseAnimatingOverlay
{

};


void F()
{
	sizeof(C_Player);
}

#endif // C_BASEPLAYER_H
