// -*- LSST-C++ -*-
/**
 * @file
 *
 * @brief Definitions of members of lsst::afw::math
 *
 * @ingroup afw
 */
#include <iostream>

#include "boost/format.hpp"

#include "lsst/afw/math/KernelFunctions.h"

/**
 * @brief Print the pixel values of a Kernel to std::cout
 *
 * Rows increase upward and columns to the right; thus the lower left pixel is (0,0).
 *
 * @ingroup afw
 */
void
lsst::afw::math::printKernel(
    lsst::afw::math::Kernel const &kernel,     ///< the kernel
    bool doNormalize,                   ///< if true, normalize kernel
    double x,                           ///< x at which to evaluate kernel
    double y,                           ///< y at which to evaluate kernel
    std::string pixelFmt                ///< format for pixel values
) {
    typedef lsst::afw::math::Kernel::PixelT PixelT;

    lsst::afw::image::Image<PixelT> kImage(kernel.dimensions());
    double kSum = kernel.computeImage(kImage, doNormalize, x, y);

    for (int y = kImage.getHeight() - 1; y >= 0; --y) {
        for (lsst::afw::image::Image<PixelT>::const_x_iterator ptr = kImage.row_begin(y);
             ptr != kImage.row_end(y); ++ptr) {
            std::cout << boost::format(pixelFmt) % *ptr << " ";
        }
        std::cout << std::endl;
    }

    if (doNormalize && std::abs(static_cast<double>(kSum) - 1.0) > 1.0e-5) {
        std::cout << boost::format("Warning! Sum of all pixels = %9.5f != 1.0\n") % kSum;
    }
    std::cout << std::endl;
}
