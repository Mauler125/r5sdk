#pragma once

#include <functional>

typedef std::function<void(void)> ThreadStart;
typedef std::function<void(void*)> ParameterizedThreadStart;