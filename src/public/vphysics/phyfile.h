//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef PHYFILE_H
#define PHYFILE_H
#pragma once

typedef struct phyheader_s
{
	int     size;
	int     id;
	short   numsolids;
	short   align;
	int     checksum; // checksum of source .rmdl file
	int     keyvalueindex;
} phyheader_t;

#endif // PHYFILE_H
