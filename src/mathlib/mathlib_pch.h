#ifndef MATHLIB_PCH_H
#define MATHLIB_PCH_H

#include <windows.h>
#include <assert.h>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <smmintrin.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <regex>
#include <mutex>
#include <tchar.h>

#include <unordered_map>
#include <map>
#include <vector>

#include "..\common\pseudodefs.h"
#include "..\common\sdkdefs.h"

#include "tier0/platform.h"
#include "tier0/basetypes.h"

#define Assert assert // TODO: Include actual assert header

#endif // MATHLIB_PCH_H
