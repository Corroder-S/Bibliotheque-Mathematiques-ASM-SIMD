#include "Benchmark/Benchmark.h"
#include "Maths/MatBatch.h"
#include <iostream>


int main(int argc, char* argv[])
{
    using M = Maths::Matrix4x4<>;
    const M matrix = M::Translation(Maths::Vec3<>{10.f, 20.f, 30.f});
    const Maths::Vec3<> input[] = {{1,2,3}, {4,5,6}, {7,8,9}};
    Maths::Vec3<> output[3] = {};

    const auto result = Benchmark::Run([&]
    {
        Maths::SSE::TransformPointBatch(matrix, input, output, 3);
        return output[0].x;
    });

    std::cout << "median: " << result.medianMs << " ms\n";

    
    return 0;
}
