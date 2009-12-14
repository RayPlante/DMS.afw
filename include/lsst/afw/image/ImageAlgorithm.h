// -*- lsst-c++ -*-
/**
 * \file
 * \brief Support for functors over Image's pixels
 */
#ifndef LSST_AFW_IMAGE_IMAGE_ALGORITHM_H
#define LSST_AFW_IMAGE_IMAGE_ALGORITHM_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <functional>

#include "boost/tr1/functional.hpp"
#include "boost/mpl/bool.hpp"
#include "boost/shared_ptr.hpp"

#include "lsst/afw/image/lsstGil.h"
#include "lsst/afw/image/Utils.h"
#include "lsst/afw/image/ImageUtils.h"
#include "lsst/daf/base.h"
#include "lsst/daf/data/LsstBase.h"
#include "lsst/pex/exceptions.h"

namespace lsst { namespace afw { namespace image {
#if !defined(SWIG)
    /**
     * A functor class equivalent to tr1::function<ValT ()>, but with a virtual operator()
     */
    template<typename ValT>
    struct pixelOp0 : std::tr1::function<ValT ()> {
        virtual ~pixelOp0() {}
        virtual ValT operator()() const = 0;
    };
    
    /**
     * A functor class equivalent to tr1::function<ValT (ValT)>, but with a virtual operator()
     */
    template<typename ValT>
    struct pixelOp1 : std::tr1::function<ValT (ValT)> {
        virtual ~pixelOp1() {}
        virtual ValT operator()(ValT lhs) const = 0;
    };
    
    /**
     * A functor class equivalent to tr1::function<ValT (int, int, ValT)>, but with a virtual operator()
     */
    template<typename ValT>
    struct pixelOp1XY : std::tr1::function<ValT (int, int, ValT)> {
        virtual ~pixelOp1XY() {}
        virtual ValT operator()(int x, int y, ValT lhs) const = 0;
    };
    
    /**
     * A functor class equivalent to tr1::function<LhsT (LhsT, RhsT)>, but with a virtual operator()
     */
    template<typename LhsT, typename RhsT>
    struct pixelOp2 : std::tr1::function<LhsT (LhsT, RhsT)>
    {
        virtual ~pixelOp2() {}
        virtual LhsT operator()(LhsT lhs, RhsT rhs) const = 0;
    };

    /**
     * A functor class equivalent to tr1::function<LhsT (int, int, LhsT, RhsT)>, but with a virtual operator()
     */
    template<typename LhsT, typename RhsT>
    struct pixelOp2XY : std::tr1::function<LhsT (int, int, LhsT, RhsT)>
    {
        virtual ~pixelOp2XY() {}
        virtual LhsT operator()(int x, int y, LhsT lhs, RhsT rhs) const = 0;
    };

    /*******************************************************************************************************/
    /**
     * Set each pixel in an Image<LhsT> to func()
     */
    template<typename LhsT>
    void for_each_pixel(Image<LhsT> &lhs,     ///< Image to set
                        pixelOp0<LhsT> const& func ///< functor to call
                       )
    {
        for (int y = 0; y != lhs.getHeight(); ++y) {
            for (typename Image<LhsT>::x_iterator lhsPtr = lhs.row_begin(y), lhsEnd = lhs.row_end(y);
                 lhsPtr != lhsEnd; ++lhsPtr) {
                *lhsPtr = func();
            }
        }
    }

    /**
     * Set each pixel in an Image<LhsT> to func(lhs)
     */
    template<typename LhsT>
    void for_each_pixel(Image<LhsT> &lhs,                ///< Image to set
                        pixelOp1<LhsT> const& func       ///< functor to call
                       )
    {
        for (int y = 0; y != lhs.getHeight(); ++y) {
            for (typename Image<LhsT>::x_iterator lhsPtr = lhs.row_begin(y), lhsEnd = lhs.row_end(y);
                 lhsPtr != lhsEnd; ++lhsPtr) {
                *lhsPtr = func(*lhsPtr);
            }
        }
    }

    /**
     * Set each pixel in an Image<LhsT> to func(x, y, lhs)
     *
     * (x, y) allow for lhs.getXY0()
     */
    template<typename LhsT>
    void for_each_pixel(Image<LhsT> &lhs,                ///< Image to set
                        pixelOp1XY<LhsT> const& func     ///< functor to call
                       )
    {
        for (int y = 0; y != lhs.getHeight(); ++y) {
            int x = lhs.getX0();
            for (typename Image<LhsT>::x_iterator lhsPtr = lhs.row_begin(y), lhsEnd = lhs.row_end(y);
                 lhsPtr != lhsEnd; ++lhsPtr, ++x) {
                *lhsPtr = func(x, y + lhs.getY0(), *lhsPtr);
            }
        }
    }

    /**
     * Set each pixel in an Image<LhsT> to func(rhs), getting the rhs from an Image<RhsT>
     */
    template<typename LhsT, typename RhsT>
    void for_each_pixel(Image<LhsT> &lhs,                ///< Image to set
                        Image<RhsT> const& rhs,          ///< other Image to pass to @c func
                        pixelOp1<RhsT> const& func       ///< functor to call
                       )
    {
        if (lhs.getDimensions() != rhs.getDimensions()) {
            throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                              (boost::format("Images are of different size, %dx%d v %dx%d") %
                               lhs.getWidth() % lhs.getHeight() % rhs.getWidth() % rhs.getHeight()).str());
        }

        for (int y = 0; y != lhs.getHeight(); ++y) {
            typename Image<RhsT>::const_x_iterator rhsPtr = rhs.row_begin(y);

            for (typename Image<LhsT>::x_iterator lhsPtr = lhs.row_begin(y), lhsEnd = lhs.row_end(y);
                 lhsPtr != lhsEnd; ++rhsPtr, ++lhsPtr) {
                *lhsPtr = func(*rhsPtr);
            }
        }
    }

    /**
     * Set each pixel in an Image<LhsT> to func(lhs, rhs), getting the rhs from an Image<RhsT>
     */ 
    template<typename LhsT, typename RhsT>
    void for_each_pixel(Image<LhsT> &lhs,                ///< Image to set
                        Image<RhsT> const& rhs,          ///< other Image to pass to @c func
                        pixelOp2<LhsT, RhsT> const& func ///< functor to call
                       )
    {
        if (lhs.getDimensions() != rhs.getDimensions()) {
            throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                              (boost::format("Images are of different size, %dx%d v %dx%d") %
                               lhs.getWidth() % lhs.getHeight() % rhs.getWidth() % rhs.getHeight()).str());
        }

        for (int y = 0; y != lhs.getHeight(); ++y) {
            typename Image<RhsT>::const_x_iterator rhsPtr = rhs.row_begin(y);

            for (typename Image<LhsT>::x_iterator lhsPtr = lhs.row_begin(y), lhsEnd = lhs.row_end(y);
                 lhsPtr != lhsEnd; ++rhsPtr, ++lhsPtr) {
                *lhsPtr = func(*lhsPtr, *rhsPtr);
            }
        }
    }
    /**
     * Set each pixel in an Image<LhsT> to func(x, y, lhs, rhs), getting the rhs from an Image<RhsT>
     *
     * (x, y) allow for lhs.getXY0()
     */ 
   template<typename LhsT, typename RhsT>
    void for_each_pixel(Image<LhsT> &lhs,                ///< Image to set
                        Image<RhsT> const& rhs,          ///< other Image to pass to @c func
                        pixelOp2XY<LhsT, RhsT> const& func ///< functor to call
                       )
    {
        if (lhs.getDimensions() != rhs.getDimensions()) {
            throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                              (boost::format("Images are of different size, %dx%d v %dx%d") %
                               lhs.getWidth() % lhs.getHeight() % rhs.getWidth() % rhs.getHeight()).str());
        }

        for (int y = 0; y != lhs.getHeight(); ++y) {
            typename Image<RhsT>::const_x_iterator rhsPtr = rhs.row_begin(y);
            int x = lhs.getX0();
            for (typename Image<LhsT>::x_iterator lhsPtr = lhs.row_begin(y), lhsEnd = lhs.row_end(y);
                 lhsPtr != lhsEnd; ++rhsPtr, ++lhsPtr, ++x) {
                *lhsPtr = func(x, y + lhs.getY0(), *lhsPtr, *rhsPtr);
            }
        }
    }
#endif
}}}  // lsst::afw::image

#endif