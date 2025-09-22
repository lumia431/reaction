# Contributing to Reaction Framework

Thank you for your interest in contributing to the Reaction framework! This document outlines the development standards and guidelines for maintaining code quality.

## Code Formatting Standards

### Automated Formatting

We use **clang-format** for consistent code formatting across the project. The configuration is defined in `.clang-format` at the project root.

**Before submitting any code:**

```bash
# Format all C++ files
find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs clang-format -i

# Verify formatting is correct
find . -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs clang-format --dry-run -Werror
```

### Key Formatting Rules

- **Indentation**: 4 spaces (no tabs)
- **Line length**: 100 characters maximum
- **Braces**: Attach style (opening brace on same line)
- **Pointer/Reference alignment**: Left-aligned (`int* ptr`, `int& ref`)
- **Include sorting**: Automatic with grouping by type
- **Spacing**: Consistent spacing around operators and keywords

### Editor Configuration

An `.editorconfig` file is provided to ensure consistent behavior across different editors and IDEs. Most modern editors support EditorConfig automatically.

## Code Quality Standards

### Static Analysis

We use **clang-tidy** for static code analysis. Configuration is in `.clang-tidy`.

```bash
# Run clang-tidy on specific files
clang-tidy src/file.cpp -- -std=c++20

# Run on all source files (requires compile_commands.json)
find . -name "*.cpp" | xargs clang-tidy
```

### Naming Conventions

- **Classes/Structs**: `PascalCase` (e.g., `ReactImpl`, `ObserverGraph`)
- **Functions/Methods**: `camelCase` (e.g., `getValue`, `setName`)
- **Variables**: `camelCase` (e.g., `currentPrice`, `weakRefCount`)
- **Private members**: `m_` prefix (e.g., `m_weakPtr`, `m_ptrMutex`)
- **Constants**: `PascalCase` (e.g., `DefaultCapacity`)
- **Namespaces**: `lowercase` (e.g., `reaction`)
- **Macros**: `UPPER_CASE` (e.g., `REACTION_THROW_NULL_POINTER`)

### C++ Best Practices

#### Modern C++ Features
- Use C++20 features where appropriate (concepts, ranges, etc.)
- Prefer `auto` for type deduction when it improves readability
- Use range-based for loops instead of traditional loops
- Utilize smart pointers (`std::unique_ptr`, `std::shared_ptr`)

#### Memory Management
- Follow RAII principles
- Use smart pointers for ownership semantics
- Avoid raw `new`/`delete` - use `std::make_unique`/`std::make_shared`
- Be explicit about object lifetimes

#### Error Handling
- Use exceptions for exceptional conditions
- Provide strong exception safety guarantees
- Document exception specifications in function comments

#### Documentation
- Use Doxygen-style comments for public APIs
- Include `@brief`, `@param`, `@return`, and `@throws` as appropriate
- Provide usage examples for complex functionality

```cpp
/**
 * @brief Sets a new expression source and dependencies.
 * 
 * This method updates the reactive computation and triggers
 * notification to all dependent nodes.
 *
 * @param f Expression function/lambda
 * @param args Additional arguments (usually reactive inputs)
 * @throws ReactionException if cyclic dependency is detected
 */
template <typename F, HasArguments... A>
void set(F&& f, A&&... args);
```

## Development Workflow

### Before Committing

1. **Format code**: Run clang-format on all modified files
2. **Run tests**: Ensure all existing tests pass
3. **Static analysis**: Check with clang-tidy for potential issues
4. **Documentation**: Update documentation for any API changes

### Commit Messages

Use conventional commit format:

```
type(scope): description

[optional body]

[optional footer]
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

Examples:
- `feat(core): add automatic dependency tracking`
- `fix(graph): resolve memory leak in observer cleanup`
- `docs(api): update reactive variable documentation`
- `style: apply clang-format to entire codebase`

### Pull Request Guidelines

1. **Small, focused changes**: Keep PRs focused on a single feature or fix
2. **Clear description**: Explain what changes were made and why
3. **Tests**: Include tests for new functionality
4. **Documentation**: Update relevant documentation
5. **Performance**: Consider performance implications of changes

## Testing

### Unit Tests
- Use Google Test framework
- Test both positive and negative cases
- Include edge cases and error conditions
- Maintain high code coverage

### Example Structure
```cpp
TEST(ReactTest, BasicValueAssignment) {
    auto var = reaction::var(42);
    EXPECT_EQ(var.get(), 42);
    
    var.value(100);
    EXPECT_EQ(var.get(), 100);
}
```

## Build System

The project uses CMake for building. Key files:
- `CMakeLists.txt`: Main build configuration
- `cmake/`: CMake modules and configuration
- `.clang-format`: Code formatting rules
- `.clang-tidy`: Static analysis configuration
- `.editorconfig`: Editor configuration

### Building
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Questions?

If you have questions about contributing or need clarification on any guidelines, please:

1. Check existing documentation and examples
2. Search through existing issues and discussions
3. Open a new issue with your question

Thank you for helping make the Reaction framework better!