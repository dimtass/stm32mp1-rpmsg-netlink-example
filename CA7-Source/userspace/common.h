#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string>

#ifdef __linux__
#include <unistd.h>
#define SSIZE_T_FORMAT "zu"
#elif _WIN32
#define SSIZE_T_FORMAT "Iu"
#endif