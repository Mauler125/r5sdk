 ////////////////////////// variant_t implementation //////////////////////////

#include "entityoutput.h"
#include "baseentity.h"

//-----------------------------------------------------------------------------
// Purpose: All types must be able to display as strings for debugging purposes.
// Output : Returns a pointer to the string that represents this value.
//
//			NOTE: The returned pointer should not be stored by the caller as
//				  subsequent calls to this function will overwrite the contents
//				  of the buffer!
//-----------------------------------------------------------------------------
const char* variant_t::ToString(void) const
{
	static char szBuf[512];

	switch (fieldType)
	{
	case FIELD_STRING:
	{
		return(STRING(iszVal));
	}

	case FIELD_BOOLEAN:
	{
		if (bVal == 0)
		{
			Q_strncpy(szBuf, "false", sizeof(szBuf));
		}
		else
		{
			Q_strncpy(szBuf, "true", sizeof(szBuf));
		}
		return(szBuf);
	}

	case FIELD_INTEGER:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "%i", iVal);
		return(szBuf);
	}

	case FIELD_FLOAT:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "%g", flVal);
		return(szBuf);
	}

	case FIELD_COLOR32:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "%d %d %d %d", (int)rgbaVal.r, (int)rgbaVal.g, (int)rgbaVal.b, (int)rgbaVal.a);
		return(szBuf);
	}

	case FIELD_VECTOR:
	{
		Q_snprintf(szBuf, sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
		return(szBuf);
	}

	case FIELD_VOID:
	{
		szBuf[0] = '\0';
		return(szBuf);
	}

	case FIELD_EHANDLE:
	{
		const char* pszName = (Entity()) ? STRING(Entity()->GetEntityName()) : "<<null entity>>";
		Q_strncpy(szBuf, pszName, 512);
		return (szBuf);
	}
	}

	return("No conversion to string");
}

//-----------------------------------------------------------------------------
// Purpose: sets the entity
//-----------------------------------------------------------------------------
void variant_t::SetEntity(CBaseEntity* val)
{
	eVal = val;
	fieldType = FIELD_EHANDLE;
}

// BUGBUG: Add support for function pointer save/restore to variants
// BUGBUG: Must pass datamap_t to read/write fields 
//void variant_t::Set(fieldtype_t ftype, void* data)
//{
//	fieldType = ftype;
//
//	switch (ftype)
//	{
//	case FIELD_BOOLEAN:		bVal = *((bool*)data);				break;
//	case FIELD_CHARACTER:	iVal = *((char*)data);				break;
//	case FIELD_SHORT:		iVal = *((short*)data);				break;
//	case FIELD_INTEGER:		iVal = *((int*)data);				break;
//	case FIELD_STRING:		iszVal = *((string_t*)data);		break;
//	case FIELD_FLOAT:		flVal = *((float*)data);			break;
//	case FIELD_COLOR32:		rgbaVal = *((color32*)data);		break;
//
//	case FIELD_VECTOR:
//	case FIELD_POSITION_VECTOR:
//	{
//		vecVal[0] = ((float*)data)[0];
//		vecVal[1] = ((float*)data)[1];
//		vecVal[2] = ((float*)data)[2];
//		break;
//	}
//
//	case FIELD_EHANDLE:		eVal = *((EHANDLE*)data);			break;
//	case FIELD_CLASSPTR:	eVal = *((CBaseEntity**)data);		break;
//	case FIELD_VOID:
//	default:
//		iVal = 0; fieldType = FIELD_VOID;
//		break;
//	}
//}

//-----------------------------------------------------------------------------
// Purpose: Copies the value in the variant into a block of memory
// Input  : *data - the block to write into
//-----------------------------------------------------------------------------
void variant_t::SetOther(void* data)
{
	switch (fieldType)
	{
	case FIELD_BOOLEAN:		*((bool*)data) = bVal != 0;			break;
	case FIELD_CHARACTER:	*((char*)data) = (char)iVal;		break;
	case FIELD_SHORT:		*((short*)data) = (short)iVal;		break;
	case FIELD_INTEGER:		*((int*)data) = iVal;				break;
	case FIELD_STRING:		*((string_t*)data) = iszVal;		break;
	case FIELD_FLOAT:		*((float*)data) = flVal;			break;
	case FIELD_COLOR32:		*((color32*)data) = rgbaVal;		break;

	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
	{
		((float*)data)[0] = vecVal[0];
		((float*)data)[1] = vecVal[1];
		((float*)data)[2] = vecVal[2];
		break;
	}

	case FIELD_EHANDLE:		*((EHANDLE*)data) = eVal;			break;
	case FIELD_CLASSPTR:	*((CBaseEntity**)data) = eVal;		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Converts the variant to a new type. This function defines which I/O
//			types can be automatically converted between. Connections that require
//			an unsupported conversion will cause an error message at runtime.
// Input  : newType - the type to convert to
// Output : Returns true on success, false if the conversion is not legal
//-----------------------------------------------------------------------------
//bool variant_t::Convert(fieldtype_t newType)
//{
//	if (newType == fieldType)
//	{
//		return true;
//	}
//
//	//
//	// Converting to a null value is easy.
//	//
//	if (newType == FIELD_VOID)
//	{
//		Set(FIELD_VOID, NULL);
//		return true;
//	}
//
//	//
//	// FIELD_INPUT accepts the variant type directly.
//	//
//	if (newType == FIELD_INPUT)
//	{
//		return true;
//	}
//
//	switch (fieldType)
//	{
//	case FIELD_INTEGER:
//	{
//		switch (newType)
//		{
//		case FIELD_FLOAT:
//		{
//			SetFloat((float)iVal);
//			return true;
//		}
//
//		case FIELD_BOOLEAN:
//		{
//			SetBool(iVal != 0);
//			return true;
//		}
//		}
//		break;
//	}
//
//	case FIELD_FLOAT:
//	{
//		switch (newType)
//		{
//		case FIELD_INTEGER:
//		{
//			SetInt((int)flVal);
//			return true;
//		}
//
//		case FIELD_BOOLEAN:
//		{
//			SetBool(flVal != 0);
//			return true;
//		}
//		}
//		break;
//	}
//
//	//
//	// Everyone must convert from FIELD_STRING if possible, since
//	// parameter overrides are always passed as strings.
//	//
//	case FIELD_STRING:
//	{
//		switch (newType)
//		{
//		case FIELD_INTEGER:
//		{
//			if (iszVal != NULL_STRING)
//			{
//				SetInt(atoi(STRING(iszVal)));
//			}
//			else
//			{
//				SetInt(0);
//			}
//			return true;
//		}
//
//		case FIELD_FLOAT:
//		{
//			if (iszVal != NULL_STRING)
//			{
//				SetFloat(atof(STRING(iszVal)));
//			}
//			else
//			{
//				SetFloat(0);
//			}
//			return true;
//		}
//
//		case FIELD_BOOLEAN:
//		{
//			if (iszVal != NULL_STRING)
//			{
//				SetBool(atoi(STRING(iszVal)) != 0);
//			}
//			else
//			{
//				SetBool(false);
//			}
//			return true;
//		}
//
//		case FIELD_VECTOR:
//		{
//			::Vector3D tmpVec = vec3_origin;
//			if (sscanf(STRING(iszVal), "[%f %f %f]", &tmpVec[0], &tmpVec[1], &tmpVec[2]) == 0)
//			{
//				// Try sucking out 3 floats with no []s
//				sscanf(STRING(iszVal), "%f %f %f", &tmpVec[0], &tmpVec[1], &tmpVec[2]);
//			}
//			SetVector3D(tmpVec);
//			return true;
//		}
//
//		case FIELD_COLOR32:
//		{
//			int nRed = 0;
//			int nGreen = 0;
//			int nBlue = 0;
//			int nAlpha = 255;
//
//			sscanf(STRING(iszVal), "%d %d %d %d", &nRed, &nGreen, &nBlue, &nAlpha);
//			SetColor32(nRed, nGreen, nBlue, nAlpha);
//			return true;
//		}
//
//		case FIELD_EHANDLE:
//		{
//			// convert the string to an entity by locating it by classname
//			CBaseEntity* ent = NULL;
//			if (iszVal != NULL_STRING)
//			{
//				// FIXME: do we need to pass an activator in here?
//				ent = gEntList.FindEntityByName(NULL, iszVal);
//			}
//			SetEntity(ent);
//			return true;
//		}
//		}
//
//		break;
//	}
//
//	case FIELD_EHANDLE:
//	{
//		switch (newType)
//		{
//		case FIELD_STRING:
//		{
//			// take the entities targetname as the string
//			string_t iszStr = NULL_STRING;
//			if (eVal != NULL)
//			{
//				SetString(eVal->GetEntityName());
//			}
//			return true;
//		}
//		}
//		break;
//	}
//	}
//
//	// invalid conversion
//	return false;
//}
