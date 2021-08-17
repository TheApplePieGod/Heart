#include "htpch.h"
#include "Shader.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanShader.h"
#include "shaderc/shaderc.hpp"

namespace Heart
{
    Ref<Shader> Shader::Create(const std::string& path, Type shaderType)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HT_ENGINE_ASSERT(false, "Cannot create shader: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanShader>(path, shaderType); }
        }
    }

    std::vector<u32> Shader::CompileSpirvFromFile(const std::string& path, Type shaderType)
    {
        shaderc::Compiler compiler;
		shaderc::CompileOptions options;

        switch (Renderer::GetApiType())
        {
            default:
            { HT_ENGINE_ASSERT(false, "Can't compile shader for unsupported API type"); } break;

            case RenderApi::Type::Vulkan:
            { options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, 0); } break;
        }
        
        // TODO: dynamic option for this
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        std::string sourceCode = FilesystemUtils::LoadFile(path);

        shaderc_shader_kind shaderKind = shaderc_glsl_vertex_shader;
        switch (shaderType)
        {
            default:
            { HT_ENGINE_ASSERT(false, "Can't compile unsupported shader type"); } break;
            case Type::Vertex: { shaderKind = shaderc_glsl_vertex_shader; } break;
            case Type::Fragment: { shaderKind = shaderc_glsl_fragment_shader; } break;
        }

        shaderc::SpvCompilationResult compiled = compiler.CompileGlslToSpv(sourceCode, shaderKind, path.c_str(), options);
        if (compiled.GetCompilationStatus() != shaderc_compilation_status_success)
            HT_ENGINE_ASSERT(false, "Shader compilation failed: {1}", compiled.GetErrorMessage());

        // TODO: reflect shader using spirv-cross
        return std::vector<u32>(compiled.cbegin(), compiled.cend());
    }

    Ref<Shader> ShaderRegistry::RegisterShader(const std::string& name, const std::string& path, Shader::Type shaderType)
    {
        HT_ENGINE_ASSERT(m_Shaders.find(name) == m_Shaders.end(), "Cannot register shader, name already exists: {1}", name);
        Ref<Shader> newShader = Shader::Create(path, shaderType);
        m_Shaders[name] = newShader;
        return newShader;
    }
    
    Ref<Shader> ShaderRegistry::LoadShader(const std::string& name)
    {
        HT_ENGINE_ASSERT(m_Shaders.find(name) != m_Shaders.end(), "Shader not registered: {1}", name);
        return m_Shaders[name];
    }
}