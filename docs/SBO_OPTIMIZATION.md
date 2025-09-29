# Small Buffer Optimization (SBO) in Reaction Framework

## Overview

The Reaction framework now supports Small Buffer Optimization (SBO) to reduce heap allocations and improve performance for small objects. This optimization stores small values directly within the reactive node object instead of allocating them on the heap.

## Benefits

- **Reduced heap allocations**: Small objects are stored on the stack, eliminating malloc/free overhead
- **Better cache locality**: Data is stored closer to the reactive node, improving CPU cache performance  
- **Lower memory fragmentation**: Fewer small heap allocations reduce memory fragmentation
- **Improved performance**: Especially beneficial for basic types and small structs

## Configuration

SBO is disabled by default to maintain backward compatibility. To enable it, define the macro before including Reaction headers:

```cpp
#define REACTION_ENABLE_SBO 1
#include <reaction/reaction.h>
```

## SBO Criteria

A type will use SBO if it meets all these conditions:

1. **Size**: `sizeof(Type) <= 32` bytes (configurable via `SBO_THRESHOLD`)
2. **Trivially destructible**: `std::is_trivially_destructible_v<Type>` is true
3. **Standard alignment**: `alignof(Type) <= alignof(std::max_align_t)`

## Supported Types

### Types that use SBO (when enabled):

- Basic types: `int`, `float`, `double`, `bool`, `char`, etc.
- Small POD structs (≤ 32 bytes)
- Small arrays: `std::array<int, 8>`, etc.
- Enums
- Simple classes with trivial destructors

### Types that still use heap allocation:

- `std::string` (typically > 32 bytes)
- `std::vector`, `std::map`, etc.
- Large structs or classes
- Types with non-trivial destructors
- Types with unusual alignment requirements

## Usage Examples

### Basic Usage

```cpp
#define REACTION_ENABLE_SBO 1
#include <reaction/reaction.h>

// Small types use SBO automatically
auto int_var = reaction::var(42);        // Uses SBO
auto double_var = reaction::var(3.14);   // Uses SBO
auto string_var = reaction::var(std::string("hello"));  // Uses heap

// Check if SBO is being used
std::cout << "int uses SBO: " << decltype(int_var)::react_type::isUsingSBO() << std::endl;
```

### Custom Small Structs

```cpp
struct Point {
    int x, y;
    float z;
    
    Point(int x = 0, int y = 0, float z = 0.0f) : x(x), y(y), z(z) {}
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
};

// This will use SBO (12 bytes < 32 byte threshold)
auto point_var = reaction::var(Point{1, 2, 3.5f});

// Reactive calculations also benefit from SBO
auto distance = reaction::calc([](const Point& p) {
    return std::sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
}, point_var);
```

### Runtime Information

```cpp
#include <reaction/reaction.h>

// Check SBO configuration
std::cout << "SBO Enabled: " << reaction::ResourceInfo::isSBOEnabled() << std::endl;
std::cout << "SBO Threshold: " << reaction::SBO_THRESHOLD << " bytes" << std::endl;

// Check storage strategy for specific types
std::cout << "int storage: " << reaction::ResourceInfo::getStorageStrategy<int>() << std::endl;
std::cout << "string storage: " << reaction::ResourceInfo::getStorageStrategy<std::string>() << std::endl;

// Get storage sizes
std::cout << "int storage size: " << reaction::ResourceInfo::getStorageSize<int>() << " bytes" << std::endl;
```

## Performance Considerations

### When SBO helps most:

- Applications with many small reactive variables
- Frequent creation/destruction of reactive nodes
- Performance-critical code with tight loops
- Cache-sensitive applications

### When SBO has minimal impact:

- Applications using mostly large objects
- Long-lived reactive nodes (creation cost amortized)
- I/O bound applications

### Memory Usage:

```cpp
// Without SBO: 
// sizeof(unique_ptr<int>) + heap allocation overhead
// Typically: 8 bytes + ~16-32 bytes overhead = 24-40 bytes total

// With SBO:
// Direct storage within the object
// Typically: 4 bytes for int + minimal overhead
```

## Backward Compatibility

SBO is fully backward compatible:

- Existing code continues to work unchanged
- API remains identical
- Only internal storage implementation changes
- Can be enabled/disabled per compilation unit

## Customization

### Adjusting the SBO threshold:

```cpp
// Before including headers
#define REACTION_ENABLE_SBO 1
#define SBO_THRESHOLD 64  // Allow larger objects in SBO

#include <reaction/reaction.h>
```

### Type-specific SBO control:

```cpp
// Specialize the trait to force heap allocation for a type
namespace reaction {
template<>
constexpr bool should_use_sbo_v<MyType> = false;
}
```

## Testing and Validation

The framework includes comprehensive tests for SBO:

```bash
# Run SBO-specific tests
./test_sbo_optimization

# Run SBO benchmarks
./bench_sbo

# Run example demonstrating SBO
./sbo_example
```

## Migration Guide

### From non-SBO to SBO:

1. Add `#define REACTION_ENABLE_SBO 1` before includes
2. Recompile - no code changes needed
3. Verify performance improvements with benchmarks
4. Test thoroughly, especially with custom types

### Considerations:

- SBO may change object sizes - important for memory layout sensitive code
- Move semantics may behave slightly differently
- Custom allocators are not supported with SBO

## Implementation Details

The SBO implementation uses a union-based approach:

```cpp
union Storage {
    alignas(Type) std::byte stack_storage[sizeof(Type)];  // SBO storage
    std::unique_ptr<Type> heap_ptr;                       // Fallback to heap
};
```

This ensures:
- Zero overhead when SBO is not used
- Proper alignment for all types
- Exception safety
- Thread safety (with appropriate locking)

## Best Practices

1. **Profile first**: Measure performance before and after enabling SBO
2. **Test thoroughly**: Ensure your types work correctly with SBO
3. **Consider alignment**: Ensure custom types have reasonable alignment
4. **Use trivial destructors**: For maximum SBO eligibility
5. **Monitor memory usage**: SBO changes memory usage patterns

## Troubleshooting

### Common issues:

1. **Type not using SBO**: Check size, alignment, and destructor requirements
2. **Compilation errors**: Ensure types are properly copyable/moveable
3. **Runtime errors**: Verify custom types have correct comparison operators

### Debugging SBO usage:

```cpp
// Check if a type qualifies for SBO
static_assert(reaction::should_use_sbo_v<MyType>, "Type should use SBO");

// Runtime checks
if (!ResourceInfo::getStorageStrategy<MyType>() == "stack (SBO)") {
    std::cerr << "Type not using SBO as expected" << std::endl;
}
```