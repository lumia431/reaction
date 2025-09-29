/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#pragma once

/**
 * @file reaction.h
 * @brief Main entry point for the Reaction reactive programming framework
 *
 * This is the primary header file that provides access to all Reaction framework
 * functionality. Include this file to get complete access to reactive programming
 * features including:
 *
 * - Reactive variables, calculations, and expressions
 * - Observer pattern implementation
 * - Batch operations for performance optimization
 * - Thread-safe reactive operations
 * - Flexible invalidation and triggering strategies
 */

// === Core Framework Components ===

// Core concepts and type traits
#include "reaction/core/concept.h"

// Thread safety management
#include "reaction/concurrency/thread_manager.h"

// Core reactive node and resource management
#include "reaction/core/observer_node.h"
#include "reaction/core/react.h"
#include "reaction/core/resource_selector.h"

// === Graph Management ===

// Observer and dependency graph management
#include "reaction/graph/field_graph.h"
#include "reaction/graph/observer_graph.h"

// === Expression System ===

// Complete expression template system
#include "reaction/expression/expression.h"

// === Batch Operations ===

// Batch processing for performance optimization
#include "reaction/graph/batch.h"

// === Policy Components ===

// Invalidation strategies
#include "reaction/policy/invalidation.h"

// Trigger policies
#include "reaction/policy/trigger.h"

// === Support Infrastructure ===

// Exception handling
#include "reaction/core/exception.h"

// Support utilities
#include "reaction/core/id_generator.h"

// === Factory Functions ===

// High-level factory functions for creating reactive components
#include "reaction/factory/reactive_factory.h"