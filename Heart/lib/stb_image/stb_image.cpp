#define STB_IMAGE_IMPLEMENTATION

#ifdef HE_PLATFORM_LINUX
    #define STBI_NO_SIMD // causing issues on linux
#endif

#include "stb_image.h"