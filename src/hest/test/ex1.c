/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "../hest.h"

/*
Fri May 13 00:51:11 CDT 2011 GLK took this example out of the build
because unsure how to handle these annoying warnings:

../hest/test/ex1.c: In function 'main':
../hest/test/ex1.c:34: warning: missing initializer
../hest/test/ex1.c:34: warning: (near initialization for 'opt[0].sawP')
../hest/test/ex1.c:36: warning: missing initializer
../hest/test/ex1.c:36: warning: (near initialization for 'opt[1].sawP')

Fri Jun 23 07:25:52 CDT 2023 GLK sees that its because the hestOpt
grew from the initial minimal set of parameters that made it like argtable,
to a much larger set, while at the same time hestOptAdd became the standard
way of using hest. Incomplete initializing of struct members has always
been allowed but warnings about it have gotten louder.  Pragmas added
below to quiet it for hopefully both clang and gcc.
*/

int
main(int argc, const char **argv) {
  int res[2], v;
  char **in, *out;
  int *mm;
  unsigned int n, mmm, numIn;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
  hestOpt opt[]
    = {{"res", "sx sy", airTypeInt, 2, 2, res, NULL, "image resolution"},
       {"v", "level", airTypeInt, 0, 1, &v, "0", "verbosity level"},
       {"VV", "level", airTypeInt, 0, 5, &mm, "33 22 11", "gonzo level", &mmm},
       {"out", "file", airTypeString, 1, 1, &out, "output.ppm", "PPM image output"},
       {NULL, "input", airTypeString, 1, -1, &in, NULL, "input image file(s)", &numIn},
       {NULL, NULL, 0}};
#pragma clang diagnostic pop
#pragma GCC diagnostic pop
  hestParm *parm;
  char *err = NULL,
       info[] = "This program does nothing in particular, though it does attempt "
                "to pose as some sort of command-line image processing program. "
                "Any implied functionality is purely coincidental, especially since "
                "this software was written by a sleep-deprived grad student.";

  parm = hestParmNew();
  parm->respFileEnable = AIR_TRUE;

  if (1 == argc) {
    /* didn't get anything at all on command line */
    hestInfo(stderr, argv[0], info, parm);
    hestUsage(stderr, opt, argv[0], parm);
    hestGlossary(stderr, opt, parm);
    parm = hestParmFree(parm);
    exit(1);
  }

  /* else we got something, see if we can parse it */
  if (hestParse(opt, argc - 1, argv + 1, &err, parm)) {
    fprintf(stderr, "ERROR: %s\n", err);
    free(err);
    hestUsage(stderr, opt, argv[0], parm);
    hestGlossary(stderr, opt, parm);
    parm = hestParmFree(parm);
    exit(1);
  }

  printf("(err = %s)\n", err);
  printf("res = %d %d\n", res[0], res[1]);
  printf("  v = %d\n", v);
  printf("out = \"%s\"\n", out);
  printf(" mm = %d ints:", mmm);
  for (n = 0; n <= mmm - 1; n++) {
    printf(" %d", mm[n]);
  }
  printf("\n");
  printf(" in = %d files:", numIn);
  for (n = 0; n <= numIn - 1; n++) {
    printf(" \"%s\"", in[n]);
  }
  printf("\n");

  hestParseFree(opt);
  parm = hestParmFree(parm);
  exit(0);
}
