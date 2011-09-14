// -*- lsst-c++ -*-
/**
 * \file
 * \brief Capture the colour of an object
 */

#ifndef LSST_AFW_IMAGE_COLOR_H
#define LSST_AFW_IMAGE_COLOR_H

#include <cmath>
#include <limits>
#include "lsst/utils/ieee.h"
#include "lsst/afw/image/Filter.h"

namespace lsst {
namespace afw {
namespace image {

/**
 * Describe the colour of a source
 *
 * We need a concept of colour more general than "g - r" in order to calculate e.g. atmospheric dispersion
 * or a source's PSF
 *
 * \note This is very much just a place holder until we work out what we need.  A full SED may be required,
 * in which case a constructor from an SED name might be appropriate, or a couple of colours, or ...
 */
class Color {
public :
    explicit Color(double g_r=std::numeric_limits<double>::quiet_NaN()) : _g_r(g_r) {}

    operator bool() const {
        return lsst::utils::isnan(_g_r);
    }

    /** Return the effective wavelength for this object in the given filter
     */
    double getLambdaEff(Filter const&   ///< The filter in question
                       ) const { return 1000*_g_r; }
private :
    double _g_r;
};

}}}  // lsst::afw::image

#endif
