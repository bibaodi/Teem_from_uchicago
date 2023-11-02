/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2010, 2009  Thomas Schultz
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

#include "seek.h"
/* clang-format off */

/*
** vertex, edge, and face numbering, and canonical edge arrangement
**
**  Z
**  ^    Y
**  |   ^
**  |  /
**  | /
**  |/
**  O--------> X
**
**     (6)---11---(7)          +----------+           +----------+
**     /|         /|          /|         /|          /|      \  /|
**    9 |       10 |         / |  /5/   / |         / |       \/ |
**   /  6       /  7        /  |    |4|/  |        / \|       /\ |
** (4)----8---(5)  |       +----------+   |       +----------+   |
**  |   |      |   |       ||2||      ||3||       |   |  \   |   |
**  |  (2)---3-|--(3)      |   +------|---+       |   +----\-|---+
**  4  /       5  /        |  / |1|   |  /        |\ /       |\ /
**  | 1        | 2         | /   /0/  | /         | /\       | /
**  |/         |/          |/         |/          |/  \      |/
** (0)----0---(1)          +----------+           +----------+
**                                               canonical edge arrangement
**                                               creates 2 triangular and
**                                               1 hexagonal surface
*/

/* According to this layout, in the code comments, "right" denotes
 * positive x, "back" denotes positive y, "top" denotes positive z */

/*
** the seekContext's vidx cache uses this numbering
**      (.)--------(.)
**      /|         /|
**     4 |        / |
**    /  |       /  |
**  (.)----3---(.)  |
**   |   |      |   |
**   |  (.)-----|--(.)
**   2  /       |  /
**   | 1        | /
**   |/         |/
**  (X)----0---(.)
*/

/* We now only need a numbering of faces. It is:
 * 0: xy plane, z=0
 * 1: xz plane, y=0
 * 2: yz plane, x=1
 * 3: xz plane, y=1
 * 4: yz plane, x=0
 * 5: xy plane, z=1
 *
 * There are four unique faces to each voxel (e.g., in facevidx):
 * 0: xy plane, z=0
 * 1: xz plane, y=0
 * 2: yz plane, x=0
 * 3: xy plane, z=1
 */

const int
seekContour3DTopoHackEdge[256] = {
  0x000, 0x013, 0x025, 0x036, 0x04A, 0x059, 0x06F, 0x07C,
  0x08C, 0x09F, 0x0A9, 0x0BA, 0x0C6, 0x0D5, 0x0E3, 0x0F0,
  0x310, 0x303, 0x335, 0x326, 0x35A, 0x349, 0x37F, 0x36C,
  0x39C, 0x38F, 0x3B9, 0x3AA, 0x3D6, 0x3C5, 0x3F3, 0x3E0,
  0x520, 0x533, 0x505, 0x516, 0x56A, 0x579, 0x54F, 0x55C,
  0x5AC, 0x5BF, 0x589, 0x59A, 0x5E6, 0x5F5, 0x5C3, 0x5D0,
  0x630, 0x623, 0x615, 0x606, 0x67A, 0x669, 0x65F, 0x64C,
  0x6BC, 0x6AF, 0x699, 0x68A, 0x6F6, 0x6E5, 0x6D3, 0x6C0,
  0xA40, 0xA53, 0xA65, 0xA76, 0xA0A, 0xA19, 0xA2F, 0xA3C,
  0xACC, 0xADF, 0xAE9, 0xAFA, 0xA86, 0xA95, 0xAA3, 0xAB0,
  0x950, 0x943, 0x975, 0x966, 0x91A, 0x909, 0x93F, 0x92C,
  0x9DC, 0x9CF, 0x9F9, 0x9EA, 0x996, 0x985, 0x9B3, 0x9A0,
  0xF60, 0xF73, 0xF45, 0xF56, 0xF2A, 0xF39, 0xF0F, 0xF1C,
  0xFEC, 0xFFF, 0xFC9, 0xFDA, 0xFA6, 0xFB5, 0xF83, 0xF90,
  0xC70, 0xC63, 0xC55, 0xC46, 0xC3A, 0xC29, 0xC1F, 0xC0C,
  0xCFC, 0xCEF, 0xCD9, 0xCCA, 0xCB6, 0xCA5, 0xC93, 0xC80,
  0xC80, 0xC93, 0xCA5, 0xCB6, 0xCCA, 0xCD9, 0xCEF, 0xCFC,
  0xC0C, 0xC1F, 0xC29, 0xC3A, 0xC46, 0xC55, 0xC63, 0xC70,
  0xF90, 0xF83, 0xFB5, 0xFA6, 0xFDA, 0xFC9, 0xFFF, 0xFEC,
  0xF1C, 0xF0F, 0xF39, 0xF2A, 0xF56, 0xF45, 0xF73, 0xF60,
  0x9A0, 0x9B3, 0x985, 0x996, 0x9EA, 0x9F9, 0x9CF, 0x9DC,
  0x92C, 0x93F, 0x909, 0x91A, 0x966, 0x975, 0x943, 0x950,
  0xAB0, 0xAA3, 0xA95, 0xA86, 0xAFA, 0xAE9, 0xADF, 0xACC,
  0xA3C, 0xA2F, 0xA19, 0xA0A, 0xA76, 0xA65, 0xA53, 0xA40,
  0x6C0, 0x6D3, 0x6E5, 0x6F6, 0x68A, 0x699, 0x6AF, 0x6BC,
  0x64C, 0x65F, 0x669, 0x67A, 0x606, 0x615, 0x623, 0x630,
  0x5D0, 0x5C3, 0x5F5, 0x5E6, 0x59A, 0x589, 0x5BF, 0x5AC,
  0x55C, 0x54F, 0x579, 0x56A, 0x516, 0x505, 0x533, 0x520,
  0x3E0, 0x3F3, 0x3C5, 0x3D6, 0x3AA, 0x3B9, 0x38F, 0x39C,
  0x36C, 0x37F, 0x349, 0x35A, 0x326, 0x335, 0x303, 0x310,
  0x0F0, 0x0E3, 0x0D5, 0x0C6, 0x0BA, 0x0A9, 0x09F, 0x08C,
  0x07C, 0x06F, 0x059, 0x04A, 0x036, 0x025, 0x013, 0x000
};

/*
  This case table implements the ideas from

  Dietrich et al. Edge Groups: an approach to understanding the mesh
  quality of marching methods. IEEE Trans. Vis. Comp. Graph. 2008

  and has been generated from the case table distributed with macet,
  written by Carlos Dietrich.
  Re-used with kind permission of the authors.
*/
const int
seekContour3DTopoHackTriangle[256][16] = {
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 5, 2, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 5, 2, 4, 4, 2, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 1, 3, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 4, 0, 6, 6, 0, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 3, 6, 1, 5, 2, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 6, 4, 3, 4, 2, 3, 4, 5, 2,-1,-1,-1,-1,-1,-1,-1},
  { 2, 7, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 1, 4, 0, 7, 3, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 5, 3, 3, 5, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 4, 5, 1, 5, 3, 1, 5, 7, 3,-1,-1,-1,-1,-1,-1,-1},
  { 6, 1, 7, 7, 1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 7, 6, 2, 6, 0, 2, 6, 4, 0,-1,-1,-1,-1,-1,-1,-1},
  { 5, 7, 0, 7, 1, 0, 7, 6, 1,-1,-1,-1,-1,-1,-1,-1},
  { 5, 6, 4, 5, 7, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 9, 8, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 8, 8, 1, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 4, 9, 8, 2, 0, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 2, 1, 5, 1, 8, 5, 1, 9, 8,-1,-1,-1,-1,-1,-1,-1},
  { 9, 8, 4, 3, 6, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 8, 0, 9, 0, 6, 9, 0, 3, 6,-1,-1,-1,-1,-1,-1,-1},
  { 8, 4, 9, 3, 6, 1, 2, 0, 5,-1,-1,-1,-1,-1,-1,-1},
  { 2, 3, 6, 2, 6, 9, 5, 2, 9, 8, 5, 9,-1,-1,-1,-1},
  { 9, 8, 4, 7, 3, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 9, 8, 1, 8, 0, 1, 7, 3, 2,-1,-1,-1,-1,-1,-1,-1},
  { 7, 3, 5, 3, 0, 5, 9, 8, 4,-1,-1,-1,-1,-1,-1,-1},
  { 5, 7, 8, 8, 7, 3, 1, 9, 8, 1, 8, 3,-1,-1,-1,-1},
  { 2, 7, 1, 7, 6, 1, 8, 4, 9,-1,-1,-1,-1,-1,-1,-1},
  { 7, 6, 9, 9, 0, 2, 8, 0, 9, 7, 9, 2,-1,-1,-1,-1},
  { 4, 9, 8, 7, 6, 1, 0, 7, 1, 5, 7, 0,-1,-1,-1,-1},
  { 7, 6, 9, 8, 7, 9, 5, 7, 8,-1,-1,-1,-1,-1,-1,-1},
  {10, 5, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 4,10, 5, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 8,10, 0, 0,10, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {10, 2, 8, 2, 4, 8, 2, 1, 4,-1,-1,-1,-1,-1,-1,-1},
  {10, 5, 8, 3, 6, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 3, 6, 0, 6, 4, 0,10, 5, 8,-1,-1,-1,-1,-1,-1,-1},
  { 8,10, 0,10, 2, 0, 6, 1, 3,-1,-1,-1,-1,-1,-1,-1},
  {10, 2, 8, 8, 2, 3, 6, 4, 8, 6, 8, 3,-1,-1,-1,-1},
  { 5, 8,10, 3, 2, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {10, 5, 8, 1, 4, 0, 3, 2, 7,-1,-1,-1,-1,-1,-1,-1},
  { 3, 0, 7, 0,10, 7, 0, 8,10,-1,-1,-1,-1,-1,-1,-1},
  { 7, 3,10,10, 3, 8, 3, 1, 8, 1, 4, 8,-1,-1,-1,-1},
  { 6, 1, 7, 1, 2, 7, 8,10, 5,-1,-1,-1,-1,-1,-1,-1},
  {10, 5, 8, 6, 4, 0, 2, 6, 0, 7, 6, 2,-1,-1,-1,-1},
  { 0, 8, 1, 1, 8,10, 7, 6, 1, 7, 1,10,-1,-1,-1,-1},
  { 6, 4, 8,10, 6, 8, 7, 6,10,-1,-1,-1,-1,-1,-1,-1},
  {10, 5, 9, 9, 5, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 1, 9, 0, 9, 5, 0, 9,10, 5,-1,-1,-1,-1,-1,-1,-1},
  { 9,10, 4,10, 0, 4,10, 2, 0,-1,-1,-1,-1,-1,-1,-1},
  {10, 1, 9, 2, 1,10,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {10, 5, 9, 5, 4, 9, 3, 6, 1,-1,-1,-1,-1,-1,-1,-1},
  { 9,10, 5, 5, 3, 6, 0, 3, 5, 9, 5, 6,-1,-1,-1,-1},
  { 1, 3, 6,10, 2, 0, 4,10, 0, 9,10, 4,-1,-1,-1,-1},
  {10, 2, 3, 6,10, 3, 9,10, 6,-1,-1,-1,-1,-1,-1,-1},
  { 4, 9, 5, 9,10, 5, 3, 2, 7,-1,-1,-1,-1,-1,-1,-1},
  { 3, 2, 7, 9,10, 5, 0, 9, 5, 1, 9, 0,-1,-1,-1,-1},
  { 3, 0, 4, 4,10, 7, 9,10, 4, 3, 4, 7,-1,-1,-1,-1},
  { 9,10, 7, 3, 9, 7, 1, 9, 3,-1,-1,-1,-1,-1,-1,-1},
  { 1, 2, 6, 2, 7, 6, 9, 5, 4, 9,10, 5,-1,-1,-1,-1},
  { 9,10, 5, 0, 9, 5, 0, 6, 9, 2, 6, 0, 7, 6, 2,-1},
  { 7, 6, 1, 0, 7, 1, 0,10, 7, 4,10, 0, 9,10, 4,-1},
  { 6, 9,10, 6,10, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 6,11, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 6,11, 9, 0, 1, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 6,11, 9, 2, 0, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 5, 2, 4, 2, 1, 4,11, 9, 6,-1,-1,-1,-1,-1,-1,-1},
  { 1, 3, 9, 9, 3,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 3, 4, 3, 9, 4, 3,11, 9,-1,-1,-1,-1,-1,-1,-1},
  {11, 9, 3, 9, 1, 3, 5, 2, 0,-1,-1,-1,-1,-1,-1,-1},
  { 3,11, 9, 9, 5, 2, 4, 5, 9, 3, 9, 2,-1,-1,-1,-1},
  {11, 9, 6, 2, 7, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 4,11, 9, 6, 7, 3, 2,-1,-1,-1,-1,-1,-1,-1},
  { 0, 5, 3, 5, 7, 3, 9, 6,11,-1,-1,-1,-1,-1,-1,-1},
  { 6,11, 9, 5, 7, 3, 1, 5, 3, 4, 5, 1,-1,-1,-1,-1},
  { 9, 1,11, 1, 7,11, 1, 2, 7,-1,-1,-1,-1,-1,-1,-1},
  { 2, 7, 0, 0, 7, 4, 7,11, 4,11, 9, 4,-1,-1,-1,-1},
  { 9, 1,11,11, 1, 0, 5, 7,11, 5,11, 0,-1,-1,-1,-1},
  { 5, 7,11, 9, 5,11, 4, 5, 9,-1,-1,-1,-1,-1,-1,-1},
  { 6,11, 4, 4,11, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {11, 8, 6, 8, 1, 6, 8, 0, 1,-1,-1,-1,-1,-1,-1,-1},
  { 6,11, 4,11, 8, 4, 2, 0, 5,-1,-1,-1,-1,-1,-1,-1},
  {11, 8, 5, 5, 1, 6, 2, 1, 5,11, 5, 6,-1,-1,-1,-1},
  { 3,11, 1,11, 4, 1,11, 8, 4,-1,-1,-1,-1,-1,-1,-1},
  { 0,11, 8, 3,11, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 2, 0, 5,11, 8, 4, 1,11, 4, 3,11, 1,-1,-1,-1,-1},
  {11, 8, 5, 2,11, 5, 3,11, 2,-1,-1,-1,-1,-1,-1,-1},
  { 8, 4,11, 4, 6,11, 2, 7, 3,-1,-1,-1,-1,-1,-1,-1},
  { 7, 3, 2, 8, 0, 1, 6, 8, 1,11, 8, 6,-1,-1,-1,-1},
  { 5, 7, 0, 7, 3, 0, 4,11, 8, 4, 6,11,-1,-1,-1,-1},
  { 3, 5, 7, 1, 5, 3, 1, 8, 5, 6, 8, 1,11, 8, 6,-1},
  { 1, 2, 4, 4, 2, 7,11, 8, 4,11, 4, 7,-1,-1,-1,-1},
  { 8, 0, 2, 7, 8, 2,11, 8, 7,-1,-1,-1,-1,-1,-1,-1},
  {11, 8, 4, 1,11, 4, 1, 7,11, 0, 7, 1, 5, 7, 0,-1},
  { 7,11, 8, 7, 8, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {10, 5, 8, 6,11, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 1, 4, 0,10, 5, 8,11, 9, 6,-1,-1,-1,-1,-1,-1,-1},
  { 2, 0,10, 0, 8,10, 6,11, 9,-1,-1,-1,-1,-1,-1,-1},
  {11, 9, 6, 2, 1, 4, 8, 2, 4,10, 2, 8,-1,-1,-1,-1},
  { 1, 3, 9, 3,11, 9, 5, 8,10,-1,-1,-1,-1,-1,-1,-1},
  { 5, 8,10, 3,11, 9, 4, 3, 9, 0, 3, 4,-1,-1,-1,-1},
  { 1,11, 9, 3,11, 1, 0, 8,10, 2, 0,10,-1,-1,-1,-1},
  { 3,11, 9, 4, 3, 9, 4, 2, 3, 8, 2, 4,10, 2, 8,-1},
  { 6,11, 9, 5, 8,10, 2, 7, 3,-1,-1,-1,-1,-1,-1,-1},
  { 3, 2, 7,10, 5, 8, 0, 1, 4,11, 9, 6,-1,-1,-1,-1},
  { 6,11, 9, 0, 8,10, 7, 0,10, 3, 0, 7,-1,-1,-1,-1},
  {10, 7, 8, 7, 3, 8, 3, 4, 8, 4, 3, 1, 6,11, 9,-1},
  {10, 5, 8, 1, 2, 7,11, 1, 7, 9, 1,11,-1,-1,-1,-1},
  { 0, 2, 4, 2, 7, 4, 7, 9, 4, 9, 7,11,10, 5, 8,-1},
  {10, 0, 8, 7, 0,10, 7, 1, 0,11, 1, 7, 9, 1,11,-1},
  { 4, 8,10, 7, 4,10, 4,11, 9, 4, 7,11,-1,-1,-1,-1},
  { 5, 4,10, 4,11,10, 4, 6,11,-1,-1,-1,-1,-1,-1,-1},
  {11,10, 5,11, 5, 0, 6,11, 0, 1, 6, 0,-1,-1,-1,-1},
  { 4, 6,11,11, 2, 0,10, 2,11, 4,11, 0,-1,-1,-1,-1},
  { 2, 1, 6,11, 2, 6,10, 2,11,-1,-1,-1,-1,-1,-1,-1},
  { 5, 4, 1, 1,11,10, 3,11, 1, 5, 1,10,-1,-1,-1,-1},
  { 3,11,10, 5, 3,10, 0, 3, 5,-1,-1,-1,-1,-1,-1,-1},
  {10, 2, 0, 4,10, 0, 4,11,10, 1,11, 4, 3,11, 1,-1},
  {11,10, 2,11, 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 2, 7, 3, 4, 6,11,10, 4,11, 5, 4,10,-1,-1,-1,-1},
  { 1, 6, 0, 6,11, 0,11, 5, 0, 5,11,10, 7, 3, 2,-1},
  {11, 4, 6,10, 4,11,10, 0, 4, 7, 0,10, 3, 0, 7,-1},
  {10, 7, 3, 1,10, 3,10, 6,11,10, 1, 6,-1,-1,-1,-1},
  { 1, 2, 7,11, 1, 7,11, 4, 1,10, 4,11, 5, 4,10,-1},
  { 0, 2, 7,11, 0, 7, 0,10, 5, 0,11,10,-1,-1,-1,-1},
  { 1, 0, 4,10, 7,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 7,11,10,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {11, 7,10,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 4, 7,10,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 2, 0, 5,11, 7,10,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 1, 4, 2, 4, 5, 2,11, 7,10,-1,-1,-1,-1,-1,-1,-1},
  { 7,10,11, 1, 3, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 4, 0, 6, 0, 3, 6,10,11, 7,-1,-1,-1,-1,-1,-1,-1},
  { 5, 2, 0, 6, 1, 3,11, 7,10,-1,-1,-1,-1,-1,-1,-1},
  { 7,10,11, 4, 5, 2, 3, 4, 2, 6, 4, 3,-1,-1,-1,-1},
  {10,11, 2, 2,11, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {10,11, 2,11, 3, 2, 4, 0, 1,-1,-1,-1,-1,-1,-1,-1},
  {11, 3,10, 3, 5,10, 3, 0, 5,-1,-1,-1,-1,-1,-1,-1},
  { 4, 5, 1, 1, 5,10,11, 3, 1,11, 1,10,-1,-1,-1,-1},
  { 1, 2, 6, 2,11, 6, 2,10,11,-1,-1,-1,-1,-1,-1,-1},
  { 6, 4,11,11, 4, 0, 2,10,11, 2,11, 0,-1,-1,-1,-1},
  {10,11, 5, 5,11, 0,11, 6, 0, 6, 1, 0,-1,-1,-1,-1},
  { 4, 5,10,11, 4,10, 6, 4,11,-1,-1,-1,-1,-1,-1,-1},
  { 8, 4, 9, 7,10,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 8, 1, 9, 8, 7,10,11,-1,-1,-1,-1,-1,-1,-1},
  { 9, 8, 4, 2, 0, 5, 7,10,11,-1,-1,-1,-1,-1,-1,-1},
  { 7,10,11, 1, 9, 8, 5, 1, 8, 2, 1, 5,-1,-1,-1,-1},
  { 4, 9, 8, 7,10,11, 3, 6, 1,-1,-1,-1,-1,-1,-1,-1},
  {11, 7,10, 0, 3, 6, 9, 0, 6, 8, 0, 9,-1,-1,-1,-1},
  { 9, 8, 4, 1, 3, 6, 2, 0, 5, 7,10,11,-1,-1,-1,-1},
  { 8, 5, 9, 5, 2, 9, 2, 6, 9, 6, 2, 3, 7,10,11,-1},
  { 3, 2,11, 2,10,11, 4, 9, 8,-1,-1,-1,-1,-1,-1,-1},
  {11, 3,10, 3, 2,10, 8, 1, 9, 8, 0, 1,-1,-1,-1,-1},
  { 8, 4, 9, 3, 0, 5,10, 3, 5,11, 3,10,-1,-1,-1,-1},
  { 1, 9, 8, 5, 1, 8, 5, 3, 1,10, 3, 5,11, 3,10,-1},
  { 9, 8, 4, 2,10,11, 6, 2,11, 1, 2, 6,-1,-1,-1,-1},
  { 2,10,11, 6, 2,11, 6, 0, 2, 9, 0, 6, 8, 0, 9,-1},
  { 5,10, 0,10,11, 0,11, 1, 0, 1,11, 6, 9, 8, 4,-1},
  { 5,10,11, 6, 5,11, 5, 9, 8, 5, 6, 9,-1,-1,-1,-1},
  {11, 7, 8, 8, 7, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {11, 7, 8, 7, 5, 8, 1, 4, 0,-1,-1,-1,-1,-1,-1,-1},
  { 0, 8, 2, 8, 7, 2, 8,11, 7,-1,-1,-1,-1,-1,-1,-1},
  { 2, 1, 4, 4,11, 7, 8,11, 4, 2, 4, 7,-1,-1,-1,-1},
  { 5, 8, 7, 8,11, 7, 1, 3, 6,-1,-1,-1,-1,-1,-1,-1},
  {11, 5, 8, 7, 5,11, 6, 4, 0, 3, 6, 0,-1,-1,-1,-1},
  { 3, 6, 1, 8,11, 7, 2, 8, 7, 0, 8, 2,-1,-1,-1,-1},
  { 8,11, 7, 2, 8, 7, 2, 4, 8, 3, 4, 2, 6, 4, 3,-1},
  { 8,11, 5,11, 2, 5,11, 3, 2,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 4,11, 3, 2, 5,11, 2, 8,11, 5,-1,-1,-1,-1},
  {11, 0, 8,11, 3, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {11, 3, 1, 4,11, 1, 8,11, 4,-1,-1,-1,-1,-1,-1,-1},
  { 8,11, 5, 5,11, 6, 1, 2, 5, 1, 5, 6,-1,-1,-1,-1},
  { 6, 4, 0, 2, 6, 0, 2,11, 6, 5,11, 2, 8,11, 5,-1},
  { 8,11, 6, 1, 8, 6, 0, 8, 1,-1,-1,-1,-1,-1,-1,-1},
  {11, 6, 4,11, 4, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 7, 5,11, 5, 9,11, 5, 4, 9,-1,-1,-1,-1,-1,-1,-1},
  { 1, 9,11,11, 5, 0, 7, 5,11, 1,11, 0,-1,-1,-1,-1},
  { 7, 2, 0, 7, 0, 4,11, 7, 4, 9,11, 4,-1,-1,-1,-1},
  { 1, 9,11, 7, 1,11, 2, 1, 7,-1,-1,-1,-1,-1,-1,-1},
  { 3, 6, 1, 5, 4, 9,11, 5, 9, 7, 5,11,-1,-1,-1,-1},
  { 0, 3, 6, 9, 0, 6, 9, 5, 0,11, 5, 9, 7, 5,11,-1},
  { 9,11, 4,11, 7, 4, 7, 0, 4, 0, 7, 2, 3, 6, 1,-1},
  { 9,11, 7, 2, 9, 7, 9, 3, 6, 9, 2, 3,-1,-1,-1,-1},
  {11, 3, 9, 9, 3, 2, 5, 4, 9, 5, 9, 2,-1,-1,-1,-1},
  {11, 3, 2, 5,11, 2, 5, 9,11, 0, 9, 5, 1, 9, 0,-1},
  { 3, 0, 4, 9, 3, 4,11, 3, 9,-1,-1,-1,-1,-1,-1,-1},
  { 3, 1, 9, 3, 9,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 5, 4, 9,11, 5, 9,11, 2, 5, 6, 2,11, 1, 2, 6,-1},
  {11, 6, 9, 0, 2, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {11, 6, 1, 0,11, 1,11, 4, 9,11, 0, 4,-1,-1,-1,-1},
  {11, 6, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 9, 6,10,10, 6, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 7,10, 6,10, 9, 6, 0, 1, 4,-1,-1,-1,-1,-1,-1,-1},
  { 9, 6,10, 6, 7,10, 0, 5, 2,-1,-1,-1,-1,-1,-1,-1},
  { 5, 1, 4, 2, 1, 5,10, 9, 6, 7,10, 6,-1,-1,-1,-1},
  {10, 9, 7, 9, 3, 7, 9, 1, 3,-1,-1,-1,-1,-1,-1,-1},
  { 0, 3, 4, 4, 3, 7,10, 9, 4,10, 4, 7,-1,-1,-1,-1},
  { 2, 0, 5, 9, 1, 3, 7, 9, 3,10, 9, 7,-1,-1,-1,-1},
  { 4, 5, 2, 3, 4, 2, 3, 9, 4, 7, 9, 3,10, 9, 7,-1},
  { 2,10, 3,10, 6, 3,10, 9, 6,-1,-1,-1,-1,-1,-1,-1},
  { 0, 1, 4,10, 9, 6, 3,10, 6, 2,10, 3,-1,-1,-1,-1},
  {10, 9, 5, 6, 3, 5, 3, 0, 5, 5, 9, 6,-1,-1,-1,-1},
  {10, 9, 6, 3,10, 6, 3, 5,10, 1, 5, 3, 4, 5, 1,-1},
  { 1,10, 9, 1, 2,10,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {10, 9, 4, 0,10, 4, 2,10, 0,-1,-1,-1,-1,-1,-1,-1},
  { 9, 1, 0, 5, 9, 0,10, 9, 5,-1,-1,-1,-1,-1,-1,-1},
  { 5,10, 9, 5, 9, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 4, 6, 8, 6,10, 8, 6, 7,10,-1,-1,-1,-1,-1,-1,-1},
  { 8, 0, 1, 1, 7,10, 6, 7, 1, 8, 1,10,-1,-1,-1,-1},
  { 5, 2, 0, 6, 7,10, 8, 6,10, 4, 6, 8,-1,-1,-1,-1},
  { 6, 7,10, 8, 6,10, 8, 1, 6, 5, 1, 8, 2, 1, 5,-1},
  { 3, 7,10, 3,10, 8, 1, 3, 8, 4, 1, 8,-1,-1,-1,-1},
  { 0, 3, 7,10, 0, 7, 8, 0,10,-1,-1,-1,-1,-1,-1,-1},
  { 4, 1, 8, 1, 3, 8, 3,10, 8,10, 3, 7, 2, 0, 5,-1},
  { 8, 5, 2, 3, 8, 2, 8, 7,10, 8, 3, 7,-1,-1,-1,-1},
  { 2,10, 8, 3, 2, 8, 6, 3, 8, 4, 6, 8,-1,-1,-1,-1},
  { 8, 0, 1, 6, 8, 1, 6,10, 8, 3,10, 6, 2,10, 3,-1},
  { 3, 0, 5,10, 3, 5,10, 6, 3, 8, 6,10, 4, 6, 8,-1},
  { 5,10, 8, 6, 3, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 2,10, 8, 4, 2, 8, 1, 2, 4,-1,-1,-1,-1,-1,-1,-1},
  {10, 8, 0,10, 0, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 1, 0, 5,10, 1, 5, 1, 8, 4, 1,10, 8,-1,-1,-1,-1},
  { 5,10, 8,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 6, 7, 9, 7, 8, 9, 7, 5, 8,-1,-1,-1,-1,-1,-1,-1},
  { 1, 4, 0, 7, 5, 8, 9, 7, 8, 6, 7, 9,-1,-1,-1,-1},
  { 6, 7, 9, 9, 7, 2, 0, 8, 9, 0, 9, 2,-1,-1,-1,-1},
  { 2, 1, 4, 8, 2, 4, 8, 7, 2, 9, 7, 8, 6, 7, 9,-1},
  { 7, 5, 8, 8, 1, 3, 9, 1, 8, 7, 8, 3,-1,-1,-1,-1},
  { 7, 5, 8, 9, 7, 8, 9, 3, 7, 4, 3, 9, 0, 3, 4,-1},
  { 9, 1, 3, 7, 9, 3, 7, 8, 9, 2, 8, 7, 0, 8, 2,-1},
  { 8, 9, 4, 3, 7, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 3, 2, 6, 6, 2, 9, 2, 5, 9, 5, 8, 9,-1,-1,-1,-1},
  { 6, 3, 9, 3, 2, 9, 2, 8, 9, 8, 2, 5, 0, 1, 4,-1},
  { 0, 8, 9, 6, 0, 9, 3, 0, 6,-1,-1,-1,-1,-1,-1,-1},
  { 8, 9, 6, 3, 8, 6, 8, 1, 4, 8, 3, 1,-1,-1,-1,-1},
  { 1, 2, 5, 8, 1, 5, 9, 1, 8,-1,-1,-1,-1,-1,-1,-1},
  { 9, 4, 0, 2, 9, 0, 9, 5, 8, 9, 2, 5,-1,-1,-1,-1},
  { 1, 0, 8, 1, 8, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 8, 9, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 6, 5, 4, 7, 5, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 7, 5, 0, 1, 7, 0, 6, 7, 1,-1,-1,-1,-1,-1,-1,-1},
  { 6, 7, 2, 0, 6, 2, 4, 6, 0,-1,-1,-1,-1,-1,-1,-1},
  { 1, 6, 7, 1, 7, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 5, 4, 1, 3, 5, 1, 7, 5, 3,-1,-1,-1,-1,-1,-1,-1},
  { 5, 0, 3, 5, 3, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 4, 1, 3, 7, 4, 3, 4, 2, 0, 4, 7, 2,-1,-1,-1,-1},
  { 7, 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 4, 6, 3, 2, 4, 3, 5, 4, 2,-1,-1,-1,-1,-1,-1,-1},
  { 6, 3, 2, 5, 6, 2, 6, 0, 1, 6, 5, 0,-1,-1,-1,-1},
  { 0, 4, 6, 0, 6, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 3, 1, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 2, 5, 4, 2, 4, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 2, 5, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  { 1, 0, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}
};
/* clang-format on */
