
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <float.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>
#include <stdarg.h>

#ifdef WIN32
#	include <io.h>
#else
#	include <dirent.h>
#	include <cstring>
#endif

#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <fstream>

#include <set>
#include <vector>
#include <algorithm>

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
#endif

#include "NavEditor/Include/imgui.h"
#include "NavEditor/Include/imguiRenderGL.h"