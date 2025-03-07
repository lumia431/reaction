// test/test.cpp
#include "gtest/gtest.h"
#include "reaction/dataSource.h"
#include "reaction/expression.h"
#include "reaction/valueCache.h"
#include "reaction/observerHelper.h"

TEST(_, ExpressionTest) {
    bool hudActive = true;
    int distance = 50;
    int progress = 75;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}