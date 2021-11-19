#include "htpch.h"
#include "UUID.h"

namespace Heart
{
    static std::random_device s_RandomDevice;
    static std::mt19937_64 s_Generator(s_RandomDevice());
    static std::uniform_int_distribution<u64> s_Distribution;

    UUID::UUID()
        : m_UUID(s_Distribution(s_Generator))
    {}
}