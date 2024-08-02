#ifndef NAVEDITORPCH_H
#define NAVEDITORPCH_H

#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <float.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>
#include <stdarg.h>

#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <fstream>

#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>

// Required for shared SDK code.
#include <regex>
#include <mutex>

#include "common/experimental.h"

#include "thirdparty/fastlz/fastlz.h"

#include "thirdparty/sdl/include/SDL.h"
#include "thirdparty/sdl/include/SDL_syswm.h"
#include "thirdparty/sdl/include/SDL_opengl.h"

#ifdef __APPLE__
#	include <OpenGL/glu.h>
#else
#	include <GL/glu.h>
#endif

#ifdef WIN32
#	define snprintf _snprintf
#	define putenv _putenv
#	include "commdlg.h"
#	include <io.h>
#else // Linux, BSD, OSX
#	include <dirent.h>
#	include <cstring>
#endif

// todo(amos): we need to make a new PCH dedicated for the recast editor and
// move the SDL2 and ImGui includes there!
#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/imgui/misc/imgui_style.h"
#include "thirdparty/imgui/misc/imgui_wrapper.h"
#include "thirdparty/imgui/misc/imgui_plotter.h"
#include "thirdparty/imgui/backends/imgui_impl_sdl2.h"
#include "thirdparty/imgui/backends/imgui_impl_opengl2.h"

// SDK types
#include "tier0/basetypes.h"
#include "tier0/commonmacros.h"
#include "common/sdkdefs.h"

#endif // NAVEDITORPCH_H
