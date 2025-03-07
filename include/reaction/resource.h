// include/reaction/Resource.h
#ifndef REACTION_RESOURCE_H
#define REACTION_RESOURCE_H

#include <memory>
#include <mutex>

namespace reaction {

    template <typename T>
    class Resource {
    public:
        explicit Resource(std::shared_ptr<T> ptr = nullptr)
            : ptr_(ptr) {}

        void lock() { mutex_.lock(); }
        void unlock() { mutex_.unlock(); }

    private:
        std::shared_ptr<T> ptr_;
        std::mutex mutex_; // 线程安全锁
    };

} // namespace reaction

#endif // REACTION_RESOURCE_H