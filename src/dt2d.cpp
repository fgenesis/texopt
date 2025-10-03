#include "dt2d.h"
#include "dt.h"

static const float MAX_VAL = 1e20f;

static void initdist(float *dist, const unsigned char *solid, size_t N)
{
    for(size_t i = 0; i < N; ++i)
        dist[i] = solid[i] ? 0.0f : MAX_VAL;
}

static void finishdist(float *dist, size_t w, size_t h)
{
    const size_t N = w*h;
    const float m = 1.0f / sqrtf(float(w*w + h*h));
    for(size_t i = 0; i < N; ++i)
        dist[i] = m * sqrtf(dist[i]);
}

static void performX(float *dist, size_t w, size_t h, void *wrk)
{
    for(unsigned y = 0; y < h; ++y)
    {
        float *p = &dist[y*w];
        memcpy(wrk, p, w * sizeof(float));
        dt::linear_1d(p, wrk, w, MAX_VAL);
    }
}

static void performY(float *dist, size_t w, size_t h, void *wrk, float *tmp)
{
    for(size_t x = 0; x < w; ++x)
    {
        for(size_t y = 0; y < h; ++y)
            ((float*)wrk)[y] = dist[y*w + x];
        dt::linear_1d(tmp, wrk, h, MAX_VAL);
        for(size_t y = 0; y < h; ++y)
            dist[y*w + x] = tmp[y];
    }
}

void dt2d_solidToDT(float *dist, const unsigned char *solid, size_t w, size_t h)
{
    const size_t N = w * h;
    initdist(dist, solid, N);

    size_t wrksz = dt::wrkSize<float>(std::max(w,h));
    size_t tmpsz = sizeof(float) * h;
    char *wrk = (char*)malloc(wrksz + tmpsz);
    float *tmp = (float*)(wrk + wrksz);
    performY(dist, w, h, wrk, tmp);
    performX(dist, w, h, wrk);
    free(wrk);

    finishdist(dist, w, h);
}
