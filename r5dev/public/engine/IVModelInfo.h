//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IVMODELINFO_H
#define IVMODELINFO_H

#ifdef _WIN32
#pragma once
#endif

struct model_t;

class IVModelInfo
{
public:
	virtual							~IVModelInfo(void) { }
	virtual const model_t* GetModel(int modelindex) const = 0;

	// !TODO: The rest if it ever becomes necessary.
};

class IVModelInfoClient : public IVModelInfo
{};

#endif // IVMODELINFO_H
