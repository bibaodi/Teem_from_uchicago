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

#define INFO "Replace a slice with a different nrrd"
static const char *_unrrdu_spliceInfoL
  = (INFO ". This is functionally the opposite of \"slice\".\n "
          "* Uses nrrdSplice");

static int
unrrdu_spliceMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout, *nslice;
  unsigned int axis;
  int pret;
  long int _pos[2];
  size_t pos;
  airArray *mop;

  OPT_ADD_AXIS(axis, "axis to splice along");
  hestOptAdd_1_Other(&opt, "p,position", "pos", _pos, NULL,
                     "position to splice at:\n "
                     "\b\bo <int> gives 0-based index\n "
                     "\b\bo M-<int> give index relative "
                     "to the last sample on the axis (M == #samples-1).",
                     &unrrduHestPosCB);
  hestOptAdd_1_Other(&opt, "s,slice", "nslice", &(nslice), NULL,
                     "slice nrrd.  This is the slice to insert into \"nin\"",
                     nrrdHestNrrd);
  OPT_ADD_NIN(nin, "input nrrd.  This is the nrrd into which the slice is "
                   "inserted");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_spliceInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);
  if (!(axis < nin->dim)) {
    fprintf(stderr, "%s: axis %u not in range [0,%u]\n", me, axis, nin->dim - 1);
    return 1;
  }
  if (_pos[0] == -1) {
    fprintf(stderr, "%s: m+<int> specification format meaningless here\n", me);
    return 1;
  }
  pos = _pos[0] * (nin->axis[axis].size - 1) + _pos[1];

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdSplice(nout, nin, nslice, axis, pos)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error splicing nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(splice, INFO);
