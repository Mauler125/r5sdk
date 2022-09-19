#pragma once

struct PluginHelpWithAnything_t;

abstract_class IPluginSystem
{
public:
	virtual void* HelpWithAnything(PluginHelpWithAnything_t * help) = 0;
};

constexpr auto INTERFACEVERSION_PLUGINSYSTEM = "VPluginSystem001";