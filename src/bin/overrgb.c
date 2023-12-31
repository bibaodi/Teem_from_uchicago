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

static const char *overInfo
  = ("Composites an RGBA nrrd over "
     "a background color (or image), after doing gamma correction, "
     "then quantizes to an 8-bit image.  Actually, the "
     "input nrrd can have more than 4 values per pixel, "
     "but only the first four are used.  If the RGBA nrrd "
     "is floating point, the values are taken at face value; "
     "if it is fixed point, the values interpreted as having "
     "been quantized (so that 8-bit RGBA images will act as "
     "you expect).  When compositing with a background image, the given "
     "background image does not have to be the same size as the input "
     "image; it will be resampled (with linear interpolation) to fit. ");

double
docontrast(double val, double cfp, double cpow) {
  double v;

  if (val < cfp) {
    v = AIR_AFFINE(0.0, val, cfp, 0.0, 1.0);
    v = pow(v, cpow);
    val = AIR_AFFINE(0.0, v, 1.0, 0.0, cfp);
  } else {
    v = AIR_AFFINE(cfp, val, 1.0, 1.0, 0.0);
    v = pow(v, cpow);
    val = AIR_AFFINE(1.0, v, 0.0, cfp, 1.0);
  }
  return val;
}

int
main(int argc, const char *argv[]) {
  hestOpt *hopt = NULL;
  hestParm *hparm;
  Nrrd *nin, *nout, /* initial input and final output */
    *ninD,          /* input converted to double */
    *_nbg,          /* given background image (optional) */
    *nbg,           /* resampled background image */
    *nrgbaD;        /* rgba input as double */
  const char *me;
  char *outS, *errS, *gammaS;
  double _gamma, contr, cfp, cpow, back[3], *rgbaD, r, g, b, a;
  airArray *mop;
  int E, srgb;
  unsigned int srgbIdx;
  size_t min[3], max[3], sx, sy, pi;
  unsigned char *outUC, *bgUC;
  NrrdResampleInfo *rinfo;
  NrrdIoState *nio = NULL;

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hparm->respectDashDashHelp = AIR_TRUE;
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, NULL, "input nrrd to composite",
                     nrrdHestNrrd);
  hestOptAdd_1_Double(&hopt, "c", "contrast", &contr, "0.0",
                      "contrast to apply to RGB values, before gamma. \"0.0\" "
                      "means no change, \"1.0\" means thresholding, \"-1.0\" "
                      "means a complete washout.");
  hestOptAdd_1_Double(&hopt, "cfp", "fixed point", &cfp, "0.5",
                      "component level that doesn't change with contrast");
  hestOptAdd_1_String(&hopt, "g", "gamma", &gammaS, "1.0",
                      "gamma to apply to image data, after contrast. Can be "
                      "a number (<1 to darken >1 to brighten) or the string "
                      "\"srgb\" to apply the roughly 2.2 gamma associated "
                      "with sRGB (see https://en.wikipedia.org/wiki/SRGB). ");
  srgbIdx = /* HEY copied to unrrdu/quantize.c */
    hestOptAdd_1_Enum(&hopt, "srgb", "intent", &srgb, "none",
                      /* the default is "none" for backwards compatibility: until now
                         Teem's support of PNG hasn't handled the sRGB intent, so
                         we shouldn't start using it without being asked */
                      "If saving to PNG (when supported), how to set the rendering "
                      "intent in the sRGB chunk of the PNG file format. Can be "
                      "absolute, relative, perceptual, saturation, or none. This is "
                      "independent of using \"srgb\" as the -g gamma",
                      nrrdFormatPNGsRGBIntent);
  hestOptAdd_3_Double(&hopt, "b", "background", back, "0 0 0",
                      "background color to composite against; white is "
                      "1 1 1, not 255 255 255.");
  hestOptAdd_1_Other(&hopt, "bi", "nbg", &_nbg, "",
                     "8-bit RGB background image to composite against", nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "filename", &outS, NULL,
                      "file to write output PPM image to");
  hestParseOrDie(hopt, argc - 1, argv + 1, hparm, me, overInfo, AIR_TRUE, AIR_TRUE,
                 AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (!(3 == nin->dim && 4 <= nin->axis[0].size)) {
    fprintf(stderr, "%s: doesn't look like an RGBA nrrd\n", me);
    airMopError(mop);
    return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    fprintf(stderr, "%s: can't use a %s nrrd\n", me,
            airEnumStr(nrrdType, nrrdTypeBlock));
    airMopError(mop);
    return 1;
  }
  /* HEY copied to unrrdu/quantize.c */
  if (!(!strcmp(gammaS, "srgb") || 1 == sscanf(gammaS, "%lf", &_gamma))) {
    fprintf(stderr,
            "%s: gamma \"%s\" neither \"srgb\" nor "
            "parseable as double",
            me, gammaS);
    airMopError(mop);
    return 1;
  }

  if (hestSourceUser == hopt[srgbIdx].source && !nrrdFormatPNG->available()) {
    fprintf(stderr,
            "%s: wanted to store sRGB intent \"%s\" in PNG output, but "
            "this Teem build does not support the PNG file format.",
            me, airEnumStr(nrrdFormatPNGsRGBIntent, srgb));
    airMopError(mop);
    return 1;
  }

  sx = nin->axis[1].size;
  sy = nin->axis[2].size;
  if (_nbg) {
    if (!(3 == _nbg->dim && 3 == _nbg->axis[0].size && 2 <= _nbg->axis[1].size
          && 2 <= _nbg->axis[2].size && nrrdTypeUChar == _nbg->type)) {
      fprintf(stderr, "%s: background not an 8-bit RGB image\n", me);
      airMopError(mop);
      return 1;
    }
    nbg = nrrdNew();
    airMopAdd(mop, nbg, (airMopper)nrrdNuke, airMopAlways);
    if (sx == _nbg->axis[1].size && sy == _nbg->axis[2].size) {
      /* no resampling needed, just copy */
      E = nrrdCopy(nbg, _nbg);
    } else {
      /* have to resample background image to fit. */
      /* because we're using the old resampler, we have to kill off any
         space direction information, which is incompatible with
         setting per-axis min and max, as is required by the old resampler */
      nrrdOrientationReduce(_nbg, _nbg, AIR_FALSE);
      rinfo = nrrdResampleInfoNew();
      airMopAdd(mop, rinfo, (airMopper)nrrdResampleInfoNix, airMopAlways);
      rinfo->kernel[0] = NULL;
      nrrdKernelParse(&(rinfo->kernel[1]), rinfo->parm[1], "tent");
      rinfo->min[1] = _nbg->axis[1].min = 0;
      rinfo->max[1] = _nbg->axis[1].max = _nbg->axis[1].size - 1;
      rinfo->samples[1] = sx;
      nrrdKernelParse(&(rinfo->kernel[2]), rinfo->parm[2], "tent");
      rinfo->min[2] = _nbg->axis[2].min = 0;
      rinfo->max[2] = _nbg->axis[2].max = _nbg->axis[2].size - 1;
      rinfo->samples[2] = sy;
      rinfo->renormalize = AIR_TRUE;
      rinfo->round = AIR_TRUE;
      E = nrrdSpatialResample(nbg, _nbg, rinfo);
    }
    if (E) {
      airMopAdd(mop, errS = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble:\n%s", me, errS);
      airMopError(mop);
      return 1;
    }
  } else {
    nbg = NULL;
  }

  ninD = nrrdNew();
  airMopAdd(mop, ninD, (airMopper)nrrdNuke, airMopAlways);
  nrgbaD = nrrdNew();
  airMopAdd(mop, nrgbaD, (airMopper)nrrdNuke, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  E = 0;
  if (nrrdTypeIsIntegral[nin->type]) {
    if (!E) E |= nrrdUnquantize(ninD, nin, nrrdTypeDouble);
  } else if (nrrdTypeFloat == nin->type) {
    if (!E) E |= nrrdConvert(ninD, nin, nrrdTypeDouble);
  } else {
    if (!E) E |= nrrdCopy(ninD, nin);
  }
  min[0] = min[1] = min[2] = 0;
  max[0] = 3;
  max[1] = sx - 1;
  max[2] = sy - 1;
  if (!E) E |= nrrdCrop(nrgbaD, ninD, min, max);
  if (!E) E |= nrrdPPM(nout, sx, sy);
  if (E) {
    airMopAdd(mop, errS = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, errS);
    airMopError(mop);
    return 1;
  }

  contr = AIR_CLAMP(-1, contr, 1);
  cpow = tan(AIR_AFFINE(-1.000001, contr, 1.000001, 0, AIR_PI / 2));
  outUC = (unsigned char *)nout->data;
  bgUC = nbg ? (unsigned char *)nbg->data : NULL;
  rgbaD = (double *)nrgbaD->data;
  for (pi = 0; pi < sx * sy; pi++) {
    r = AIR_CLAMP(0, rgbaD[0], 1);
    g = AIR_CLAMP(0, rgbaD[1], 1);
    b = AIR_CLAMP(0, rgbaD[2], 1);
    a = AIR_CLAMP(0, rgbaD[3], 1);
    if (1 != cpow) {
      r = docontrast(r, cfp, cpow);
      g = docontrast(g, cfp, cpow);
      b = docontrast(b, cfp, cpow);
    }
    if (!strcmp(gammaS, "srgb")) {
      r = nrrdSRGBGamma(r);
      g = nrrdSRGBGamma(g);
      b = nrrdSRGBGamma(b);
    } else {
      r = pow(r, 1.0 / _gamma);
      g = pow(g, 1.0 / _gamma);
      b = pow(b, 1.0 / _gamma);
    }
    if (bgUC) {
      r = a * r + (1 - a) * bgUC[0 + 3 * pi] / 255;
      g = a * g + (1 - a) * bgUC[1 + 3 * pi] / 255;
      b = a * b + (1 - a) * bgUC[2 + 3 * pi] / 255;
    } else {
      r = a * r + (1 - a) * back[0];
      g = a * g + (1 - a) * back[1];
      b = a * b + (1 - a) * back[2];
    }
    outUC[0] = AIR_UCHAR(airIndex(0.0, r, 1.0, 256));
    outUC[1] = AIR_UCHAR(airIndex(0.0, g, 1.0, 256));
    outUC[2] = AIR_UCHAR(airIndex(0.0, b, 1.0, 256));
    outUC += 3;
    rgbaD += 4;
  }

  if (hestSourceUser == hopt[srgbIdx].source) {
    /* HEY copied to unrrdu/quantize.c */
    nio = nrrdIoStateNew();
    airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);
    nio->PNGsRGBIntentKnown = AIR_TRUE;
    nio->PNGsRGBIntent = srgb; /* even if it is nrrdFormatPNGsRGBIntentNone;
                                  that's handled by the writer */
  }
  if (nrrdSave(outS, nout, nio)) {
    airMopAdd(mop, errS = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, errS);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
