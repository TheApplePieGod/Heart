#include "htpch.h"
#include "Shader.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanShader.h"
#include "Heart/Platform/OpenGL/OpenGLShader.h"
#include "shaderc/shaderc.hpp"
#include "spirv_cross/spirv_cross.hpp"
#include "spirv_cross/spirv_glsl.hpp"

namespace Heart
{
    Ref<Shader> Shader::Create(const std::string& path, Type shaderType)
    {
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create shader: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanShader>(path, shaderType); }
            case RenderApi::Type::OpenGL:
            { return CreateRef<OpenGLShader>(path, shaderType); }
        }
    }

    std::vector<u32> Shader::CompileSpirvFromFile(const std::string& path, Type shaderType)
    {
        shaderc::Compiler compiler;
		shaderc::CompileOptions options;

        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Can't compile shader for unsupported API type"); } break;
            case RenderApi::Type::Vulkan:
            { options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, 0); } break;
            case RenderApi::Type::OpenGL:
            { options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_opengl, 0); } break;
        }
        
        // TODO: dynamic option for this
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        std::string sourceCode = FilesystemUtils::LoadFile(path);

        shaderc_shader_kind shaderKind = shaderc_glsl_vertex_shader;
        switch (shaderType)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Can't compile unsupported shader type"); } break;
            case Type::Vertex: { shaderKind = shaderc_glsl_vertex_shader; } break;
            case Type::Fragment: { shaderKind = shaderc_glsl_fragment_shader; } break;
        }

        shaderc::SpvCompilationResult compiled = compiler.CompileGlslToSpv(sourceCode, shaderKind, path.c_str(), options);
        if (compiled.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            HE_ENGINE_LOG_ERROR("Shader compilation failed: {0}", compiled.GetErrorMessage());
            HE_ENGINE_ASSERT(false);
        }

        return std::vector<u32>(compiled.cbegin(), compiled.cend());
    }

    void Shader::Reflect(Type shaderType, const std::vector<u32>& compiledData)
    {
        m_ReflectionData.clear();

        spirv_cross::Compiler compiler(compiledData);
		spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        ShaderResourceAccessType accessType = ShaderResourceAccessType::Vertex;
        if (shaderType == Type::Fragment)
            accessType = ShaderResourceAccessType::Fragment;

		HE_ENGINE_LOG_TRACE("GLSL {0} shader @ {1}", TypeStrings[static_cast<u16>(shaderType)], m_Path);
		HE_ENGINE_LOG_TRACE("    {0} uniform buffers", resources.uniform_buffers.size());
        HE_ENGINE_LOG_TRACE("    {0} storage buffers", resources.storage_buffers.size());
		HE_ENGINE_LOG_TRACE("    {0} resources", resources.sampled_images.size());

        if (resources.uniform_buffers.size() > 0)
		    HE_ENGINE_LOG_TRACE("  Uniform buffers:");
		for (const auto& resource : resources.uniform_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			size_t bufferSize = compiler.get_declared_struct_size(bufferType);
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			size_t memberCount = bufferType.member_types.size();

            m_ReflectionData.emplace_back(resource.id, ShaderResourceType::UniformBuffer, accessType, binding, set, 1);

			HE_ENGINE_LOG_TRACE("    {0}", resource.name);
			HE_ENGINE_LOG_TRACE("      Size = {0}", bufferSize);
            HE_ENGINE_LOG_TRACE("      Set = {0}", set);
			HE_ENGINE_LOG_TRACE("      Binding = {0}", binding);
			HE_ENGINE_LOG_TRACE("      Members = {0}", memberCount);

            HE_ENGINE_ASSERT(set == 0, "The 'set' glsl qualifier is currently unsupported and must be zero");
		}

        if (resources.storage_buffers.size() > 0)
		    HE_ENGINE_LOG_TRACE("  Storage buffers:");
		for (const auto& resource : resources.storage_buffers)
		{
			const auto& bufferType = compiler.get_type(resource.base_type_id);
			size_t bufferSize = compiler.get_declared_struct_size(bufferType);
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			size_t memberCount = bufferType.member_types.size();

            m_ReflectionData.emplace_back(resource.id, ShaderResourceType::StorageBuffer, accessType, binding, set, 1);

			HE_ENGINE_LOG_TRACE("    {0}", resource.name);
			HE_ENGINE_LOG_TRACE("      Size = {0}", bufferSize);
            HE_ENGINE_LOG_TRACE("      Set = {0}", set);
			HE_ENGINE_LOG_TRACE("      Binding = {0}", binding);
			HE_ENGINE_LOG_TRACE("      Members = {0}", memberCount);

            HE_ENGINE_ASSERT(set == 0, "The 'set' glsl qualifier is currently unsupported and must be zero");
		}

        if (resources.sampled_images.size() > 0)
		    HE_ENGINE_LOG_TRACE("  Sampled Images:");
		for (const auto& resource : resources.sampled_images)
		{
            const auto& imageType = compiler.get_type(resource.type_id);
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            
            m_ReflectionData.emplace_back(resource.id, ShaderResourceType::Texture, accessType, binding, set, imageType.array.empty() ? 1 : imageType.array[0]);

			HE_ENGINE_LOG_TRACE("    Image", resource.name);
            HE_ENGINE_LOG_TRACE("      ArrayCount = {0}", imageType.array[0]);
            HE_ENGINE_LOG_TRACE("      Set = {0}", set);
			HE_ENGINE_LOG_TRACE("      Binding = {0}", binding);

            HE_ENGINE_ASSERT(set == 0, "The 'set' glsl qualifier is currently unsupported and must be zero");
		}
    }

    Ref<Shader> ShaderRegistry::RegisterShader(const std::string& name, const std::string& path, Shader::Type shaderType)
    {
        if (m_Shaders.find(name) != m_Shaders.end())
        {
            HE_ENGINE_LOG_ERROR("Cannot register shader, name already exists: {0}", name);
            HE_ENGINE_ASSERT(false);
        }

        HE_ENGINE_LOG_TRACE("Registering shader '{0}' @ '{1}'", name, path);

        Ref<Shader> newShader = Shader::Create(path, shaderType);
        m_Shaders[name] = newShader;
        return newShader;
    }
    
    Ref<Shader> ShaderRegistry::LoadShader(const std::string& name)
    {
        if (m_Shaders.find(name) == m_Shaders.end())
        {
            HE_ENGINE_LOG_ERROR("Shader not registered: {0}", name);
            HE_ENGINE_ASSERT(false);
        }
        return m_Shaders[name];
    }
}