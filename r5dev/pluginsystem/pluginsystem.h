#pragma once
#include "ipluginsystem.h"

#define PLUGIN_INSTALL_DIR "bin\\x64_retail\\plugins"

class CModAppSystemGroup;
class CServer;
class CClient;
struct user_creds_s;

template<typename T>
class CPluginCallbackList
{
public:
	CPluginCallbackList() {}
	CPluginCallbackList(const CUtlVector<T>& cbs)
	{
		for (auto it : cbs)
			m_vCallbacks.AddToTail(it);
	}

	CUtlVector<T>& GetCallbacks() { return m_vCallbacks; }

	operator bool()
	{
		return !this->m_vCallbacks.IsEmpty();
	}

	CUtlVector<T>& operator!()
	{
		return this->m_vCallbacks;
	}

	CPluginCallbackList<T>& operator+=(const T& rhs)
	{
		if (rhs)
			this->m_vCallbacks.AddToTail(rhs);

		return *this;
	}

	CPluginCallbackList<T>& operator+=(const CUtlVector<T>& rhs)
	{
		for (auto it : rhs)
		{
			if (it)
				this->m_vCallbacks.AddToTail(it);
		}

		return *this;
	}

	CPluginCallbackList<T>& operator-=(const T& rhs)
	{
		if (rhs)
		{
			const int fnd = m_vCallbacks.Find(rhs);

			if (fnd != m_vCallbacks.InvalidIndex())
				m_vCallbacks.Remove(fnd);
		}

		return *this;
	}

	CPluginCallbackList<T>& operator-=(const CUtlVector<T>& rhs)
	{
		for (auto itc : rhs)
		{
			if (itc) {
				const int fnd = m_vCallbacks.Find(itc);

				if (fnd != m_vCallbacks.InvalidIndex())
					m_vCallbacks.Remove(fnd);
			}
		}

		return *this;
	}

private:
	CUtlVector<T> m_vCallbacks;
};

class CPluginSystem : IPluginSystem
{
public:	
	struct PluginInstance_t
	{
		PluginInstance_t(const char* pName, const char* pPath, const char* pDescription = "")
			: m_Name(pName)
			, m_Path(pPath)
			, m_Description(pDescription)
			, m_bIsLoaded(false)
		{
		};

		// Might wanna make a status code system.
		typedef bool(*OnLoad)(const char*, const char*);
		typedef void(*OnUnload)();

		CModule m_hModule;
		CUtlString m_Name;
		CUtlString m_Path;
		CUtlString m_Description;
		bool m_bIsLoaded; // [ PIXIE ]: I don't like this and it's bad.
		// I will make a module manager later which will grab all modules from the process and adds each module / removes module that passes through DLLMain.
	};

	void Init();

	bool LoadInstance(PluginInstance_t& pluginInst);
	bool UnloadInstance(PluginInstance_t& pluginInst);
	bool ReloadInstance(PluginInstance_t& pluginInst);

	void AddCallback(PluginHelpWithAnything_t* help);
	void RemoveCallback(PluginHelpWithAnything_t* help);

	CUtlVector<PluginInstance_t>& GetInstances();

	virtual void* HelpWithAnything(PluginHelpWithAnything_t* help);

#define CREATE_PLUGIN_CALLBACK(typeName, type, funcName, varName) public: using typeName = type; CPluginCallbackList<typeName>& funcName() { return varName; } private: CPluginCallbackList<typeName> varName;

	CREATE_PLUGIN_CALLBACK(CreateFn, bool(*)(CModAppSystemGroup*), GetCreateCallbacks, createCallbacks);
	CREATE_PLUGIN_CALLBACK(ConnectClientFn, bool(*)(CServer*, CClient*, user_creds_s*), GetConnectClientCallbacks, connectClientCallbacks);

#undef CREATE_PLUGIN_CALLBACK

private:
	CUtlVector<PluginInstance_t> m_Instances;
};
extern CPluginSystem* g_pPluginSystem;

FORCEINLINE CPluginSystem* PluginSystem()
{
	return g_pPluginSystem;
}

// Monitor this and performance profile this if fps drops are detected.
#define CALL_PLUGIN_CALLBACKS(callback, ...)      \
	for (auto& cb : !callback)                    \
		cb(__VA_ARGS__)                            
