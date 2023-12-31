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

#include "ell.h"

/* lop A
  fprintf(stderr, "_ellAlign3: ----------\n");
  fprintf(stderr, "_ellAlign3: v0 = %g %g %g\n", (v+0)[0], (v+0)[1], (v+0)[2]);
  fprintf(stderr, "_ellAlign3: v3 = %g %g %g\n", (v+3)[0], (v+3)[1], (v+3)[2]);
  fprintf(stderr, "_ellAlign3: v6 = %g %g %g\n", (v+6)[0], (v+6)[1], (v+6)[2]);
  fprintf(stderr, "_ellAlign3: d = %g %g %g -> %d %d %d\n",
          d0, d1, d2, Mi, ai, bi);
  fprintf(stderr, "_ellAlign3:  pre dot signs (03, 06, 36): %d %d %d\n",
          airSgn(ELL_3V_DOT(v+0, v+3)),
          airSgn(ELL_3V_DOT(v+0, v+6)),
          airSgn(ELL_3V_DOT(v+3, v+6)));
  */

/* lop B
  fprintf(stderr, "_ellAlign3: v0 = %g %g %g\n", (v+0)[0], (v+0)[1], (v+0)[2]);
  fprintf(stderr, "_ellAlign3: v3 = %g %g %g\n", (v+3)[0], (v+3)[1], (v+3)[2]);
  fprintf(stderr, "_ellAlign3: v6 = %g %g %g\n", (v+6)[0], (v+6)[1], (v+6)[2]);
  fprintf(stderr, "_ellAlign3:  post dot signs %d %d %d\n",
          airSgn(ELL_3V_DOT(v+0, v+3)),
          airSgn(ELL_3V_DOT(v+0, v+6)),
          airSgn(ELL_3V_DOT(v+3, v+6)));
  if (airSgn(ELL_3V_DOT(v+0, v+3)) < 0
      || airSgn(ELL_3V_DOT(v+0, v+6)) < 0
      || airSgn(ELL_3V_DOT(v+3, v+6)) < 0) {
    exit(1);
  }
  */

/*
******** ell_quadratic()
**
** finds real roots of A*x^2 + B*x + C.
**
** records the found roots in the given root array, and returns a
** value from the ell_quadratic_root* enum:
**
**   ell_quadratic_root_two:
**      two distinct roots root[0] > root[1]
**   ell_quadratic_root_complex:
**      two complex conjugate roots at root[0] +/- i*root[1]
**   ell_quadratic_root_double:
**      a repeated root root[0] == root[1]
**
** HEY simple as this code may seem, it has definitely numerical
** issues that have not been explored or fixed, such as what if A is
** near 0.  Also correctly handling the transition from double root to
** complex roots needs to be re-thought, as well as this issue:
** http://people.csail.mit.edu/bkph/articles/Quadratics.pdf Should
** also understand http://www.cs.berkeley.edu/~wkahan/Qdrtcs.pdf
*/
int /* Biff: nope */
ell_quadratic(double root[2], double A, double B, double C) {
  /* static const char me[] = "ell_quadratic"; */
  int ret;
  double disc, rd, tmp, eps = 1.0E-12;

  disc = B * B - 4 * A * C;
  if (disc > 0) {
    rd = sqrt(disc);
    root[0] = (-B + rd) / (2 * A);
    root[1] = (-B - rd) / (2 * A);
    if (root[0] < root[1]) {
      ELL_SWAP2(root[0], root[1], tmp);
    }
    ret = ell_quadratic_root_two;
  } else if (disc < -eps) {
    root[0] = -B / (2 * A);
    root[1] = sqrt(-disc) / (2 * A);
    ret = ell_quadratic_root_complex;
  } else {
    /* 0 == disc or only *very slightly* negative */
    root[0] = root[1] = -B / (2 * A);
    ret = ell_quadratic_root_double;
  }
  return ret;
}

int /* Biff: nope */
ell_2m_eigenvalues_d(double eval[2], const double m[4]) {
  double A, B, C;
  int ret;

  A = 1;
  B = -m[0] - m[3];
  C = m[0] * m[3] - m[1] * m[2];
  ret = ell_quadratic(eval, A, B, C);
  return ret;
}

void
ell_2m_1d_nullspace_d(double ans[2], const double _n[4]) {
  /* static const char me[] = "ell_2m_1d_nullspace_d"; */
  double n[4], dot, len, rowv[2];

  ELL_4V_COPY(n, _n);
  dot = ELL_2V_DOT(n + 2 * 0, n + 2 * 1);
  /*
  fprintf(stderr, "!%s: n = {{%g,%g},{%g,%g}}\n", me,
          n[0], n[1], n[2], n[3]);
  fprintf(stderr, "!%s: dot = %g\n", me, dot);
  */
  if (dot > 0) {
    ELL_2V_ADD2(rowv, n + 2 * 0, n + 2 * 1);
  } else {
    ELL_2V_SUB(rowv, n + 2 * 0, n + 2 * 1);
  }
  /* fprintf(stderr, "!%s: rowv = %g %g\n", me, rowv[0], rowv[1]); */
  /* have found good description of what's perpendicular nullspace,
     so now perpendicularize it */
  ans[0] = rowv[1];
  ans[1] = -rowv[0];
  ELL_2V_NORM(ans, ans, len);
  /*
  if (!(AIR_EXISTS(ans[0]) && AIR_EXISTS(ans[1]))) {
    fprintf(stderr, "!%s: bad! %g %g\n", me, ans[0], ans[1]);
  }
  */
  return;
}

/*
******** ell_2m_eigensolve_d
**
** Eigensolve 2x2 matrix, which may be asymmetric
*/
int /* Biff: nope */
ell_2m_eigensolve_d(double eval[2], double evec[4], const double m[4]) {
  /* static const char me[] = "ell_2m_eigensolve_d"; */
  double nul[4], ident[4] = {1, 0, 0, 1};
  int ret;

  ret = ell_2m_eigenvalues_d(eval, m);
  /*
  fprintf(stderr, "!%s: m = {{%.17g,%.17g},{%.17g,%.17g}} -> "
          "%s evals (%.17g,%.17g)\n", me, m[0], m[1], m[2], m[3],
          airEnumStr(ell_quadratic_root, ret), eval[0], eval[1]);
  */
  switch (ret) {
  case ell_quadratic_root_two:
    ELL_4V_SCALE_ADD2(nul, 1.0, m, -eval[0], ident);
    ell_2m_1d_nullspace_d(evec + 2 * 0, nul);
    /*
    fprintf(stderr, "!%s: eval=%.17g -> nul {{%.17g,%.17g},{%.17g,%.17g}} "
            "-> evec %.17g %.17g\n", me, eval[0],
            nul[0], nul[1], nul[2], nul[3],
            (evec + 2*0)[0], (evec + 2*0)[1]);
    */
    ELL_4V_SCALE_ADD2(nul, 1.0, m, -eval[1], ident);
    ell_2m_1d_nullspace_d(evec + 2 * 1, nul);
    /*
    fprintf(stderr, "!%s: eval=%.17g -> nul {{%.17g,%.17g},{%.17g,%.17g}} "
            "-> evec %.17g %.17g\n", me, eval[1],
            nul[0], nul[1], nul[2], nul[3],
            (evec + 2*1)[0], (evec + 2*1)[1]);
    */
    break;
  case ell_quadratic_root_double:
    /* fprintf(stderr, "!%s: double eval=%.17g\n", me, eval[0]); */
    ELL_4V_SCALE_ADD2(nul, 1.0, m, -eval[0], ident);
    /*
    fprintf(stderr, "!%s: nul = {{%.17g,%.17g},{%.17g,%.17g}} (len %.17g)\n",
            me, nul[0], nul[1], nul[2], nul[3], ELL_4V_LEN(nul));
    */
    if (ELL_4V_DOT(nul, nul)) {
      /* projecting out the nullspace produced non-zero matrix,
         (possibly from an asymmetric matrix) so there is real
         orientation to recover */
      ell_2m_1d_nullspace_d(evec + 2 * 0, nul);
      ELL_2V_COPY(evec + 2 * 1, evec + 2 * 0);
    } else {
      /* so this was isotropic symmetric; invent orientation */
      ELL_2V_SET(evec + 2 * 0, 1, 0);
      ELL_2V_SET(evec + 2 * 1, 0, 1);
    }
    break;
  case ell_quadratic_root_complex:
    /* HEY punting for now */
    ELL_2V_SET(evec + 2 * 0, 0.5, 0);
    ELL_2V_SET(evec + 2 * 1, 0, 0.5);
    break;
  default:
    /* fprintf(stderr, "%s: unexpected solution indicator %d\n", me, ret); */
    break;
  }
  return ret;
}

static void
_ell_align3_d(double v[9]) {
  double d0, d1, d2;
  int Mi, ai, bi;

  d0 = ELL_3V_DOT(v + 0, v + 0);
  d1 = ELL_3V_DOT(v + 3, v + 3);
  d2 = ELL_3V_DOT(v + 6, v + 6);
  Mi = ELL_MAX3_IDX(d0, d1, d2);
  ai = (Mi + 1) % 3;
  bi = (Mi + 2) % 3;
  /* lop A */
  if (ELL_3V_DOT(v + 3 * Mi, v + 3 * ai) < 0) {
    ELL_3V_SCALE(v + 3 * ai, -1, v + 3 * ai);
  }
  if (ELL_3V_DOT(v + 3 * Mi, v + 3 * bi) < 0) {
    ELL_3V_SCALE(v + 3 * bi, -1, v + 3 * bi);
  }
  /* lob B */
  /* we can't guarantee that dot(v+3*ai,v+3*bi) > 0 . . . */
}

/*
** leaves v+3*0 untouched, but makes sure that v+3*0, v+3*1, and v+3*2
** are mutually orthogonal.  Also leaves the magnitudes of all
** vectors unchanged.
*/
static void
_ell_3m_enforce_orthogonality(double v[9]) {
  double d00, d10, d11, d20, d21, d22, scl, tv[3];

  d00 = ELL_3V_DOT(v + 3 * 0, v + 3 * 0);
  d10 = ELL_3V_DOT(v + 3 * 1, v + 3 * 0);
  d11 = ELL_3V_DOT(v + 3 * 1, v + 3 * 1);
  ELL_3V_SCALE_ADD2(tv, 1, v + 3 * 1, -d10 / d00, v + 3 * 0);
  scl = sqrt(d11 / ELL_3V_DOT(tv, tv));
  ELL_3V_SCALE(v + 3 * 1, scl, tv);
  d20 = ELL_3V_DOT(v + 3 * 2, v + 3 * 0);
  d21 = ELL_3V_DOT(v + 3 * 2, v + 3 * 1);
  d22 = ELL_3V_DOT(v + 3 * 2, v + 3 * 2);
  ELL_3V_SCALE_ADD3(tv, 1, v + 3 * 2, -d20 / d00, v + 3 * 0, -d21 / d00, v + 3 * 1);
  scl = sqrt(d22 / ELL_3V_DOT(tv, tv));
  ELL_3V_SCALE(v + 3 * 2, scl, tv);
  return;
}

/*
** makes sure that v+3*2 has a positive dot product with
** cross product of v+3*0 and v+3*1
*/
static void
_ell_3m_make_right_handed_d(double v[9]) {
  double x[3];

  ELL_3V_CROSS(x, v + 3 * 0, v + 3 * 1);
  if (0 > ELL_3V_DOT(x, v + 3 * 2)) {
    ELL_3V_SCALE(v + 3 * 2, -1, v + 3 * 2);
  }
}

/* lop A
  fprintf(stderr, "===  pre ===\n");
  fprintf(stderr, "crosses:  %g %g %g\n", (t+0)[0], (t+0)[1], (t+0)[2]);
  fprintf(stderr, "          %g %g %g\n", (t+3)[0], (t+3)[1], (t+3)[2]);
  fprintf(stderr, "          %g %g %g\n", (t+6)[0], (t+6)[1], (t+6)[2]);
  fprintf(stderr, "cross dots:  %g %g %g\n",
          ELL_3V_DOT(t+0, t+3), ELL_3V_DOT(t+0, t+6), ELL_3V_DOT(t+3, t+6));
*/

/* lop B
  fprintf(stderr, "=== post ===\n");
  fprintf(stderr, "crosses:  %g %g %g\n", (t+0)[0], (t+0)[1], (t+0)[2]);
  fprintf(stderr, "          %g %g %g\n", (t+3)[0], (t+3)[1], (t+3)[2]);
  fprintf(stderr, "          %g %g %g\n", (t+6)[0], (t+6)[1], (t+6)[2]);
  fprintf(stderr, "cross dots:  %g %g %g\n",
          ELL_3V_DOT(t+0, t+3), ELL_3V_DOT(t+0, t+6), ELL_3V_DOT(t+3, t+6));
*/

/*
******** ell_3m_1d_nullspace_d()
**
** the given matrix is assumed to have a nullspace of dimension one.
** A normalized vector which spans the nullspace is put into ans.
**
** The given nullspace matrix is NOT modified.
**
** This does NOT use biff
*/
void
ell_3m_1d_nullspace_d(double ans[3], const double _n[9]) {
  double t[9], n[9], norm;

  ELL_3M_TRANSPOSE(n, _n);
  /* find the three cross-products of pairs of column vectors of n */
  ELL_3V_CROSS(t + 0, n + 0, n + 3);
  ELL_3V_CROSS(t + 3, n + 0, n + 6);
  ELL_3V_CROSS(t + 6, n + 3, n + 6);
  /* lop A */
  _ell_align3_d(t);
  /* lop B */
  /* add them up (longer, hence more accurate, should dominate) */
  ELL_3V_ADD3(ans, t + 0, t + 3, t + 6);

  /* normalize */
  ELL_3V_NORM(ans, ans, norm);

  return;
}

/*
******** ell_3m_2d_nullspace_d()
**
** the given matrix is assumed to have a nullspace of dimension two.
**
** The given nullspace matrix is NOT modified
**
** This does NOT use biff
*/
void
ell_3m_2d_nullspace_d(double ans0[3], double ans1[3], const double _n[9]) {
  double n[9], tmp[3], norm;

  ELL_3M_TRANSPOSE(n, _n);
  _ell_align3_d(n);
  ELL_3V_ADD3(tmp, n + 0, n + 3, n + 6);
  ELL_3V_NORM(tmp, tmp, norm);

  /* any two vectors which are perpendicular to the (supposedly 1D)
     span of the column vectors span the nullspace */
  ell_3v_perp_d(ans0, tmp);
  ELL_3V_NORM(ans0, ans0, norm);
  ELL_3V_CROSS(ans1, tmp, ans0);

  return;
}

/*
******** ell_3m_eigenvalues_d()
**
** finds eigenvalues of given matrix.
**
** returns information about the roots according to ellCubeRoot enum,
** see header for ellCubic for details.
**
** given matrix is NOT modified
**
** This does NOT use biff
**
** Doing the frobenius normalization proved successfull in avoiding the
** the creating of NaN eigenvalues when the coefficients of the matrix
** were really large (> 50000).  Also, when the matrix norm was really
** small, the comparison to "epsilon" in ell_cubic mistook three separate
** roots for a single and a double, with this matrix in particular:
**  1.7421892  0.0137642  0.0152975
**  0.0137642  1.7565432 -0.0062296
**  0.0152975 -0.0062296  1.7700019
** (actually, this is prior to tenEigensolve's isotropic removal)
**
** HEY: tenEigensolve_d and tenEigensolve_f start by removing the
** isotropic part of the tensor.  It may be that that smarts should
** be migrated here, but GLK is uncertain how it would change the
** handling of non-symmetric matrices.
*/
int /* Biff: nope */
ell_3m_eigenvalues_d(double _eval[3], const double _m[9], const int newton) {
  double A, B, C, scale, frob, m[9], eval[3];
  int roots;

  frob = ELL_3M_FROB(_m);
  scale = frob ? 1.0 / frob : 1.0;
  ELL_3M_SCALE(m, scale, _m);
  /*
  printf("!%s: m = %g %g %g; %g %g %g; %g %g %g\n", "ell_3m_eigenvalues_d",
         m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]);
  */
  /*
  ** from gordon with mathematica; these are the coefficients of the
  ** cubic polynomial in x: det(x*I - M).  The full cubic is
  ** x^3 + A*x^2 + B*x + C.
  */
  A = -m[0] - m[4] - m[8];
  B = m[0] * m[4] - m[3] * m[1] + m[0] * m[8] - m[6] * m[2] + m[4] * m[8] - m[7] * m[5];
  C = (m[6] * m[4] - m[3] * m[7]) * m[2] + (m[0] * m[7] - m[6] * m[1]) * m[5]
    + (m[3] * m[1] - m[0] * m[4]) * m[8];
  /*
  printf("!%s: A B C = %g %g %g\n", "ell_3m_eigenvalues_d", A, B, C);
  */
  roots = ell_cubic(eval, A, B, C, newton);
  /* no longer need to sort here */
  ELL_3V_SCALE(_eval, 1.0 / scale, eval);
  return roots;
}

/*
** it's a weird function because eval is modified!
*/
static void
_ell_3m_evecs_d(double evec[9], double eval[3], int roots, const double m[9]) {
  double n[9], e0 = 0, e1 = 0.0, e2 = 0.0, t /* , tmpv[3] */;

  ELL_3V_GET(e0, e1, e2, eval);
  /* if (ell_debug) {
    printf("ell_3m_evecs_d: numroots = %d\n", numroots);
    } */

  /* we form m - lambda*I by doing a memcpy from m, and then
     (repeatedly) over-writing the diagonal elements */
  ELL_3M_COPY(n, m);
  switch (roots) {
  case ell_cubic_root_three:
    /* if (ell_debug) {
      printf("ell_3m_evecs_d: evals: %20.15f %20.15f %20.15f\n",
             eval[0], eval[1], eval[2]);
             } */
    ELL_3M_DIAG_SET(n, m[0] - e0, m[4] - e0, m[8] - e0);
    ell_3m_1d_nullspace_d(evec + 0, n);
    ELL_3M_DIAG_SET(n, m[0] - e1, m[4] - e1, m[8] - e1);
    ell_3m_1d_nullspace_d(evec + 3, n);
    ELL_3M_DIAG_SET(n, m[0] - e2, m[4] - e2, m[8] - e2);
    ell_3m_1d_nullspace_d(evec + 6, n);
    _ell_3m_enforce_orthogonality(evec);
    _ell_3m_make_right_handed_d(evec);
    ELL_3V_SET(eval, e0, e1, e2);
    break;
  case ell_cubic_root_single_double:
    ELL_SORT3(e0, e1, e2, t);
    if (e0 > e1) {
      /* one big (e0) , two small (e1, e2) : more like a cigar */
      ELL_3M_DIAG_SET(n, m[0] - e0, m[4] - e0, m[8] - e0);
      ell_3m_1d_nullspace_d(evec + 0, n);
      ELL_3M_DIAG_SET(n, m[0] - e1, m[4] - e1, m[8] - e1);
      ell_3m_2d_nullspace_d(evec + 3, evec + 6, n);
    } else {
      /* two big (e0, e1), one small (e2): more like a pancake */
      ELL_3M_DIAG_SET(n, m[0] - e0, m[4] - e0, m[8] - e0);
      ell_3m_2d_nullspace_d(evec + 0, evec + 3, n);
      ELL_3M_DIAG_SET(n, m[0] - e2, m[4] - e2, m[8] - e2);
      ell_3m_1d_nullspace_d(evec + 6, n);
    }
    _ell_3m_enforce_orthogonality(evec);
    _ell_3m_make_right_handed_d(evec);
    ELL_3V_SET(eval, e0, e1, e2);
    break;
  case ell_cubic_root_triple:
    /* one triple root; use any basis as the eigenvectors */
    ELL_3V_SET(evec + 0, 1, 0, 0);
    ELL_3V_SET(evec + 3, 0, 1, 0);
    ELL_3V_SET(evec + 6, 0, 0, 1);
    ELL_3V_SET(eval, e0, e1, e2);
    break;
  case ell_cubic_root_single:
    /* only one real root */
    ELL_3M_DIAG_SET(n, m[0] - e0, m[4] - e0, m[8] - e0);
    ell_3m_1d_nullspace_d(evec + 0, n);
    ELL_3V_SET(evec + 3, AIR_NAN, AIR_NAN, AIR_NAN);
    ELL_3V_SET(evec + 6, AIR_NAN, AIR_NAN, AIR_NAN);
    ELL_3V_SET(eval, e0, AIR_NAN, AIR_NAN);
    break;
  }
  /* if (ell_debug) {
    printf("ell_3m_evecs_d (numroots = %d): evecs: \n", numroots);
    ELL_3MV_MUL(tmpv, m, evec[0]);
    printf(" (%g:%g): %20.15f %20.15f %20.15f\n",
           eval[0], ELL_3V_DOT(evec[0], tmpv),
           evec[0][0], evec[0][1], evec[0][2]);
    ELL_3MV_MUL(tmpv, m, evec[1]);
    printf(" (%g:%g): %20.15f %20.15f %20.15f\n",
           eval[1], ELL_3V_DOT(evec[1], tmpv),
           evec[1][0], evec[1][1], evec[1][2]);
    ELL_3MV_MUL(tmpv, m, evec[2]);
    printf(" (%g:%g): %20.15f %20.15f %20.15f\n",
           eval[2], ELL_3V_DOT(evec[2], tmpv),
           evec[2][0], evec[2][1], evec[2][2]);
           } */
  return;
}

/*
******** ell_3m_eigensolve_d()
**
** finds eigenvalues and eigenvectors of given matrix m
**
** returns information about the roots according to ell_cubic_root enum;
** When eval[i] is set, evec+3*i is set to a corresponding eigenvector.
** The eigenvectors are (evec+0)[], (evec+3)[], and (evec+6)[]
**
** NOTE: Even in the post-Teem-1.7 switch from column-major to
** row-major- its still the case that the eigenvectors are at
** evec+0, evec+3, evec+6: this means that they USED to be the
** "columns" of the matrix, and NOW they're the rows.
**
** The eigenvalues (and associated eigenvectors) are sorted in
** descending order.
*/
int /* Biff: nope */
ell_3m_eigensolve_d(double eval[3], double evec[9], const double m[9],
                    const int newton) {
  int roots;

  /* if (ell_debug) {
    printf("ell_3m_eigensolve_d: input matrix:\n");
    printf("{{%20.15f,\t%20.15f,\t%20.15f},\n", m[0], m[1], m[2]);
    printf(" {%20.15f,\t%20.15f,\t%20.15f},\n", m[3], m[4], m[5]);
    printf(" {%20.15f,\t%20.15f,\t%20.15f}};\n",m[6], m[7], m[8]);
    } */

  roots = ell_3m_eigenvalues_d(eval, m, newton);
  _ell_3m_evecs_d(evec, eval, roots, m);

  return roots;
}

/* ____________________________ 3m2sub ____________________________ */
/*
******** ell_3m2sub_eigenvalues_d
**
** for doing eigensolve of the upper-left 2x2 matrix sub-matrix of a
** 3x3 matrix.  The other entries are assumed to be zero.  A 0 root is
** put last (in eval[2]), possibly in defiance of the usual eigenvalue
** ordering.
*/
int /* Biff: nope */
ell_3m2sub_eigenvalues_d(double eval[3], const double _m[9]) {
  double A, B, m[4], D, Dsq, eps = 1.0E-11;
  int roots;
  /* static const char me[] = "ell_3m2sub_eigenvalues_d"; */

  m[0] = _m[0];
  m[1] = _m[1];
  m[2] = _m[3];
  m[3] = _m[4];

  /* cubic characteristic equation is L^3 + A*L^2 + B*L = 0 */
  A = -m[0] - m[3];
  B = m[0] * m[3] - m[1] * m[2];
  Dsq = A * A - 4 * B;
  /*
  fprintf(stderr, "!%s: m = {{%f,%f},{%f,%f}} -> A=%f B=%f Dsq=%.17f %s 0 (%.17f)\n", me,
          m[0], m[1], m[2], m[3], A, B, Dsq,
          (Dsq > 0 ? ">" : (Dsq < 0 ? "<" : "==")), eps);
  fprintf(stderr, "!%s: Dsq = \n", me);
  airFPFprintf_d(stderr, Dsq);
  fprintf(stderr, "!%s: eps = \n", me);
  airFPFprintf_d(stderr, eps);
  ell_3m_print_d(stderr, _m);
  */
  if (Dsq > eps) {
    D = sqrt(Dsq);
    eval[0] = (-A + D) / 2;
    eval[1] = (-A - D) / 2;
    eval[2] = 0;
    roots = ell_cubic_root_three;
  } else if (Dsq < -eps) {
    /* no quadratic roots; only the implied zero */
    ELL_3V_SET(eval, AIR_NAN, AIR_NAN, 0);
    roots = ell_cubic_root_single;
  } else {
    /* a quadratic double root */
    ELL_3V_SET(eval, -A / 2, -A / 2, 0);
    roots = ell_cubic_root_single_double;
  }
  /*
  fprintf(stderr, "!%s: Dsq=%f, roots=%d (%f %f %f)\n",
          me, Dsq, roots, eval[0], eval[1], eval[2]);
  */
  return roots;
}

static void
_ell_22v_enforce_orthogonality(double uu[2], double _vv[2]) {
  double dot, vv[2], len;

  dot = ELL_2V_DOT(uu, _vv);
  ELL_2V_SCALE_ADD2(vv, 1, _vv, -dot, uu);
  ELL_2V_NORM(_vv, vv, len);
  return;
}

/*
** NOTE: assumes that eval and roots have come from
** ell_3m2sub_eigenvalues_d(m)
*/
static void
_ell_3m2sub_evecs_d(double evec[9], double eval[3], int roots, const double m[9]) {
  double n[4];
  static const char me[] = "_ell_3m2sub_evecs_d";

  if (ell_cubic_root_three == roots) {
    /* set off-diagonal entries once */
    n[1] = m[1];
    n[2] = m[3];
    /* find first evec */
    n[0] = m[0] - eval[0];
    n[3] = m[4] - eval[0];
    ell_2m_1d_nullspace_d(evec + 3 * 0, n);
    (evec + 3 * 0)[2] = 0;
    /* find second evec */
    n[0] = m[0] - eval[1];
    n[3] = m[4] - eval[1];
    ell_2m_1d_nullspace_d(evec + 3 * 1, n);
    (evec + 3 * 1)[2] = 0;
    _ell_22v_enforce_orthogonality(evec + 3 * 0, evec + 3 * 1);
    /* make right-handed */
    ELL_3V_CROSS(evec + 3 * 2, evec + 3 * 0, evec + 3 * 1);
  } else if (ell_cubic_root_single_double == roots) {
    /* can pick any 2D basis */
    ELL_3V_SET(evec + 3 * 0, 1, 0, 0);
    ELL_3V_SET(evec + 3 * 1, 0, 1, 0);
    ELL_3V_SET(evec + 3 * 2, 0, 0, 1);
  } else {
    /* ell_cubic_root_single == roots, if assumptions are met */
    ELL_3V_SET(evec + 3 * 0, AIR_NAN, AIR_NAN, 0);
    ELL_3V_SET(evec + 3 * 1, AIR_NAN, AIR_NAN, 0);
    ELL_3V_SET(evec + 3 * 2, 0, 0, 1);
  }
  if (!ELL_3M_EXISTS(evec)) {
    fprintf(stderr, "%s: given m = \n", me);
    ell_3m_print_d(stderr, m);
    fprintf(stderr, "%s: got roots = %s (%d) and evecs = \n", me,
            airEnumStr(ell_cubic_root, roots), roots);
    ell_3m_print_d(stderr, evec);
  }
  return;
}

int /* Biff: nope */
ell_3m2sub_eigensolve_d(double eval[3], double evec[9], const double m[9]) {
  int roots;

  roots = ell_3m2sub_eigenvalues_d(eval, m);
  _ell_3m2sub_evecs_d(evec, eval, roots, m);

  return roots;
}

/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ 3m2sub ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

/*
******** ell_3m_svd_d
**
** singular value decomposition:
** mat = uu * diag(sval) * vv
**
** singular values are square roots of eigenvalues of mat * mat^T
** columns of uu are eigenvectors of mat * mat^T
** rows of vv are eigenvectors of mat^T * mat
**
** returns info about singular values according to ellCubeRoot enum
**
** HEY: I think this does the wrong thing when given a symmetric
** matrix with negative eigenvalues . . .
*/
int /* Biff: nope */
ell_3m_svd_d(double uu[9], double sval[3], double vv[9], const double mat[9],
             const int newton) {
  double trn[9], msqr[9], eval[3], evec[9];
  int roots;

  ELL_3M_TRANSPOSE(trn, mat);
  ELL_3M_MUL(msqr, mat, trn);
  roots = ell_3m_eigensolve_d(eval, evec, msqr, newton);
  sval[0] = sqrt(eval[0]);
  sval[1] = sqrt(eval[1]);
  sval[2] = sqrt(eval[2]);
  ELL_3M_TRANSPOSE(uu, evec);
  ELL_3M_MUL(msqr, trn, mat);
  _ell_3m_evecs_d(vv, eval, roots, msqr);

  return roots;
}

/*
** NOTE: profiling showed that about a quarter of the execution time of
** ell_6ms_eigensolve_d() is spent here; so reconsider its need and
** implementation . . . (fabs vs. AIR_ABS() made no difference)
*/
static void
_maxI_sum_find(unsigned int maxI[2], double *sumon, double *sumoff, double mat[6][6]) {
  double maxm, tmp;
  unsigned int rrI, ccI;

  /* we hope that all these loops are unrolled by the optimizer */
  *sumon = *sumoff = 0.0;
  for (rrI = 0; rrI < 6; rrI++) {
    *sumon += AIR_ABS(mat[rrI][rrI]);
  }
  maxm = -1;
  maxI[0] = maxI[1] = 0;
  for (rrI = 0; rrI < 5; rrI++) {
    for (ccI = rrI + 1; ccI < 6; ccI++) {
      tmp = AIR_ABS(mat[rrI][ccI]);
      *sumoff += tmp;
      if (tmp > maxm) {
        maxm = tmp;
        maxI[0] = rrI;
        maxI[1] = ccI;
      }
    }
  }

  /*
  if (1) {
    double nrm, trc;
    nrm = trc = 0;
    for (rrI=0; rrI<6; rrI++) {
      trc += mat[rrI][rrI];
      nrm += mat[rrI][rrI]*mat[rrI][rrI];
    }
    for (rrI=0; rrI<5; rrI++) {
      for (ccI=rrI+1; ccI<6; ccI++) {
        nrm += 2*mat[rrI][ccI]*mat[rrI][ccI];
      }
    }
    fprintf(stderr, "---------------- invars = %g %g\n", trc, nrm);
  }
  */

  return;
}

static int
_compar(const void *A_void, const void *B_void) {
  const double *A, *B;
  A = AIR_CAST(const double *, A_void);
  B = AIR_CAST(const double *, B_void);
  return (A[0] < B[0] ? 1 : (A[0] > B[0] ? -1 : 0));
}

/*
******* ell_6ms_eigensolve_d
**
** uses Jacobi iterations to find eigensystem of 6x6 symmetric matrix,
** given in sym[21], to within convergence threshold eps.  Puts
** eigenvalues, in descending order, in eval[6], and corresponding
** eigenvectors in _evec+0, _evec+6, . . ., _evec+30.  NOTE: you can
** pass a NULL _evec if eigenvectors aren't needed.
*/
int /* Biff: nope */
ell_6ms_eigensolve_d(double eval[6], double _evec[36], const double sym[21],
                     const double eps) {
  /* static const char me[] = "ell_6ms_eigensolve_d"; */
  double mat[2][6][6], evec[2][6][6], sumon, sumoff, evtmp[12];
  unsigned int cur, rrI, ccI, maxI[2] /*, iter */;

  if (!(eval && sym && eps >= 0)) {
    return 1;
  }
  /* copy symmetric matrix sym[] into upper tris of mat[0][][] & mat[1][][] */
  mat[0][0][0] = sym[0];
  mat[0][0][1] = sym[1];
  mat[0][0][2] = sym[2];
  mat[0][0][3] = sym[3];
  mat[0][0][4] = sym[4];
  mat[0][0][5] = sym[5];
  mat[0][1][1] = sym[6];
  mat[0][1][2] = sym[7];
  mat[0][1][3] = sym[8];
  mat[0][1][4] = sym[9];
  mat[0][1][5] = sym[10];
  mat[0][2][2] = sym[11];
  mat[0][2][3] = sym[12];
  mat[0][2][4] = sym[13];
  mat[0][2][5] = sym[14];
  mat[0][3][3] = sym[15];
  mat[0][3][4] = sym[16];
  mat[0][3][5] = sym[17];
  mat[0][4][4] = sym[18];
  mat[0][4][5] = sym[19];
  mat[0][5][5] = sym[20];
  if (_evec) {
    /* initialize evec[0]; */
    for (rrI = 0; rrI < 6; rrI++) {
      for (ccI = 0; ccI < 6; ccI++) {
        evec[0][ccI][rrI] = (rrI == ccI);
      }
    }
  }
  /*
  fprintf(stderr, "!%s(INIT): m = [", me);
  for (rrI=0; rrI<6; rrI++) {
    for (ccI=0; ccI<6; ccI++) {
      fprintf(stderr, "%f%s",
              (rrI <= ccI ? mat[0][rrI][ccI] : mat[0][ccI][rrI]),
              ccI<5 ? "," : (rrI<5 ? ";" : "]"));
    }
    fprintf(stderr, "\n");
  }
  */
  maxI[0] = maxI[1] = UINT_MAX; /* quiet warnings about using maxI unset */
  _maxI_sum_find(maxI, &sumon, &sumoff, mat[0]);
  cur = 1; /* fake out anticipating first line of loop */
  /* iter = 0; */
  while (sumoff / sumon > eps) {
    double th, tt, cc, ss;
    const unsigned int P = maxI[0];
    const unsigned int Q = maxI[1];
    /* make sure that P and Q are within the bounds for mat[2][6][6] */
    if (P >= 6 || Q >= 6) {
      break;
    }
    /*
    fprintf(stderr, "!%s(%u): sumoff/sumon = %g/%g = %g > %g\n", me, iter,
            sumoff, sumon, sumoff/sumon, eps);
    */
    cur = 1 - cur;

    th = (mat[cur][Q][Q] - mat[cur][P][P]) / (2 * mat[cur][P][Q]);
    tt = (th > 0 ? +1 : -1) / (AIR_ABS(th) + sqrt(th * th + 1));
    cc = 1 / sqrt(tt * tt + 1);
    ss = cc * tt;
    /*
    fprintf(stderr, "!%s(%u): maxI = (P,Q) = (%u,%u) --> ss=%f, cc=%f\n",
            me, iter, P, Q, ss, cc);
    fprintf(stderr, "     r = [");
    for (rrI=0; rrI<6; rrI++) {
      for (ccI=0; ccI<6; ccI++) {
        fprintf(stderr, "%g%s",
                (rrI == ccI
                 ? (rrI == P || rrI == Q ? cc : 1.0)
                 : (rrI == P && ccI == Q
                    ? ss
                    : (rrI == Q && ccI == P
                       ? -ss
                       : 0))),
                ccI<5 ? "," : (rrI<5 ? ";" : "]"));
      }
      fprintf(stderr, "\n");
    }
    */
    /* initialize by copying whole matrix */
    for (rrI = 0; rrI < 6; rrI++) {
      for (ccI = rrI; ccI < 6; ccI++) {
        mat[1 - cur][rrI][ccI] = mat[cur][rrI][ccI];
      }
    }
    /* perform Jacobi rotation */
    for (rrI = 0; rrI < P; rrI++) {
      mat[1 - cur][rrI][P] = cc * mat[cur][rrI][P] - ss * mat[cur][rrI][Q];
    }
    for (ccI = P + 1; ccI < 6; ccI++) {
      mat[1 - cur][P][ccI] = cc * mat[cur][P][ccI]
                           - ss * (Q <= ccI ? mat[cur][Q][ccI] : mat[cur][ccI][Q]);
    }
    for (rrI = 0; rrI < Q; rrI++) {
      mat[1 - cur][rrI][Q] = ss * (rrI <= P ? mat[cur][rrI][P] : mat[cur][P][rrI])
                           + cc * mat[cur][rrI][Q];
    }
    for (ccI = Q + 1; ccI < 6; ccI++) {
      mat[1 - cur][Q][ccI] = ss * mat[cur][P][ccI] + cc * mat[cur][Q][ccI];
    }
    /* set special entries */
    mat[1 - cur][P][P] = mat[cur][P][P] - tt * mat[cur][P][Q];
    mat[1 - cur][Q][Q] = mat[cur][Q][Q] + tt * mat[cur][P][Q];
    mat[1 - cur][P][Q] = 0.0;
    if (_evec) {
      /* NOTE: the eigenvectors use transpose of indexing of mat */
      /* start by copying all */
      for (rrI = 0; rrI < 6; rrI++) {
        for (ccI = 0; ccI < 6; ccI++) {
          evec[1 - cur][ccI][rrI] = evec[cur][ccI][rrI];
        }
      }
      for (rrI = 0; rrI < 6; rrI++) {
        evec[1 - cur][P][rrI] = cc * evec[cur][P][rrI] - ss * evec[cur][Q][rrI];
        evec[1 - cur][Q][rrI] = ss * evec[cur][P][rrI] + cc * evec[cur][Q][rrI];
      }
    }

    _maxI_sum_find(maxI, &sumon, &sumoff, mat[1 - cur]);

    /*
    fprintf(stderr, "!%s(%u): m = [", me, iter);
    for (rrI=0; rrI<6; rrI++) {
      for (ccI=0; ccI<6; ccI++) {
        fprintf(stderr, "%f%s",
                (rrI <= ccI ? mat[1-cur][rrI][ccI] : mat[1-cur][ccI][rrI]),
                ccI<5 ? "," : (rrI<5 ? ";" : "]"));
      }
      fprintf(stderr, "\n");
    }
    */
    /* iter++; */
  }
  /* 1-cur is index of final solution */

  /* sort evals */
  for (ccI = 0; ccI < 6; ccI++) {
    evtmp[0 + 2 * ccI] = mat[1 - cur][ccI][ccI];
    evtmp[1 + 2 * ccI] = ccI;
  }
  qsort(evtmp, 6, 2 * sizeof(double), _compar);

  /* copy out solution */
  for (ccI = 0; ccI < 6; ccI++) {
    eval[ccI] = evtmp[0 + 2 * ccI];
    if (_evec) {
      unsigned eeI;
      for (rrI = 0; rrI < 6; rrI++) {
        eeI = AIR_UINT(evtmp[1 + 2 * ccI]);
        _evec[rrI + 6 * ccI] = evec[1 - cur][eeI][rrI];
      }
    }
  }

  return 0;
}
