// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

// World's tiniest linear algebra library //

namespace rs
{
    namespace utils
    {
        struct int2 { int x,y; };
        struct float3 { float x,y,z; float & operator [] (int i) { return (&x)[i]; } };
        struct float3x3 { float3 x,y,z; float & operator () (int i, int j) { return (&x)[j][i]; } }; // column-major
        struct pose { float3x3 orientation; float3 position; };
        inline bool operator == (const float3 & a, const float3 & b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
        inline float3 operator + (const float3 & a, const float3 & b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
        inline float3 operator * (const float3 & a, float b) { return {a.x*b, a.y*b, a.z*b}; }
        inline bool operator == (const float3x3 & a, const float3x3 & b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
        inline float3 operator * (const float3x3 & a, const float3 & b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
        inline float3x3 operator * (const float3x3 & a, const float3x3 & b) { return {a*b.x, a*b.y, a*b.z}; }
        inline float3x3 transpose(const float3x3 & a) { return {{a.x.x,a.y.x,a.z.x}, {a.x.y,a.y.y,a.z.y}, {a.x.z,a.y.z,a.z.z}}; }
        inline bool operator == (const pose & a, const pose & b) { return a.orientation==b.orientation && a.position==b.position; }
        inline float3 operator * (const pose & a, const float3 & b) { return a.orientation * b + a.position; }
        inline pose operator * (const pose & a, const pose & b) { return {a.orientation * b.orientation, a * b.position}; }
        inline pose inverse(const pose & a) { auto inv = transpose(a.orientation); return {inv, inv * a.position * -1}; }
    }
}
