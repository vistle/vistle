#include <cstdio>
#include "depthquant.h"
#include "compdecomp.h"
#include "depthcompare.h"
#include <util/stopwatch.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <limits>
#include <iostream>
#include <random>
#include <string>
#include <core/message.h>

#include <util/netpbmimage.h>

using vistle::Clock;
using vistle::DepthFloat;

void measure(vistle::DepthCompressionParameters depthParam, const std::string &name, const float *depth, size_t w, size_t h, int precision, int num_runs) {

   std::string codec = "zfp";
   if (depthParam.depthZfp) {
       switch (depthParam.depthZfpMode) {
       case vistle::DepthCompressionParameters::ZfpAccuracy:
           codec += " accuracy";
           break;
       case vistle::DepthCompressionParameters::ZfpFixedRate:
           codec += " fixed_rate";
           break;
       case vistle::DepthCompressionParameters::ZfpPrecision:
           codec += " precision";
           break;
       }
   } else if (depthParam.depthQuant) {
       codec = "quant";
   } else {
       codec = "raw";
   }
   std::cout << name << ", precision: " << precision << ", " << codec << std::endl;

   size_t num_pix = w*h;
   double mpix = num_pix * 1e-6;
   double osize = num_pix*3.0;

   double slow = 0., dslow = 0.;
   double fast = std::numeric_limits<double>::max(), dfast = std::numeric_limits<double>::max();
   double total = 0., dtotal = 0.;

   vistle::CompressionParameters param(depthParam);

   for (int i=0; i<num_runs; ++i) {
      double start = Clock::time();

      size_t compressedSize = 0;
      auto comp = vistle::compressDepth(depth, 0, 0, w, h, w, depthParam);
      auto comp0 = comp;
      compressedSize = comp.size();
      vistle::message::Buffer msg;
      msg.setPayloadSize(compressedSize);
      auto comp2 = vistle::message::compressPayload(vistle::message::CompressionLz4, msg, comp);
      auto comp3 = vistle::message::compressPayload(vistle::message::CompressionZstd, msg, comp0);

      double dur = Clock::time() - start;

      total += dur;
      if (dur < fast)
         fast = dur;
      if (dur > slow)
         slow = dur;

      std::vector<float> dequant(num_pix);
      double dstart = Clock::time();
      if (!vistle::decompressTile(reinterpret_cast<char *>(dequant.data()), comp, param, 0, 0, w, h, w)) {
      std::cerr << "decompression error" << std::endl;
      }
      double ddur = Clock::time() - dstart;
      double psnr = depthcompare((const char *)depth, (const char *)&dequant[0], DepthFloat, 4, 0, 0, w, h, w, false);

      dtotal += ddur;
      if (ddur < dfast)
         dfast = ddur;
      if (ddur > dslow)
         dslow = ddur;

#ifdef EACH_RUN
      std::cout << "run " << i << ": " << dur << " s, " << mpix/dur << " MPix/s"
         " (decomp: " << ddur << " s, " << mpix/ddur << " MPix/s)" << std::endl;
#endif

      // check for idempotence
      auto comptest = vistle::compressDepth(depth, 0, 0, w, h, w, depthParam);
      std::vector<float> dequant0(num_pix);
      if (!vistle::decompressTile(reinterpret_cast<char *>(dequant0.data()), comptest, param, 0, 0, w, h, w)) {
          std::cerr << "decompression error" << std::endl;
      }
      double psnr0 = depthcompare((const char *)&dequant[0], (const char *)&dequant0[0], DepthFloat, 4, 0, 0, w, h, w, false);
      if (i == 0)
         std::cout << "PSNR: " << psnr << " dB (recompressed: " << psnr0 << " dB): size=" << compressedSize << " " << compressedSize/osize*100 << "%"
             << " +LZ4=" << comp2.size() << " " << comp2.size()/osize*100 << "%"
             << " +Zstd=" << comp3.size() << " " << comp3.size()/osize*100 << "%"
             << std::endl;
   }

#ifdef EACH_RUN
   std::cout << "--" << std::endl;
#endif
#if 0
   std::cout << "comp mean:   " << total/num_runs << " s, " << (mpix*num_runs)/total << " MPix/s" << std::endl;
   std::cout << "comp slow:   " << slow << " s, " << mpix/slow << " MPix/s" << std::endl;
#endif
   std::cout << "comp fast:   " << fast << " s, " << mpix/fast << " MPix/s" << std::endl;

#if 0
   std::cout << "decomp mean: " << dtotal/num_runs << " s, " << (mpix*num_runs)/dtotal << " MPix/s" << std::endl;
   std::cout << "decomp slow: " << dslow << " s, " << mpix/dslow << " MPix/s" << std::endl;
#endif
   std::cout << "decomp fast: " << dfast << " s, " << mpix/dfast << " MPix/s" << std::endl;
   std::cout << std::endl;
}

int main(int argc, char *argv[]) {

    std::string name = "depthmap.pgm";
    if (argc > 1)
        name = argv[1];

   int num_runs=5;
   if (argc > 2) {
      num_runs = atoi(argv[2]);
   }

    vistle::NetpbmImage img(name);
    std::cerr << "read " << name << ": " << img << std::endl;
    auto w = img.width(), h = img.height();
    size_t num_pix = w*h;

#if 0
    std::string n = name + "_low.pgm";
    vistle::NetpbmImage img2(n, img.width(), img.height());

    for (size_t i=0; i<num_pix; ++i) {
        img2.append(img.gray()[i]);
    }
    img2.close();
#endif

   auto rand = std::minstd_rand0();
   auto dist = std::uniform_real_distribution<float>();
   std::vector<float> depth_rand(num_pix), depth_far(num_pix), depth_uni(num_pix);
   for (size_t i=0; i<num_pix; ++i) {
      depth_rand[i] = dist(rand);
      depth_far[i] = 1.f;
      depth_uni[i] = 0.3f;
   }

   vistle::DepthCompressionParameters depthParam;
   measure(depthParam, name, &img.gray()[0], w, h, 4, num_runs);

   depthParam.depthQuant = false;
   depthParam.depthZfp = true;
   depthParam.depthZfpMode = vistle::DepthCompressionParameters::ZfpPrecision;
   measure(depthParam, name, &img.gray()[0], w, h, 4, num_runs);

   depthParam.depthZfpMode = vistle::DepthCompressionParameters::ZfpAccuracy;
   measure(depthParam, name, &img.gray()[0], w, h, 4, num_runs);

   depthParam.depthZfpMode = vistle::DepthCompressionParameters::ZfpFixedRate;
   measure(depthParam, name, &img.gray()[0], w, h, 4, num_runs);

   depthParam.depthZfp = false;
   depthParam.depthQuant = true;
   measure(depthParam, name, &img.gray()[0], w, h, 4, num_runs);

   return 0;
}
