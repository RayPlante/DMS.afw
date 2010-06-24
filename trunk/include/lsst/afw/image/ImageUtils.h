// -*- lsst-c++ -*-
/**
 * @file
 *
 * @brief Image utility functions
 *
 * @defgroup afw LSST framework 
 */
#ifndef LSST_AFW_IMAGE_IMAGEUTILS_H
#define LSST_AFW_IMAGE_IMAGEUTILS_H

#include <cmath>

namespace lsst {
namespace afw {
namespace image {
    enum xOrY {X, Y};

    const double PixelZeroPos = 0.0;    ///< position of center of pixel 0
    ///< FITS uses 1.0, SDSS uses 0.5, LSST uses 0.0 (http://dev.lsstcorp.org/trac/wiki/BottomLeftPixelProposalII%3A)

    /**
     * @brief Convert image index to image position
     *
     * The LSST indexing convention is:
     * * the index of the bottom left pixel is 0,0
     * * the position of the center of the bottom left pixel is PixelZeroPos, PixelZeroPos
     *
     * @return image position
     */
    inline double indexToPosition(
        int ind ///< image index
    ) {
        return static_cast<double>(ind) + PixelZeroPos;
    }
    
    /**
     * @brief Convert image position to nearest integer index
     *
     * The LSST indexing convention is:
     * * the index of the bottom left pixel is 0,0
     * * the position of the center of the bottom left pixel is PixelZeroPos, PixelZeroPos
     *
     * @return nearest integer index
     */
    inline int positionToIndex(
        double pos ///< image position
    ) {
        return static_cast<int>(std::floor(pos + 0.5 - PixelZeroPos));
    }
    
    /**
     * @brief Convert image position to index (nearest integer and fractional parts)
     *
     * The LSST indexing convention is:
     * * the index of the bottom left pixel is 0,0
     * * the position of the center of the bottom left pixel is PixelZeroPos, PixelZeroPos
     *
     * Note: in python this is called positionToIndexAndResidual
     *
     * @return nearest integer index
     */
    inline int positionToIndex(
        double &residual, ///< fractional part of index
        double pos ///< image position
    ) {
        double fullIndex = pos - PixelZeroPos;
        double roundedIndex = std::floor(fullIndex + 0.5);
        residual = fullIndex - roundedIndex;
        return static_cast<int>(roundedIndex);
    }     
    /**
     * @brief Convert image position to index (nearest integer and fractional parts)
     *
     * @return std::pair(nearest integer index, fractional part)
     */
    inline std::pair<int, double> positionToIndex(double const pos,                ///< image position
                                                  bool                             ///< ignored; just to disambiguate
    ) {
        double residual;                // fractional part of index
        int const ind = positionToIndex(residual, pos); // integral part

        return std::pair<int, double>(ind, residual);
    }     

}}} // lsst::afw::image

#endif // LSST_AFW_IMAGE_IMAGEUTILS_H

