#pragma once

namespace Heart
{
    class RenderApi
    {
    public:
        enum class Type
        {
            None = 0, Vulkan = 1
        };

    public:
        virtual ~RenderApi() = default;

    private:
        
    };
}