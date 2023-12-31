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

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Map nrrd through *irregular* univariate map (\"colormap\")"
static const char *_unrrdu_imapInfoL
  = (INFO ". A map is irregular if the control points are not evenly "
          "spaced along the domain, and hence their position must be "
          "explicitly represented in the map.  As nrrds, these maps "
          "are necessarily 2D.  Along axis 0, the first value is the "
          "location of the control point, and the remaining values "
          "give are the range of the map for that control point. "
          "The output value(s) is the result of linearly "
          "interpolating between value(s) from the map.\n "
          "* Uses nrrdApply1DIrregMap");

static int
unrrdu_imapMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nmap, *nacl, *nout;
  airArray *mop;
  NrrdRange *range = NULL;
  unsigned int aclLen;
  int typeOut, rescale, pret, blind8BitRange;
  double min, max;

  hestOptAdd_1_Other(&opt, "m,map", "map", &nmap, NULL,
                     "irregular map to map input nrrd through", nrrdHestNrrd);
  hestOptAdd_1_UInt(&opt, "l,length", "aclLen", &aclLen, "0",
                    "length of accelerator array, used to try to speed-up "
                    "task of finding between which pair of control points "
                    "a given value lies.  Not terribly useful for small maps "
                    "(about 10 points or less).  Use 0 to turn accelorator off. ");
  hestOptAdd_Flag(&opt, "r,rescale", &rescale,
                  "rescale the input values from the input range to the "
                  "map domain");
  hestOptAdd_1_Double(&opt, "min,minimum", "value", &min, "nan",
                      "Low end of input range. Defaults to lowest value "
                      "found in input nrrd.  Explicitly setting this is useful "
                      "only with rescaling (\"-r\")");
  hestOptAdd_1_Double(&opt, "max,maximum", "value", &max, "nan",
                      "High end of input range. Defaults to highest value "
                      "found in input nrrd.  Explicitly setting this is useful "
                      "only with rescaling (\"-r\")");
  hestOptAdd_1_Bool(&opt, "blind8", "bool", &blind8BitRange,
                    nrrdStateBlind8BitRange ? "true" : "false",
                    "Whether to know the range of 8-bit data blindly "
                    "(uchar is always [0,255], signed char is [-128,127]). "
                    "Explicitly setting this is useful only with rescaling (\"-r\")");
  hestOptAdd_1_Other(&opt, "t,type", "type", &typeOut, "default",
                     "specify the type (\"int\", \"float\", etc.) of the output "
                     "nrrd. By default (not using this option), the output type "
                     "is the map's type.",
                     &unrrduHestMaybeTypeCB);
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_imapInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (aclLen) {
    nacl = nrrdNew();
    airMopAdd(mop, nacl, (airMopper)nrrdNuke, airMopAlways);
    if (nrrd1DIrregAclGenerate(nacl, nmap, aclLen)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble generating accelerator:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
  } else {
    nacl = NULL;
  }
  if (rescale) {
    range = nrrdRangeNew(min, max);
    airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
    nrrdRangeSafeSet(range, nin, blind8BitRange);
  }
  if (nrrdTypeDefault == typeOut) {
    typeOut = nmap->type;
  }
  /* some very non-exhaustive tests seemed to indicate that the
     accelerator does not in fact reliably speed anything up.
     This of course depends on the size of the imap (# points),
     but chances are most imaps will have only a handful of points,
     in which case the binary search in _nrrd1DIrregFindInterval()
     will finish quickly ... */
  if (nrrdApply1DIrregMap(nout, nin, range, nmap, nacl, typeOut, rescale)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble applying map:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(imap, INFO);
