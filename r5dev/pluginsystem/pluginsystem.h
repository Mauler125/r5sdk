#pragma once
#include "ipluginsystem.h"

class CModAppSystemGroup;

struct PluginHelpWithAnything_t
{
	enum class ePluginHelp : int16_t
	{
		PLUGIN_GET_FUNCTION = 0,
		PLUGIN_REGISTER_CALLBACK,
		PLUGIN_UNREGISTER_CALLBACK
	};

	enum ePluginCallback : int16_t
	{
		CModAppSystemGroup_Create = 0
	};

	ePluginHelp m_nHelpID;
	ePluginCallback m_nCallbackID;
	const char* m_pszName;
	void* m_pFunction;
};

template<typename T>
class CPluginCallbackList
{
public:
	CPluginCallbackList() : m_vCallbacks() {}
	CPluginCallbackList(const vector<T>& cbs) : m_vCallbacks(cbs) {}

	vector<T>& GetCallbacks() { return m_vCallbacks; }

	operator bool()
	{
		return !this->m_vCallbacks.empty;
	}

	vector<T>& operator!()
	{
		return this->m_vCallbacks;
	}

	CPluginCallbackList<T>& operator+=(const T& rhs)
	{
		if (rhs)
			this->m_vCallbacks.push_back(rhs);

		return *this;
	}

	CPluginCallbackList<T>& operator+=(const vector<T>& rhs)
	{
		for (auto it : rhs)
		{
			if (it)
				this->m_vCallbacks.push_back(it);
		}

		return *this;
	}

	CPluginCallbackList<T>& operator-=(const T& rhs)
	{
		if (rhs) {
			auto it = std::find(m_vCallbacks.begin(), m_vCallbacks.end(), rhs);
			if (it != m_vCallbacks.end())
				m_vCallbacks.erase(it);
		}

		return *this;
	}

	CPluginCallbackList<T>& operator-=(const vector<T>& rhs)
	{
		for (auto itc : rhs)
		{
			if (itc) {
				auto it = std::find(m_vCallbacks.begin(), m_vCallbacks.end(), itc);
				if (it != m_vCallbacks.end())
					m_vCallbacks.erase(it);
			}
		}

		return *this;
	}

private:
	vector<T> m_vCallbacks;
};

class CPluginSystem : IPluginSystem
{
public:	
	struct PluginInstance_t
	{
		PluginInstance_t(string svPluginName, string svPluginFullPath) : m_svPluginName(svPluginName), m_svPluginFullPath(svPluginFullPath), m_svDescription(std::string()), m_bIsLoaded(false) {};

		// Might wanna make a status code system.
		typedef bool(*OnLoad)(const char*);
		typedef void(*OnUnload)();

		CModule m_hModule;
		string m_svPluginName;
		string m_svPluginFullPath;
		string m_svDescription;
		bool m_bIsLoaded; // [ PIXIE ]: I don't like this and it's bad.
		// I will make a module manager later which will grab all modules from the process and adds each module / removes module that passes through DLLMain.
	};

	void PluginSystem_Init();
	bool ReloadPluginInstance(PluginInstance_t& pluginInst);
	bool LoadPluginInstance(PluginInstance_t& pluginInst);
	bool UnloadPluginInstance(PluginInstance_t& pluginInst);
	void AddPluginCallback(PluginHelpWithAnything_t* help);
	void RemovePluginCallback(PluginHelpWithAnything_t* help);

	vector<PluginInstance_t>& GetPluginInstances();

	virtual void* HelpWithAnything(PluginHelpWithAnything_t* help);

#define CREATE_PLUGIN_CALLBACK(typeName, type, funcName, varName) public: using typeName = type; CPluginCallbackList<typeName>& funcName() { return varName; } private: CPluginCallbackList<typeName> varName;

	CREATE_PLUGIN_CALLBACK(CreateFn, bool(*)(CModAppSystemGroup*), GetCreateCallbacks, createCallbacks);

#undef CREATE_PLUGIN_CALLBACK

private:
	vector<PluginInstance_t> pluginInstances;
};
extern CPluginSystem* g_pPluginSystem;

// Monitor this and performance profile this if fps drops are detected.
#define CALL_PLUGIN_CALLBACKS(callback, ...)      \
	for (auto& cb : !callback)                    \
		cb(__VA_ARGS__)                                   