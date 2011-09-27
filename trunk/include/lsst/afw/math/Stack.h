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
 
#if !defined(LSST_AFW_MATH_STACK_H)
#define LSST_AFW_MATH_STACK_H
/**
 * @file Stack.h
 * @brief Functions to stack images
 * @ingroup stack
 */ 
#include <vector>
#include "lsst/afw/image/Image.h"
#include "lsst/afw/math/Statistics.h"

namespace lsst {
namespace afw {
namespace math {    

/********************************************************************
 *
 * z stacks
 *
 *********************************************************************/

/**
 * @brief A function to compute some statistics of a stack of Images
 */
template<typename PixelT>
typename lsst::afw::image::Image<PixelT>::Ptr statisticsStack(
        std::vector<typename lsst::afw::image::Image<PixelT>::Ptr > &images,      ///< Images to process
        Property flags, ///< statistics requested
        StatisticsControl const& sctrl=StatisticsControl(),   ///< Control structure
        std::vector<PixelT> const& wvector=std::vector<PixelT>(0) ///< vector containing weights
                                                             );

/**
 * @brief A function to compute some statistics of a stack of MaskedImages
 */
template<typename PixelT>
typename lsst::afw::image::MaskedImage<PixelT>::Ptr statisticsStack(
        std::vector<typename lsst::afw::image::MaskedImage<PixelT>::Ptr > &images,///< MaskedImages to process
        Property flags, ///< statistics requested
        StatisticsControl const& sctrl=StatisticsControl(), ///< control structure
        std::vector<PixelT> const& wvector=std::vector<PixelT>(0) ///< vector containing weights
                                                                   );


/**
 * @brief A function to compute some statistics of a stack of std::vectors
 */
template<typename PixelT>
typename boost::shared_ptr<std::vector<PixelT> > statisticsStack(
        std::vector<boost::shared_ptr<std::vector<PixelT> > > &vectors,      ///< Vectors to process
        Property flags,              ///< statistics requested
        StatisticsControl const& sctrl=StatisticsControl(),  ///< control structure
        std::vector<PixelT> const& wvector=std::vector<PixelT>(0) ///< vector containing weights
                                                                );
    


/********************************************************************
 *
 * x,y stacks
 *
 *********************************************************************/

/**
 * @brief A function to compute statistics on the rows or columns of an image
 */
template<typename PixelT>
typename lsst::afw::image::MaskedImage<PixelT>::Ptr statisticsStack(
        lsst::afw::image::Image<PixelT> const &image,  
        Property flags,               
        char dimension,
        StatisticsControl const& sctrl=StatisticsControl()
                                                                   );
/**
 * @brief A function to compute statistics on the rows or columns of an image
 */
template<typename PixelT>
typename lsst::afw::image::MaskedImage<PixelT>::Ptr statisticsStack(
        lsst::afw::image::MaskedImage<PixelT> const &image,  
        Property flags,               
        char dimension,
        StatisticsControl const& sctrl=StatisticsControl()
								    );




}}}

#endif
