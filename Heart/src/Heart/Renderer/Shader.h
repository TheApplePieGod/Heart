#pragma once

#include "Heart/Container/HString.h"

namespace Heart
{
    enum class ShaderResourceType
    {
        None = 0,
        UniformBuffer, StorageBuffer, Texture, SubpassInput
    };

    enum class ShaderResourceAccessType
    {
        None = 0,
        Vertex, Fragment, Both, Compute
    };

    struct ReflectionDataElement
    {
        ReflectionDataElement(u32 uniqueId, ShaderResourceType resourceType, ShaderResourceAccessType accessType, u32 bindingIndex, u32 setIndex, u32 arrayCount)
            : UniqueId(uniqueId), ResourceType(resourceType), AccessType(accessType), BindingIndex(bindingIndex), SetIndex(setIndex), ArrayCount(arrayCount)
        {}

        u32 UniqueId;
        ShaderResourceType ResourceType;
        ShaderResourceAccessType AccessType;
        u32 BindingIndex;
        u32 SetIndex;
        u32 ArrayCount;
    };

    class Shader
    {
    public:
        enum class Type
        {
            None = 0, Vertex = 1, Fragment = 2, Compute = 3
        };
        inline static const char* TypeStrings[] = {
            "None", "Vertex", "Fragment", "Compute"
        };
        
    public:
        Shader(const HString& path, Type shaderType)
            : m_Path(path), m_Type(shaderType)
        {}
        virtual ~Shader() = default;

        inline const std::vector<ReflectionDataElement>& GetReflectionData() { return m_ReflectionData; }

    public:
        static Ref<Shader> Create(const HString& path, Type shaderType);

    protected:
        std::vector<u32> CompileSpirvFromFile(const HString& path, Type shaderType);
        void Reflect(Type shaderType, const std::vector<u32>& compiledData);

    protected:
        Type m_Type;
        HString m_Path;
        bool m_Loaded;
        std::vector<ReflectionDataElement> m_ReflectionData;
    };
}