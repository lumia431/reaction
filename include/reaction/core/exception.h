/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace reaction {

/**
 * @brief Base exception class for all reaction framework exceptions.
 *
 * This class provides common functionality for all exceptions thrown by
 * the reaction framework, including error codes, context information,
 * and debugging support.
 */
class ReactionException : public std::runtime_error {
public:
    /**
     * @brief Error codes for different types of exceptions.
     */
    enum class ErrorCode {
        UNKNOWN = 0,
        DEPENDENCY_CYCLE,
        SELF_OBSERVATION,
        NULL_POINTER_ACCESS,
        RESOURCE_NOT_INITIALIZED,
        TYPE_MISMATCH,
        INVALID_STATE,
        THREAD_SAFETY_VIOLATION,
        BATCH_OPERATION_CONFLICT,
        MEMORY_ALLOCATION_FAILURE,
        GRAPH_CORRUPTION,
        OBSERVER_LIMIT_EXCEEDED
    };

    /**
     * @brief Construct a ReactionException with message and error code.
     *
     * @param message Descriptive error message
     * @param code Error code identifying the exception type
     * @param file Source file where the exception occurred
     * @param line Line number where the exception occurred
     * @param function Function name where the exception occurred
     */
    ReactionException(const std::string& message,
                      ErrorCode code              = ErrorCode::UNKNOWN,
                      const std::string& file     = "",
                      int line                    = 0,
                      const std::string& function = "")
        : std::runtime_error(formatMessage(message, code, file, line, function)),
          m_errorCode(code),
          m_file(file),
          m_line(line),
          m_function(function),
          m_originalMessage(message) {}

    /**
     * @brief Get the error code associated with this exception.
     * @return ErrorCode identifying the type of error
     */
    ErrorCode getErrorCode() const noexcept { return m_errorCode; }

    /**
     * @brief Get the source file where the exception occurred.
     * @return File path as string
     */
    const std::string& getFile() const noexcept { return m_file; }

    /**
     * @brief Get the line number where the exception occurred.
     * @return Line number
     */
    int getLine() const noexcept { return m_line; }

    /**
     * @brief Get the function name where the exception occurred.
     * @return Function name as string
     */
    const std::string& getFunction() const noexcept { return m_function; }

    /**
     * @brief Get the original error message without formatting.
     * @return Original error message
     */
    const std::string& getOriginalMessage() const noexcept { return m_originalMessage; }

    /**
     * @brief Convert error code to human-readable string.
     * @param code Error code to convert
     * @return String representation of the error code
     */
    static std::string errorCodeToString(ErrorCode code) {
        switch (code) {
            case ErrorCode::DEPENDENCY_CYCLE:
                return "DEPENDENCY_CYCLE";
            case ErrorCode::SELF_OBSERVATION:
                return "SELF_OBSERVATION";
            case ErrorCode::NULL_POINTER_ACCESS:
                return "NULL_POINTER_ACCESS";
            case ErrorCode::RESOURCE_NOT_INITIALIZED:
                return "RESOURCE_NOT_INITIALIZED";
            case ErrorCode::TYPE_MISMATCH:
                return "TYPE_MISMATCH";
            case ErrorCode::INVALID_STATE:
                return "INVALID_STATE";
            case ErrorCode::THREAD_SAFETY_VIOLATION:
                return "THREAD_SAFETY_VIOLATION";
            case ErrorCode::BATCH_OPERATION_CONFLICT:
                return "BATCH_OPERATION_CONFLICT";
            case ErrorCode::MEMORY_ALLOCATION_FAILURE:
                return "MEMORY_ALLOCATION_FAILURE";
            case ErrorCode::GRAPH_CORRUPTION:
                return "GRAPH_CORRUPTION";
            case ErrorCode::OBSERVER_LIMIT_EXCEEDED:
                return "OBSERVER_LIMIT_EXCEEDED";
            default:
                return "UNKNOWN";
        }
    }

protected:
    /**
     * @brief Format exception message with additional context information.
     */
    static std::string formatMessage(const std::string& message,
                                     ErrorCode code,
                                     const std::string& file,
                                     int line,
                                     const std::string& function) {
        std::ostringstream oss;
        oss << "[" << errorCodeToString(code) << "] " << message;

        if (!file.empty() || line > 0 || !function.empty()) {
            oss << " (";
            if (!function.empty()) {
                oss << "in " << function;
            }
            if (!file.empty()) {
                oss << " at " << file;
                if (line > 0) {
                    oss << ":" << line;
                }
            }
            oss << ")";
        }

        return oss.str();
    }

private:
    ErrorCode m_errorCode;
    std::string m_file;
    int m_line;
    std::string m_function;
    std::string m_originalMessage;
};

/**
 * @brief Exception thrown when a dependency cycle is detected in the reactive graph.
 */
class DependencyCycleException : public ReactionException {
public:
    DependencyCycleException(const std::string& sourceName,
                             const std::string& targetName,
                             const std::string& file     = "",
                             int line                    = 0,
                             const std::string& function = "")
        : ReactionException("Detected cycle dependency: source '" + sourceName + "' -> target '" +
                                targetName + "'",
                            ErrorCode::DEPENDENCY_CYCLE,
                            file,
                            line,
                            function),
          m_sourceName(sourceName),
          m_targetName(targetName) {}

    const std::string& getSourceName() const noexcept { return m_sourceName; }
    const std::string& getTargetName() const noexcept { return m_targetName; }

private:
    std::string m_sourceName;
    std::string m_targetName;
};

/**
 * @brief Exception thrown when a node attempts to observe itself.
 */
class SelfObservationException : public ReactionException {
public:
    SelfObservationException(const std::string& nodeName,
                             const std::string& file     = "",
                             int line                    = 0,
                             const std::string& function = "")
        : ReactionException("Detected self-observation: node '" + nodeName +
                                "' cannot observe itself",
                            ErrorCode::SELF_OBSERVATION,
                            file,
                            line,
                            function),
          m_nodeName(nodeName) {}

    const std::string& getNodeName() const noexcept { return m_nodeName; }

private:
    std::string m_nodeName;
};

/**
 * @brief Exception thrown when accessing a null or expired pointer.
 */
class NullPointerAccessException : public ReactionException {
public:
    NullPointerAccessException(const std::string& context  = "Unknown context",
                               const std::string& file     = "",
                               int line                    = 0,
                               const std::string& function = "")
        : ReactionException("Null or expired pointer access in " + context,
                            ErrorCode::NULL_POINTER_ACCESS,
                            file,
                            line,
                            function),
          m_context(context) {}

    const std::string& getContext() const noexcept { return m_context; }

private:
    std::string m_context;
};

/**
 * @brief Exception thrown when accessing an uninitialized resource.
 */
class ResourceNotInitializedException : public ReactionException {
public:
    ResourceNotInitializedException(const std::string& resourceType = "Resource",
                                    const std::string& file         = "",
                                    int line                        = 0,
                                    const std::string& function     = "")
        : ReactionException(resourceType + " is not initialized",
                            ErrorCode::RESOURCE_NOT_INITIALIZED,
                            file,
                            line,
                            function),
          m_resourceType(resourceType) {}

    const std::string& getResourceType() const noexcept { return m_resourceType; }

private:
    std::string m_resourceType;
};

/**
 * @brief Exception thrown when there's a type mismatch during operations.
 */
class TypeMismatchException : public ReactionException {
public:
    TypeMismatchException(const std::string& expectedType,
                          const std::string& actualType,
                          const std::string& file     = "",
                          int line                    = 0,
                          const std::string& function = "")
        : ReactionException("Type mismatch: expected '" + expectedType + "', got '" + actualType +
                                "'",
                            ErrorCode::TYPE_MISMATCH,
                            file,
                            line,
                            function),
          m_expectedType(expectedType),
          m_actualType(actualType) {}

    const std::string& getExpectedType() const noexcept { return m_expectedType; }
    const std::string& getActualType() const noexcept { return m_actualType; }

private:
    std::string m_expectedType;
    std::string m_actualType;
};

/**
 * @brief Exception thrown when an operation is performed in an invalid state.
 */
class InvalidStateException : public ReactionException {
public:
    InvalidStateException(const std::string& currentState,
                          const std::string& requiredState,
                          const std::string& file     = "",
                          int line                    = 0,
                          const std::string& function = "")
        : ReactionException("Invalid state: current '" + currentState + "', required '" +
                                requiredState + "'",
                            ErrorCode::INVALID_STATE,
                            file,
                            line,
                            function),
          m_currentState(currentState),
          m_requiredState(requiredState) {}

    const std::string& getCurrentState() const noexcept { return m_currentState; }
    const std::string& getRequiredState() const noexcept { return m_requiredState; }

private:
    std::string m_currentState;
    std::string m_requiredState;
};

/**
 * @brief Exception thrown when thread safety is violated.
 */
class ThreadSafetyViolationException : public ReactionException {
public:
    ThreadSafetyViolationException(const std::string& operation,
                                   const std::string& file     = "",
                                   int line                    = 0,
                                   const std::string& function = "")
        : ReactionException("Thread safety violation during " + operation,
                            ErrorCode::THREAD_SAFETY_VIOLATION,
                            file,
                            line,
                            function),
          m_operation(operation) {}

    const std::string& getOperation() const noexcept { return m_operation; }

private:
    std::string m_operation;
};

/**
 * @brief Exception thrown when there's a conflict with batch operations.
 */
class BatchOperationConflictException : public ReactionException {
public:
    BatchOperationConflictException(const std::string& conflictDescription,
                                    const std::string& file     = "",
                                    int line                    = 0,
                                    const std::string& function = "")
        : ReactionException("Batch operation conflict: " + conflictDescription,
                            ErrorCode::BATCH_OPERATION_CONFLICT,
                            file,
                            line,
                            function),
          m_conflictDescription(conflictDescription) {}

    const std::string& getConflictDescription() const noexcept { return m_conflictDescription; }

private:
    std::string m_conflictDescription;
};

/**
 * @brief Convenience macros for throwing exceptions with file/line information.
 */
#define REACTION_THROW(ExceptionType, ...)                                                         \
    throw ExceptionType(__VA_ARGS__, __FILE__, __LINE__, __FUNCTION__)

#define REACTION_THROW_DEPENDENCY_CYCLE(sourceName, targetName)                                    \
    REACTION_THROW(DependencyCycleException, sourceName, targetName)

#define REACTION_THROW_SELF_OBSERVATION(nodeName) REACTION_THROW(SelfObservationException, nodeName)

#define REACTION_THROW_NULL_POINTER(context) REACTION_THROW(NullPointerAccessException, context)

#define REACTION_THROW_RESOURCE_NOT_INITIALIZED(resourceType)                                      \
    REACTION_THROW(ResourceNotInitializedException, resourceType)

#define REACTION_THROW_TYPE_MISMATCH(expectedType, actualType)                                     \
    REACTION_THROW(TypeMismatchException, expectedType, actualType)

#define REACTION_THROW_INVALID_STATE(currentState, requiredState)                                  \
    REACTION_THROW(InvalidStateException, currentState, requiredState)

#define REACTION_THROW_THREAD_SAFETY_VIOLATION(operation)                                          \
    REACTION_THROW(ThreadSafetyViolationException, operation)

#define REACTION_THROW_BATCH_CONFLICT(description)                                                 \
    REACTION_THROW(BatchOperationConflictException, description)

} // namespace reaction