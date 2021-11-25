#pragma once

namespace Heart
{
    class Asset
    {
    public:
        enum class Type
        {
            None = 0,
            Texture, Shader, Mesh, Material
        };
        inline static const char* TypeStrings[] = {
            "None", "Texture", "Shader", "Mesh", "Material"
        };

    public:
        Asset(const std::string& path, const std::string& absolutePath);

        virtual void Load() = 0;
        virtual void Unload() = 0;
        void Reload();

        inline const std::string& GetPath() const { return m_Path; }
        inline const std::string& GetAbsolutePath() const { return m_AbsolutePath; }
        inline const std::string& GetFilename() const { return m_Filename; }
        inline void* GetRawData() const { return m_Data; }
        inline bool IsLoaded() const { return m_Loaded; }
        inline bool IsLoading() const { return m_Loading; }
        inline bool IsValid() const { return m_Valid; }
        inline Type GetType() const { return m_Type; }

    public:
        static Ref<Asset> Create(Type type, const std::string& path, const std::string& absolutePath);

        static std::vector<unsigned char> Base64Decode(const std::string& encoded);
        inline static bool IsBase64(unsigned char c) { return (isalnum(c) || (c == '+') || (c == '/')); }

    protected:
        std::string m_Path;
        std::string m_AbsolutePath;
        std::string m_ParentPath;
        std::string m_Filename;
        std::string m_Extension;
        void* m_Data = nullptr;
        bool m_Loaded = false;
        bool m_Loading = false;
        bool m_Valid = false;
        Type m_Type = Type::None;

    protected:
        static inline const std::string s_Base64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
    };
}