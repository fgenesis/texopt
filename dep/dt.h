// 1D distance transform over an array

#pragma once

namespace dt {

template<typename T> size_t wrkSize(size_t n)
{
    return n * sizeof(size_t) // v
        + (n + 1) * sizeof(T) // z
        + n * sizeof(T); // in
}

// To use: fill the first n elements of (T*)wrk with distance values, then call this function.
// Inf is a large value, larger than n*n.
// Writes updated distance values to out.
template<typename T>
void linear_1d(T * const out, void * const wrk, size_t n, const T inf)
{
    const T * const in = (const T*)wrk;
    size_t * const v = (size_t*)(((T*)wrk) + n);
    T * const z = (T*)(v + n);

    v[0] = 0;
    z[0] = -inf;
    z[1] = inf;

    {
        unsigned k = 0;
        for(size_t q = 1; q < n; ++q)
        {
            const T tmp = in[q] + T(size_t(q*q));
            T s;
            while(true)
            {
                const size_t vk = v[k];
                s = (tmp - (in[vk] + float(vk*vk))) / float(2*q - 2*vk);
                if(s > z[k])
                    break;
                --k;
            }
            ++k;
            v[k] = q;
            z[k] = s;
            z[k+1] = inf;
        }
    }

    {
        unsigned k = 0;
        for(size_t q = 0; q < n; ++q)
        {
            while(z[k+1] < q)
                ++k;
            const size_t vk = v[k];
            const size_t tmp = q - vk;
            const T d = T(tmp*tmp) + in[vk];
            out[q] = d;
        }
    }
}

}