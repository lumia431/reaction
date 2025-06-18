/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#include <reaction/reaction.h>
#include <iomanip>
#include <list>
#include <map>
#include <set>
#include <vector>

int main() {
    using namespace reaction;
    constexpr int STUDENT_COUNT = 5;

    // 1. Student grades container - using vector to store VarExpr
    std::vector<Var<double>> grades;
    for (int i = 0; i < STUDENT_COUNT; ++i) {
        grades.push_back(create(70.0 + i * 5));
    }

    // 2. Grade statistics container - using list to store CalcExpr
    std::list<Calc<double>> stats;
    stats.push_back(create([&] {
        double sum = 0;
        for (auto &grade : grades)
            sum += grade();
        return sum / grades.size();
    }));

    stats.push_back(create([&] {
        double max = grades[0].get();
        for (auto &grade : grades)
            max = std::max(max, grade());
        return max;
    }));

    // 3. Grade change monitors - using set to store Action
    std::set<Action<>> monitors;
    for (int i = 0; i < STUDENT_COUNT; ++i) {
        monitors.insert(create([i, &grades] {
            std::cout << "[Monitor] Student " << i << " grade updated: " << grades[i]() << "\n";
        }));
    }

    // 4. Grade level mapping - using map to store CalcExpr
    std::map<int, Calc<const char *>> gradeLevels;
    for (int i = 0; i < STUDENT_COUNT; ++i) {
        gradeLevels.insert({i, create([i, &grades] {
                                double g = grades[i]();
                                if (g >= 90) return "A";
                                if (g >= 80) return "B";
                                if (g >= 70) return "C";
                                return "D";
                            })});
    }

    // Print initial state
    auto printStats = [&] {
        std::cout << "\n===== Current Stats =====\n";
        std::cout << "Average: " << stats.begin()->get() << "\n";
        std::cout << "Max Grade: " << stats.rbegin()->get() << "\n";

        std::cout << "Grade Levels:\n";
        for (int i = 0; i < STUDENT_COUNT; ++i) {
            std::cout << "  Student " << i << ": " << gradeLevels[i].get() << " (" << grades[i].get() << ")\n";
        }
        std::cout << "========================\n\n";
    };

    printStats();

    // Simulate grade changes
    std::cout << "--- Updating Student 2's grade to 85 ---\n";
    grades[2].value(85.0);
    printStats();

    std::cout << "--- Updating Student 4's grade to 95 ---\n";
    grades[4].value(95.0);
    printStats();

    std::cout << "--- Updating Student 0's grade to 65 ---\n";
    grades[0].value(65.0);
    printStats();

    return 0;
}