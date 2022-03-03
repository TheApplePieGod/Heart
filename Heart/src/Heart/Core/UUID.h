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
            return hash<u64>()(uuid);
        }
    };
}