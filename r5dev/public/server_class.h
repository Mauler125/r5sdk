#pragma once

class SendTable;

//-----------------------------------------------------------------------------
// Purpose: Server side class definition
//-----------------------------------------------------------------------------
class ServerClass
{
public:
	const char* GetName(void) const
	{
		return m_pNetworkName;
	}

public:
	char* m_pNetworkName;
	SendTable* m_pSendTable;
	ServerClass* m_pNext;
	int m_Unknown1;
	int m_ClassID;
	int m_InstanceBaselineIndex;
};

// If you do a DECLARE_SERVERCLASS, you need to do this inside the class definition.
#define DECLARE_SERVERCLASS()									\
	public:														\
		virtual ServerClass* GetServerClass();					\
		static SendTable *m_pClassSendTable;					\
		template <typename T> friend int ServerClassInit(T *);	\
		virtual int YouForgotToImplementOrDeclareServerClass();	\

#define DECLARE_SERVERCLASS_NOBASE()							\
	public:														\
		template <typename T> friend int ServerClassInit(T *);	\