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

#include "gage.h"
#include "privateGage.h"

/*
static int debugging = 0;
static int debugii;
*/

static airArray *debugReconErrArr = NULL;
static double *debugReconErr = NULL;
static char *debugReconErrName = NULL;

/*
** learned:
**
** -- debug high/discontinuous errors at the low sigmas: was because
** cut-off was insufficient to prevent some discontinuous change in
** kernel values: increased minimum support in the kernel itself, and
** now using larger cut-offs.
**
** -- also, separately from this problem, you can have minima in the
** inf error (in imgMeasr) *not* at sample points, apparently simply
** because of how the hermite interpolation works (but this is
** troubling)
**
** -- do now have a different minimization scheme for allMeasr=Linf,
** but this may still be a work in progress.  Recognizing that this is
** essentially seeking to find a uniform re-parameterization of
** something with a hidden non-uniform parameterization, we could
** probably implement a simple global warping of control points within
** the implied non-uniform domain.
*/

/* this limits how big the kernel can get with a single evaluation
   of nrrdKernelDiscreteGaussian; there are some numerical issues
   with large kernels that need ironing out */
#define GOOD_SIGMA_MAX 5

#define N -1

/*
** NOTE: The idea for this table originated with Raul San Jose Estepar;
** GLK recomputed it optimizing for 3D recon, but
** NOTE: there are probably still be bugs in this; look at the
** "HEY: bug?" notes below, the same problem occurs elsewhere
**
** Basic indexing idea: [sigma max][total # samples][which sample]
**
** "sigma max" can't be 0; smallest value is 1
** ==> index with (sigma max)-1
** biggest value is GAGE_OPTIMSIG_SIGMA_MAX,
** ==> biggest index is GAGE_OPTIMSIG_SIGMA_MAX-1
** ==> allocate for GAGE_OPTIMSIG_SIGMA_MAX
**
** "total # samples" can't be 0, or 1, smallest value is 2
** ==> index with (total # samples)-2
** biggest value is GAGE_OPTIMSIG_SAMPLES_MAXNUM
** ==> biggest index is GAGE_OPTIMSIG_SAMPLES_MAXNUM-2
** ==> allocate for GAGE_OPTIMSIG_SAMPLES_MAXNUM-1
**
** "which sample" ranges from 0 to GAGE_OPTIMSIG_SAMPLES_MAXNUM-1
** ==> allocate for GAGE_OPTIMSIG_SAMPLES_MAXNUM
*/
/* clang-format off */
static double
_optimSigTable[GAGE_OPTIMSIG_SIGMA_MAX][GAGE_OPTIMSIG_SAMPLES_MAXNUM-1][GAGE_OPTIMSIG_SAMPLES_MAXNUM] = {
  {
    {0,1,N,N,N,N,N,N,N,N,N},
    {0,0.5279398,1,N,N,N,N,N,N,N,N},
    {0,0.30728838,0.59967405,1,N,N,N,N,N,N,N},
    {0,0.25022203,0.47050092,0.69525677,1,N,N,N,N,N,N},
    {0,0.17127343,0.39234546,0.56356072,0.75660759,1,N,N,N,N,N},
    {0,0.16795139,0.37100673,0.51324213,0.65655005,0.81952846,1,N,N,N,N},
    {0,0.1662873,0.34969759,0.46556041,0.55324608,0.68717259,0.83465695,1,N,N,N},
    {0,0.12720504,0.22565289,0.28316727,0.44209728,0.58615023,0.75034028,0.87391609,1,N,N},
    {0,0.12836272 /* HEY: bug? should be < 0.12720504 */,0.22926401,0.27715567,0.43546647,0.56471503,0.69411868,0.80830419,0.89314467,1,N},
    {0,0.13169055 /* HEY: bug? should be < 0.12720504 */,0.23498112,0.26570156,0.42672107,0.54272485,0.62969965,0.73375762,0.76996493,0.89293921,1}
  }, {
    {0,2,N,N,N,N,N,N,N,N,N},
    {0,0.75118494,2,N,N,N,N,N,N,N,N},
    {0,0.55478472,1.1535828,2,N,N,N,N,N,N,N},
    {0,0.49007216,0.8412028,1.308665,2,N,N,N,N,N,N},
    {0,0.29460263,0.57445061,0.93797231,1.368475,2,N,N,N,N,N},
    {0,0.2506085,0.49080029,0.73882496,1.069332,1.4497081,2,N,N,N,N},
    {0,0.18255657,0.42056954,0.62766695,0.87999368,1.1692151,1.5175625,2,N,N,N},
    {0,0.17582123,0.40522173,0.58696139,0.79624867,1.0485514,1.2950466,1.5977446,2,N,N},
    {0,0.17304537,0.39376548,0.56427032,0.75127059,0.96672511,1.187861,1.4141362,1.6921321,2,N},
    {0,0.16970521,0.38116929,0.53575242,0.69498152,0.88430929,1.0844854,1.2899524,1.5211773,1.7645421,2}
  }, {
    {0,3,N,N,N,N,N,N,N,N,N},
    {0,0.92324787,3,N,N,N,N,N,N,N,N},
    {0,0.59671402,1.3871731,3,N,N,N,N,N,N,N},
    {0,0.53303385,1.0274624,1.6725048,3,N,N,N,N,N,N},
    {0,0.47298154,0.79659319,1.2379739,1.8434249,3,N,N,N,N,N},
    {0,0.29337707,0.56664073,0.94871783,1.3666322,1.949043,3,N,N,N,N},
    {0,0.25583801,0.52919179,0.78387552,1.1250161,1.516176,2.0632432,3,N,N,N},
    {0,0.25013804,0.48255014,0.72428173,1.0308567,1.3638159,1.7629964,2.2885511,3,N,N},
    {0,0.25038671,0.46448985,0.67336935,0.94502586,1.2324173,1.5780864,1.9883285,2.5002999,3,N},
    {0,0.25034565,0.44725224,0.63590652,0.8669008,1.1130947,1.3942779,1.7180597,2.1408446,2.5466051,3}
  }, {
    {0,4,N,N,N,N,N,N,N,N,N},
    {0,1.0342592,4,N,N,N,N,N,N,N,N},
    {0,0.6341188,1.5414433,4,N,N,N,N,N,N,N},
    {0,0.5523203,1.1400089,1.9595566,4,N,N,N,N,N,N},
    {0,0.51082283,0.91567439,1.4275582,2.2504199,4,N,N,N,N,N},
    {0,0.46390373,0.76406777,1.1620381,1.6579833,2.470386,4,N,N,N,N},
    {0,0.29957226,0.58226484,0.90447241,1.318499,1.8011117,2.5972142,4,N,N,N},
    {0,0.29072434,0.5657317,0.8687849,1.2413157,1.7351674,2.2752147,3.1038468,4,N,N},
    {0,0.25000414,0.5027808,0.75375289,1.0744231,1.4267329,1.8665372,2.4665236,3.2203004,4,N},
    {0,0.19010291,0.44269502,0.66081244,0.95829803,1.2627038,1.6005131,2.0043969,2.6440792,3.2979164,4}
  }, {
    {0,5,N,N,N,N,N,N,N,N,N},
    {0,1.1176668,5,N,N,N,N,N,N,N,N},
    {0,0.66791451,1.6688319,5,N,N,N,N,N,N,N},
    {0,0.56513244,1.2151262,2.2046661,5,N,N,N,N,N,N},
    {0,0.51955444,0.96157616,1.5293243,2.5639,5,N,N,N,N,N},
    {0,0.50639188,0.83235806,1.2596023,1.8475783,2.8751452,5,N,N,N,N},
    {0,0.30821687,0.60048282,1.0057166,1.4351804,2.0372179,3.0747592,5,N,N,N},
    {0,0.28437388,0.560866,0.92278755,1.3049414,1.7620444,2.4607313,3.5198457,5,N,N},
    {0,0.26883101,0.53947717,0.84076571,1.1986721,1.6077875,2.165575,2.9591467,3.931181,5,N},
    {0,0.25029126,0.50162876,0.75587535,1.0861237,1.4452776,1.8865763,2.5002809,3.2476835,4.0337272,5}
  }, {
    {0,6,N,N,N,N,N,N,N,N,N},
    {0,1.185726,6,N,N,N,N,N,N,N,N},
    {0,0.69637311,1.7772807,6,N,N,N,N,N,N,N},
    {0,0.57470578,1.2709187,2.4227901,6,N,N,N,N,N,N},
    {0,0.52996641,1.0128419,1.632214,2.8718762,6,N,N,N,N,N},
    {0,0.50426048,0.87729794,1.3428378,2.0053113,3.2981832,6,N,N,N,N},
    {0,0.46658435,0.76617205,1.1726109,1.6950468,2.5514688,4.1463666,6,N,N,N},
    {0,0.50030917,0.78596908,1.1486269,1.5887094,2.2150676,3.2805684,4.4828262,6,N,N},
    {0,0.27919531,0.56878412,0.88591647,1.2631332,1.7201432,2.3851209,3.392889,4.6255312,6,N},
    {0,0.25088972,0.50369233,0.78494686,1.1030188,1.482311,1.9812444,2.6906328,3.734978,4.7532525,6}
  }, {
    {0,7,N,N,N,N,N,N,N,N,N},
    {0,1.2437892,7,N,N,N,N,N,N,N,N},
    {0,0.72099203,1.8771845,7,N,N,N,N,N,N,N},
    {0,0.58251196,1.3139123,2.6157444,7,N,N,N,N,N,N},
    {0,0.5371021,1.0473768,1.7166929,3.1448426,7,N,N,N,N,N},
    {0,0.51312029,0.92989367,1.4221185,2.2125893,3.6739931,7,N,N,N,N},
    {0,0.50083971,0.84841007,1.2561073,1.8532455,2.8668625,4.7535434,7,N,N,N},
    {0,0.3375614,0.63945627,1.0301709,1.4884938,2.073761,3.1614799,5.0744987,7,N,N},
    {0,0.29428458,0.58668923,0.93714356,1.3736334,1.8300356,2.6405344,3.9042048,5.3097196,7,N},
    {0,0.25234449,0.52068585,0.79422623,1.1273863,1.5991755,2.1453006,2.8984315,4.1899557,5.4597921,7}
  }, {
    {0,8,N,N,N,N,N,N,N,N,N},
    {0,1.2942501,8,N,N,N,N,N,N,N,N},
    {0,0.74332041,1.9693407,8,N,N,N,N,N,N,N},
    {0,0.58823597,1.3483386,2.7880962,8,N,N,N,N,N,N},
    {0,0.56661958,1.2263036,1.9593971,3.6037345,8,N,N,N,N,N},
    {0,0.52106231,0.97026396,1.486012,2.3670862,4.1632919,8,N,N,N,N},
    {0,0.50727636,0.86810225,1.3293955,2.0115428,3.1358411,5.3943086,8,N,N,N},
    {0,0.47202346,0.77812189,1.1608884,1.6648751,2.4694417,3.9094045,5.7665443,8,N,N},
    {0,0.37446901,0.66116196,1.038642,1.4625595,2.0528309,2.9814169,4.4429126,5.9815373,8,N},
    {0,0.26310974,0.54373014,0.84282249,1.2090484,1.6551158,2.3275802,3.3196113,4.7216973,6.1578932,8}
  }, {
    {0,9,N,N,N,N,N,N,N,N,N},
    {0,1.3413963,9,N,N,N,N,N,N,N,N},
    {0,0.76222414,2.0542119,9,N,N,N,N,N,N,N},
    {0,0.59559792,1.3777219,2.946173,9,N,N,N,N,N,N},
    {0,0.56240517,1.1527119,1.9145473,3.6841569,9,N,N,N,N,N},
    {0,0.52387071,0.98832464,1.5376476,2.5417714,4.4669261,9,N,N,N,N},
    {0,0.50359035,0.87327009,1.3558764,2.0646384,3.3180211,5.9420524,9,N,N,N},
    {0,0.50140077,0.83020425,1.256588,1.7709454,2.7100441,4.4434023,6.3934889,9,N,N},
    {0,0.36521655,0.65757704,1.0627806,1.5081434,2.1497617,3.1920822,4.870122,6.6418982,9,N},
    {0,0.31160679,0.59032226,0.94745982,1.3620865,1.8115216,2.6007423,3.8324564,5.2064519,6.8468728,9}
  }, {
    {0,10,N,N,N,N,N,N,N,N,N},
    {0,1.3838946,10,N,N,N,N,N,N,N,N},
    {0,0.77946955,2.1342247,10,N,N,N,N,N,N,N},
    {0,0.60070014,1.4040204,3.0944126,10,N,N,N,N,N,N},
    {0,0.55609542,1.1508646,1.9495349,3.9375696,10,N,N,N,N,N},
    {0,0.5350194,1.031119,1.6607633,2.8520992,5.4718146,10,N,N,N,N},
    {0,0.5083549,0.90783268,1.4059756,2.1796026,3.571064,6.5497985,10,N,N,N},
    {0,0.50199872,0.85233968,1.2647815,1.8777326,2.8592849,4.7821364,7.0110598,10,N,N},
    {0,0.46663594,0.75212663,1.1302133,1.6134665,2.3560972,3.6558499,5.3189116,7.2945781,10,N},
    {0,0.3789258,0.64023608,1.0374272,1.4685256,2.0717783,3.0241971,4.2591534,5.6669927,7.5286098,10}
  }, {
    {0,11,N,N,N,N,N,N,N,N,N},
    {0,1.4234025,11,N,N,N,N,N,N,N,N},
    {0,0.79513794,2.2098076,11,N,N,N,N,N,N,N},
    {0,0.60728961,1.4287171,3.2358651,11,N,N,N,N,N,N},
    {0,0.55890071,1.165283,2.0149148,4.1530919,11,N,N,N,N,N},
    {0,0.55071467,1.0660659,1.7177736,3.0094495,6.0395317,11,N,N,N,N},
    {0,0.5066433,0.89661205,1.4050072,2.2117786,3.7080047,7.0954437,11,N,N,N},
    {0,0.50242329,0.86727452,1.3264461,1.9118301,2.9509099,5.1184769,7.624764,11,N,N},
    {0,0.47785854,0.78873962,1.1769236,1.6880652,2.4978926,4.0288033,5.7288432,7.9420485,11,N},
    {0,0.50532979,0.79486167,1.1706896,1.6148115,2.2648265,3.3499777,4.5595574,6.116312,8.2049971,11}
  }
};
/* clang-format on */

/*
** this is only for retreiving part of the table above
*/
int /* Biff: 1 */
gageOptimSigSet(double *scale, unsigned int num, unsigned int sigmaMax) {
  static const char me[] = "gageOptimSigSet";
  unsigned int si;

  if (!scale) {
    biffAddf(GAGE, "%s: got NULL pointer", me);
    return 1;
  }
  if (!AIR_IN_CL(2, num, GAGE_OPTIMSIG_SAMPLES_MAXNUM)) {
    biffAddf(GAGE, "%s: requested # sigma samples %u not in known range [2,%u]", me, num,
             GAGE_OPTIMSIG_SAMPLES_MAXNUM);
    return 1;
  }
  if (!AIR_IN_CL(1, sigmaMax, GAGE_OPTIMSIG_SIGMA_MAX)) {
    biffAddf(GAGE, "%s: requested sigma max %u not in known range [1,%u]", me, sigmaMax,
             GAGE_OPTIMSIG_SIGMA_MAX);
    return 1;
  }

  for (si = 0; si < num; si++) {
    scale[si] = _optimSigTable[sigmaMax - 1][num - 2][si];
  }
  return 0;
}

/* ------- from here down is the stuff for computing the table ------ */

/* rho is a stand-in for tau - and something that will likely change
   based on the findings from using this code; the idea is that it
   reflects the needed density of samples for optimal scale-space
   reconstruction. In order to be used for the internal workings of
   the sigma optimization, its important that the conversion between
   sigma and rho be accurately invertible. */

/*
**
** This is a decent approximation of tau(sigma), made of a slightly
** scaled version of the taylor expansion of tau(sigma=0) which meets
** up with the large-scale approximation of tau from Lindeberg.
** However, because its so flat at sigma=0, its not really invertible
** there, so it's a poor basis for computations that are parameterized
** by rho.  Keeping it around for reference.
*
*static double
*_RhoOfSig(double sig) {
*  double rho;
*
*  if (sig < 1.05189095) {
*    rho = sig*sig*(0.2775733212544225 + 0.13078298856958057*sig*sig);
*  } else {
*    double tee;
*    tee = sig*sig;
*    rho = 0.53653222368715360118 + log(tee)/2.0 + log(1.0 - 1.0/(8.0*tee));
*  }
*  return rho;
*}
*
*static double
*_SigOfRho(double rho) {
*  double sig;
*
*  if (rho < 0.46724360022171363) {
*    sig = 0.00033978812426865065 *
*      sqrt(-9.191366355042886e6 + 245.3752559286824 *
*           sqrt(1.403132301e9 + 9.526961876920057e9*rho));
*  } else {
*    double ee, tee;
*    ee = exp(2.0*rho);
*    tee = 0.0063325739776461107152*(27.0*ee + 2*AIR_PI*AIR_PI
*                                    + 3.0*sqrt(81.0*ee*ee
*                                               + 12*ee*AIR_PI*AIR_PI));
*    sig = sqrt(tee);
*  }
*  return sig;
*}
*/

static double
_RhoOfSig(double sig) {

  return log(sig + 1);
}

static double
_SigOfRho(double rho) {

  return exp(rho) - 1;
}

/*
** allocates context, with error checking
** does use biff
*/
gageOptimSigContext * /* Biff: NULL */
gageOptimSigContextNew(unsigned int dim, unsigned int sampleNumMax,
                       unsigned int trueImgNum, double sigmaMin, double sigmaMax,
                       double cutoff) {
  static const char me[] = "gageOptimSigContextNew";
  gageOptimSigContext *oscx;
  unsigned int support, ii;
  double kparm[2];

  oscx = AIR_CALLOC(1, gageOptimSigContext);
  if (!oscx) {
    biffAddf(GAGE, "%s: couldn't allocate context", me);
    return NULL;
  }
  if (!AIR_IN_CL(1, dim, 3)) {
    biffAddf(GAGE, "%s: dim %u not 1, 2, or 3", me, dim);
    return NULL;
  }
  if (!(sampleNumMax >= 3)) {
    biffAddf(GAGE, "%s: sampleNumMax %u not >= 3", me, sampleNumMax);
    return NULL;
  }
  if (!(trueImgNum >= 3)) {
    biffAddf(GAGE, "%s: trueImgNum %u not >= 3", me, trueImgNum);
    return NULL;
  }
  if (!(sigmaMin >= 0 && sigmaMax > sigmaMin && cutoff > 0)) {
    biffAddf(GAGE, "%s: sigmaMin %g, sigmaMax %g, cutoff %g not valid", me, sigmaMin,
             sigmaMax, cutoff);
    return NULL;
  }

  oscx->dim = dim;
  oscx->sampleNumMax = sampleNumMax;
  oscx->trueImgNum = trueImgNum;
  oscx->sigmaRange[0] = sigmaMin;
  oscx->sigmaRange[1] = sigmaMax;
  oscx->cutoff = cutoff;

  /* will be set later */
  oscx->kssSpec = NULL;
  oscx->sampleNum = 0;
  oscx->maxIter = 0;
  oscx->imgMeasr = nrrdMeasureUnknown;
  oscx->allMeasr = nrrdMeasureUnknown;
  oscx->convEps = AIR_NAN;

  /* allocate internal buffers based on arguments */
  kparm[0] = oscx->sigmaRange[1];
  kparm[1] = oscx->cutoff;
  support = AIR_ROUNDUP(nrrdKernelDiscreteGaussian->support(kparm));
  oscx->sx = 2 * support - 1;
  oscx->sy = dim >= 2 ? 2 * support - 1 : 1;
  oscx->sz = dim >= 3 ? 2 * support - 1 : 1;
  /*
  fprintf(stderr, "%s: max sigma = %g, cutoff %g ==> support=%u, "
          "3D vol size=%u x %u x %u\n", me,
          sigmaMax, cutoff, support, oscx->sx, oscx->sy, oscx->sz);
  */
  oscx->nerr = nrrdNew();
  oscx->ninterp = nrrdNew();
  oscx->ndiff = nrrdNew();
  if (nrrdMaybeAlloc_va(oscx->nerr, nrrdTypeDouble, 1, AIR_SIZE_T(oscx->trueImgNum))
      || nrrdMaybeAlloc_va(oscx->ninterp, nrrdTypeDouble, 3, AIR_SIZE_T(oscx->sx),
                           AIR_SIZE_T(oscx->sy), AIR_SIZE_T(oscx->sz))
      || nrrdMaybeAlloc_va(oscx->ndiff, nrrdTypeDouble, 3, AIR_SIZE_T(oscx->sx),
                           AIR_SIZE_T(oscx->sy), AIR_SIZE_T(oscx->sz))) {
    biffMovef(GAGE, NRRD, "%s: couldn't allocate buffers", me);
    return NULL;
  }
  nrrdAxisInfoSet_va(oscx->ninterp, nrrdAxisInfoSpacing, 1.0, 1.0, 1.0);
  nrrdAxisInfoSet_va(oscx->ndiff, nrrdAxisInfoSpacing, 1.0, 1.0, 1.0);
  oscx->rhoRange[0] = _RhoOfSig(oscx->sigmaRange[0]);
  oscx->rhoRange[1] = _RhoOfSig(oscx->sigmaRange[1]);
  nrrdAxisInfoSet_va(oscx->nerr, nrrdAxisInfoMin, oscx->rhoRange[0]);
  nrrdAxisInfoSet_va(oscx->nerr, nrrdAxisInfoMax, oscx->rhoRange[1]);

  fprintf(stderr, "!%s: sigma [%g,%g] -> rho [%g,%g]\n", me, oscx->sigmaRange[0],
          oscx->sigmaRange[1], oscx->rhoRange[0], oscx->rhoRange[1]);
  fprintf(stderr, "!%s: rho %g -- %g\n", me, oscx->rhoRange[0], oscx->rhoRange[1]);
  for (ii = 0; ii < oscx->trueImgNum; ii++) {
    double rr, ss, rc, eps = 1e-13;
    rr = AIR_AFFINE(0, ii, oscx->trueImgNum - 1, oscx->rhoRange[0], oscx->rhoRange[1]);
    ss = _SigOfRho(rr);
    rc = _RhoOfSig(ss);
    /*
    fprintf(stderr, "!%s: (%u) rho %.17g -> sig %.17g -> rho %.17g (%.17g)\n",
            me, ii, rr, ss, rc, AIR_ABS(rr - rc)/(rr + eps/2));
    */
    if (AIR_ABS(rr - rc) / (rr + eps) > eps) {
      biffAddf(GAGE,
               "%s: rho %g -> sig %g -> rho %g has error %g > %g; "
               "_SigOfRho() and _RhoOfSig() not invertible",
               me, rr, ss, rc, AIR_ABS(rr - rc) / (rr + eps / 2), eps);
      return NULL;
    }
  }

  oscx->kloc = AIR_CALLOC(oscx->sx, double);
  oscx->kern = AIR_CALLOC(oscx->sx, double);
  oscx->ktmp1 = AIR_CALLOC(oscx->sx, double);
  oscx->ktmp2 = AIR_CALLOC(oscx->sx, double);
  if (!(oscx->kloc && oscx->kern && oscx->ktmp1 && oscx->ktmp2)) {
    biffAddf(GAGE, "%s: couldn't allocate kernel buffers", me);
    return NULL;
  }
  for (ii = 0; ii < oscx->sx; ii++) {
    oscx->kloc[ii] = AIR_CAST(double, ii) - ((oscx->sx + 1) / 2 - 1);
  }
  oscx->kone[0] = 1.0;

  oscx->gctx = NULL;
  oscx->pvlBase = NULL;
  oscx->pvlSS = AIR_CALLOC(oscx->sampleNumMax, gagePerVolume *);
  oscx->nsampleImg = AIR_CALLOC(oscx->sampleNumMax, Nrrd *);
  oscx->sampleSigma = AIR_CALLOC(oscx->sampleNumMax, double);
  oscx->sampleRho = AIR_CALLOC(oscx->sampleNumMax, double);
  oscx->sampleTmp = AIR_CALLOC(oscx->sampleNumMax, double);
  oscx->sampleErrMax = AIR_CALLOC(oscx->sampleNumMax, double);
  oscx->step = AIR_CALLOC(oscx->sampleNumMax, double);
  if (!(oscx->pvlSS && oscx->nsampleImg && oscx->sampleSigma && oscx->sampleRho
        && oscx->sampleTmp && oscx->sampleErrMax && oscx->step)) {
    biffAddf(GAGE, "%s: couldn't allocate per-sample arrays", me);
    return NULL;
  }
  for (ii = 0; ii < oscx->sampleNumMax; ii++) {
    oscx->nsampleImg[ii] = nrrdNew();
    if (nrrdMaybeAlloc_va(oscx->nsampleImg[ii], nrrdTypeDouble, 3, AIR_SIZE_T(oscx->sx),
                          AIR_SIZE_T(oscx->sy), AIR_SIZE_T(oscx->sz))) {
      biffMovef(GAGE, NRRD, "%s: couldn't allocate vol[%u]", me, ii);
      return NULL;
    }
    nrrdAxisInfoSet_va(oscx->nsampleImg[ii], nrrdAxisInfoSpacing, 1.0, 1.0, 1.0);
  }

  /* implementation not started
  oscx->nsampleHist = nrrdNew();
  */

  return oscx;
}

gageOptimSigContext * /* Biff: nope */
gageOptimSigContextNix(gageOptimSigContext *oscx) {

  if (oscx) {
    unsigned int si;
    nrrdKernelSpecNix(oscx->kssSpec);
    nrrdNuke(oscx->nerr);
    nrrdNuke(oscx->ninterp);
    nrrdNuke(oscx->ndiff);
    airFree(oscx->kloc);
    airFree(oscx->kern);
    airFree(oscx->ktmp1);
    airFree(oscx->ktmp2);
    gageContextNix(oscx->gctx);
    /* airFree(oscx->pvlSS); needed? */
    for (si = 0; si < oscx->sampleNumMax; si++) {
      nrrdNuke(oscx->nsampleImg[si]);
    }
    airFree(oscx->nsampleImg);
    airFree(oscx->sampleSigma);
    airFree(oscx->sampleRho);
    airFree(oscx->sampleTmp);
    airFree(oscx->sampleErrMax);
    airFree(oscx->step);
    /* nrrdNuke(oscx->nsampleHist); */
    airFree(oscx);
  }
  return NULL;
}

static int /* Biff: 1 */
_volInterp(Nrrd *ninterp, double rho, gageOptimSigContext *oscx) {
  static const char me[] = "_volInterp";
  double *interp, scaleIdx, sigma;
  const double *answer;
  unsigned int xi, yi, zi;
  int outside;

  /*
  debugging = rho > 1.197;
  gageParmSet(oscx->gctx, gageParmVerbose, 2*debugging);
  */
  sigma = _SigOfRho(rho);
  scaleIdx = gageStackWtoI(oscx->gctx, sigma, &outside);
  /* Because of limited numerical precision, _SigOfRho(rhoRange[1])
     can end up "outside" stack, which should really be a bug.
     However, since the use of gage is pretty straight-forward here,
     we're okay with ignoring the "outside" here, and also clamping
     the probe below */
  answer = gageAnswerPointer(oscx->gctx, oscx->pvlBase, gageSclValue);
  interp = AIR_CAST(double *, ninterp->data);
  for (zi = 0; zi < oscx->sz; zi++) {
    for (yi = 0; yi < oscx->sy; yi++) {
      for (xi = 0; xi < oscx->sx; xi++) {
        if (gageStackProbeSpace(oscx->gctx, xi, yi, zi, scaleIdx,
                                AIR_TRUE /* index space */, AIR_TRUE /* clamping */)) {
          biffAddf(GAGE, "%s: probe error at (%u,%u,%u,%.17g): %s (%d)", me, xi, yi, zi,
                   scaleIdx, oscx->gctx->errStr, oscx->gctx->errNum);
          return 1;
        }
        interp[xi + oscx->sx * (yi + oscx->sy * zi)] = answer[0];
      }
    }
  }
  /*
  gageParmSet(oscx->gctx, gageParmVerbose, 0);
  */
  /*
  if (debugging) {
    char fname[AIR_STRLEN_SMALL+1];
    sprintf(fname, "interp-%04u.nrrd", debugii);
    nrrdSave(fname, ninterp, NULL);
  }
  */
  return 0;
}

static void
_kernset(double **kzP, double **kyP, double **kxP, gageOptimSigContext *oscx,
         double rho) {
  const NrrdKernel *dg;
  double sig, kparm[NRRD_KERNEL_PARMS_NUM], *kloc, *kern, *ktmp1, *ktmp2;
  unsigned int ki, kj, kk, sx;

  kern = oscx->kern;
  kloc = oscx->kloc;
  ktmp1 = oscx->ktmp1;
  ktmp2 = oscx->ktmp2;
  sx = oscx->sx;
  sig = _SigOfRho(rho);
  dg = nrrdKernelDiscreteGaussian;
  kparm[1] = oscx->cutoff;
  if (sig < GOOD_SIGMA_MAX) {
    /* for small sigma, can evaluate directly into kern */
    kparm[0] = sig;
    dg->evalN_d(kern, kloc, sx, kparm);
  } else {
    double timeleft, tdelta;
    unsigned int rx;
    rx = (sx + 1) / 2 - 1;
    /* we have to iteratively blur */
    kparm[0] = GOOD_SIGMA_MAX;
    dg->evalN_d(kern, kloc, sx, kparm);
    timeleft = sig * sig - GOOD_SIGMA_MAX * GOOD_SIGMA_MAX;
    do {
      tdelta = AIR_MIN(GOOD_SIGMA_MAX * GOOD_SIGMA_MAX, timeleft);
      kparm[0] = sqrt(tdelta);
      dg->evalN_d(ktmp1, kloc, sx, kparm);
      for (ki = 0; ki < sx; ki++) {
        double csum = 0.0;
        for (kj = 0; kj < sx; kj++) {
          kk = ki - kj + rx;
          if (kk < sx) {
            csum += kern[kk] * ktmp1[kj];
          }
        }
        ktmp2[ki] = csum;
      }
      for (ki = 0; ki < sx; ki++) {
        kern[ki] = ktmp2[ki];
      }
      timeleft -= tdelta;
    } while (timeleft);
  }
  *kzP = oscx->dim >= 3 ? kern : oscx->kone;
  *kyP = oscx->dim >= 2 ? kern : oscx->kone;
  *kxP = kern;
  return;
}

/*
** sets one of the sampleImg, to be used as a sample in scale-space interp
*/
static void
_sampleSet(gageOptimSigContext *oscx, unsigned int si, double rho) {
  double *vol, *kz, *ky, *kx;
  unsigned int ii, xi, yi, zi;

  oscx->sampleSigma[si] = _SigOfRho(rho);
  oscx->sampleRho[si] = rho;
  vol = AIR_CAST(double *, oscx->nsampleImg[si]->data);
  _kernset(&kz, &ky, &kx, oscx, rho);
  ii = 0;
  for (zi = 0; zi < oscx->sz; zi++) {
    for (yi = 0; yi < oscx->sy; yi++) {
      for (xi = 0; xi < oscx->sx; xi++) {
        vol[ii] = kz[zi] * ky[yi] * kx[xi];
        ii++;
      }
    }
  }
  if (oscx->gctx) {
    /* the gage stack needs to know new scale pos */
    oscx->gctx->stackPos[si] = oscx->sampleSigma[si];
    /* HEY: GLK forgets why this is needed,
       but remembers it was a tricky bug to find */
    gagePointReset(&(oscx->gctx->point));
  }
  return;
}

static int /* Biff: 1 */
_errSingle(double *retP, gageOptimSigContext *oscx, double rho) {
  static const char me[] = "_errSingle";
  double *diff, *kx, *ky, *kz;
  const double *interp;
  unsigned int ii, xi, yi, zi;

  if (_volInterp(oscx->ninterp, rho, oscx)) {
    biffAddf(GAGE, "%s: trouble at rho %.17g\n", me, rho);
    return 1;
  }
  /*
  if (debugging) {
    char fname[AIR_STRLEN_SMALL+1];
    sprintf(fname, "interp-%04u.nrrd", debugii);
    nrrdSave(fname, oscx->ninterp, NULL);
  }
  */
  interp = AIR_CAST(const double *, oscx->ninterp->data);
  diff = AIR_CAST(double *, oscx->ndiff->data);
  _kernset(&kz, &ky, &kx, oscx, rho);
  ii = 0;
  for (zi = 0; zi < oscx->sz; zi++) {
    for (yi = 0; yi < oscx->sy; yi++) {
      for (xi = 0; xi < oscx->sx; xi++) {
        double tru;
        tru = kz[zi] * ky[yi] * kx[xi];
        diff[ii] = interp[ii] - tru;
        if (debugReconErrArr) {
          unsigned int idx = airArrayLenIncr(debugReconErrArr, 2);
          debugReconErr[idx + 0] = tru;
          debugReconErr[idx + 1] = interp[ii];
        }
        ii++;
      }
    }
  }
  nrrdMeasureLine[oscx->imgMeasr](retP, nrrdTypeDouble, diff, nrrdTypeDouble,
                                  ii /* HEY: cleverness */, AIR_NAN, AIR_NAN);
  return 0;
}

static int /* Biff: 1 */
_errTotal(double *retP, gageOptimSigContext *oscx) {
  static const char me[] = "_errTotal";
  unsigned int ii;
  double *err;

  err = AIR_CAST(double *, oscx->nerr->data);
  for (ii = 0; ii < oscx->trueImgNum; ii++) {
    /* debugii = ii; */
    if (_errSingle(err + ii, oscx,
                   AIR_AFFINE(0, ii, oscx->trueImgNum - 1, oscx->rhoRange[0],
                              oscx->rhoRange[1]))) {
      biffAddf(GAGE, "%s: trouble at ii %u", me, ii);
      return 1;
    }
  }
  nrrdMeasureLine[oscx->allMeasr](retP, nrrdTypeDouble, err, nrrdTypeDouble,
                                  oscx->trueImgNum, AIR_NAN, AIR_NAN);
  /*
  if (debugging) {
    char fname[AIR_STRLEN_SMALL+1];
    unsigned int ni;
    for (ni=0; ni<oscx->sampleNum; ni++) {
      sprintf(fname, "sample-%04u.nrrd", ni);
      nrrdSave(fname, oscx->nsampleImg[ni], NULL);
    }
  }
  */
  if (0) {
    static unsigned int call = 0;
    char fname[AIR_STRLEN_SMALL + 1];
    sprintf(fname, "err-%04u.nrrd", call);
    nrrdSave(fname, oscx->nerr, NULL);
    call++;
  }
  return 0;
}

static int /* Biff: 1 */
_errTotalLinf(double *retP, gageOptimSigContext *oscx, unsigned int mmIdx[2],
              double mmErr[2]) {
  static const char me[] = "_errTotalLinf";
  unsigned int ii, pi;
  double *err, *sem, *rr, rho, sig, pid;
  int outside;

  err = AIR_CAST(double *, oscx->nerr->data);
  sem = oscx->sampleErrMax;
  rr = oscx->rhoRange;
  for (pi = 0; pi <= oscx->sampleNum - 2; pi++) {
    sem[pi] = 0;
  }
  /* NOTE: we don't bother with last "true image": it will always be a
     low error, and not meaningfully associated with a gap */
  for (ii = 0; ii < oscx->trueImgNum - 1; ii++) {
    /* debugii = ii; */
    rho = AIR_AFFINE(0, ii, oscx->trueImgNum - 1, rr[0], rr[1]);
    if (_errSingle(err + ii, oscx, rho)) {
      biffAddf(GAGE, "%s: trouble at ii %u", me, ii);
      return 1;
    }
    sig = _SigOfRho(rho);
    pid = gageStackWtoI(oscx->gctx, sig, &outside);
    pi = AIR_UINT(pid);
    if (outside || !(pi <= oscx->sampleNum - 2)) {
      biffAddf(GAGE, "%s: ii %u -> rho %g -> sig %g -> pi %u-> OUTSIDE", me, ii, rho,
               sig, pi);
      return 1;
    }
    sem[pi] = AIR_MAX(sem[pi], err[ii]);
  }
  mmIdx[0] = mmIdx[1] = 0;
  mmErr[0] = mmErr[1] = sem[0];
  for (pi = 1; pi <= oscx->sampleNum - 2; pi++) {
    if (sem[pi] < mmErr[0]) {
      mmIdx[0] = pi;
      mmErr[0] = sem[pi];
    }
    if (sem[pi] > mmErr[1]) {
      mmIdx[1] = pi;
      mmErr[1] = sem[pi];
    }
  }
  /* returned error invented, not really Linf, but minimizing this
     does the right thing */
  *retP = 1000 * oscx->sampleNum * (mmErr[1] - mmErr[0]) / (rr[1] - rr[0]);
  if (0) {
    static unsigned int call = 0;
    char fname[AIR_STRLEN_SMALL + 1];
    sprintf(fname, "err-%04u.nrrd", call);
    nrrdSave(fname, oscx->nerr, NULL);
    call++;
  }
  return 0;
}

/*
** this can be called repeatedly
*/
static int /* Biff: 1 */
_gageSetup(gageOptimSigContext *oscx) {
  static const char me[] = "_gageSetup";
  double kparm[NRRD_KERNEL_PARMS_NUM];
  int E;

  if (oscx->gctx) {
    gageContextNix(oscx->gctx);
  }
  oscx->gctx = gageContextNew();
  gageParmSet(oscx->gctx, gageParmVerbose, 0);
  gageParmSet(oscx->gctx, gageParmRenormalize, AIR_FALSE);
  gageParmSet(oscx->gctx, gageParmCheckIntegrals, AIR_FALSE);
  gageParmSet(oscx->gctx, gageParmOrientationFromSpacing, AIR_TRUE);
  gageParmSet(oscx->gctx, gageParmStackUse, AIR_TRUE);
  E = 0;
  if (!E)
    E |= !(oscx->pvlBase = gagePerVolumeNew(oscx->gctx, oscx->nsampleImg[0],
                                            gageKindScl));
  if (!E)
    E |= gageStackPerVolumeNew(oscx->gctx, oscx->pvlSS,
                               AIR_CAST(const Nrrd *const *, oscx->nsampleImg),
                               oscx->sampleNum, gageKindScl);
  if (!E)
    E |= gageStackPerVolumeAttach(oscx->gctx, oscx->pvlBase, oscx->pvlSS,
                                  oscx->sampleSigma, oscx->sampleNum);
  kparm[0] = 1;
  if (!E) E |= gageKernelSet(oscx->gctx, gageKernel00, nrrdKernelBox, kparm);
  if (!E)
    E |= gageKernelSet(oscx->gctx, gageKernelStack, oscx->kssSpec->kernel,
                       oscx->kssSpec->parm);
  if (!E) E |= gageQueryItemOn(oscx->gctx, oscx->pvlBase, gageSclValue);
  if (!E) E |= gageUpdate(oscx->gctx);
  if (E) {
    biffAddf(GAGE, "%s: problem setting up gage", me);
    return 1;
  }
  return 0;
}

static char *
_timefmt(char tstr[AIR_STRLEN_MED + 1], double deltim) {

  if (deltim < 60) {
    sprintf(tstr, "%g secs", deltim);
    return tstr;
  }
  deltim /= 60;
  if (deltim < 60) {
    sprintf(tstr, "%g mins", deltim);
    return tstr;
  }
  deltim /= 60;
  if (deltim < 24) {
    sprintf(tstr, "%g hours", deltim);
    return tstr;
  }
  deltim /= 24;
  if (deltim < 7) {
    sprintf(tstr, "%g days", deltim);
    return tstr;
  }
  deltim /= 7;
  sprintf(tstr, "%g weeks", deltim);
  return tstr;
}

static int /* Biff: 1 */
_optsigrun(gageOptimSigContext *oscx) {
  static const char me[] = "_optsigrun";
  char tstr[AIR_STRLEN_MED + 1];
  unsigned int iter, pnt;
  double lastErr, newErr, rhoeps, oppor, lastPos, backoff, decavg, time0;
  int badStep;

  /* used to debug failure to measure error meaningfully
  {
    unsigned int hi, hn=200;
    double rr;
    for (hi=0; hi<hn; hi++) {
      rr = AIR_AFFINE(-20, hi, hn+20, oscx->rhoRange[0], oscx->rhoRange[1]);
      _sampleSet(oscx, 1, rr);
      _errTotal(oscx);
    }
  }
  */

  time0 = airTime();
  if (_errTotal(&lastErr, oscx)) {
    biffAddf(GAGE, "%s: first error measurement", me);
    return 1;
  }
  fprintf(stderr, "%s: (%s for initial error measr)\n", me,
          _timefmt(tstr, airTime() - time0));
  newErr = AIR_NAN;
  decavg = oscx->sampleNum; /* hack */
  /* meaningful discrete difference for looking at error gradient is
     bounded by the resolution of the sampling we're doing along scale */
  rhoeps = 0.1 * (oscx->rhoRange[1] - oscx->rhoRange[0]) / oscx->trueImgNum;
  oppor = 1.3333;
  backoff = 0.25;
  /* initialize step for the moving samples: 1 through oscx->sampleNum-2 */
  for (pnt = 1; pnt < oscx->sampleNum - 1; pnt++) {
    oscx->step[pnt] = 10;
  }
  for (iter = 0; iter < oscx->maxIter; iter++) {
    double limit, err1, grad, delta;
    unsigned int tryi;
    int zerodelta, esgn;
    esgn = 2 * AIR_INT(airRandInt(2)) - 1;
    pnt = 1 + (iter % (oscx->sampleNum - 2));
    lastPos = oscx->sampleRho[pnt];
    fprintf(stderr, "%s: ***** iter %u; [[ err %g ]] %s\n", me, iter, lastErr,
            _timefmt(tstr, airTime() - time0));
    limit = AIR_MIN((oscx->sampleRho[pnt] - oscx->sampleRho[pnt - 1]) / 3,
                    (oscx->sampleRho[pnt + 1] - oscx->sampleRho[pnt]) / 3);
    fprintf(stderr, ". pnt %u: pos %g, step %g\n", pnt, lastPos, oscx->step[pnt]);
    fprintf(stderr, ". limit = min((%g-%g)/3,(%g-%g)/3) = %g\n", oscx->sampleRho[pnt],
            oscx->sampleRho[pnt - 1], oscx->sampleRho[pnt + 1], oscx->sampleRho[pnt],
            limit);
    _sampleSet(oscx, pnt, lastPos + esgn * rhoeps);
    if (_errTotal(&err1, oscx)) {
      biffAddf(GAGE, "%s: for err1 (%u -> %.17g)", me, pnt, lastPos + esgn * rhoeps);
      return 1;
    }
    _sampleSet(oscx, pnt, lastPos);
    grad = (err1 - lastErr) / (esgn * rhoeps);
    fprintf(stderr, ". grad = %g\n", grad);
    delta = -grad * oscx->step[pnt];
    if (!AIR_EXISTS(delta)) {
      biffAddf(GAGE, "%s: got non-exist delta %g on iter %u (pnt %u) err %g", me, delta,
               iter, pnt, lastErr);
      return 1;
    }
    if (AIR_ABS(delta) > limit) {
      oscx->step[pnt] *= limit / AIR_ABS(delta);
      fprintf(stderr, ". step *= %g/%g -> %g\n", limit, AIR_ABS(delta), oscx->step[pnt]);
      delta = -grad * oscx->step[pnt];
    }
    fprintf(stderr, ". delta = %g\n", delta);
    tryi = 0;
    badStep = AIR_FALSE;
    do {
      if (tryi == oscx->maxIter) {
        biffAddf(GAGE, "%s: confusion (tryi %u) on iter %u (pnt %u) err %g", me, tryi,
                 iter, pnt, lastErr);
        return 1;
      }
      if (!delta) {
        fprintf(stderr, "... try %u: delta = 0; nothing to do\n", tryi);
        newErr = lastErr;
        zerodelta = AIR_TRUE;
      } else {
        zerodelta = AIR_FALSE;
        _sampleSet(oscx, pnt, lastPos + delta);
        if (_errTotal(&newErr, oscx)) {
          biffAddf(GAGE, "%s: for newErr (%u -> %.17g)", me, pnt, lastPos + delta);
          return 1;
        }
        badStep = newErr > lastErr;
        fprintf(stderr,
                "... try %u: pos[%u] %g + %g = %g;\n"
                "%s: err %g %s %g\n",
                tryi, pnt, lastPos, delta, oscx->sampleRho[pnt],
                badStep ? "*BAD*" : "good", newErr,
                newErr > lastErr ? ">" : "<=", lastErr);
        if (badStep) {
          oscx->step[pnt] *= backoff;
          if (oscx->step[pnt] < rhoeps / 1000) {
            /* step got so small its stupid to be moving this point */
            fprintf(stderr,
                    "... !! step %g < %g pointlessly small, "
                    "moving on\n",
                    oscx->step[pnt], rhoeps / 1000);
            _sampleSet(oscx, pnt, lastPos);
            newErr = lastErr;
            badStep = AIR_FALSE;
          } else {
            delta = -grad * oscx->step[pnt];
          }
        }
      }
      tryi++;
    } while (badStep);
    if (!zerodelta) {
      /* don't update decavg if we moved on because slope was EXACTLY zero */
      decavg = AIR_AFFINE(0, 1, oscx->sampleNum, decavg, (lastErr - newErr) / lastErr);
      oscx->step[pnt] *= oppor;
    }
    if (decavg <= oscx->convEps) {
      fprintf(stderr, "%s: converged (%g <= %g) after %u iters\n", me, decavg,
              oscx->convEps, iter);
      break;
    } else {
      fprintf(stderr, "%s: _____ iter %u done; decavg = %g > eps %g\n", me, iter, decavg,
              oscx->convEps);
    }
    lastErr = newErr;
  }
  if (iter == oscx->maxIter && decavg > oscx->convEps) {
    biffAddf(GAGE, "%s: failed to converge (%g > %g) after %u iters\n", me, decavg,
             oscx->convEps, iter);
    return 1;
  }
  oscx->finalErr = lastErr;

  return 0;
}

static int /* Biff: 1 */
_optsigrunLinf(gageOptimSigContext *oscx) {
  static const char me[] = "_optsigrunLinf";
  char tstr[AIR_STRLEN_MED + 1];
  double *srho, *stmp, time0, lastErr, newErr, decavg, step, oppor, backoff, ceps,
    mmErr[2];
  unsigned int iter, si, sn, mmIdx[2];
  int shrink;

  time0 = airTime();
  if (_errTotalLinf(&lastErr, oscx, mmIdx, mmErr)) {
    biffAddf(GAGE, "%s: first error measurement", me);
    return 1;
  }
  fprintf(stderr, "%s: (init)  min %u %g          max %u %g\n", me, mmIdx[0], mmErr[0],
          mmIdx[1], mmErr[1]);
  fprintf(stderr, "%s: (%s for initial error measr)\n", me,
          _timefmt(tstr, airTime() - time0));
  newErr = AIR_NAN;

  /* shorcuts */
  sn = oscx->sampleNum;
  srho = oscx->sampleRho;
  stmp = oscx->sampleTmp;

  /* Linf uses a single scalar step istead of oscx->step array */
  step = 0.1;
  oppor = 1.1;
  backoff = 0.5;

  /* more demanding for more points */
  ceps = oscx->convEps / sn;

  decavg = 2 * ceps;
  for (iter = 0; iter < oscx->maxIter; iter++) {
    double midp, wlo, whi, glo, ghi, gerr;
    unsigned int gap, tryi;
    int badStep;

    if (iter % 2) {
      /* we will grow gap around smallest peak */
      gap = mmIdx[0];
      gerr = mmErr[0];
      shrink = AIR_FALSE;
    } else {
      /* we will shrink gap around tallest peak */
      gap = mmIdx[1];
      gerr = mmErr[1];
      shrink = AIR_TRUE;
    }
    midp = (srho[gap] + srho[gap + 1]) / 2;
    fprintf(stderr, "%s: ---- iter %u (step %g): gap [%u]/%g (%s)\n", me, iter, step,
            gap, gerr, shrink ? "shrinking tallest" : "growing lowest");
    /* save last set of positions to restore after bad step */
    for (si = 1; si < sn - 1; si++) {
      stmp[si] = srho[si];
    }
    tryi = 0;
    badStep = AIR_FALSE;
    do {
      if (tryi == oscx->maxIter) {
        biffAddf(GAGE, "%s: confusion (tryi %u) on iter %u err %g", me, tryi, iter,
                 lastErr);
        return 1;
      }
      if (shrink) {
        wlo = AIR_AFFINE(0, step, 1, srho[gap], midp);
        whi = AIR_AFFINE(0, step, 1, srho[gap + 1], midp);
      } else {
        wlo = AIR_AFFINE(0, step, -2, srho[gap], midp);
        whi = AIR_AFFINE(0, step, -2, srho[gap + 1], midp);
      }
      glo = srho[gap];
      ghi = srho[gap + 1];
      fprintf(stderr, "%s:     rho[%u] %g | %g  -- rho[%u] %g | %g\n", me, gap,
              srho[gap], wlo, gap + 1, srho[gap + 1], whi);
      for (si = 1; si < sn - 1; si++) {
        _sampleSet(oscx, si,
                   (si <= gap
                      ? AIR_AFFINE(srho[0], srho[si], glo, srho[0], wlo)
                      : AIR_AFFINE(ghi, srho[si], srho[sn - 1], whi, srho[sn - 1])));
      }
      if (_errTotalLinf(&newErr, oscx, mmIdx, mmErr)) {
        biffAddf(GAGE, "%s: iter %u", me, iter);
        return 1;
      }
      fprintf(stderr, "%s:        min %u %g          max %u %g\n", me, mmIdx[0],
              mmErr[0], mmIdx[1], mmErr[1]);
      if (iter % 3) {
        badStep = newErr > lastErr;
        fprintf(stderr,
                "... try %u [%u] step %g -> newErr %g %s "
                "lastErr %g %s\n",
                tryi, gap, step, newErr, badStep ? ">" : "<=", lastErr,
                badStep ? "*BAD*" : "good");
        if (badStep) {
          step *= backoff;
          for (si = 1; si < sn - 1; si++) {
            srho[si] = stmp[si];
          }
        }
        tryi++;
      }
    } while (badStep);
    step *= oppor;
    decavg = (decavg + (lastErr - newErr)) / 2;
    if (AIR_IN_OP(0, decavg, ceps)) {
      fprintf(stderr, "%s: converged (%g <= %g) after %u iters\n", me, decavg, ceps,
              iter);
      break;
    } else {
      fprintf(stderr, "%s:      iter %u done; decavg = %g > eps %g\n", me, iter, decavg,
              ceps);
    }
    lastErr = newErr;
  } /* for iter */
  if (oscx->maxIter && iter == oscx->maxIter && decavg > ceps) {
    biffAddf(GAGE, "%s: failed to converge (%g > %g) after %u iters\n", me, decavg, ceps,
             iter);
    return 1;
  }
  oscx->finalErr = lastErr;

  return 0;
}

int /* Biff: 1 */
gageOptimSigCalculate(gageOptimSigContext *oscx,
                      /* output */ double *sigma, unsigned int sigmaNum,
                      const NrrdKernelSpec *kssSpec, int imgMeasr, int allMeasr,
                      unsigned int maxIter, double convEps) {
  static const char me[] = "gageOptimSigCalculate";
  unsigned int ii;

  if (!(oscx && sigma && kssSpec)) {
    biffAddf(GAGE, "%s: got NULL pointer", me);
    return 1;
  }
  if (sigmaNum > oscx->sampleNumMax) {
    biffAddf(GAGE, "%s: initialized for max %u samples, not %u", me, oscx->sampleNumMax,
             sigmaNum);
    return 1;
  }

  /* initialize to uniform samples in rho */
  oscx->sampleNum = sigmaNum;
  fprintf(stderr, "%s: initializing %u samples ... ", me, oscx->sampleNum);
  fflush(stderr);
  for (ii = 0; ii < oscx->sampleNum; ii++) {
    _sampleSet(oscx, ii,
               AIR_AFFINE(0, ii, oscx->sampleNum - 1, oscx->rhoRange[0],
                          oscx->rhoRange[1]));
  }
  fprintf(stderr, "done.\n");

  /* copy remaining input parameters */
  nrrdKernelSpecNix(oscx->kssSpec);
  oscx->kssSpec = nrrdKernelSpecCopy(kssSpec);
  oscx->imgMeasr = imgMeasr;
  oscx->allMeasr = allMeasr;
  oscx->convEps = convEps;
  oscx->maxIter = maxIter;
  oscx->convEps = convEps;

  /* set up gage */
  fprintf(stderr, "%s: setting up gage ... \n", me);
  if (_gageSetup(oscx)) {
    biffAddf(GAGE, "%s: problem setting up gage", me);
    return 1;
  }
  fprintf(stderr, "%s: ... gage setup done.\n", me);

  /* run the optimization */
  if (oscx->sampleNum > 2) {
    int EE;
    EE = (nrrdMeasureLinf == oscx->allMeasr ? _optsigrunLinf(oscx) : _optsigrun(oscx));
    if (EE) {
      biffAddf(GAGE, "%s: trouble", me);
      return 1;
    }
  } else {
    fprintf(stderr, "%s: num == 2, no optimization, finding error ... ", me);
    fflush(stderr);
    if (_errTotal(&(oscx->finalErr), oscx)) {
      biffAddf(GAGE, "%s: for finalErr", me);
      return 1;
    }
    fprintf(stderr, "done.\n");
  }

  /* save output */
  for (ii = 0; ii < oscx->sampleNum; ii++) {
    sigma[ii] = oscx->sampleSigma[ii];
  }

  return 0;
}

int /* Biff: 1 */
gageOptimSigErrorPlot(gageOptimSigContext *oscx, Nrrd *nout, const double *sigma,
                      unsigned int sigmaNum, const NrrdKernelSpec *kssSpec,
                      int imgMeasr) {
  static const char me[] = "gageOptimSigErrorPlot";
  char doneStr[AIR_STRLEN_SMALL + 1];
  double *out;
  unsigned int ii;

  if (!(oscx && nout && sigma)) {
    biffAddf(GAGE, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(sigmaNum >= 2)) {
    biffAddf(GAGE, "%s: need sigmaNum >= 2 (not %u)", me, sigmaNum);
    return 1;
  }
  if (sigmaNum > oscx->sampleNumMax) {
    biffAddf(GAGE, "%s: initialized for max %u samples, not %u", me, oscx->sampleNumMax,
             sigmaNum);
    return 1;
  }

  /* copy remaining input parms */
  nrrdKernelSpecNix(oscx->kssSpec);
  oscx->kssSpec = nrrdKernelSpecCopy(kssSpec);
  oscx->sampleNum = sigmaNum;
  oscx->maxIter = 0;
  oscx->imgMeasr = imgMeasr;
  oscx->allMeasr = nrrdMeasureUnknown;
  oscx->convEps = AIR_NAN;
  if (nrrdMaybeAlloc_va(nout, nrrdTypeDouble, 2, AIR_SIZE_T(2),
                        AIR_SIZE_T(oscx->trueImgNum))) {
    biffMovef(GAGE, NRRD, "%s: trouble allocating output", me);
    return 1;
  }
  out = AIR_CAST(double *, nout->data);

  /* set up requested samples */
  for (ii = 0; ii < oscx->sampleNum; ii++) {
    _sampleSet(oscx, ii, _RhoOfSig(sigma[ii]));
  }
  if (_gageSetup(oscx)) {
    biffAddf(GAGE, "%s: problem setting up gage", me);
    return 1;
  }
  fprintf(stderr, "%s: plotting ...       ", me);
  fflush(stderr);
  for (ii = 0; ii < oscx->trueImgNum; ii++) {
    double rho, err;
    fprintf(stderr, "%s", airDoneStr(0, ii, oscx->trueImgNum, doneStr));
    fflush(stderr);
    rho = AIR_AFFINE(0, ii, oscx->trueImgNum - 1, oscx->rhoRange[0], oscx->rhoRange[1]);
    out[0 + 2 * ii] = rho;
    /* debugii = ii; */
    if (_errSingle(&err, oscx, rho)) {
      biffAddf(GAGE, "%s: plotting %u", me, ii);
      return 1;
    }
    out[1 + 2 * ii] = err;
    /*
    if (AIR_IN_CL(69,ii,71)) {
      fprintf(stderr, "!%s: ----- %u : %g\n", me, ii, err);
    }
    */
  }
  fprintf(stderr, "%s\n", airDoneStr(0, ii, oscx->trueImgNum, doneStr));

  /*
  if (0) {
    static unsigned int call=0;
    char fname[AIR_STRLEN_SMALL+1];
    unsigned int ni;
    if (0) {
      sprintf(fname, "err-%04u.nrrd", call);
      nrrdSave(fname, oscx->nerr, NULL);
    }
    if (debugging) {
      for (ni=0; ni<oscx->sampleNum; ni++) {
        sprintf(fname, "sample-%04u.nrrd", ni);
        nrrdSave(fname, oscx->nsampleImg[ni], NULL);
      }
    }
    call++;
    debugging = (2 == call);
  }
  */

  return 0;
}

int /* Biff: 1 */
gageOptimSigErrorPlotSliding(gageOptimSigContext *oscx, Nrrd *nout, double windowRho,
                             unsigned int sampleNum, const NrrdKernelSpec *kssSpec,
                             int imgMeasr) {
  static const char me[] = "gageOptimSigErrorPlotSliding";
  char doneStr[AIR_STRLEN_SMALL + 1];
  unsigned int ii;
  double *out;
  char hackKeyStr[] = "TEEM_OPTSIG_RECONERR";

  if (!(oscx && nout && kssSpec)) {
    biffAddf(GAGE, "%s: got NULL pointer", me);
    return 1;
  }
  if (windowRho <= 0) {
    biffAddf(GAGE, "%s: need positive windowRho (not %g)", me, windowRho);
    return 1;
  }
  if (windowRho > oscx->rhoRange[1] - oscx->rhoRange[0]) {
    biffAddf(GAGE, "%s: window %g > rhorange %g-%g=%g", me, windowRho, oscx->rhoRange[1],
             oscx->rhoRange[0], oscx->rhoRange[1] - oscx->rhoRange[0]);
    return 1;
  }

  if (nrrdGetenvString(&debugReconErrName, hackKeyStr)) {
    fprintf(stderr, "%s: %s hack on: will save recon results to %s\n", me, hackKeyStr,
            debugReconErrName);
    debugReconErrArr = airArrayNew(AIR_CAST(void **, &(debugReconErr)), NULL,
                                   sizeof(double), 1000);
  }

  /* copy remaining input parms */
  nrrdKernelSpecNix(oscx->kssSpec);
  oscx->kssSpec = nrrdKernelSpecCopy(kssSpec);
  oscx->sampleNum = 3; /* hacky */
  oscx->maxIter = 0;
  oscx->imgMeasr = imgMeasr;
  oscx->allMeasr = nrrdMeasureUnknown;
  oscx->convEps = AIR_NAN;
  oscx->sampleSigma[0] = oscx->sigmaRange[0]; /* just for gage setup */
  oscx->sampleSigma[1] = oscx->sigmaRange[1]; /* just for gage setup */
  oscx->sampleSigma[2] = oscx->sigmaRange[1] + 1;
  if (_gageSetup(oscx)) {
    biffAddf(GAGE, "%s: problem setting up gage", me);
    return 1;
  }
  if (nrrdMaybeAlloc_va(nout, nrrdTypeDouble, 2, AIR_SIZE_T(2), AIR_SIZE_T(sampleNum))) {
    biffMovef(GAGE, NRRD, "%s: trouble allocating output", me);
    return 1;
  }
  out = AIR_CAST(double *, nout->data);
  fprintf(stderr, "%s: plotting ...       ", me);
  fflush(stderr);
  for (ii = 0; ii < sampleNum; ii++) {
    double rho, rlo, rhi, err;
    fprintf(stderr, "%s", airDoneStr(0, ii, oscx->trueImgNum, doneStr));
    fflush(stderr);
    rho = AIR_AFFINE(0, ii, sampleNum, oscx->rhoRange[0], oscx->rhoRange[1]);
    rlo = rho - windowRho / 2;
    rhi = rho + windowRho / 2;
    if (rlo < oscx->rhoRange[0] || rhi > oscx->rhoRange[1]) {
      if (debugReconErrArr) {
        /* we have to simulate the updates to debugReconErrArr
           that would happen with a call to _errSingle */
        airArrayLenIncr(debugReconErrArr, 2 * oscx->sz * oscx->sy * oscx->sx);
      }
      out[0 + 2 * ii] = _SigOfRho(rho);
      out[1 + 2 * ii] = AIR_NAN;
      continue;
    }
    /* required samples will slide along with plotting */
    _sampleSet(oscx, 0, rlo);
    _sampleSet(oscx, 1, rhi);
    if (_errSingle(&err, oscx, rho)) {
      biffAddf(GAGE, "%s: plotting/sliding %u", me, ii);
      return 1;
    }
    out[0 + 2 * ii] = _SigOfRho(rho);
    out[1 + 2 * ii] = err;
  }
  fprintf(stderr, "%s\n", airDoneStr(0, ii, oscx->trueImgNum, doneStr));

  if (debugReconErrArr) {
    Nrrd *nre = nrrdNew();
    nrrdWrap_va(nre, debugReconErr, nrrdTypeDouble, 3, AIR_SIZE_T(2),
                AIR_SIZE_T(oscx->sz * oscx->sy * oscx->sx), AIR_SIZE_T(sampleNum));
    nrrdSave(debugReconErrName, nre, NULL);
    nrrdNix(nre);
  }

  return 0;
}
