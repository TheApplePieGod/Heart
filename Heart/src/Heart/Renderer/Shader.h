#pragma once

#include "Heart/Renderer/RenderApi.h"

namespace Heart
{
    class Shader
    {
    public:
        enum class Type
        {
            None = 0, Vertex = 1, Fragment = 2
        };

    public:
        Shader(const std::string& path, Type shaderType)
            : m_Path(path), m_Type(shaderType)
        {}
        virtual ~Shader() = default;

    public:
        static Ref<Shader> Create(const std::string& path, Type shaderType);

    protected:
        std::vector<u32> CompileSpirvFromFile(const std::string& path, Type shaderType);

    protected:
        Type m_Type;
        std::string m_Path;
        bool m_Loaded;
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