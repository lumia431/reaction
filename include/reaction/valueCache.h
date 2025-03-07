// include/reaction/ValueCache.h
#ifndef REACTION_VALUECACHE_H
#define REACTION_VALUECACHE_H

namespace reaction {

template <typename T>
class ValueCache {
public:
    ValueCache(T initialValue) : value_(initialValue) {}

    void setValue(T newValue) { value_ = newValue; }
    T getValue() const { return value_; }

private:
    T value_;
};

} // namespace reaction

#endif // REACTION_VALUECACHE_H