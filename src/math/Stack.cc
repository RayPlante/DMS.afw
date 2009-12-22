// -*- lsst-c++ -*-
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
#include "lsst/afw/math/Stack.h"
#include "lsst/afw/math/MaskedVector.h"

namespace image = lsst::afw::image;
namespace math  = lsst::afw::math;
namespace ex    = lsst::pex::exceptions;


namespace details {


template <typename PixelT>    
void loadVariance(std::vector<PixelT> const &wvector, math::MaskedVector<PixelT> &pixelSet) {
    unsigned int j = 0;
    for (typename std::vector<PixelT>::const_iterator pVec = wvector.begin();
         pVec != wvector.end(); ++pVec) {
        if (*pVec == 0) {
            (*pixelSet.getVariance())(j, 0) = static_cast<image::VariancePixel>(1.0e99);
        } else {
            (*pixelSet.getVariance())(j, 0) = static_cast<image::VariancePixel>(1.0/(*pVec));
        }
        ++j;
    }
}



}


/************************************************************************************************************/
/**
 * A function to compute some statistics of a stack of regular images
 * @relates Statistics
 */
template<typename PixelT>
typename image::Image<PixelT>::Ptr math::statisticsStack(
        std::vector<typename image::Image<PixelT>::Ptr > &images,  
        math::Property flags,               
        math::StatisticsControl const& sctrl,
        std::vector<PixelT> const &wvector
                                                        ) {

    // create the image to be returned
    typedef image::Image<PixelT> ImageT;
    typename ImageT::Ptr imgStack(new ImageT(images[0]->getDimensions(), 0.0));


    // if we're going to use contant weights
    if ( wvector.size() == images.size() ) {

        // set the mask to be an infinite iterator
        MaskImposter<image::MaskPixel> msk;
        // copy the weights in to the variance vector
        math::MaskedVector<typename ImageT::Pixel> pixelSet(images.size());
        details::loadVariance(wvector, pixelSet);
        
        // get the desired statistic
        for (int y = 0; y != imgStack->getHeight(); ++y) {
            for (int x = 0; x != imgStack->getWidth(); ++x) {
                for (unsigned int i = 0; i != images.size(); ++i) {
                    (*pixelSet.getImage())(i, 0) = (*images[i])(x, y);
                }
                math::Statistics stat = math::makeStatistics(*pixelSet.getImage(),
                                                             msk,
                                                             *pixelSet.getVariance(),
                                                             flags, sctrl);
                (*imgStack)(x, y) = stat.getValue(flags);
            }
        }


    // if we're going to weight by the pixel variance
    } else if ( wvector.size() == 0 ) {

        
        std::vector<PixelT> pixelSet(images.size()); // values from a given pixel of each image
        
        // get the desired statistic
        for (int y = 0; y != imgStack->getHeight(); ++y) {
            for (int x = 0; x != imgStack->getWidth(); ++x) {
                for (unsigned int i = 0; i != images.size(); ++i) {
                    pixelSet[i] = (*images[i])(x, y);
                }
                (*imgStack)(x, y) = math::makeStatistics(pixelSet, flags, sctrl).getValue(flags);
            }
        }

    // Fail if the number weights isn't the same as the number of images to be weighted.
    } else {
        throw LSST_EXCEPT(ex::InvalidParameterException,
                          "Weight vector must have same length as number of Images to be stacked.");
    }

    
    return imgStack;
}

/**
 * A function to compute some statistics of a stack of Masked Images
 * @relates Statistics
 */
template<typename PixelT>
typename image::MaskedImage<PixelT>::Ptr math::statisticsStack(
        std::vector<typename image::MaskedImage<PixelT>::Ptr > &images,
        math::Property flags,               
        math::StatisticsControl const& sctrl,
        std::vector<PixelT> const &wvector
                                                              ) {

    // create the image to be returned
    typedef image::MaskedImage<PixelT> ImageT;
    typename ImageT::Ptr imgStack(new ImageT(images[0]->getDimensions()));


    // if we're going to use constant weights 
    if ( wvector.size() == images.size() ) {
        
        math::MaskedVector<PixelT> pixelSet(images.size());
        
        // copy the weights in to the variance vector
        details::loadVariance(wvector, pixelSet);

        // collect elements from the stack into the MaskedVector to do stats
        // get the desired statistic
        typedef typename image::MaskedImage<PixelT>::x_iterator x_iterator;

        std::vector<x_iterator> rows;
        rows.reserve(images.size());

        for (int y = 0; y != imgStack->getHeight(); ++y) {
            for (unsigned int i = 0; i < images.size(); ++i) {
                x_iterator ptr = images[i]->row_begin(y);
                if (y == 0) {
                    rows.push_back(ptr);
                } else {
                    rows[i] = ptr;
                }
            }
            
            for (x_iterator ptr = imgStack->row_begin(y), end = imgStack->row_end(y); ptr != end; ++ptr) {
                typename math::MaskedVector<PixelT>::iterator psPtr = pixelSet.begin();
                image::MaskPixel msk(0x0);
                for (unsigned int i = 0; i < images.size(); ++i, ++psPtr) {
                    image::MaskPixel mskTmp = rows[i].mask();
                    psPtr.value() = rows[i].image();
                    psPtr.mask()  = mskTmp;
                    msk |= mskTmp;

                    ++rows[i];
                }
                math::Statistics stat = math::makeStatistics(pixelSet, flags, sctrl);
                
                *ptr = typename image::MaskedImage<PixelT>::Pixel(stat.getValue(),
                                                                  msk,
                                                                  stat.getError()*stat.getError());
            }
        }
        
        
    // if we're weighting by the pixel variance        
    } else if ( wvector.size() == 0 ) {
        
        // get the desired statistic
        for (int y = 0; y != imgStack->getHeight(); ++y) {
            for (int x = 0; x != imgStack->getWidth(); ++x) {
                image::MaskPixel msk = 0x0;
            
                math::MaskedVector<PixelT> pixelSet(images.size());
                for (unsigned int i = 0; i < images.size(); ++i) {
                    image::MaskPixel mskTmp = (*images[i]->getMask())(x, y);
                    (*pixelSet.getImage())(i, 0)     = (*images[i]->getImage())(x, y);
                    (*pixelSet.getMask())(i, 0)      = mskTmp;
                    (*pixelSet.getVariance())(i, 0)  = (*images[i]->getVariance())(x, y);
                    msk |= mskTmp;
                }
                math::Statistics stat = math::makeStatistics(pixelSet, flags, sctrl);
                (*imgStack->getImage())(x, y)    = stat.getValue(flags);
                (*imgStack->getMask())(x, y)     = msk;
                (*imgStack->getVariance())(x, y) = stat.getError(flags)*stat.getError(flags);
            }
        }

    // Fail if the number weights isn't the same as the number of images to be weighted.
    } else {
        throw LSST_EXCEPT(ex::InvalidParameterException,
                          "Weight vector must have same length as number of MaskedImages to be stacked.");
    }

    
    return imgStack;
}




/**
 * A function to compute some statistics of a stack of vectors
 * @relates Statistics
 */
template<typename PixelT>
typename boost::shared_ptr<std::vector<PixelT> > math::statisticsStack(
        std::vector<boost::shared_ptr<std::vector<PixelT> > > &vectors,  
        math::Property flags,               
        math::StatisticsControl const& sctrl,
        std::vector<PixelT> const &wvector
                                                                      ) {

    // create the image to be returned
    typedef std::vector<PixelT> VectT;
    typename boost::shared_ptr<VectT> vecStack(new VectT(vectors[0]->size(), 0.0));

    // Use constant weights for each layer
    if ( wvector.size() == vectors.size() ) {

        math::MaskedVector<PixelT> pixelSet(vectors.size()); // values from a given pixel of each image

        // set the mask to be an infinite iterator
        MaskImposter<image::MaskPixel> msk;
        // copy the weights in to the variance vector
        details::loadVariance(wvector, pixelSet);
        
        // collect elements from the stack into the MaskedVector to do stats
        for (unsigned int x = 0; x < vectors[0]->size(); ++x) {
            for (unsigned int i = 0; i < vectors.size(); ++i) {
                (*pixelSet.getImage())(i, 0)     = (*vectors[i])[x];
            }
            math::Statistics stat = math::makeStatistics(*pixelSet.getImage(),
                                                         msk,
                                                         *pixelSet.getVariance(),
                                                         flags, sctrl);
            (*vecStack)[x] = stat.getValue(flags);
        }

    // Use no weights.
    } else if ( wvector.size() == 0 ) {
        std::vector<PixelT> pixelSet(vectors.size()); // values from a given pixel of each image
        // get the desired statistic
        for (unsigned int x = 0; x < vectors[0]->size(); ++x) {
            for (unsigned int i = 0; i != vectors.size(); ++i) {
                pixelSet[i] = (*vectors[i])[x];
            }
            (*vecStack)[x] = math::makeStatistics(pixelSet, flags, sctrl).getValue(flags);
        }

    // Fail if the number weights isn't the same as the number of vectors to be weighted.
    } else {
        throw LSST_EXCEPT(ex::InvalidParameterException,
                          "Weight vector must have same length as number of vectors to be stacked.");
    }
    
    return vecStack;
}




/*
 * Explicit Instantiations
 *
 */
#define INSTANTIATE_STACKS(TYPE) \
    template image::Image<TYPE>::Ptr math::statisticsStack<TYPE>(       \
            std::vector<image::Image<TYPE>::Ptr > &images, \
            lsst::afw::math::Property flags, \
            lsst::afw::math::StatisticsControl const& sctrl,    \
            std::vector<TYPE> const &wvector);                          \
    template image::MaskedImage<TYPE>::Ptr math::statisticsStack<TYPE>( \
            std::vector<image::MaskedImage<TYPE>::Ptr > &images, \
            lsst::afw::math::Property flags, \
            lsst::afw::math::StatisticsControl const& sctrl,    \
            std::vector<TYPE> const &wvector);                          \
    template boost::shared_ptr<std::vector<TYPE> > math::statisticsStack<TYPE>( \
            std::vector<boost::shared_ptr<std::vector<TYPE> > > &vectors, \
            lsst::afw::math::Property flags, \
            lsst::afw::math::StatisticsControl const& sctrl,    \
            std::vector<TYPE> const &wvector);

INSTANTIATE_STACKS(double);
INSTANTIATE_STACKS(float);
