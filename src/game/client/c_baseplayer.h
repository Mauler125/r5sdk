#ifndef C_BASEPLAYER_H
#define C_BASEPLAYER_H

#include "public/icliententity.h"
#include "public/icliententitylist.h"
#include "public/iclientnetworkable.h"
#include "public/iclientrenderable.h"
#include "public/iclientthinkable.h"
#include "public/iclientunknown.h"
#include "public/ihandleentity.h"
#include "public/vscript/ivscript.h"


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

#endif // C_BASEPLAYER_H
