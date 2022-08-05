#include "hepch.h"
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
    class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
    {
    public:
        ShaderIncluder(const HString& basePath)
            : m_BasePath(basePath)
        {}

        shaderc_include_result* GetInclude (
            const char* requestedSource,
            shaderc_include_type type,
            const char* requestingSource,
            size_t includeDepth)
        {
            HString name(requestedSource);
            HString contents = FilesystemUtils::ReadFileToString(
                std::filesystem::path(m_BasePath.DataUTF8()).append(requestedSource).generic_u8string()
            );

            auto container = new std::array<HString, 2>;
            (*container)[0] = name;
            (*container)[1] = contents;

            auto data = new shaderc_include_result;

            data->user_data = container;

            data->source_name = (*container)[0].DataUTF8();
            data->source_name_length = (*container)[0].GetCountUTF8();

            data->content = (*container)[1].DataUTF8();
            data->content_length = (*container)[1].GetCountUTF8();

            return data;
        };

        void ReleaseInclude(shaderc_include_result* data) override
        {
            delete static_cast<std::array<HString, 2>*>(data->user_data);
            delete data;
        };

    private:
        HString m_BasePath;
    };

    Ref<Shader> Shader::Create(const HString& path, Type shaderType)
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

    std::vector<u32> Shader::CompileSpirvFromFile(const HString& path, Type shaderType)
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
        
        HString basePath = std::filesystem::path(path.DataUTF8()).parent_path().generic_u8string();
        options.SetIncluder(CreateScope<ShaderIncluder>(basePath));

        #ifdef HE_DEBUG
            options.SetOptimizationLevel(shaderc_optimization_level_zero);
        #else
            options.SetOptimizationLevel(shaderc_optimization_level_performance);
        #endif
        HString sourceCode = FilesystemUtils::ReadFileToString(path);
        if (sourceCode == "")
        {
            HE_ENGINE_LOG_ERROR("Failed to load file {0}", path.DataUTF8());
            HE_ENGINE_ASSERT(false);
        }

        shaderc_shader_kind shaderKind = shaderc_glsl_vertex_shader;
        switch (shaderType)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Can't compile unsupported shader type"); } break;
            case Type::Vertex: { shaderKind = shaderc_glsl_vertex_shader; } break;
            case Type::Fragment: { shaderKind = shaderc_glsl_fragment_shader; } break;
            case Type::Compute: { shaderKind = shaderc_glsl_compute_shader; } break;
        }

        shaderc::PreprocessedSourceCompilationResult preprocessedResult = compiler.PreprocessGlsl(sourceCode.DataUTF8(), shaderKind, path.DataUTF8(), options);
        if (preprocessedResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            HE_ENGINE_LOG_ERROR("Shader preprocessing failed: {0}", preprocessedResult.GetErrorMessage());
            HE_ENGINE_ASSERT(false);
        }

        HString preprocessed(preprocessedResult.begin());

        // run the custom preprocessor over the source code
        // {
        //     const char* token = "#use_dynamic_offsets";
        //     size_t tokenLen = strlen(token);

        //     size_t pos = preprocessed.find(token, 0);
        //     while (pos != HString::npos)
        //     {
        //         size_t eol = preprocessed.find_first_of("\r\n", pos);
        //         if (eol == HString::npos)
        //             eol = preprocessed.find_first_of("\n", pos);
        //         HE_ENGINE_ASSERT(eol != HString::npos, "Token must be followed by a newline");

        //         size_t numStart = pos + tokenLen + 1;
        //         u32 bindingIndex = atoi(preprocessed.substr(numStart, eol - numStart).c_str());
        //         m_PreprocessData.DynamicBindings.emplace_back(bindingIndex);

        //         pos = preprocessed.find(token, eol);
        //     }
        // }

        shaderc::SpvCompilationResult compiled = compiler.CompileGlslToSpv(preprocessed.DataUTF8(), shaderKind, path.DataUTF8(), options);
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
        else if (shaderType == Type::Compute)
            accessType = ShaderResourceAccessType::Compute;

		HE_ENGINE_LOG_TRACE("GLSL {0} shader @ {1}", TypeStrings[static_cast<u16>(shaderType)], m_Path.DataUTF8());
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

        if (resources.subpass_inputs.size() > 0)
		    HE_ENGINE_LOG_TRACE("  Subpass Inputs:");
		for (const auto& resource : resources.subpass_inputs)
		{
            const auto& subpassType = compiler.get_type(resource.type_id);
			u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            u32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            u32 attachmentIndex = compiler.get_decoration(resource.id, spv::DecorationInputAttachmentIndex);
            
            m_ReflectionData.emplace_back(resource.id, ShaderResourceType::SubpassInput, accessType, binding, set, 1);

			HE_ENGINE_LOG_TRACE("    Input", resource.name);
            HE_ENGINE_LOG_TRACE("      Set = {0}", set);
			HE_ENGINE_LOG_TRACE("      Binding = {0}", binding);
            HE_ENGINE_LOG_TRACE("      AttachmentIndex = {0}", attachmentIndex);

            HE_ENGINE_ASSERT(set == 0, "The 'set' glsl qualifier is currently unsupported and must be zero");
		}
    }
}