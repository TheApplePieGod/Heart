#pragma once

namespace Heart
{
    class Asset
    {
    public:
        enum class Type
        {
            None = 0,
            Texture, Shader, Mesh
        };
        inline static const char* TypeStrings[] = {
            "None", "Texture", "Shader", "Mesh"
        };

    public:
        Asset(const std::string& path);

        virtual void Load() = 0;
        virtual void Unload() = 0;
        void Reload();

        const std::string& GetPath() const { return m_Path; }
        const std::string& GetFilename() const { return m_Filename; }
        const void* GetRawData() const { return m_Data; }
        const bool IsLoaded() const { return m_Loaded; }
        const Type GetType() const { return m_Type; }

    public:
        static Ref<Asset> Create(Type type, const std::string& path);

    protected:
        std::string m_Path;
        std::string m_Filename;
        std::string m_Extension;
        void* m_Data = nullptr;
        bool m_Loaded = false;
        Type m_Type = Type::None;
    };
}