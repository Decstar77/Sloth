// Single translation unit that instantiates the stb single-header library
// implementations (https://github.com/nothings/stb). Every other file in
// the engine includes these headers declaration-only.

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
