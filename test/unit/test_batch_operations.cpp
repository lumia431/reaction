/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include "../common/test_fixtures.h"
#include "../common/test_helpers.h"

// Test handling of repeated dependencies in the graph
TEST(BatchOperationsTest, TestRepeatDependency) {
    // ds → A, ds → a, A → a
    auto a = reaction::var(1).setName("a");
    auto b = reaction::var(2).setName("b");

    int triggerCount = 0;
    auto dsA = reaction::calc([&]() { return a() + b(); }).setName("dsA");

    auto dsB = reaction::calc([&]() {++triggerCount; return a() + dsA(); }).setName("dsB");

    triggerCount = 0;

    reaction::batchExecute([&]() { a.value(2); });

    EXPECT_EQ(triggerCount, 1);
    EXPECT_EQ(dsB.get(), 6);
}

// Test more complex repeated dependency scenarios
TEST(BatchOperationsTest, TestRepeatDependency2) {
    // ds → A, ds → B, ds → C, A → a, B → a
    int triggerCount = 0;
    auto a = reaction::var(1).setName("a");
    auto A = reaction::calc([&]() { return a() + 1; }).setName("A");
    auto B = reaction::calc([&]() { return a() + 2; }).setName("B");
    auto C = reaction::calc([&]() { return 5; }).setName("C");
    auto ds = reaction::calc([&]() { ++triggerCount; return A() + B() + C(); }).setName("ds");

    auto ba = reaction::batch([&]() { a.value(2); });

    triggerCount = 0;
    a.value(3);
    EXPECT_EQ(triggerCount, 2);
    EXPECT_EQ(ds.get(), 14);

    triggerCount = 0;
    ba.execute();
    EXPECT_EQ(triggerCount, 1);
    EXPECT_EQ(ds.get(), 12);

    triggerCount = 0;
    a.value(1);
    EXPECT_EQ(triggerCount, 2);
    EXPECT_EQ(ds.get(), 10);
}

// Test deep nested dependency scenarios
TEST(BatchOperationsTest, TestRepeatDependency3) {
    // ds → A, ds → B, A → A1, A1 → A2, A2 → a, B → B1, B1 → a
    auto a = reaction::var(1).setName("a");
    auto b = reaction::var(1).setName("b");

    int triggerCount = 0;
    auto A2 = reaction::calc([&]() { return a() * 2; }).setName("A2");
    auto A1 = reaction::calc([&]() { return A2() + 1; }).setName("A1");
    auto A = reaction::calc([&]() { return A1() - 1; }).setName("A");

    auto B1 = reaction::calc([&]() { return a() - 1; }).setName("B1");
    auto B = reaction::calc([&]() { return B1() + 1; }).setName("B");

    auto ds = reaction::calc([&]() { ++triggerCount; return A() + B(); }).setName("ds");
    triggerCount = 0;

    reaction::batchExecute([&]() { a.value(2); });

    EXPECT_EQ(triggerCount, 1);
    EXPECT_EQ(ds.get(), 6);
    A1.reset([&]() { return b(); });
    EXPECT_EQ(ds.get(), 2);
}

// Test repeat nodes repeat dependented by others
TEST(BatchOperationsTest, TestRepeatDependency4) {

    int triggerCountA = 0;
    int triggerCountB = 0;
    int triggerCountC = 0;

    auto a = reaction::var(1).setName("a");
    auto A = reaction::calc([&]() { return a() + 1; }).setName("A");
    auto B = reaction::calc([&]() { return a() + 2; }).setName("B");
    auto C = reaction::calc([&]() { return 5; }).setName("C");
    auto ds_a = reaction::calc([&]() { ++triggerCountA; return A() + B() + C(); }).setName("ds_a");
    auto ds_b = reaction::calc([&]() { ++triggerCountB; return A() + B() + C(); }).setName("ds_b");
    auto ds_c = reaction::calc([&]() { ++triggerCountC; return ds_a() + ds_b(); }).setName("ds_c");

    triggerCountA = 0;
    triggerCountB = 0;
    triggerCountC = 0;

    reaction::batchExecute([&]() { a.value(2); });

    EXPECT_EQ(triggerCountA, 1);
    EXPECT_EQ(ds_a.get(), 12);
    EXPECT_EQ(triggerCountB, 1);
    EXPECT_EQ(ds_b.get(), 12);
    EXPECT_EQ(triggerCountC, 1);
    EXPECT_EQ(ds_c.get(), 24);
}

// Test complex dependency scenarios with multiple assignments in a single batch
TEST(BatchOperationsTest, TestComplexDependenciesWithMultipleAssignments) {
    // Create base variables
    auto a = reaction::var(1).setName("a");
    auto b = reaction::var(2).setName("b");
    auto c = reaction::var(3).setName("c");
    auto d = reaction::var(4).setName("d");

    // Track trigger counts for all computed values
    int triggerCountA = 0, triggerCountB = 0, triggerCountC = 0;
    int triggerCountSum = 0, triggerCountProduct = 0, triggerCountCombined = 0;
    int triggerCountFinal = 0;

    // Create computed values with complex dependencies
    // A: Depends on a and b
    auto A = reaction::calc([&]() {
        triggerCountA++;
        return a() + b();
    }).setName("A");

    // B: Depends on b and c (shared dependency with A on b)
    auto B = reaction::calc([&]() {
        triggerCountB++;
        return b() * c();
    }).setName("B");

    // C: Depends on c and d (shared dependency with B on c)
    auto C = reaction::calc([&]() {
        triggerCountC++;
        return c() + d();
    }).setName("C");

    // Sum: Depends on A, B, and C (aggregates all intermediates)
    auto Sum = reaction::calc([&]() {
        triggerCountSum++;
        return A() + B() + C();
    }).setName("Sum");

    // Product: Depends on A and B (shared dependencies with Sum)
    auto Product = reaction::calc([&]() {
        triggerCountProduct++;
        return A() * B();
    }).setName("Product");

    // Combined: Depends on Sum and Product (shared dependencies through A and B)
    auto Combined = reaction::calc([&]() {
        triggerCountCombined++;
        return Sum() - Product();
    }).setName("Combined");

    // Final: Depends on Combined and C (shared dependency through C)
    auto Final = reaction::calc([&]() {
        triggerCountFinal++;
        return Combined() * C();
    }).setName("Final");

    // Precompute initial values
    EXPECT_EQ(A.get(), 3);         // 1+2
    EXPECT_EQ(B.get(), 6);         // 2*3
    EXPECT_EQ(C.get(), 7);         // 3+4
    EXPECT_EQ(Sum.get(), 16);      // 3+6+7
    EXPECT_EQ(Product.get(), 18);  // 3*6
    EXPECT_EQ(Combined.get(), -2); // 16-18
    EXPECT_EQ(Final.get(), -14);   // -2*7

    // Reset counters
    triggerCountA = triggerCountB = triggerCountC = 0;
    triggerCountSum = triggerCountProduct = triggerCountCombined = 0;
    triggerCountFinal = 0;

    // Execute batch with multiple assignments
    reaction::batchExecute([&]() {
        a.value(10); // Affects A, then everything that depends on A
        b.value(20); // Affects A and B, then their dependents
        c.value(30); // Affects B and C, then their dependents
        d.value(40); // Affects C, then its dependents
    });

    // Verify trigger counts (each computed value should trigger only once)
    EXPECT_EQ(triggerCountA, 1);
    EXPECT_EQ(triggerCountB, 1);
    EXPECT_EQ(triggerCountC, 1);
    EXPECT_EQ(triggerCountSum, 1);
    EXPECT_EQ(triggerCountProduct, 1);
    EXPECT_EQ(triggerCountCombined, 1);
    EXPECT_EQ(triggerCountFinal, 1);

    // Verify final values
    EXPECT_EQ(A.get(), 30);            // 10+20
    EXPECT_EQ(B.get(), 600);           // 20*30
    EXPECT_EQ(C.get(), 70);            // 30+40
    EXPECT_EQ(Sum.get(), 700);         // 30+600+70
    EXPECT_EQ(Product.get(), 18000);   // 30*600
    EXPECT_EQ(Combined.get(), -17300); // 700-18000
    EXPECT_EQ(Final.get(), -17300 * 70);

    // Additional test: Verify that updating only one variable triggers minimal reactions
    triggerCountA = triggerCountB = triggerCountC = 0;
    triggerCountSum = triggerCountProduct = triggerCountCombined = 0;
    triggerCountFinal = 0;

    reaction::batchExecute([&]() {
        d.value(50); // Only affects C and its dependents
    });

    // Verify only affected computations are re-evaluated
    EXPECT_EQ(triggerCountA, 0);        // Unchanged
    EXPECT_EQ(triggerCountB, 0);        // Unchanged
    EXPECT_EQ(triggerCountC, 1);        // Changed
    EXPECT_EQ(triggerCountSum, 1);      // Changed (depends on C)
    EXPECT_EQ(triggerCountProduct, 0);  // Unchanged
    EXPECT_EQ(triggerCountCombined, 1); // Changed (depends on Sum)
    EXPECT_EQ(triggerCountFinal, 1);    // Changed (depends on Combined and C)

    // Verify updated values
    EXPECT_EQ(C.get(), 80);    // 30+50
    EXPECT_EQ(Sum.get(), 710); // 30+600+80
    EXPECT_EQ(Combined.get(), 710 - 18000);
    EXPECT_EQ(Final.get(), (710 - 18000) * 80);
}

/**
 * @brief Test multiple batches with overlapping data sources and complex dependencies
 *
 * This test verifies:
 * 1. Multiple batches can be created and executed sequentially
 * 2. Batches with overlapping data sources don't interfere
 * 3. Complex dependency graphs with repeated nodes are handled correctly
 * 4. State is properly maintained between batch executions
 */
TEST(BatchOperationsTest, TestMultipleBatchesWithSharedDependencies) {
    // Create shared data sources
    auto a = reaction::var(1).setName("a");
    auto b = reaction::var(2).setName("b");
    auto c = reaction::var(3).setName("c");

    // Create computed values with complex dependencies
    auto X = reaction::calc([&]() { return a() + b(); }).setName("X");
    auto Y = reaction::calc([&]() { return b() * c(); }).setName("Y");
    auto Z = reaction::calc([&]() { return X() + Y(); }).setName("Z");

    // Trackers for computed values
    int triggerCountX = 0, triggerCountY = 0, triggerCountZ = 0;
    int triggerCountA = 0, triggerCountB = 0, triggerCountC = 0;

    // Create observers with complex dependencies
    auto obs1 = reaction::calc([&]() {
        triggerCountX++;
        return X() + c();
    }).setName("obs1");

    auto obs2 = reaction::calc([&]() {
        triggerCountY++;
        return Y() + a();
    }).setName("obs2");

    auto obs3 = reaction::calc([&]() {
        triggerCountZ++;
        return Z() + obs1() + obs2();
    }).setName("obs3");

    auto obs4 = reaction::calc([&]() {
        triggerCountA++;
        return a() + obs3();
    }).setName("obs4");

    auto obs5 = reaction::calc([&]() {
        triggerCountB++;
        return b() + obs4();
    }).setName("obs5");

    auto obs6 = reaction::calc([&]() {
        triggerCountC++;
        return c() + obs5() + obs3();
    }).setName("obs6");

    // Precompute initial values
    EXPECT_EQ(X.get(), 3);     // 1+2
    EXPECT_EQ(Y.get(), 6);     // 2*3
    EXPECT_EQ(Z.get(), 9);     // 3+6
    EXPECT_EQ(obs1.get(), 6);  // 3+3
    EXPECT_EQ(obs2.get(), 7);  // 6+1
    EXPECT_EQ(obs3.get(), 22); // 9+6+7
    EXPECT_EQ(obs4.get(), 23); // 1+22
    EXPECT_EQ(obs5.get(), 25); // 2+23
    EXPECT_EQ(obs6.get(), 50); // 3+25+22

    // Reset counters
    triggerCountX = triggerCountY = triggerCountZ = 0;
    triggerCountA = triggerCountB = triggerCountC = 0;

    // Create multiple batches (but don't execute yet)
    auto batch1 = reaction::batch([&]() {
        a.value(10); // Affects X, obs2, obs4, and all downstream
    });

    auto batch2 = reaction::batch([&]() {
        b.value(20); // Affects X, Y, and all downstream
        c.value(30); // Affects Y, obs1, obs6, and all downstream
    });

    auto batch3 = reaction::batch([&]() {
        a.value(100); // Overrides previous a change
        c.value(300); // Overrides previous c change
    });

    // Verify state hasn't changed yet
    EXPECT_EQ(a.get(), 1);
    EXPECT_EQ(b.get(), 2);
    EXPECT_EQ(c.get(), 3);
    EXPECT_EQ(obs6.get(), 50);
    EXPECT_EQ(triggerCountX, 0);
    EXPECT_EQ(triggerCountA, 0);

    // Execute batches in sequence
    batch1.execute(); // Only a changes to 10

    // Verify after batch1
    EXPECT_EQ(a.get(), 10);
    EXPECT_EQ(b.get(), 2);
    EXPECT_EQ(c.get(), 3);
    EXPECT_EQ(X.get(), 12);     // 10+2
    EXPECT_EQ(Y.get(), 6);      // unchanged
    EXPECT_EQ(Z.get(), 18);     // 12+6
    EXPECT_EQ(obs1.get(), 15);  // 12+3
    EXPECT_EQ(obs2.get(), 16);  // 6+10
    EXPECT_EQ(obs3.get(), 49);  // 18+15+16
    EXPECT_EQ(obs4.get(), 59);  // 10+49
    EXPECT_EQ(obs5.get(), 61);  // 2+59
    EXPECT_EQ(obs6.get(), 113); // 3+61+49

    // Trigger counts after batch1
    EXPECT_EQ(triggerCountX, 1); // X recalculated
    EXPECT_EQ(triggerCountY, 1); // Y unchanged
    EXPECT_EQ(triggerCountZ, 1); // Z changed (depends on X)
    EXPECT_EQ(triggerCountA, 1); // obs4 recalculated (depends on a)
    EXPECT_EQ(triggerCountB, 1); // obs5 recalculated (depends on obs4)
    EXPECT_EQ(triggerCountC, 1); // obs6 recalculated

    // Reset counters for next batch
    triggerCountX = triggerCountY = triggerCountZ = 0;
    triggerCountA = triggerCountB = triggerCountC = 0;

    batch2.execute(); // b->20, c->30

    // Verify after batch2
    EXPECT_EQ(a.get(), 10);
    EXPECT_EQ(b.get(), 20);
    EXPECT_EQ(c.get(), 30);
    EXPECT_EQ(X.get(), 30);      // 10+20
    EXPECT_EQ(Y.get(), 600);     // 20*30
    EXPECT_EQ(Z.get(), 630);     // 30+600
    EXPECT_EQ(obs1.get(), 60);   // 30+30
    EXPECT_EQ(obs2.get(), 610);  // 600+10
    EXPECT_EQ(obs3.get(), 1300); // 630+60+610
    EXPECT_EQ(obs4.get(), 1310); // 10+1300
    EXPECT_EQ(obs5.get(), 1330); // 20+1310
    EXPECT_EQ(obs6.get(), 2660); // 30+1330+1300

    // Trigger counts after batch2
    EXPECT_EQ(triggerCountX, 1); // X recalculated (a and b changed)
    EXPECT_EQ(triggerCountY, 1); // Y recalculated (b and c changed)
    EXPECT_EQ(triggerCountZ, 1); // Z recalculated (X and Y changed)
    EXPECT_EQ(triggerCountA, 1); // obs4 recalculated (obs3 changed)
    EXPECT_EQ(triggerCountB, 1); // obs5 recalculated (obs4 changed)
    EXPECT_EQ(triggerCountC, 1); // obs6 recalculated (obs5 and obs3 changed)

    // Reset counters for next batch
    triggerCountX = triggerCountY = triggerCountZ = 0;
    triggerCountA = triggerCountB = triggerCountC = 0;

    batch3.execute(); // a->100, c->300 (overriding previous changes)

    // Verify after batch3
    EXPECT_EQ(a.get(), 100);
    EXPECT_EQ(b.get(), 20); // unchanged from batch2
    EXPECT_EQ(c.get(), 300);
    EXPECT_EQ(X.get(), 120);      // 100+20
    EXPECT_EQ(Y.get(), 6000);     // 20*300
    EXPECT_EQ(Z.get(), 6120);     // 120+6000
    EXPECT_EQ(obs1.get(), 420);   // 120+300
    EXPECT_EQ(obs2.get(), 6100);  // 6000+100
    EXPECT_EQ(obs3.get(), 12640); // 6120+420+6100
    EXPECT_EQ(obs4.get(), 12740); // 100+12640
    EXPECT_EQ(obs5.get(), 12760); // 20+12740
    EXPECT_EQ(obs6.get(), 25700); // 300+12760+12640

    // Trigger counts after batch3
    EXPECT_EQ(triggerCountX, 1); // X recalculated (a changed)
    EXPECT_EQ(triggerCountY, 1); // Y recalculated (c changed)
    EXPECT_EQ(triggerCountZ, 1); // Z recalculated (X and Y changed)
    EXPECT_EQ(triggerCountA, 1); // obs4 recalculated (a and obs3 changed)
    EXPECT_EQ(triggerCountB, 1); // obs5 recalculated (obs4 changed)
    EXPECT_EQ(triggerCountC, 1); // obs6 recalculated (c, obs5, and obs3 changed)
}