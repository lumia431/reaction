#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <reaction/reaction.h>
#include <set>

int main() {
    using namespace reaction;
    constexpr int STUDENT_COUNT = 5;

    // 1. Student grades container - using vector to store VarExpr
    ReactContain<VarExpr, double> grades;
    for (int i = 0; i < STUDENT_COUNT; ++i) {
        grades.push_back(unique(70.0 + i * 5));
    }

    // 2. Grade statistics container - using set to store CalcExpr
    ReactContain<CalcExpr, double, std::set> stats;
    stats.insert(unique([&] {
        double sum = 0;
        for (auto &grade : grades) sum += (*grade)();
        return sum / grades.size();
    }));

    stats.insert(unique([&] {
        double max = grades[0]->get();
        for (auto &grade : grades) max = std::max(max, (*grade)());
        return max;
    }));

    // 3. Grade change monitors - using list to store Action
    ReactContain<CalcExpr, VoidWrapper, std::list> monitors;
    for (int i = 0; i < STUDENT_COUNT; ++i) {
        monitors.push_back(unique([i, &grades] {
            std::cout << "[Monitor] Student " << i << " grade updated: " << (*grades[i])() << "\n";
        }));
    }

    // 4. Grade level mapping - using map to store CalcExpr
    ReactMap<CalcExpr, int, const char *> gradeLevels;
    for (int i = 0; i < STUDENT_COUNT; ++i) {
        gradeLevels.insert({i, unique([i, &grades] {
                                double g = (*grades[i])();
                                if (g >= 90) return "A";
                                if (g >= 80) return "B";
                                if (g >= 70) return "C";
                                return "D";
        })});
    }

    // Print initial state
    auto printStats = [&] {
        std::cout << "\n===== Current Stats =====\n";
        std::cout << "Average: " << (*stats.begin())->get() << "\n";
        std::cout << "Max Grade: " << (*++stats.begin())->get() << "\n";

        std::cout << "Grade Levels:\n";
        for (int i = 0; i < STUDENT_COUNT; ++i) {
            std::cout << "  Student " << i << ": " << gradeLevels[i]->get() << " (" << grades[i]->get() << ")\n";
        }
        std::cout << "========================\n\n";
    };

    printStats();

    // Simulate grade changes
    std::cout << "--- Updating Student 2's grade to 85 ---\n";
    grades[2]->value(85.0);
    printStats();

    std::cout << "--- Updating Student 4's grade to 95 ---\n";
    grades[4]->value(95.0);
    printStats();

    std::cout << "--- Updating Student 0's grade to 65 ---\n";
    grades[0]->value(65.0);
    printStats();

    return 0;
}