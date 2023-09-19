#pragma once

#include <cstdint>
#include "RegistryKey.h"

namespace Win32
{
	class Registry
	{
	public:
		// This key should be used as the root for all user specific settings.
		static RegistryKey CurrentUser;
		// This key should be used as the root for all machine specific settings.
		static RegistryKey LocalMachine;
		// This is the root key of class information.
		static RegistryKey ClassesRoot;
		// This is the root of users.
		static RegistryKey Users;
		// This is where current configuration information is stored.
		static RegistryKey CurrentConfig;
	};
}