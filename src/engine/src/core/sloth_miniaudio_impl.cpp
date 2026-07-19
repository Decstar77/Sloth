// Single translation unit that instantiates the miniaudio single-header
// library implementation (https://github.com/mackron/miniaudio). Every
// other file in the engine includes miniaudio.h declaration-only.

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
