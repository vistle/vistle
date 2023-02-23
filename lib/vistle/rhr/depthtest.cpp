#include <cstdio>
#include "depthquant.h"
#include "depthcompare.h"
#include <vistle/util/stopwatch.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <limits>
#include <iostream>
#include <random>

using vistle::Clock;
using vistle::DepthFloat;

//#define TOTALTIME

void measure(const std::string &name, const float *depth, size_t sz, int precision, int num_runs)
{
    std::cout << name << ", precision: " << precision << std::endl;

    size_t num_pix = sz * sz;
    double mpix = num_pix * 1e-6;

    size_t csz = depthquant_size(DepthFloat, 3, sz, sz);
    std::vector<char> quant(csz);
    double slow = 0., dslow = 0.;
    double fast = std::numeric_limits<double>::max(), dfast = std::numeric_limits<double>::max();
#ifdef TOTALTIME
    double total = 0., dtotal = 0.;
#endif
    for (int i = 0; i < num_runs; ++i) {
        double start = Clock::time();
        depthquant(&quant[0], (const char *)&depth[0], DepthFloat, precision, 0, 0, sz, sz);
        double dur = Clock::time() - start;

#ifdef TOTALTIME
        total += dur;
#endif
        if (dur < fast)
            fast = dur;
        if (dur > slow)
            slow = dur;

        std::vector<float> dequant(num_pix);
        double dstart = Clock::time();
        depthdequant((char *)&dequant[0], &quant[0], DepthFloat, precision, 0, 0, sz, sz);
        double ddur = Clock::time() - dstart;
        double psnr =
            depthcompare((const char *)depth, (const char *)&dequant[0], DepthFloat, 4, 0, 0, sz, sz, sz, false);

#ifdef TOTALTIME
        dtotal += ddur;
#endif
        if (ddur < dfast)
            dfast = ddur;
        if (ddur > dslow)
            dslow = ddur;

#ifdef EACH_RUN
        std::cout << "run " << i << ": " << dur << " s, " << mpix / dur
                  << " MPix/s"
                     " (decomp: "
                  << ddur << " s, " << mpix / ddur << " MPix/s)" << std::endl;
#endif

        // check for idempotence
        std::vector<char> quant2(csz);
        depthquant(&quant2[0], (const char *)&dequant[0], DepthFloat, precision, 0, 0, sz, sz);
        std::vector<float> dequant2(num_pix);
        depthdequant((char *)&dequant2[0], &quant2[0], DepthFloat, precision, 0, 0, sz, sz);
        double psnr2 =
            depthcompare((const char *)&dequant[0], (const char *)&dequant2[0], DepthFloat, 4, 0, 0, sz, sz, sz, false);
        if (i == 0)
            std::cout << "PSNR: " << psnr << " dB (recompressed: " << psnr2 << " dB)" << std::endl;
    }

#ifdef EACH_RUN
    std::cout << "--" << std::endl;
#endif
#ifdef TOTALTIME
    std::cout << "comp mean:   " << total / num_runs << " s, " << (mpix * num_runs) / total << " MPix/s" << std::endl;
    std::cout << "comp slow:   " << slow << " s, " << mpix / slow << " MPix/s" << std::endl;
#endif
    std::cout << "comp fast:   " << fast << " s, " << mpix / fast << " MPix/s" << std::endl;

#ifdef TOTALTIME
    std::cout << "decomp mean: " << dtotal / num_runs << " s, " << (mpix * num_runs) / dtotal << " MPix/s" << std::endl;
    std::cout << "decomp slow: " << dslow << " s, " << mpix / dslow << " MPix/s" << std::endl;
#endif
    std::cout << "decomp fast: " << dfast << " s, " << mpix / dfast << " MPix/s" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    int sz = 1440;
    if (argc > 1) {
        sz = atoi(argv[1]);
    }
    int num_runs = 5;
    if (argc > 2) {
        num_runs = atoi(argv[2]);
    }

    size_t num_pix = sz * sz;
    auto rand = std::minstd_rand0();
    auto dist = std::uniform_real_distribution<float>();
    std::vector<float> depth_rand(num_pix), depth_far(num_pix), depth_uni(num_pix);
    for (size_t i = 0; i < num_pix; ++i) {
        depth_rand[i] = dist(rand);
        depth_far[i] = 1.f;
        depth_uni[i] = 0.3f;
    }

    measure("uniform", &depth_uni[0], sz, 4, num_runs);
    measure("uniform", &depth_uni[0], sz, 2, num_runs);
    measure("far", &depth_far[0], sz, 4, num_runs);
    measure("far", &depth_far[0], sz, 2, num_runs);
    measure("random", &depth_rand[0], sz, 4, num_runs);
    measure("random", &depth_rand[0], sz, 2, num_runs);

    return 0;
}
