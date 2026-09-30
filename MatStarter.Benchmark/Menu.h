#pragma once
#include <iostream>
#include <vector>
#include <functional>
#include <random>
#include <string>
#include "Benchmark/Benchmark.h"
#include "Maths/MatBatch.h"
#include "Maths/Matrix3x3.h"
#include "Benchmark/MatBenchmark.h"
#include "Benchmark/VecBenchmark.h"
#include "Benchmark/BenchmarkData.h"




struct Operation
{
    std::string name;
    std::function<Benchmark::Result(BenchmarkData&)> ref;
    std::function<Benchmark::Result(BenchmarkData&)> sse;
};





class BenchmarkMenu
{
    std::mt19937 rng;
    std::vector<Operation> operations;

public:
    BenchmarkMenu(unsigned int seed);
    ~BenchmarkMenu() = default;

    void MainMenu();
    void runBenchmark(std::size_t functionID, std::size_t count);
};
