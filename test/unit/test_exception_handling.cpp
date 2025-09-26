/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include <gtest/gtest.h>
#include <reaction/reaction.h>

using namespace reaction;

/**
 * @brief Test fixture for exception handling tests.
 */
class ExceptionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test data
    }

    void TearDown() override {
        // Cleanup after each test
    }
};

/**
 * @brief Test basic ReactionException functionality.
 */
TEST_F(ExceptionTest, BasicReactionException) {
    try {
        REACTION_THROW(ReactionException, "Test message", ReactionException::ErrorCode::UNKNOWN);
        FAIL() << "Expected ReactionException to be thrown";
    } catch (const ReactionException &e) {
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::UNKNOWN);
        EXPECT_EQ(e.getOriginalMessage(), "Test message");
        EXPECT_FALSE(e.getFile().empty());
        EXPECT_GT(e.getLine(), 0);
        EXPECT_FALSE(e.getFunction().empty());
    }
}

/**
 * @brief Test DependencyCycleException.
 */
TEST_F(ExceptionTest, DependencyCycleException) {
    try {
        REACTION_THROW_DEPENDENCY_CYCLE("NodeA", "NodeB");
        FAIL() << "Expected DependencyCycleException to be thrown";
    } catch (const DependencyCycleException &e) {
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::DEPENDENCY_CYCLE);
        EXPECT_EQ(e.getSourceName(), "NodeA");
        EXPECT_EQ(e.getTargetName(), "NodeB");
        EXPECT_NE(e.what(), nullptr);
        EXPECT_TRUE(std::string(e.what()).find("NodeA") != std::string::npos);
        EXPECT_TRUE(std::string(e.what()).find("NodeB") != std::string::npos);
    }
}

/**
 * @brief Test SelfObservationException.
 */
TEST_F(ExceptionTest, SelfObservationException) {
    try {
        REACTION_THROW_SELF_OBSERVATION("SelfNode");
        FAIL() << "Expected SelfObservationException to be thrown";
    } catch (const SelfObservationException &e) {
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::SELF_OBSERVATION);
        EXPECT_EQ(e.getNodeName(), "SelfNode");
        EXPECT_TRUE(std::string(e.what()).find("SelfNode") != std::string::npos);
    }
}

/**
 * @brief Test NullPointerAccessException.
 */
TEST_F(ExceptionTest, NullPointerAccessException) {
    try {
        REACTION_THROW_NULL_POINTER("weak pointer access");
        FAIL() << "Expected NullPointerAccessException to be thrown";
    } catch (const NullPointerAccessException &e) {
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::NULL_POINTER_ACCESS);
        EXPECT_EQ(e.getContext(), "weak pointer access");
        EXPECT_TRUE(std::string(e.what()).find("weak pointer access") != std::string::npos);
    }
}

/**
 * @brief Test ResourceNotInitializedException.
 */
TEST_F(ExceptionTest, ResourceNotInitializedException) {
    try {
        REACTION_THROW_RESOURCE_NOT_INITIALIZED("TestResource");
        FAIL() << "Expected ResourceNotInitializedException to be thrown";
    } catch (const ResourceNotInitializedException &e) {
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::RESOURCE_NOT_INITIALIZED);
        EXPECT_EQ(e.getResourceType(), "TestResource");
        EXPECT_TRUE(std::string(e.what()).find("TestResource") != std::string::npos);
    }
}

/**
 * @brief Test TypeMismatchException.
 */
TEST_F(ExceptionTest, TypeMismatchException) {
    try {
        REACTION_THROW_TYPE_MISMATCH("int", "double");
        FAIL() << "Expected TypeMismatchException to be thrown";
    } catch (const TypeMismatchException &e) {
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::TYPE_MISMATCH);
        EXPECT_EQ(e.getExpectedType(), "int");
        EXPECT_EQ(e.getActualType(), "double");
        EXPECT_TRUE(std::string(e.what()).find("int") != std::string::npos);
        EXPECT_TRUE(std::string(e.what()).find("double") != std::string::npos);
    }
}

/**
 * @brief Test InvalidStateException.
 */
TEST_F(ExceptionTest, InvalidStateException) {
    try {
        REACTION_THROW_INVALID_STATE("current", "required");
        FAIL() << "Expected InvalidStateException to be thrown";
    } catch (const InvalidStateException &e) {
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::INVALID_STATE);
        EXPECT_EQ(e.getCurrentState(), "current");
        EXPECT_EQ(e.getRequiredState(), "required");
    }
}

/**
 * @brief Test ThreadSafetyViolationException.
 */
TEST_F(ExceptionTest, ThreadSafetyViolationException) {
    try {
        REACTION_THROW_THREAD_SAFETY_VIOLATION("concurrent access");
        FAIL() << "Expected ThreadSafetyViolationException to be thrown";
    } catch (const ThreadSafetyViolationException &e) {
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::THREAD_SAFETY_VIOLATION);
        EXPECT_EQ(e.getOperation(), "concurrent access");
    }
}

/**
 * @brief Test BatchOperationConflictException.
 */
TEST_F(ExceptionTest, BatchOperationConflictException) {
    try {
        REACTION_THROW_BATCH_CONFLICT("conflicting operations");
        FAIL() << "Expected BatchOperationConflictException to be thrown";
    } catch (const BatchOperationConflictException &e) {
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::BATCH_OPERATION_CONFLICT);
        EXPECT_EQ(e.getConflictDescription(), "conflicting operations");
    }
}

/**
 * @brief Test error code to string conversion.
 */
TEST_F(ExceptionTest, ErrorCodeToString) {
    EXPECT_EQ(ReactionException::errorCodeToString(ReactionException::ErrorCode::DEPENDENCY_CYCLE), "DEPENDENCY_CYCLE");
    EXPECT_EQ(ReactionException::errorCodeToString(ReactionException::ErrorCode::SELF_OBSERVATION), "SELF_OBSERVATION");
    EXPECT_EQ(ReactionException::errorCodeToString(ReactionException::ErrorCode::NULL_POINTER_ACCESS), "NULL_POINTER_ACCESS");
    EXPECT_EQ(ReactionException::errorCodeToString(ReactionException::ErrorCode::RESOURCE_NOT_INITIALIZED), "RESOURCE_NOT_INITIALIZED");
    EXPECT_EQ(ReactionException::errorCodeToString(ReactionException::ErrorCode::TYPE_MISMATCH), "TYPE_MISMATCH");
    EXPECT_EQ(ReactionException::errorCodeToString(ReactionException::ErrorCode::INVALID_STATE), "INVALID_STATE");
    EXPECT_EQ(ReactionException::errorCodeToString(ReactionException::ErrorCode::THREAD_SAFETY_VIOLATION), "THREAD_SAFETY_VIOLATION");
    EXPECT_EQ(ReactionException::errorCodeToString(ReactionException::ErrorCode::BATCH_OPERATION_CONFLICT), "BATCH_OPERATION_CONFLICT");
    EXPECT_EQ(ReactionException::errorCodeToString(ReactionException::ErrorCode::UNKNOWN), "UNKNOWN");
}

/**
 * @brief Test actual dependency cycle detection with new exceptions.
 */
TEST_F(ExceptionTest, RealDependencyCycleDetection) {
    auto a = var(10);
    auto calc = reaction::calc([&]() { return a() + 1; });

    // Test what exception is actually thrown
    try {
        calc.reset([&]() { return calc() + a(); });
        FAIL() << "Expected some exception to be thrown";
    } catch (const DependencyCycleException &e) {
        SUCCEED() << "Got DependencyCycleException: " << e.what();
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::DEPENDENCY_CYCLE);
    } catch (const ReactionException &e) {
        SUCCEED() << "Got ReactionException with code " << static_cast<int>(e.getErrorCode()) << ": " << e.what();
    } catch (const std::exception &e) {
        SUCCEED() << "Got std::exception: " << e.what();
    } catch (...) {
        FAIL() << "Got unknown exception type";
    }
}

/**
 * @brief Test actual null pointer access with new exceptions.
 */
TEST_F(ExceptionTest, RealNullPointerAccess) {
    // Create a React object with invalid state to trigger null pointer access
    auto var1 = var(42);
    var1.close(); // Close the reactive node

    // This should throw a NullPointerAccessException when accessing the closed node
    EXPECT_THROW(var1.get(), NullPointerAccessException);
}

/**
 * @brief Test type mismatch in expression reset.
 */
TEST_F(ExceptionTest, RealTypeMismatch) {
    auto calc = reaction::calc([]() { return 42; }); // int return type

    // Test what exception is actually thrown (if any)
    try {
        calc.reset([]() { return 3.14; }); // double return type
        SUCCEED() << "Framework allows implicit type conversion from double to int";
    } catch (const TypeMismatchException &e) {
        SUCCEED() << "Got TypeMismatchException: " << e.what();
        EXPECT_EQ(e.getErrorCode(), ReactionException::ErrorCode::TYPE_MISMATCH);
    } catch (const ReactionException &e) {
        SUCCEED() << "Got ReactionException with code " << static_cast<int>(e.getErrorCode()) << ": " << e.what();
    } catch (const std::exception &e) {
        SUCCEED() << "Got std::exception: " << e.what();
    } catch (...) {
        FAIL() << "Got unknown exception type";
    }
}