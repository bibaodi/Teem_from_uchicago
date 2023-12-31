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

#include "echo.h"

/* clang-format off */
/* ------------------------------- jitter --------------------------- */

static const char *
_echoJitterStr[ECHO_JITTER_NUM+1] = {
  "(unknown_jitter)",
  "none",
  "grid",
  "jitter",
  "random"
};

static const int
_echoJitterVal[ECHO_JITTER_NUM+1] = {
  echoJitterUnknown,
  echoJitterNone,
  echoJitterGrid,
  echoJitterJitter,
  echoJitterRandom
};

static const char *
_echoJitterDesc[ECHO_JITTER_NUM+1] = {
  "unknown jitter",
  "nothing- samples are ALWAYS at center of region",
  "no jittering- samples are at regular grid vertices",
  "normal jittering- samples are randomly located within grid cells",
  "samples are randomly located within region"
};

static const char *
_echoJitterStrEqv[] = {
  "none",
  "grid", "regular",
  "jitter",
  "random",
  ""
};

static const int
_echoJitterValEqv[] = {
  echoJitterNone,
  echoJitterGrid, echoJitterGrid,
  echoJitterJitter,
  echoJitterRandom
};

static const airEnum
_echoJitter = {
  "jitter",
  ECHO_JITTER_NUM,
  _echoJitterStr,  _echoJitterVal,
  _echoJitterDesc,
  _echoJitterStrEqv, _echoJitterValEqv,
  AIR_FALSE
};
const airEnum *const
echoJitter = &_echoJitter;

/* ------------------------------- object type --------------------------- */

static const char *
_echoTypeStr[ECHO_TYPE_NUM+1] = {
  "(unknown_object)",
  "sphere",
  "cylinder",
  "superquad",
  "cube",
  "triangle",
  "rectangle",
  "mesh",
  "isosurface",
  "AABoundingBox",
  "split",
  "list",
  "instance"
};

static const int
_echoTypeVal[ECHO_TYPE_NUM+1] = {
  echoTypeUnknown,
  echoTypeSphere,
  echoTypeCylinder,
  echoTypeSuperquad,
  echoTypeCube,
  echoTypeTriangle,
  echoTypeRectangle,
  echoTypeTriMesh,
  echoTypeIsosurface,
  echoTypeAABBox,
  echoTypeSplit,
  echoTypeList,
  echoTypeInstance
};

static const char *
_echoTypeDesc[ECHO_TYPE_NUM+1] = {
  "unknown_object",
  "sphere",
  "axis-aligned cylinder",
  "superquadric (actually, superellipsoid)",
  "unit cube, centered at the origin",
  "triangle",
  "rectangle",
  "mesh of triangles",
  "isosurface of scalar volume",
  "axis-aligned bounding box",
  "split",
  "list",
  "instance"
};

static const char *
_echoTypeStrEqv[] = {
  "sphere",
  "cylinder", "cylind", "rod",
  "superquad", "squad",
  "cube", "box",
  "triangle", "tri",
  "rectangle", "rect",
  "mesh", "tri-mesh", "trimesh",
  "isosurface",
  "aabbox", "AABoundingBox",
  "split",
  "list",
  "instance",
  ""
};

static const int
_echoTypeValEqv[] = {
  echoTypeSphere,
  echoTypeCylinder, echoTypeCylinder, echoTypeCylinder,
  echoTypeSuperquad, echoTypeSuperquad,
  echoTypeCube, echoTypeCube,
  echoTypeTriangle, echoTypeTriangle,
  echoTypeRectangle, echoTypeRectangle,
  echoTypeTriMesh, echoTypeTriMesh, echoTypeTriMesh,
  echoTypeIsosurface,
  echoTypeAABBox, echoTypeAABBox,
  echoTypeSplit,
  echoTypeList,
  echoTypeInstance
};

static const airEnum
_echoType = {
  "object type",
  ECHO_TYPE_NUM,
  _echoTypeStr,  _echoTypeVal,
  _echoTypeDesc,
  _echoTypeStrEqv, _echoTypeValEqv,
  AIR_FALSE
};
const airEnum *const
echoType = &_echoType;

/* ------------------------------ material types --------------------------- */

static const char *
_echoMatterStr[ECHO_MATTER_MAX+1] = {
  "(unknown_matter)",
  "phong",
  "glass",
  "metal",
  "light"
};

static const char *
_echoMatterDesc[ECHO_MATTER_MAX+1] = {
  "unknown material",
  "phong shaded surface",
  "glass",
  "metal",
  "light emitter"
};

static const airEnum
_echoMatter = {
  "matter",
  ECHO_MATTER_MAX,
  _echoMatterStr,  NULL,
  _echoMatterDesc,
  NULL, NULL,
  AIR_FALSE
};
const airEnum *const
echoMatter = &_echoMatter;
/* clang-format on */
