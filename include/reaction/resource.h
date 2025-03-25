// include/reaction/Resource.h
#ifndef REACTION_RESOURCE_H
#define REACTION_RESOURCE_H

#include <memory>
#include <mutex>
#include <exception>

namespace reaction
{
    template <typename T>
    class Resource
    {
    public:
        Resource() : m_ptr(nullptr) {}

        template <typename Type>
        Resource(Type &&t) : m_ptr(std::make_unique<T>(std::forward<Type>(t))) {}

    protected:
        T getValue()
        {
            if (!m_ptr)
            {
                throw std::runtime_error("Attempt to use a null pointer");
            }
            return *m_ptr;
        }

        template <typename Type>
        void updateValue(Type &&t)
        {
            if (!m_ptr)
            {
                m_ptr = std::make_unique<T>(std::forward<Type>(t));
            }
            else
            {
                *m_ptr = std::forward<Type>(t);
            }
        }

    private:
        std::unique_ptr<T> m_ptr;
    };

} // namespace reaction

#endif // REACTION_RESOURCE_H