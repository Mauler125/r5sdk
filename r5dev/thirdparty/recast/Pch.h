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
#include <vector>
#include <algorithm>

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

#endif // DTPCH_H
