#include <atomic>
#include <unordered_set>
#include <unordered_map>
namespace reaction
{

    class UniqueID
    {
    public:
        UniqueID() : m_id(generate())
        {
        }

        operator uint64_t()
        {
            return m_id;
        }

        bool operator==(const UniqueID &other) const
        {
            return m_id == other.m_id;
        }

    private:
        uint64_t m_id;

        static uint64_t generate()
        {
            static std::atomic<uint64_t> counter{0};
            return counter.fetch_add(1, std::memory_order_relaxed);
        }
        friend struct std::hash<UniqueID>;
    };
} // namespace reaction

namespace std
{
    template <>
    struct hash<reaction::UniqueID>
    {
        std::size_t operator()(const reaction::UniqueID &uid) const noexcept
        {
            return std::hash<uint64_t>{}(uid.m_id);
        }
    };

} // namespace std
