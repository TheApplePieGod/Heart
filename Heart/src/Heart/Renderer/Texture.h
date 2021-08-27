#pragma once

namespace Heart
{
    class Texture
    {
    public:
        Texture(const std::string& path)
            : m_Path(path)
        {}
        virtual ~Texture() = default;

    public:
        static Ref<Texture> Create(const std::string& path);

    protected:
        const int m_DesiredChannelCount = 4; // all images will load as RGBA
        std::string m_Path;
        int m_Width, m_Height, m_Channels;
    };

    class TextureRegistry
    {
    public:
        Ref<Texture> RegisterTexture(const std::string& name, const std::string& path);
        Ref<Texture> LoadTexture(const std::string& name);

    private:
        std::unordered_map<std::string, Ref<Texture>> m_Textures;
    };
}