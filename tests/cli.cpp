#define RK_IMPL
#define TRIAX_IMPL
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <array>
#include <string>
#include <vector>
#ifdef __has_include
# if __has_include(<string_view>)
#  include <string_view>
# endif
#endif
#include "conf.hpp"

int main(int argc, char* argv[]) {
  Triax_RunConfig defaults = {};
  defaults.outpaths        = {TRIAX_OUTPATH_DEFAULT, "./outputs_isolation/jsonout.json",
                              "./outputs_isolation/tapout.tap", "./outputs_isolation/junitout.xml"};
  defaults.attrs.isolation = TRIAX_ISOLATION_ON;
  triax_run(triax_parse_argv(argc, argv, defaults));
}
