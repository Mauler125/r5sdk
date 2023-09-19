#include "stdafx.h"
#include "Registry.h"

namespace Win32
{
	RegistryKey Registry::CurrentUser = RegistryKey::OpenBaseKey(RegistryHive::CurrentUser, RegistryView::Default);
	RegistryKey Registry::LocalMachine = RegistryKey::OpenBaseKey(RegistryHive::LocalMachine, RegistryView::Default);
	RegistryKey Registry::ClassesRoot = RegistryKey::OpenBaseKey(RegistryHive::ClassesRoot, RegistryView::Default);
	RegistryKey Registry::Users = RegistryKey::OpenBaseKey(RegistryHive::Users, RegistryView::Default);
	RegistryKey Registry::CurrentConfig = RegistryKey::OpenBaseKey(RegistryHive::CurrentConfig, RegistryView::Default);
}