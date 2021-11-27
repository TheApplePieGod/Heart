#pragma once

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
        Vertex, Fragment, Both
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

    struct ShaderPreprocessData
    {
        std::vector<u32> DynamicBindings;
    };

    class Shader
    {
    public:
        enum class Type
        {
            None = 0, Vertex = 1, Fragment = 2
        };
        inline static const char* TypeStrings[] = {
            "None", "Vertex", "Fragment"
        };
        
    public:
        Shader(const std::string& path, Type shaderType)
            : m_Path(path), m_Type(shaderType)
        {}
        virtual ~Shader() = default;

        inline const std::vector<ReflectionDataElement>& GetReflectionData() { return m_ReflectionData; }
        inline const ShaderPreprocessData& GetPreprocessData() const { return m_PreprocessData; }

    public:
        static Ref<Shader> Create(const std::string& path, Type shaderType);

    protected:
        std::vector<u32> CompileSpirvFromFile(const std::string& path, Type shaderType);
        void Reflect(Type shaderType, const std::vector<u32>& compiledData);

    protected:
        Type m_Type;
        std::string m_Path;
        bool m_Loaded;
        std::vector<ReflectionDataElement> m_ReflectionData;
        ShaderPreprocessData m_PreprocessData;
    };
}