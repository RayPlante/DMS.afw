// -*- LSST-C++ -*-

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
 
#if !defined(LSST_AFW_MATH_STATISTICS_H)
#define LSST_AFW_MATH_STATISTICS_H
/**
 * @file Statistics.h
 * @brief Compute Image Statistics
 * @ingroup afw
 * @author Steve Bickerton
 *
 * @note The Statistics class itself can only handle lsst::afw::image::MaskedImage() types.
 *       The philosophy has been to handle other types by making them look like
 *       lsst::afw::image::MaskedImage() and reusing that code.
 *       Users should have no need to instantiate a Statistics object directly,
 *       but should use the overloaded makeStatistics() factory functions.
 */

#include <algorithm>
#include <cassert>
#include <limits>
#include "boost/iterator/iterator_adaptor.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/shared_ptr.hpp"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/math/MaskedVector.h"

namespace lsst {
namespace afw {
namespace math {
            
/**
 * @brief control what is calculated
 */
enum Property {
    NOTHING = 0x0,         ///< We don't want anything
    ERRORS = 0x1,          ///< Include errors of requested quantities
    NPOINT = 0x2,          ///< number of sample points
    MEAN = 0x4,            ///< estimate sample mean
    STDEV = 0x8,           ///< estimate sample standard deviation
    VARIANCE = 0x10,       ///< estimate sample variance
    MEDIAN = 0x20,         ///< estimate sample median
    IQRANGE = 0x40,        ///< estimate sample inter-quartile range
    MEANCLIP = 0x80,       ///< estimate sample N-sigma clipped mean (N set in StatisticsControl, default=3)
    STDEVCLIP = 0x100,     ///< estimate sample N-sigma clipped stdev (N set in StatisticsControl, default=3)
    VARIANCECLIP = 0x200,  ///< estimate sample N-sigma clipped variance
                           ///<  (N set in StatisticsControl, default=3)
    MIN = 0x400,           ///< estimate sample minimum
    MAX = 0x800,           ///< estimate sample maximum
    SUM = 0x1000,          ///< find sum of pixels in the image
    MEANSQUARE = 0x2000,   ///< find mean value of square of pixel values
    ORMASK = 0x4000        ///< get the or-mask of all pixels used.
};
Property stringToStatisticsProperty(std::string const property);

    
/**
 * @brief Pass parameters to a Statistics object
 * @ingroup afw
 *
 * A class to pass parameters which control how the stats are calculated.
 * 
 */
class StatisticsControl {
public:

    typedef boost::shared_ptr<StatisticsControl> Ptr;
    typedef boost::shared_ptr<StatisticsControl> const ConstPtr;
    
    StatisticsControl(
        double numSigmaClip = 3.0, ///< number of standard deviations to clip at
        int numIter = 3,           ///< Number of iterations
        lsst::afw::image::MaskPixel andMask = 0x0, ///< and-Mask: defines which mask bits cause a value to be ignored
        bool isNanSafe = true,     ///< flag NaNs
        bool isWeighted = false    ///< use inverse Variance plane for weights
                     ) :
        _numSigmaClip(numSigmaClip),
        _numIter(numIter),
        _andMask(andMask),
        _noGoodPixelsMask(lsst::afw::image::Mask<>::getPlaneBitMask("EDGE")),
        _isNanSafe(isNanSafe),
        _isWeighted(isWeighted),
        _isMultiplyingWeights(false) {
        
        assert(_numSigmaClip > 0);
        assert(_numIter > 0);
    }

    double getNumSigmaClip() const { return _numSigmaClip; }
    int getNumIter() const { return _numIter; }
    int getAndMask() const { return _andMask; }
    int getNoGoodPixelsMask() const { return _noGoodPixelsMask; }
    bool getNanSafe() const { return _isNanSafe; }
    bool getWeighted() const { return _isWeighted; }
    bool getMultiplyWeights() const { return _isMultiplyingWeights; }
    
    
    void setNumSigmaClip(double numSigmaClip) { assert(numSigmaClip > 0); _numSigmaClip = numSigmaClip; }
    void setNumIter(int numIter) { assert(numIter > 0); _numIter = numIter; }
    void setAndMask(int andMask) { _andMask = andMask; }
    void setNoGoodPixelsMask(int noGoodPixelsMask) { _noGoodPixelsMask = noGoodPixelsMask; }
    void setNanSafe(bool isNanSafe) { _isNanSafe = isNanSafe; }
    void setWeighted(bool isWeighted) { _isWeighted = isWeighted; }
    void setMultiplyWeights(bool isMultiplyingWeights) { _isMultiplyingWeights = isMultiplyingWeights; }
    

private:
    double _numSigmaClip;                 // Number of standard deviations to clip at
    int _numIter;                         // Number of iterations
    int _andMask;               // and-Mask to specify which mask planes to ignore
    int _noGoodPixelsMask;      // mask to set if no values are acceptable
    bool _isNanSafe;                      // Check for NaNs before running (slower)
    bool _isWeighted;                     // Use inverse variance to weight statistics.
    bool _isMultiplyingWeights;           // Treat variance plane as weights and multiply instead of dividing
};

            
/**
 * @ingroup afw
 *
 * A class to evaluate %image statistics
 *
 * The basic strategy is to construct a Statistics object from an Image and
 * a statement of what we want to know.  The desired results can then be
 * returned using Statistics methods.  A StatisticsControl object is used to
 * pass parameters.  The statistics currently implemented are listed in the
 * enum Properties in Statistics.h.
 *
 * @code
        // sets NumSigclip (3.0), and NumIter (3) for clipping
        lsst::afw::math::StatisticsControl sctrl(3.0, 3); 

        sctrl.setNumSigmaClip(4.0);            // reset number of standard deviations for N-sigma clipping
        sctrl.setNumIter(5);                   // reset number of iterations for N-sigma clipping
        sctrl.setAndMask(0x1);                 // ignore pixels with these mask bits set
        sctrl.setNanSafe(true);                // check for NaNs, a bit slower (default=true)
        
        lsst::afw::math::Statistics statobj =
            lsst::afw::math::makeStatistics(*img, afwMath::NPOINT | 
                                                  afwMath::MEAN | afwMath::MEANCLIP, sctrl);
        double const n = statobj.getValue(lsst::afw::math::NPOINT);
        std::pair<double, double> const mean =
                                         statobj.getResult(lsst::afw::math::MEAN); // Returns (value, error)
        double const meanError = statobj.getError(lsst::afw::math::MEAN);                // just the error
 * @endcode
 *
 * @note Factory function: We used a helper function, \c makeStatistics, rather that the constructor
 *       directly so that the compiler could deduce the types -- cf. \c std::make_pair)
 *
 * @note Inputs: The class Statistics is templatized, and makeStatistics() can take either:
 *       (1) an image, (2) a maskedImage, or (3) a std::vector<>
 *       Overloaded makeStatistics() functions then wrap what they were passed in Image/Mask-like classes
 *       and call the Statistics constructor.
 * @note Clipping: The clipping is done iteratively with numSigmaClip and numIter specified in
 *       the StatisticsControl object.  The first clip (ie. the first iteration) is performed at:
 *       median +/- numSigmaClip*IQ_TO_STDEV*IQR, where IQ_TO_STDEV=~0.74 is the conversion factor
 *       between the IQR and sigma for a Gaussian distribution.  All subsequent iterations perform
 *       clips at mean +/- numSigmaClip*stdev.
 *
 */
class Statistics {
public:
    /// The type used to report (value, error) for desired statistics
    typedef std::pair<double, double> Value;
    
    template<typename ImageT, typename MaskT, typename VarianceT>
    explicit Statistics(ImageT const &img,
                        MaskT const &msk,
                        VarianceT const &var,
                        int const flags,
                        StatisticsControl const& sctrl = StatisticsControl());

    template<typename ImageT, typename MaskT, typename VarianceT, typename WeightT>
    explicit Statistics(ImageT const &img,
                        MaskT const &msk,
                        VarianceT const &var,
                        WeightT const &weights,
                        int const flags,
                        StatisticsControl const& sctrl = StatisticsControl());

    Value getResult(Property const prop = NOTHING) const;
    
    double getError(Property const prop = NOTHING) const;
    double getValue(Property const prop = NOTHING) const;
    lsst::afw::image::MaskPixel getOrMask() const {
        return _allPixelOrMask;
    }
    
private:
    long _flags;                        // The desired calculation

    int _n;                             // number of pixels in the image
    double _mean;                       // the image's mean
    double _variance;                   // the image's variance
    double _min;                        // the image's minimum
    double _max;                        // the image's maximum
    double _sum;                        // the sum of all the image's pixels
    double _meanclip;                   // the image's N-sigma clipped mean
    double _varianceclip;               // the image's N-sigma clipped variance
    double _median;                     // the image's median
    double _iqrange;                    // the image's interquartile range
    lsst::afw::image::MaskPixel _allPixelOrMask;   //  the 'or' of all masked pixels

    StatisticsControl _sctrl;           // the control structure

    template<typename ImageT, typename MaskT, typename VarianceT, typename WeightT>
    void doStatistics(ImageT const &img,
                      MaskT const &msk,
                      VarianceT const &var,
                      WeightT const &weights,
                      int const flags,
                      StatisticsControl const& sctrl);
};
            
/*************************************  The factory functions **********************************/
/**
 * @brief This iterator will never increment.  It is returned by row_begin() in the MaskImposter class
 *        (below) to allow phony mask pixels to be iterated over for non-mask images within Statistics.
 */
template <typename ValueT>
class infinite_iterator
    : public boost::iterator_adaptor<infinite_iterator<ValueT>,
                                     const ValueT*, const ValueT,
                                     boost::forward_traversal_tag> {
public:
    infinite_iterator() : infinite_iterator::iterator_adaptor_(0) {}
    explicit infinite_iterator(const ValueT* p) : infinite_iterator::iterator_adaptor_(p) {}
private:
    friend class boost::iterator_core_access;
    void increment() { ; }              // never actually advance the iterator
};
/**
 * @brief A Mask wrapper to provide an infinite_iterator for Mask::row_begin().  This allows a fake
 *        Mask to be passed in to Statistics with a regular (non-masked) Image.
 */
template<typename ValueT>
class MaskImposter {
public:
    typedef infinite_iterator<ValueT> x_iterator;
    explicit MaskImposter(ValueT val = 0) { _val[0] = val; }
    x_iterator row_begin(int) const { return x_iterator(_val); }
private:
    ValueT _val[1];
};

    
/**
 * @brief Handle a watered-down front-end to the constructor (no variance)
 * @relates Statistics
 */
template<typename Pixel>
Statistics makeStatistics(lsst::afw::image::Image<Pixel> const &img,
                          lsst::afw::image::Mask<image::MaskPixel> const &msk, 
                          int const flags,  
                          StatisticsControl const& sctrl = StatisticsControl() 
                         ) {
    MaskImposter<lsst::afw::image::VariancePixel> var;
    return Statistics(img, msk, var, flags, sctrl);
}


/**
 * @brief Handle a straight front-end to the constructor
 * @relates Statistics
 */
template<typename ImageT, typename MaskT, typename VarianceT>
Statistics makeStatistics(ImageT const &img,
                          MaskT const &msk,
                          VarianceT const &var,
                          int const flags,  
                          StatisticsControl const& sctrl = StatisticsControl() 
                         ) {
    return Statistics(img, msk, var, flags, sctrl);
}

/**
 * @brief Handle MaskedImages, just pass the getImage() and getMask() values right on through.
 * @relates Statistics
 */
template<typename Pixel>
Statistics makeStatistics(
        lsst::afw::image::MaskedImage<Pixel> const &mimg, 
        int const flags,  
        StatisticsControl const& sctrl = StatisticsControl() 
                         )
{
    if (sctrl.getWeighted()) {
        return Statistics(*mimg.getImage(), *mimg.getMask(), *mimg.getVariance(), flags, sctrl);
    } else {
        MaskImposter<lsst::afw::image::VariancePixel> var;
        return Statistics(*mimg.getImage(), *mimg.getMask(), var, flags, sctrl);
    }
}

/**
 * @brief Handle MaskedImages, just pass the getImage() and getMask() values right on through.
 * @relates Statistics
 */
template<typename Pixel>
Statistics makeStatistics(
        lsst::afw::image::MaskedImage<Pixel> const &mimg,
        lsst::afw::image::Image<lsst::afw::image::VariancePixel> const &weights,
        int const flags,  
        StatisticsControl const& sctrl = StatisticsControl() 
                         )
{
    if (sctrl.getWeighted()) {
        return Statistics(*mimg.getImage(), *mimg.getMask(), *mimg.getVariance(), weights, flags, sctrl);
    } else {
        MaskImposter<lsst::afw::image::VariancePixel> var;
        return Statistics(*mimg.getImage(), *mimg.getMask(), var, weights, flags, sctrl);
    }
}

/**
 * @brief Front end for specialization to handle Masks
 * @note The definition (in Statistics.cc) simply calls the specialized constructor
 * @relates Statistics
 */            
Statistics makeStatistics(lsst::afw::image::Mask<lsst::afw::image::MaskPixel> const &msk, 
                          int const flags,  
                          StatisticsControl const& sctrl = StatisticsControl());
            
                        
            
/**
 * @brief The makeStatistics() overload to handle regular (non-masked) Images
 * @relates Statistics
 */
template<typename Pixel>
Statistics makeStatistics(
        lsst::afw::image::Image<Pixel> const &img, ///< Image (or Image) whose properties we want
        int const flags,   ///< Describe what we want to calculate
        StatisticsControl const& sctrl = StatisticsControl() ///< Control calculation
) {
    // make a phony mask that will be compiled out
    MaskImposter<lsst::afw::image::MaskPixel> const msk;
    MaskImposter<lsst::afw::image::VariancePixel> const var;
    return Statistics(img, msk, var, flags, sctrl);
}


/**
 * @brief A vector wrapper to provide a vector with the necessary methods and typedefs to
 *        be processed by Statistics as though it were an Image.
 */
template<typename ValueT>
class ImageImposter {
public:
    
    // types we'll use in Statistics
    typedef typename std::vector<ValueT>::const_iterator x_iterator;
    typedef typename std::vector<ValueT>::const_iterator fast_iterator;
    typedef ValueT Pixel;

    // constructors for std::vector<>, and copy constructor
    // These are both shallow! ... no actual copying of values
    explicit ImageImposter(std::vector<ValueT> const &v) : _v(v) { }
    explicit ImageImposter(ImageImposter<ValueT> const &img) : _v(img._getVector()) {}

    // The methods we'll use in Statistics
    x_iterator row_begin(int) const { return _v.begin(); }
    x_iterator row_end(int) const { return _v.end(); }
    int getWidth() const { return _v.size(); }
    int getHeight() const { return 1; }
    
private:
    std::vector<ValueT> const &_v;                  // a private reference to the data
    std::vector<ValueT> const &_getVector() const { return _v; } // get the ref for the copyCon
};

/**
 * @brief The makeStatistics() overload to handle std::vector<>
 * @relates Statistics
 */
template<typename EntryT>
Statistics makeStatistics(std::vector<EntryT> const &v, ///< Image (or MaskedImage) whose properties we want
                          int const flags,   ///< Describe what we want to calculate
                          StatisticsControl const& sctrl = StatisticsControl() ///< Control calculation
                         ) {
    ImageImposter<EntryT> img(v);           // wrap the vector in a fake image
    MaskImposter<lsst::afw::image::MaskPixel> msk;     // instantiate a fake mask that will be compiled out.
    MaskImposter<lsst::afw::image::VariancePixel> var;
    return Statistics(img, msk, var, flags, sctrl);
}

/**
 * @brief The makeStatistics() overload to handle std::vector<>
 * @relates Statistics
 */
template<typename EntryT>
Statistics makeStatistics(std::vector<EntryT> const &v, ///< Image (or MaskedImage) whose properties we want
                          std::vector<lsst::afw::image::VariancePixel> const &vweights, ///< Weights
                          int const flags,   ///< Describe what we want to calculate
                          StatisticsControl const& sctrl = StatisticsControl() ///< Control calculation
                         ) {
    ImageImposter<EntryT> img(v);           // wrap the vector in a fake image
    MaskImposter<lsst::afw::image::MaskPixel> msk;     // instantiate a fake mask that will be compiled out.
    MaskImposter<lsst::afw::image::VariancePixel> var;

    ImageImposter<lsst::afw::image::VariancePixel> weights(vweights);
    
    return Statistics(img, msk, var, weights, flags, sctrl);
}

/**
 * @brief The makeStatistics() overload to handle lsst::afw::math::MaskedVector<>
 * @relates Statistics
 */
template<typename EntryT>
Statistics makeStatistics(lsst::afw::math::MaskedVector<EntryT> const &mv, ///< MaskedVector
                          int const flags,   ///< Describe what we want to calculate
                          StatisticsControl const& sctrl = StatisticsControl() ///< Control calculation
                         ) {
    if (sctrl.getWeighted()) {
        return Statistics(*mv.getImage(), *mv.getMask(), *mv.getVariance(), flags, sctrl);
    } else {
        MaskImposter<lsst::afw::image::VariancePixel> var;
        return Statistics(*mv.getImage(), *mv.getMask(), var, flags, sctrl);
    }
}

/**
 * @brief The makeStatistics() overload to handle lsst::afw::math::MaskedVector<>
 * @relates Statistics
 */
template<typename EntryT>
Statistics makeStatistics(lsst::afw::math::MaskedVector<EntryT> const &mv, ///< MaskedVector
                          std::vector<lsst::afw::image::VariancePixel> const &vweights, ///< weights
                          int const flags,   ///< Describe what we want to calculate
                          StatisticsControl const& sctrl = StatisticsControl() ///< Control calculation
                         ) {
    ImageImposter<lsst::afw::image::VariancePixel> weights(vweights);

    if (sctrl.getWeighted()) {
        return Statistics(*mv.getImage(), *mv.getMask(), *mv.getVariance(), weights, flags, sctrl);
    } else {
        MaskImposter<lsst::afw::image::VariancePixel> var;
        return Statistics(*mv.getImage(), *mv.getMask(), var, weights, flags, sctrl);
    }
}


    
}}}

#endif
