/**
 * \file
 * \brief An include file to include the public header files for lsst::afw::math
 *
 * Note: this header file intentionally excludes math/detail header files because they define
 * classes and functions which are not part of the public API.
 */
#ifndef LSST_AFW_MATH_H
#define LSST_AFW_MATH_H

#include "lsst/afw/math/Background.h"
#include "lsst/afw/math/Function.h"
#include "lsst/afw/math/FunctionLibrary.h"
#include "lsst/afw/math/Kernel.h"
#include "lsst/afw/math/KernelFunctions.h"
#include "lsst/afw/math/minimize.h"
#include "lsst/afw/math/warpExposure.h"
#include "lsst/afw/math/SpatialCell.h"
#include "lsst/afw/math/offsetImage.h"
#include "lsst/afw/math/Statistics.h"
#include "lsst/afw/math/Integrate.h"
#include "lsst/afw/math/Interpolate.h"
#include "lsst/afw/math/Random.h"

#endif // LSST_AFW_MATH_H
