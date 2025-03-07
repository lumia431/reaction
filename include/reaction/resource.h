// include/reaction/Resource.h
#ifndef REACTION_RESOURCE_H
#define REACTION_RESOURCE_H

#include <memory>

namespace reaction
{
    template <typename T>
    class Resource
    {
    public:
        Resource(std::shared_ptr<T> ptr = nullptr) : m_ptr(ptr) {}

        template <typename Type>
        Resource(Type &&t) : m_ptr(std::make_shared<T>(std::forward<Type>(t))) {}

    protected:
        T getValue()
        {
            if (!m_ptr)
            {
                throw std::runtime_error("Attempt to dereference a null pointer");
            }
            return *m_ptr;
        }

        template <typename Type>
        void updateValue(Type &&t)
        {
            if (!m_ptr)
            {
                m_ptr = std::make_shared<T>(t);
            }
            *m_ptr = std::forward<Type>(t);
        }

    private:
        std::shared_ptr<T> m_ptr;
    };

} // namespace reaction

#endif // REACTION_RESOURCE_H