#ifndef DTPCH_H
#define DTPCH_H

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

#include "NavEditor/Include/imgui.h"
#include "NavEditor/Include/imguiRenderGL.h"

// SDK types
#include "common/sdkdefs.h"

typedef uint8_t byte;
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

#endif // DTPCH_H
