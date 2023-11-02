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

#include "ten.h"
#include "privateTen.h"

#define INFO "Convert between different shape triples"
static const char *_tend_tconvInfoL
  = (INFO ".  The triples can be eignvalues, invariants (J, K, R), "
          "and lots of other things.");

static int
tend_tconvMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int ttype[2];
  Nrrd *nin, *nout;
  char *outS;

  hestOptAdd_2_Enum(&hopt, "t", "inType outType", ttype, NULL,
                    "given input and desired output type of triples", tenTripleType);
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-", "input array of triples",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output array");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_tend_tconvInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (tenTripleConvert(nout, ttype[1], nin, ttype[0])) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble converting:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(tconv, INFO);
