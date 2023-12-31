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

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/hest.h>
#include <teem/nrrd.h>
#include <teem/limn.h>

static const char *emapInfo
  = ("Creates environment maps based on limn's \"checker\" "
     "normal quantization methods.  By taking into account "
     "camera parameters, this allows for both lights in "
     "both world and view space.  Solely out of laziness, "
     "the nrrd format is used for specifying the lights, but not "
     "to worry: you can use a simple un-adorned text file, "
     "defining one light per line, with 7 values per light: "
     "0/1 (world/view space), R\tG\tB color, and "
     "X\tY\tZ position.");

int
main(int argc, const char *argv[]) {
  hestOpt *hopt = NULL;
  hestParm *hparm;
  Nrrd *nlight, *nmap, *ndebug;
  const char *me;
  char *outS, *errS, *debugS;
  airArray *mop;
  float amb[3], *linfo, *debug, *map, vscl;
  unsigned int li, ui, vi, bits;
  int qn, method, doerr;
  limnLight *light;
  limnCamera *cam;
  double u, v, r, w, V2W[9], diff, WW[3], VV[3];

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hparm->elideSingleEmptyStringDefault = AIR_TRUE;
  hparm->respectDashDashHelp = AIR_TRUE;
  cam = limnCameraNew();
  airMopAdd(mop, cam, (airMopper)limnCameraNix, airMopAlways);
  hestOptAdd_1_Other(&hopt, "i", "nlight", &nlight, NULL,
                     "input nrrd containing light information", nrrdHestNrrd);
  hestOptAdd_1_UInt(&hopt, "b", "# bits", &bits, "16",
                    "number of bits to use for normal quantization, "
                    "between 8 and 16 inclusive. ");
  hestOptAdd_3_Float(&hopt, "amb", "ambient RGB", amb, "0 0 0", "ambient light color");
  hestOptAdd_3_Double(&hopt, "fr", "from point", cam->from, "1 0 0",
                      "position of camera, used to determine view vector");
  hestOptAdd_3_Double(&hopt, "at", "at point", cam->at, "0 0 0",
                      "camera look-at point, used to determine view vector");
  hestOptAdd_3_Double(&hopt, "up", "up vector", cam->up, "0 0 1",
                      "camera pseudo-up vector, used to determine view coordinates");
  hestOptAdd_Flag(&hopt, "rh", &(cam->rightHanded),
                  "use a right-handed UVN frame (V points down)");
  hestOptAdd_1_Float(&hopt, "vs", "view-dir scaling", &vscl, "1",
                     "scaling along view-direction of location of "
                     "view-space lights");
  hestOptAdd_1_String(&hopt, "o", "filename", &outS, NULL,
                      "file to write output envmap to");
  hestOptAdd_1_String(&hopt, "d", "filename", &debugS, "",
                      "Use this option to save out (to the given filename) a rendering "
                      "of the front (on the left) and back (on the right) of a sphere "
                      "as shaded with the new environment map.  U increases "
                      "right-ward, V increases downward.  The back sphere half is "
                      "rendered as though the front half was removed");
  hestOptAdd_Flag(&hopt, "err", &doerr,
                  "If using \"-d\", make the image represent the error between the "
                  "real and quantized vector");
  hestParseOrDie(hopt, argc - 1, argv + 1, hparm, me, emapInfo, AIR_TRUE, AIR_TRUE,
                 AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  switch (bits) {
  case 16:
    method = limnQN16octa;
    break;
  case 15:
    method = limnQN15octa;
    break;
  case 14:
    method = limnQN14octa;
    break;
  case 13:
    method = limnQN13octa;
    break;
  case 12:
    method = limnQN12octa;
    break;
  case 11:
    method = limnQN11octa;
    break;
  case 10:
    method = limnQN10octa;
    break;
  case 9:
    method = limnQN9octa;
    break;
  case 8:
    method = limnQN8octa;
    break;
  default:
    fprintf(stderr, "%s: requested #bits (%u) not in valid range [8,16]\n", me, bits);
    airMopError(mop);
    return 1;
  }

  if (!(nrrdTypeFloat == nlight->type && 2 == nlight->dim && 7 == nlight->axis[0].size
        && LIMN_LIGHT_NUM >= nlight->axis[1].size)) {
    fprintf(stderr,
            "%s: nlight isn't valid format for light specification, "
            "must be: float type, 2-dimensional, 7\tx\tN size, N <= %d\n",
            me, LIMN_LIGHT_NUM);
    airMopError(mop);
    return 1;
  }

  cam->neer = -0.000000001;
  cam->dist = 0;
  cam->faar = 0.0000000001;
  cam->atRelative = AIR_TRUE;
  if (limnCameraUpdate(cam)) {
    airMopAdd(mop, errS = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: problem with camera:\n%s\n", me, errS);
    airMopError(mop);
    return 1;
  }

  light = limnLightNew();
  airMopAdd(mop, light, (airMopper)limnLightNix, airMopAlways);
  limnLightAmbientSet(light, amb[0], amb[1], amb[2]);
  for (li = 0; li < nlight->axis[1].size; li++) {
    int vsp;
    float lxyz[3];
    linfo = (float *)(nlight->data) + 7 * li;
    vsp = !!linfo[0];
    ELL_3V_COPY(lxyz, linfo + 4);
    if (vsp) {
      lxyz[2] *= vscl;
    }
    limnLightSet(light, li, vsp, linfo[1], linfo[2], linfo[3], lxyz[0], lxyz[1],
                 lxyz[2]);
  }
  if (limnLightUpdate(light, cam)) {
    airMopAdd(mop, errS = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: problem with lights:\n%s\n", me, errS);
    airMopError(mop);
    return 1;
  }

  nmap = nrrdNew();
  airMopAdd(mop, nmap, (airMopper)nrrdNuke, airMopAlways);
  if (limnEnvMapFill(nmap, limnLightDiffuseCB, method, light)) {
    airMopAdd(mop, errS = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: problem making environment map:\n%s\n", me, errS);
    airMopError(mop);
    return 1;
  }
  map = (float *)nmap->data;

  if (nrrdSave(outS, nmap, NULL)) {
    fprintf(stderr, "%s: trouble:\n%s", me, errS = biffGetDone(NRRD));
    free(errS);
    return 1;
  }

  if (airStrlen(debugS)) {
    ELL_34M_EXTRACT(V2W, cam->V2W);
    ndebug = nrrdNew();
    nrrdMaybeAlloc_va(ndebug, nrrdTypeFloat, 3, AIR_SIZE_T(3), AIR_SIZE_T(1024),
                      AIR_SIZE_T(512));
    airMopAdd(mop, ndebug, (airMopper)nrrdNuke, airMopAlways);
    debug = (float *)ndebug->data;
    for (vi = 0; vi <= 511; vi++) {
      v = AIR_AFFINE(0, vi, 511, -0.999, 0.999);
      for (ui = 0; ui <= 511; ui++) {
        u = AIR_AFFINE(0, ui, 511, -0.999, 0.999);
        r = sqrt(u * u + v * v);
        if (r > 1) {
          continue;
        }
        w = sqrt(1 - r * r);

        /* first, the near side of the sphere */
        ELL_3V_SET(VV, u, v, -w);
        ELL_3MV_MUL(WW, V2W, VV);
        qn = limnVtoQN_d[method](WW);
        if (doerr) {
          limnQNtoV_d[method](VV, qn);
          ELL_3V_SUB(WW, WW, VV);
          diff = ELL_3V_LEN(WW);
          ELL_3V_SET_TT(debug + 3 * (ui + 1024 * vi), float, diff, diff, diff);
        } else {
          ELL_3V_COPY(debug + 3 * (ui + 1024 * vi), map + 3 * qn);
        }

        /* second, the far side of the sphere */
        ELL_3V_SET(VV, u, v, w);
        ELL_3MV_MUL(WW, V2W, VV);
        qn = limnVtoQN_d[method](WW);
        if (doerr) {
          limnQNtoV_d[method](VV, qn);
          ELL_3V_SUB(WW, WW, VV);
          diff = ELL_3V_LEN(WW);
          ELL_3V_SET_TT(debug + 3 * (ui + 512 + 1024 * vi), float, diff, diff, diff);
        } else {
          ELL_3V_COPY(debug + 3 * (ui + 512 + 1024 * vi), map + 3 * qn);
        }
      }
    }
    nrrdSave(debugS, ndebug, NULL);
  }

  airMopOkay(mop);
  return 0;
}
