#pragma once

namespace Heart
{
    class UUID
    {
    public:
        /*! @brief Default constructor. */
        UUID();

        /**
         * @brief Default constructor.
         *
         * @param uuid A UUID represented as a 64-bit integer.
         */
        UUID(u64 uuid)
            : m_UUID(uuid)
        {}

        inline operator u64() const { return m_UUID; }
        inline bool operator==(const UUID& other) const { return m_UUID == other; }

    private:
        u64 m_UUID;
    };
}

// Implement hash functionality for UUID
namespace std
{
    template<>
    struct hash<Heart::UUID>
    {
        std::size_t operator()(const Heart::UUID& uuid) const
        {
            return static_cast<std::size_t>(uuid);
        }
    };
}

// Implement format functionality for UUID
template<>
struct fmt::formatter<Heart::UUID> : fmt::formatter<std::string>
{
    auto format(Heart::UUID& val, format_context &ctx) const -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{}", (u64)val);
    }
};
