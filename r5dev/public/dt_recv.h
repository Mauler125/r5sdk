#pragma once
#include "public/dt_common.h"

class CRecvProp;

class CRecvTable
{
public:
	char pad_0000[0x8];
	CRecvProp** m_pProps;
	int m_nProps;
	char pad_0014[0x4AC];
	void* m_pDecoder;
	char* m_pNetTableName;
	bool m_bInitialized;
	bool m_bInMainList;
};

class CRecvProp
{
public:
	SendPropType m_RecvType;
	int m_Offset;
	char pad_0008[24];
	CRecvTable* m_pDataTable;
	char* m_pVarName;
	bool m_bInsideArray;
	char pad_0031[0x7];
	CRecvProp* m_pArrayProp;
	void* m_ProxyFn;
	char pad_0048[0xC];
	int m_nFlags;
	char pad_0058[0x4];
	int m_nElements;
};