#pragma once

namespace Heart
{
    enum class ShaderResourceType
    {
        None = 0,
        UniformBuffer, StorageBuffer, Texture
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
    };

    class ShaderRegistry
    {
    public:
        Ref<Shader> RegisterShader(const std::string& name, const std::string& path, Shader::Type shaderType);
        Ref<Shader> LoadShader(const std::string& name);

    private:
        std::unordered_map<std::string, Ref<Shader>> m_Shaders;
    };
}