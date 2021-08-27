#pragma once

#include "Heart/Renderer/Shader.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Texture.h"

namespace Heart
{
    enum class ShaderInputType
    {
        None = 0, Texture, UniformBuffer, StorageBuffer
    };

    enum class ShaderBindType
    {
        None = 0, Vertex, Fragment, Both
    };

    struct ShaderInputElement
    {
        ShaderInputType InputType;
        ShaderBindType BindType;
        u32 BindIndex;
        u32 ArrayCount = 1; // if the input element is an array
    };

    struct ShaderInputBindElement
    {
        Ref<Buffer> TargetBuffer = nullptr;
        Ref<Texture> TargetTexture = nullptr;
    };

    struct ShaderInputBindPoint
    {
        void* BindData;
        u32 BufferCount;
        u32 ImageCount;
    };

    class ShaderInputSet
    {
    public:
        ShaderInputSet(std::initializer_list<ShaderInputElement> elements)
            : m_Elements(elements)
        {}
        virtual ~ShaderInputSet() = default;

        virtual ShaderInputBindPoint CreateBindPoint(const std::vector<ShaderInputBindElement>& bindElements) = 0;

    public:
        static Ref<ShaderInputSet> Create(std::initializer_list<ShaderInputElement> elements);

    protected:
        std::vector<ShaderInputElement> m_Elements;
    };

    class ShaderInputRegistry
    {
    public:
        Ref<ShaderInputSet> RegisterShaderInputSet(const std::string& name, std::initializer_list<ShaderInputElement> elements);
        Ref<ShaderInputSet> LoadShaderInputSet(const std::string& name);

    private:
        std::unordered_map<std::string, Ref<ShaderInputSet>> m_ShaderInputSets;
    };
}