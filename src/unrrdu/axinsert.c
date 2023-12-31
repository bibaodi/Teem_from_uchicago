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

#define INFO "Add a \"stub\" (length 1) axis to a nrrd"
static const char *_unrrdu_axinsertInfoL
  = (INFO ". The underlying linear ordering of the samples is "
          "unchanged, and the information about the other axes is "
          "shifted upwards as needed.\n "
          "* Uses nrrdAxesInsert, and with \"-s\", nrrdPad_nva");

static int
unrrdu_axinsertMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, *label;
  Nrrd *nin, *nout;
  int pret, kind, center;
  unsigned int axis, size, opi, centOptIdx;
  double mm[2];
  airArray *mop;
  NrrdBoundarySpec *bspec;

  hparm->elideSingleOtherDefault = AIR_FALSE;
  OPT_ADD_AXIS(axis, "dimension (axis index) at which to insert the new axis");
  hestOptAdd_1_String(&opt, "l,label", "label", &label, "",
                      "label to associate with new axis");
  opi = hestOptAdd_1_Enum(&opt, "k,kind", "kind", &kind, "stub",
                          "axis kind to associate with new axis", nrrdKind);
  hestOptAdd_2_Double(&opt, "mm,minmax", "min max", mm, "nan nan",
                      "min and max values along new axis");
  centOptIdx = hestOptAdd_1_Enum(&opt, "c,center", "center", &center, "cell",
                                 "centering of inserted axis: \"cell\" or \"node\"",
                                 nrrdCenter);
  hestOptAdd_1_UInt(&opt, "s,size", "size", &size, "1",
                    "after inserting stub axis, also pad out to some length, "
                    "according to the \"-b\" option");
  hestOptAdd_1_Other(&opt, "b,boundary", "behavior", &bspec, "bleed",
                     "How to handle samples beyond the input bounds:\n "
                     "\b\bo \"pad:<val>\": use specified value\n "
                     "\b\bo \"bleed\": extend border values outward\n "
                     "\b\bo \"mirror\": repeated reflections\n "
                     "\b\bo \"wrap\": wrap-around to other side",
                     nrrdHestBoundarySpec);
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_axinsertInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdAxesInsert(nout, nin, axis)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error inserting axis:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (hestSourceUser == opt[centOptIdx].source) {
    nout->axis[axis].center = center;
  }
  if (1 < size) {
    /* we also do padding here */
    ptrdiff_t min[NRRD_DIM_MAX], max[NRRD_DIM_MAX];
    unsigned int ai;
    Nrrd *npad;
    for (ai = 0; ai < nout->dim; ai++) {
      min[ai] = 0;
      max[ai] = nout->axis[ai].size - 1;
    }
    max[axis] = size - 1;
    npad = nrrdNew();
    airMopAdd(mop, npad, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdPad_nva(npad, nout, min, max, bspec->boundary, bspec->padValue)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error padding:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    /* sneaky, but ok; nothing changes in the mops */
    nout = npad;
    /* only set output kind if explicitly requested
       (since the default is not appropriate) */
    if (hestSourceUser == opt[opi].source) {
      nout->axis[axis].kind = kind;
    }
  } else {
    /* no request to pad; setting the default "stub" kind is sensible */
    nout->axis[axis].kind = kind;
  }
  if (strlen(label)) {
    nout->axis[axis].label = (char *)airFree(nout->axis[axis].label);
    nout->axis[axis].label = airStrdup(label);
  }
  if (AIR_EXISTS(mm[0])) {
    nout->axis[axis].min = mm[0];
  }
  if (AIR_EXISTS(mm[1])) {
    nout->axis[axis].max = mm[1];
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(axinsert, INFO);
