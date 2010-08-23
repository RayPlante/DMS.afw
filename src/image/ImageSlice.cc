// -*- lsst-c++ -*-

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
 * @file Stack.cc
 * @brief Provide functions to stack images
 * @ingroup stack
 * @author Steve Bickerton
 *
 */
#include <vector>
#include "boost/shared_ptr.hpp"

#include "lsst/pex/exceptions.h"
#include "lsst/afw/image/ImageSlice.h"

namespace afwImage      = lsst::afw::image;
namespace afwMath       = lsst::afw::math;
namespace ex            = lsst::pex::exceptions;


/**
 * @brief Constructor for ImageSlice
 *
 */
template<typename PixelT>
afwImage::ImageSlice<PixelT>::ImageSlice(
    image::Image<PixelT> const &img ///< The image to represent as a slice.
                                        ) : 
    afwImage::Image<PixelT>(img),
    _sliceType(ROW)
{

    // verify the img is a slice (row or column)
    if (img.getWidth() != 1 && img.getHeight() != 1) {
        throw LSST_EXCEPT(ex::OutOfRangeException, "Input image must be a slice (width or height == 1)");
    }  else if (img.getWidth() == 1 && img.getHeight() == 1) {
        throw LSST_EXCEPT(ex::InvalidParameterException, 
                          "1x1 image ambiguous (could be row or column).  "
                          "Perhaps a constant would be better than a slice? ");
    } else if (img.getWidth() == 1 && img.getHeight() != 1) {
        _sliceType = COLUMN;
    } else if (img.getHeight() == 1 && img.getWidth() != 1) {
        _sliceType = ROW;
    }

    // what about 1xn images where a 1x1 row slice is desired? ... use a constant instead of a slice
    // what about nx1 images wehre a 1x1 column slice is desired? ... use a constant instead of a slice
}




/**************************************************************************
 *
 * column operators
 *
 **************************************************************************/


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// overload +

/**
 * @brief Overload operator+()
 *
 * We require two of these, one for image+slice (this one) and one for slice+image (next one down)
 */
template<typename PixelT>    
typename afwImage::Image<PixelT>::Ptr afwImage::operator+(
    afwImage::Image<PixelT> const &img,     ///< The Image     
    afwImage::ImageSlice<PixelT> const &slc ///< The ImageSlice
                                                         ) {
    typename afwImage::Image<PixelT>::Ptr retImg(new afwImage::Image<PixelT>(img, true));
    *retImg += slc;
    return retImg;
}



/**
 * @brief Overload operator+()
 *
 * We require two of these, one for image+slice (previous one) and one for slice+image (this)
 */
template<typename PixelT>    
typename afwImage::Image<PixelT>::Ptr afwImage::operator+(
    afwImage::ImageSlice<PixelT> const &slc, ///< The ImageSlice
    afwImage::Image<PixelT> const &img       ///< The Image
                                                         ) {
    return afwImage::operator+(img, slc);
}



/**
 * @brief Overload operator+=()
 *
 * We'll only allow 'image += slice'.  It doesn't make sense to add an image to a slice.
 */
template<typename PixelT>    
void afwImage::operator+=(
                          afwImage::Image<PixelT> &img,     ///< The Image     
                          afwImage::ImageSlice<PixelT> const &slc ///< The ImageSlice
                         ) {
    afwImage::details::operate<afwImage::details::Plus<PixelT> >(img, slc, slc.getImageSliceType());
}



// -----------------------------------------------------------------
// overload -



/**
 * @brief Overload operator-()
 *
 * We'll only allow 'image - slice', as 'slice - image' doesn't make sense.
 */
template<typename PixelT>    
typename afwImage::Image<PixelT>::Ptr afwImage::operator-(
    afwImage::Image<PixelT> const &img,     ///< The Image     
    afwImage::ImageSlice<PixelT> const &slc ///< The ImageSlice
                                                         ) {
    typename afwImage::Image<PixelT>::Ptr retImg(new afwImage::Image<PixelT>(img, true));
    *retImg -= slc;
    return retImg;
}


/**
 * @brief Overload operator-=()
 *
 * Only 'image -= slice' is defined.  'slice -= image' wouldn't make sense.
 */
template<typename PixelT>    
void afwImage::operator-=(
                          afwImage::Image<PixelT> &img,     ///< The Image     
                          afwImage::ImageSlice<PixelT> const &slc ///< The ImageSlice
                         ) {
    details::operate<details::Minus<PixelT> >(img, slc, slc.getImageSliceType());
}


// ******************************************************************
// overload *


/**
 * @brief Overload operator*()
 *
 * We'll define both 'image*slice' (this one) and 'slice*image' (next one down).
 */
template<typename PixelT>    
typename afwImage::Image<PixelT>::Ptr afwImage::operator*(
    afwImage::Image<PixelT> const &img,      ///< The Image     
    afwImage::ImageSlice<PixelT> const &slc  ///< The ImageSlice
                                                         ) {
    typename afwImage::Image<PixelT>::Ptr retImg(new afwImage::Image<PixelT>(img, true));
    *retImg *= slc;
    return retImg;
}


/**
 * @brief Overload operator*()
 *
 * We'll define both 'image*slice' (this one) and 'slice*image' (next one down).
 */
template<typename PixelT>    
typename afwImage::Image<PixelT>::Ptr afwImage::operator*(
    afwImage::ImageSlice<PixelT> const &slc, ///< The Image     
    afwImage::Image<PixelT> const &img       ///< The ImageSlice
                                                         ) {
    return afwImage::operator*(img, slc);
}

/**
 * @brief Overload operator*=()
 *
 * Only 'image *= slice' is defined, as 'slice *= image' doesn't make sense.
 */
template<typename PixelT>    
void afwImage::operator*=(
                          afwImage::Image<PixelT> &img,     ///< The Image     
                          afwImage::ImageSlice<PixelT> const &slc ///< The ImageSlice
                         ) {
    details::operate<details::Mult<PixelT> >(img, slc, slc.getImageSliceType());
}




// ///////////////////////////////////////////////////////////////////
// overload /


/**
 * @brief Overload operator/()
 *
 * Only 'image / slice' is defined, as 'slice / image' doesn't make sense.
 */
template<typename PixelT>    
typename afwImage::Image<PixelT>::Ptr afwImage::operator/(
    afwImage::Image<PixelT> const &img,     ///< The Image     
    afwImage::ImageSlice<PixelT> const &slc ///< The ImageSlice
                                                         ) {
    typename afwImage::Image<PixelT>::Ptr retImg(new afwImage::Image<PixelT>(img, true));
    *retImg /= slc;
    return retImg;
}


/**
 * @brief Overload operator/=()
 *
 * Only 'image /= slice' is defined, as 'slice /= image' doesn't make sense.
 */
template<typename PixelT>    
void afwImage::operator/=(
                          afwImage::Image<PixelT> &img,     ///< The Image     
                          afwImage::ImageSlice<PixelT> const &slc ///< The ImageSlice
                         ) {
    details::operate<details::Div<PixelT> >(img, slc, slc.getImageSliceType());
}




/*
 * Explicit Instantiations
 *
 */

#define INSTANTIATE_SLICE_OP_SYM(TYPE, OP) \
    template afwImage::Image<TYPE>::Ptr afwImage::operator OP(afwImage::Image<TYPE> const &img, \
                                                              afwImage::ImageSlice<TYPE> const &slc); \
    template afwImage::Image<TYPE>::Ptr afwImage::operator OP(afwImage::ImageSlice<TYPE> const &slc, \
                                                              afwImage::Image<TYPE> const &img)


#define INSTANTIATE_SLICE_OP_ASYM(TYPE, OP) \
    template afwImage::Image<TYPE>::Ptr afwImage::operator OP(afwImage::Image<TYPE> const &img, \
                                                              afwImage::ImageSlice<TYPE> const &slc)
    

#define INSTANTIATE_SLICE_OPEQ(TYPE, OP)                                \
    template void afwImage::operator OP(afwImage::Image<TYPE> &img,     \
                                        afwImage::ImageSlice<TYPE> const &slc)



#define INSTANTIATE_SLICES(TYPE) \
    template afwImage::ImageSlice<TYPE>::ImageSlice(afwImage::Image<TYPE> const &image); \
    INSTANTIATE_SLICE_OP_SYM(TYPE, +);                                  \
    INSTANTIATE_SLICE_OP_ASYM(TYPE, -);                                 \
    INSTANTIATE_SLICE_OP_SYM(TYPE, *);                                  \
    INSTANTIATE_SLICE_OP_ASYM(TYPE, /);                                 \
    INSTANTIATE_SLICE_OPEQ(TYPE, +=);                                   \
    INSTANTIATE_SLICE_OPEQ(TYPE, -=);                                   \
    INSTANTIATE_SLICE_OPEQ(TYPE, *=);                                   \
    INSTANTIATE_SLICE_OPEQ(TYPE, /=)


INSTANTIATE_SLICES(double);
INSTANTIATE_SLICES(float);

