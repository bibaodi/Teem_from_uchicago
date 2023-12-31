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

#include "../air.h"
#include <teemQnanhibit.h>

char *me;

int
main(int argc, char *argv[]) {
  int c, ret;
  unsigned int hibit;
  float f, g, parsed_f;
  double d, parsed_d;
  char str[128];

  AIR_UNUSED(argc);
  me = argv[0];

  g = 0.0;
  g = g / g;
  printf("0.0/0.0 = %f\n", g);
  airFPFprintf_f(stdout, g);
  hibit = (*((unsigned int *)&g) >> 22) & 1;
  printf("hi bit of 23-bit fraction field = %u\n", hibit);
  if (hibit == airMyQNaNHiBit) {
    printf("(agrees with airMyQNaNHiBit)\n");
  } else {
    printf("%s: !!!!\n", me);
    printf("%s: !!!! PROBLEM: nan's hi bit is NOT airMyQNaNHiBit (%d)\n", me,
           airMyQNaNHiBit);
    printf("%s: !!!!\n", me);
  }

  printf(" - - - - - - - - - - - - - - - -\n");
  printf(" - - - - -  FLOATS - - - - - - -\n");
  printf(" - - - - - - - - - - - - - - - -\n");

  for (c = airFP_Unknown + 1; c < airFP_Last; c++) {
    f = airFPGen_f(c);
    sprintf(str, "%.9g", f);
    ret = airSingleSscanf(str, "%f", &parsed_f);
    printf("********** airFPGen_f(%d=%s) = %s (-> %.9g(%d)) "
           "(AIR_EXISTS %d; airExists %d)\n",
           c, airEnumStr(airFPClass_ae, c), str, parsed_f, ret, AIR_EXISTS(f),
           airExists(f));
    airSinglePrintf(stdout, NULL, "airSinglePrintf: %.9g\n", f);
    if (c != airFPClass_f(f)) {
      printf("\n\n%s: Silly hardware!!!\n", me);
      printf("%s: can't return a float of class %d=%s from a function\n\n\n", me, c,
             airEnumStr(airFPClass_ae, c));
    }
    airFPFprintf_f(stdout, f);
    d = f;
    /* I think solaris turns the SNAN into a QNAN */
    printf("to double and back:\n");
    airFPFprintf_f(stdout, AIR_FLOAT(d));
    printf("AIR_ISNAN_F = %d\n", AIR_ISNAN_F(f));
  }

  printf(" - - - - - - - - - - - - - - - -\n");
  printf(" - - - - - DOUBLES - - - - - - -\n");
  printf(" - - - - - - - - - - - - - - - -\n");

  for (c = airFP_Unknown + 1; c < airFP_Last; c++) {
    d = airFPGen_d(c);
    sprintf(str, "%0.17g", d);
    ret = airSingleSscanf(str, "%lf", &parsed_d);
    printf("********** airFPGen_d(%d=%s) = %s (-> %f(%d)) "
           "(AIR_EXISTS %d; airExists %d)\n",
           c, airEnumStr(airFPClass_ae, c), str, parsed_d, ret, AIR_EXISTS(d),
           airExists(d));
    airSinglePrintf(stdout, NULL, "airSinglePrintf: %.17g\n", d);
    if (c != airFPClass_d(d)) {
      printf("\n\n%s: Silly hardware!!!\n", me);
      printf("%s: can't return a double of class %d=%s from a function\n\n\n", me, c,
             airEnumStr(airFPClass_ae, c));
    }
    airFPFprintf_d(stdout, d);
  }

  printf(" - - - - - - - - - - - - - - - -\n");
  printf(" - - - - - - - - - - - - - - - -\n");

  f = AIR_SNAN;
  printf("SNaN test: f = SNaN = float(0x%x) = %.9g; (QNaNHiBit = %u)\n", airFloatSNaN.i,
         f, airMyQNaNHiBit);
  airFPFprintf_f(stdout, f);
  g = f * f;
  printf("g = f*f = %.9g\n", g);
  airFPFprintf_f(stdout, g);
  g = sinf(f);
  printf("g = sin(f) = %.9g\n", g);
  airFPFprintf_f(stdout, g);

  printf("\n");

  printf("FLT_MAX:\n");
  airFPFprintf_f(stdout, FLT_MAX);
  printf("\n");
  printf("FLT_MIN:\n");
  airFPFprintf_f(stdout, FLT_MIN);
  printf("\n");
  printf("DBL_MAX:\n");
  airFPFprintf_d(stdout, DBL_MAX);
  printf("\n");
  printf("DBL_MIN:\n");
  airFPFprintf_d(stdout, DBL_MIN);
  printf("\n");

  printf("AIR_NAN = %f; AIR_EXISTS(AIR_NAN) = %d\n", AIR_NAN, AIR_EXISTS(AIR_NAN));
  printf("AIR_POS_INF = %f; AIR_EXISTS(AIR_POS_INF) = %d\n", AIR_POS_INF,
         AIR_EXISTS(AIR_POS_INF));
  printf("AIR_NEG_INF = %f; AIR_EXISTS(AIR_NEG_INF) = %d\n", AIR_NEG_INF,
         AIR_EXISTS(AIR_NEG_INF));
  exit(0);
}
