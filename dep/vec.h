#pragma once

#include <math.h>

#define TFORALLS(a, b, op) do { for(unsigned i = 0; i < elems; ++i) { (a).ptr()[i] op b; }} while(0)
#define TCLONERETS(b, op) do { V a = asCVR(); TFORALL(a, b, +=); return a; } while(0)
#define TFORALL(a, b, op) TFORALLS(a, (b).cptr()[i], op)
#define TCLONERET(b, op) TCLONERETS((b).cptr()[i], op)

struct tvecbase {};

template<typename T, unsigned N>
struct tvecn : public tvecbase
{
    T e[N];
};

template<typename T, unsigned N, typename V>
struct vecops
{
    typedef T value_type;
    enum { elems = N };

    inline V& operator=(const V& b) { TFORALL(*this, b, =); return asVR(); }

    inline V operator+(const V b) const { V a = asCVR(); TFORALL(a, b, +=); return a; }
    inline V operator-(const V b) const { V a = asCVR(); TFORALL(a, b, -=); return a; }
    inline V operator*(const V b) const { V a = asCVR(); TFORALL(a, b, *=); return a; }
    inline V operator/(const V b) const { V a = asCVR(); TFORALL(a, b, /=); return a; }

    inline V operator+(const T b) const { V a = asCVR(); TFORALLS(a, b, +=); return a; }
    inline V operator-(const T b) const { V a = asCVR(); TFORALLS(a, b, -=); return a; }
    inline V operator*(const T b) const { V a = asCVR(); TFORALLS(a, b, *=); return a; }
    inline V operator/(const T b) const { V a = asCVR(); TFORALLS(a, b, /=); return a; }

    inline V& operator+=(const V b) { TFORALL(*this, b, +=); return asVR(); }
    inline V& operator-=(const V b) { TFORALL(*this, b, -=); return asVR(); }
    inline V& operator*=(const V b) { TFORALL(*this, b, *=); return asVR(); }
    inline V& operator/=(const V b) { TFORALL(*this, b, /=); return asVR(); }

    inline V& operator+=(const T b) { TFORALLS(*this, b, +=); return asVR(); }
    inline V& operator-=(const T b) { TFORALLS(*this, b, -=); return asVR(); }
    inline V& operator*=(const T b) { TFORALLS(*this, b, *=); return asVR(); }
    inline V& operator/=(const T b) { TFORALLS(*this, b, /=); return asVR(); }


    inline T operator[](unsigned i) const { return cptr()[i]; }
    inline T& operator[](unsigned i)      { return ptr()[i]; }

    inline V to(V other) const { return other.asCVR() - asCVR(); }

    inline bool operator==(const V& o) const
    {
        for(u32 i = 0; i < N; ++i)
            if((*this)[i] != o[i])
                return false;
        return true;
    }
    inline bool operator!=(const V o) { return !(*this == o); }

private:
    inline V *asV() { return static_cast<V*>(this); }
    inline const V *asCV() const { return static_cast<const V*>(this); }
    inline V& asVR() { return *asV(); }
    inline const V& asCVR() const { return *asCV(); }

    inline T *ptr() { return reinterpret_cast<T*>(asV()); }
    inline const T *cptr() const { return reinterpret_cast<const T*>(asCV()); }
};

template<typename V>
inline typename V::value_type dot(V a, V b)
{
    typename V::value_type accu(0);
    for(u32 i = 0; i < V::elems; ++i)
        accu += (a[i] * b[i]);
    return accu;
}

template<typename V>
inline float length(V v)
{
    return sqrtf((float)dot(v,v));
}
template<typename V>
inline float distance(V a, V b)
{
    return length(a.to(b));
}

template<typename V>
inline V normalize(V v)
{
    const float len = length(v);
    if(!len)
        return V(0);
    v *= (1.0f / len);
    return v;
}

template<typename T>
struct tvec2 : public vecops<T, 2, tvec2<T> >
{
    tvec2() { x = y = T(0); }
    tvec2(T p) { x = y = p; }
    tvec2(T p1, T p2) { x = p1, y = p2; }
    tvec2(const tvec2& o) { v = o.v; }

    template<typename X>
    explicit tvec2(const tvec2<X>& o) { x = T(o.x); y = T(o.y); }

    union
    {
        struct { T x,y; };
        struct { T r,g; };
        tvecn<T, 2> v;
    };
};

template<typename T>
struct tvec3 : public vecops<T, 3, tvec3<T> >
{
    tvec3() { x = y = z = T(0); }
    tvec3(T p) { x = y = z = p; }
    tvec3(T p1, T p2, T p3) { x = p1; y = p2; z = p3; }
    tvec3(const tvec3& o) { v = o.v; }

    template<typename X>
    explicit tvec3(const tvec3<X>& o) { x = T(o.x); y = T(o.y); z = T(o.z); }

    union
    {
        struct { T x,y,z; };
        struct { T r,g,b; };
        struct { T s,t,p; };
        tvecn<T, 3> v;
    };
};

template<typename T>
struct tvec4 : public vecops<T, 4, tvec4<T> >
{
    tvec4() { x = y = z = w = T(0); }
    tvec4(T p) { x = y = z = w = p; }
    tvec4(T p1, T p2, T p3, T p4) { x = p1; y = p2; z = p3; w = p4; }
    tvec4(const tvec4& o) { v.e = o.e; }

    template<typename X>
    explicit tvec4(const tvec4<X>& o) { x = T(o.x); y = T(o.y); z = T(o.z); w = T(o.w);}

    union
    {
        struct { T x,y,z,w; };
        struct { T r,g,b,a; };
        struct { T s,t,p,q; };
        tvecn<T, 4> v;
    };
};

#undef TFORALL
#undef TCLONERET

template<typename T>
inline T cross(const tvec2<T>& a, const tvec2<T>& b)
{
    return a.x * b.y - a.y * b.x;
}


typedef tvec2<float> vec2;
typedef tvec3<float> vec3;
typedef tvec4<float> vec4;

typedef tvec2<double> dvec2;
typedef tvec3<double> dvec3;
typedef tvec4<double> dvec4;

typedef tvec2<int> ivec2;
typedef tvec3<int> ivec3;
typedef tvec4<int> ivec4;

typedef tvec2<unsigned> uvec2;
typedef tvec3<unsigned> uvec3;
typedef tvec4<unsigned> uvec4;
