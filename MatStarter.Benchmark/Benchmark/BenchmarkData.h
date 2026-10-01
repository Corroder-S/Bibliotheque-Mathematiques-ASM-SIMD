#pragma once

struct BenchmarkData
{
    std::vector<Maths::Vec3<>> vec3A;
    std::vector<Maths::Vec3<>> vec3B;
    Maths::Vec3fSoA vec3ASOA;
    Maths::Vec3fSoA vec3BSOA;
    Maths::Vec3fSoA test;
    std::vector<Maths::Vec3<>> vec3Out;
    std::vector<Maths::Vec4<>> vec4A;
    std::vector<Maths::Vec4<>> vec4B;
    std::vector<Maths::Vec4<>> vec4Out;
    std::vector<float> floatOut;
    Maths::Matrix4x4<> matrix4x4;
    Maths::Matrix3x3<> matrix3x3;
};

inline BenchmarkData GenerateData(std::size_t count, std::mt19937& rng)
{
    std::uniform_real_distribution<float> dist(-100.f, 100.f);
    
    BenchmarkData data;
    
    data.vec3A.resize(count);
    data.vec3B.resize(count);
    data.vec3Out.resize(count);
    data.vec4A.resize(count);
    data.vec4B.resize(count);
    data.vec4Out.resize(count);
    data.floatOut.resize(count);
    
    for (std::size_t i = 0; i < count; ++i)
    {
        data.vec3A[i] = {dist(rng), dist(rng), dist(rng)};
        data.vec3B[i] = {dist(rng), dist(rng), dist(rng)};
        data.vec4A[i] = {dist(rng), dist(rng), dist(rng), dist(rng)};
        data.vec4B[i] = {dist(rng), dist(rng), dist(rng), dist(rng)};
    }

    data.vec3ASOA = Maths::ConvertToSOA(data.vec3A.data(), count);
    data.vec3BSOA = Maths::ConvertToSOA(data.vec3B.data(), count);
    
    data.matrix4x4 = Maths::Matrix4x4<>::Translation({dist(rng), dist(rng), dist(rng)});
    data.matrix3x3 = Maths::Matrix3x3<>::Identity();
    
    return data;
}