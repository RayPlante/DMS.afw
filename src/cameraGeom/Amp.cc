/* 
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
 * 
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the LSST License Statement and 
 * the GNU General Public License along with this program.  If not, 
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */
 
/**
 * \file
 */
#include <algorithm>
#include "lsst/afw/image/Image.h"
#include "lsst/afw/math/offsetImage.h"
#include "lsst/afw/cameraGeom/Amp.h"
#include "lsst/afw/cameraGeom/Detector.h"

namespace afwGeom = lsst::afw::geom;
namespace afwImage = lsst::afw::image;
namespace afwMath = lsst::afw::math;
namespace cameraGeom = lsst::afw::cameraGeom;

cameraGeom::ElectronicParams::ElectronicParams(
        float gain,                     ///< Amplifier's gain
        float readNoise,                ///< Amplifier's read noise (DN)
        float saturationLevel           ///< Amplifier's saturation level. N.b. float in case we scale data
                                           )
    : _gain(gain), _readNoise(readNoise), _saturationLevel(saturationLevel)
{}

/************************************************************************************************************/

cameraGeom::Amp::Amp(
    cameraGeom::Id id,                            ///< The amplifier's ID
    afwGeom::Box2I const& allPixels,           ///< Bounding box of the pixels read off this amplifier
    afwGeom::Box2I const& biasSec,             ///< Bounding box of amplifier's bias section
    afwGeom::Box2I const& dataSec,             ///< Bounding box of amplifier's data section
    ElectronicParams::Ptr eParams              ///< electronic properties of Amp
) : Detector(id, true),
    _biasSec(biasSec), 
    _dataSec(dataSec), 
    _eParams(eParams)
{
    if (biasSec.getWidth() > 0 && biasSec.getHeight() > 0 &&
        (!allPixels.contains(biasSec))
    ) {
        throw LSST_EXCEPT(
            lsst::pex::exceptions::OutOfRangeException,
            (boost::format(
                "%||'s bias section doesn't fit in allPixels") % id
            ).str()
        );
    }
    if (dataSec.getWidth() > 0 && dataSec.getHeight() > 0 &&
        (!allPixels.contains(dataSec))
    ) {
        throw LSST_EXCEPT(
            lsst::pex::exceptions::OutOfRangeException,
            (boost::format(
                "%||'s data section doesn't fit in allPixels") % id
            ).str()
        );
        
    }

    getAllPixels() = allPixels;

    _originInDetector = afwGeom::Point2I(0, 0);
    _nQuarter = 0;
    _flipLR = _flipTB = false;
    
    setTrimmedGeom();
}
/**
 * Set the geometry of the Amp after trimming the overclock/extended regions
 *
 * Note that the trimmed BBoxes are relative to a trimmed detector
 */
void cameraGeom::Amp::setTrimmedGeom() {
    //
    // Figure out which Amp we are
    //
    int const iX = getAllPixels().getMinX()/getAllPixels().getWidth();
    int const iY = getAllPixels().getMinY()/getAllPixels().getHeight();
    
    int const dataHeight = _dataSec.getHeight();
    int const dataWidth = _dataSec.getWidth();

    _trimmedDataSec = afwGeom::Box2I(
        afwGeom::Point2I(iX*dataWidth, iY*dataHeight), 
        afwGeom::Extent2I(dataWidth, dataHeight)
    );
    getAllTrimmedPixels() = _trimmedDataSec;
}

void cameraGeom::Amp::setElectronicToChipLayout(
        lsst::afw::geom::Point2I pos,         // Position of Amp data (in Detector coords)
        int nQuarter,                         // number of quarter-turns in +ve direction
        bool flipLR,                          // Flip the Amp data left <--> right before rotation
        bool flipTB                           // Flip the Amp data top <--> bottom before rotation
                  ) {
    _nQuarter = nQuarter;
    _flipLR = flipLR;
    _flipTB = flipTB;

    if (_flipLR && _flipTB)
        _readoutCorner = URC;
    else if (_flipLR)
        _readoutCorner = LRC;
    else if (_flipTB)
        _readoutCorner = ULC;
    else
        _readoutCorner = LLC;

    _readoutCorner = static_cast<ReadoutCorner>((_readoutCorner + nQuarter)%4);
    _biasSec = _mapFromElectronic(_biasSec);
    _dataSec = _mapFromElectronic(_dataSec);
    getAllPixels() = _mapFromElectronic(getAllPixels());
    this->shift(pos.getX()*getAllPixels().getWidth(), pos.getY()*getAllPixels().getHeight());
    setTrimmedGeom();
    _originInDetector = getAllPixels().getMin();
}

/// Offset an Amp by the specified amount
void cameraGeom::Amp::shift(int dx,        ///< How much to offset in x (pixels)
                            int dy         ///< How much to offset in y (pixels)
                        ) {
    geom::Extent2I d(dx,dy);
    getAllPixels().shift(d);
    _biasSec.shift(d);
    _dataSec.shift(d);
    getAllTrimmedPixels().shift(d);
    _trimmedDataSec.shift(d);
}

/// Rotate an Amp by some number of 90degree anticlockwise turns about centerPixel
void cameraGeom::Amp::rotateBy90(
        afwGeom::Extent2I const& dimensions, ///< Size of CCD
        int n90                              ///< How many 90-degree anti-clockwise turns should I apply?
                                ) {
    while (n90 < 0) {
        n90 += 4;
    }
    n90 %= 4;                           // normalize

    if (n90 == 0) {                     // nothing to do.  Good.
        return;
    }
    //
    // Rotate the amps to the right orientation
    //
    getAllPixels() = cameraGeom::detail::rotateBBoxBy90(getAllPixels(), n90, dimensions);
    _biasSec = cameraGeom::detail::rotateBBoxBy90(_biasSec, n90, dimensions);
    _dataSec = cameraGeom::detail::rotateBBoxBy90(_dataSec, n90, dimensions);

    setTrimmedGeom();
    //
    // Fix the readout corner
    //
    _readoutCorner = static_cast<ReadoutCorner>((_readoutCorner + n90)%4);
}

/**
 * Convert a Amp's BBox assuming it's been assembled into an entire-CCD image to its value
 * as read from disk.
 *
 * This is intended to be used when each amp is in its separate file (or HDU) on disk
 */
lsst::afw::geom::Box2I cameraGeom::Amp::_mapToElectronic(lsst::afw::geom::Box2I bbox) const {
    // Reset the BBox's origin within the Detector to reflect the on-disk value
    bbox.shift(-geom::Extent2I(_originInDetector));
    // Rotate the BBox to reflect the on-disk orientation
    afwGeom::Extent2I dimensions = getAllPixels(false).getDimensions();
    afwGeom::Box2I tbbox = afwGeom::Box2I(afwGeom::Point2I(0,0), dimensions);
    bbox = cameraGeom::detail::rotateBBoxBy90(bbox, -_nQuarter, dimensions);
    tbbox = cameraGeom::detail::rotateBBoxBy90(tbbox, -_nQuarter, dimensions);
    if(_flipTB){
        bbox.flipTB(tbbox.getDimensions()[1]);
    }
    if(_flipLR){
        bbox.flipLR(tbbox.getDimensions()[0]);
    }
    return bbox;
}

/**
 * Convert a Amp's BBox from electronic coordinates where the first pixel is at (0,0)
 * to the appropriate orientation for inclusion in a sensor in the camera
 * coordinate system.  The origin of the amp is set in the addAmp() method on
 * the Ccd class.
 *
 * This is intended to be used when each amp is in its separate file (or HDU) on disk
 */
lsst::afw::geom::Box2I cameraGeom::Amp::_mapFromElectronic(lsst::afw::geom::Box2I bbox) const {
    // Rotate the BBox to reflect the on-disk orientation
    afwGeom::Extent2I dimensions = getAllPixels(false).getDimensions();
    if(_flipLR){
        bbox.flipLR(dimensions[0]);
    }
    if(_flipTB){
        bbox.flipTB(dimensions[1]);
    }
    return cameraGeom::detail::rotateBBoxBy90(bbox, _nQuarter, dimensions);
}

/**
 * Prepare an Amp that's just been read from disk to be copied into the image of the complete Detector
 *
 * This is only important if the Amps are stored in separate files, rather than being assembled into
 * complete Detectors by the data acquisition system
 */
template<typename ImageT>
typename ImageT::Ptr cameraGeom::Amp::prepareAmpData(ImageT const& inImage)
{
    typename ImageT::Ptr flippedImage = afwMath::flipImage(inImage, _flipLR, _flipTB);

    return afwMath::rotateImageBy90(*flippedImage, _nQuarter);
}

/************************************************************************************************************/

//
// Explicit instantiations
// \cond
//
#define INSTANTIATE(TYPE) \
    template afwImage::Image<TYPE>::Ptr cameraGeom::Amp::prepareAmpData(afwImage::Image<TYPE> const&);
#define INSTANTIATEMASK(TYPE) \
    template afwImage::Mask<TYPE>::Ptr cameraGeom::Amp::prepareAmpData(afwImage::Mask<TYPE> const&);

INSTANTIATE(boost::uint16_t)
INSTANTIATE(float)
INSTANTIATE(double)
INSTANTIATEMASK(boost::uint16_t)
// \endcond
