#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef HE_PLATFORM_LINUX
    #define STBI_NO_SIMD // causing issues on linux
#endif

#include "stb_image.h"
#include "stb_image_write.h"