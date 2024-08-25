#include "hepch.h"

#ifdef HE_ENABLE_TRACY
void* operator new(decltype(sizeof(0)) n) noexcept(false)
{
    auto ptr = malloc(n);
    TracyAlloc(ptr, n);
    return ptr;
}

void operator delete(void* p) noexcept
{
    TracyFree(p);
    free(p);
}
#endif
