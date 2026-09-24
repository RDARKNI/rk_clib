#define RK_IMPL
#define TRIAX_IMPL

#include "conf.h"

int main(int argc, char* argv[]) {
  int64_t         t0       = triaxi_now_ms();
  Triax_RunConfig defaults = {TRIAXI_ZINIT};
  Triax_RunConfig conf     = triax_parse_argv(argc, argv, defaults);
  if (!conf.outpaths.text) { conf.outpaths.text = "./outputs/out.txt"; }
  if (!conf.outpaths.json) { conf.outpaths.json = "./outputs/out.json"; }
  if (!conf.outpaths.junit) { conf.outpaths.junit = "./outputs/out.xml"; }
  if (!conf.outpaths.tap) { conf.outpaths.tap = "./outputs/out.tap"; }

  triax_run(conf);
  uint32_t duration = (uint32_t)(triaxi_now_ms() - t0);
  printf("total time: %" PRIu32 "ms\n", duration);
}
