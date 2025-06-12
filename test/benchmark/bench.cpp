#include "reaction/reaction.h"
#include <benchmark/benchmark.h>
#include <chrono>
#include <numeric>
#include <vector>

struct ProcessedData {
    std::string info;
    int checksum;

    bool operator==(ProcessedData &p) const {
        return info == p.info && checksum == p.checksum;
    }
};

static void BM_DeepDependencyChain(benchmark::State &state) {
    using namespace reaction;

    auto base1 = var(1);
    auto base2 = var(2.0);
    auto base3 = var(true);
    auto base4 = var(std::string{"3"});
    auto base5 = var(4);

    auto layer1 = calc([](int a, double b) { return a + b; }, base1, base2);
    auto layer2 = calc([](double val, bool flag) { return flag ? val * 2 : val / 2; }, layer1, base3);
    auto layer3 = calc([](double val) { return "Value:" + std::to_string(val); }, layer2);
    auto layer4 = calc([](const std::string &s, const std::string &s4) { return s + "_" + s4; }, layer3, base4);
    auto layer5 = calc([](const std::string &s) { return s.length(); }, layer4);
    auto layer6 = calc([](size_t len, int b5) { return std::vector<int>(len, b5); }, layer5, base5);
    auto layer7 = calc([](const std::vector<int> &vec) { return std::accumulate(vec.begin(), vec.end(), 0); }, layer6);
    auto layer8 = calc([](int sum) { return ProcessedData{"ProcessedData", sum}; }, layer7);
    auto layer9 = calc([](const ProcessedData &d) { return d.info + "|" + std::to_string(d.checksum); }, layer8);
    auto finalLayer = calc([](const std::string &s) { return "Final:" + s; }, layer9);

    int i = 0;
    for (auto _ : state) {
        base1.value(i % 100);
        base2.value((i % 100) * 0.1);
        base3.value(i % 2 == 0);
        benchmark::DoNotOptimize(finalLayer.get());
        ++i;
    }

    state.SetItemsProcessed(i);
}
BENCHMARK(BM_DeepDependencyChain);
BENCHMARK_MAIN();
