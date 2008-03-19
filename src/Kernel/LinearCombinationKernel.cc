// -*- LSST-C++ -*-
/**
 * \file
 *
 * \brief Definitions of LinearCombinationKernel member functions and explicit instantiations of the class.
 *
 * \author Russell Owen
 *
 * \ingroup fw
 */
#include <stdexcept>

#include <boost/format.hpp>
#include <vw/Image.h>

#include <lsst/mwi/exceptions.h>
#include <lsst/fw/Kernel.h>

// This file is meant to be included by lsst/fw/Kernel.h

/**
 * \brief Construct an empty LinearCombinationKernel of size 0x0
 */
lsst::fw::LinearCombinationKernel::LinearCombinationKernel()
:
    Kernel(),
    _kernelList(),
    _kernelImagePtrList(),
    _kernelParams()
{ }

/**
 * \brief Construct a spatially invariant LinearCombinationKernel
 */
lsst::fw::LinearCombinationKernel::LinearCombinationKernel(
    KernelListType const &kernelList,    ///< list of (shared pointers to) kernels
    std::vector<double> const &kernelParameters) ///< kernel coefficients
:
    Kernel(kernelList[0]->getCols(), kernelList[0]->getRows(), kernelList.size()),
    _kernelList(kernelList),
    _kernelImagePtrList(),
    _kernelParams(kernelParameters)
{
    checkKernelList(kernelList);
    _computeKernelImageList();
}

/**
 * \brief Construct a spatially varying LinearCombinationKernel with spatial parameters initialized to 0
 */
lsst::fw::LinearCombinationKernel::LinearCombinationKernel(
    KernelListType const &kernelList,    ///< list of (shared pointers to) kernels
    Kernel::SpatialFunctionPtrType spatialFunction)  ///< spatial function
:
    Kernel(kernelList[0]->getCols(), kernelList[0]->getRows(), kernelList.size(), spatialFunction),
    _kernelList(kernelList),
    _kernelImagePtrList(),
    _kernelParams(std::vector<double>(kernelList.size()))
{
    checkKernelList(kernelList);
    _computeKernelImageList();
}

/**
 * \brief Construct a spatially varying LinearCombinationKernel with the spatially varying parameters specified
 *
 * See setSpatialParameters for the form of the spatial parameters.
 */
lsst::fw::LinearCombinationKernel::LinearCombinationKernel(
    KernelListType const &kernelList,    ///< list of (shared pointers to) kernels
    Kernel::SpatialFunctionPtrType spatialFunction,  ///< spatial function
    std::vector<std::vector<double> > const &spatialParameters)  ///< spatial coefficients
:
    Kernel(kernelList[0]->getCols(), kernelList[0]->getRows(),
        kernelList.size(), spatialFunction, spatialParameters),
    _kernelList(kernelList),
    _kernelImagePtrList(),
    _kernelParams(std::vector<double>(kernelList.size()))
{
    checkKernelList(kernelList);
    _computeKernelImageList();
}

void lsst::fw::LinearCombinationKernel::computeImage(
    Image<PixelT> &image,
    PixelT &imSum,
    double x,
    double y,
    bool doNormalize
) const {
    if ((image.getCols() != this->getCols()) || (image.getRows() != this->getRows())) {
        throw lsst::mwi::exceptions::InvalidParameter("image is the wrong size");
    }
    if (this->isSpatiallyVarying()) {
        this->computeKernelParametersFromSpatialModel(this->_kernelParams, x, y);
    }
    Image<PixelT>::ImageIVwPtrT vwImagePtr = image.getIVwPtr();
    
    bool isFirst = true;
    std::vector<boost::shared_ptr<Image<PixelT> > >::const_iterator
        kImPtrIter = _kernelImagePtrList.begin();
    std::vector<double>::const_iterator kParIter = _kernelParams.begin();
    for ( ; kImPtrIter != _kernelImagePtrList.end(); ++kImPtrIter, ++kParIter, isFirst = false) {
        if (isFirst) {
            *vwImagePtr = (*((**kImPtrIter).getIVwPtr())) * static_cast<PixelT>(*kParIter);
        } else {
            *vwImagePtr += (*((**kImPtrIter).getIVwPtr())) * static_cast<PixelT>(*kParIter);
        }
    }
    imSum = vw::sum_of_channel_values(*vwImagePtr);
    if (doNormalize) {
        image /= imSum;
        imSum = 1;
    }
}

/**
 * \brief Get the fixed basis kernels
 */
lsst::fw::LinearCombinationKernel::KernelListType const &
lsst::fw::LinearCombinationKernel::getKernelList() const {
    return _kernelList;
}
    
/**
 * \brief Check that all kernels have the same size and center and that none are spatially varying
 *
 * \throw lsst::mwi::exceptions::InvalidParameter if the check fails
 */
void lsst::fw::LinearCombinationKernel::checkKernelList(const KernelListType &kernelList) const {
    if (kernelList.size() < 1) {
        throw lsst::mwi::exceptions::InvalidParameter("kernelList has no elements");
    }

    unsigned int cols = kernelList[0]->getCols();
    unsigned int rows = kernelList[0]->getRows();
    unsigned int ctrCol = kernelList[0]->getCtrCol();
    unsigned int ctrRow = kernelList[0]->getCtrRow();
    
    for (unsigned int ii = 0; ii < kernelList.size(); ii++) {
        if (ii > 0) {
            if ((cols != kernelList[ii]->getCols()) ||
                (rows != kernelList[ii]->getRows())) {
                throw lsst::mwi::exceptions::InvalidParameter(
                    boost::format("kernel %d has different size than kernel 0") % ii);
            }
            if ((ctrCol != kernelList[ii]->getCtrCol()) ||
                (ctrRow != kernelList[ii]->getCtrRow())) {
                throw lsst::mwi::exceptions::InvalidParameter(
                    boost::format("kernel %d has different center than kernel 0") % ii);
            }
        }
        if (kernelList[ii]->isSpatiallyVarying()) {
            throw lsst::mwi::exceptions::InvalidParameter(
                boost::format("kernel %d is spatially varying") % ii);
        }
    }
}

std::vector<double> lsst::fw::LinearCombinationKernel::getCurrentKernelParameters() const {
    return _kernelParams;
}

//
// Protected Member Functions
//
void lsst::fw::LinearCombinationKernel::basicSetKernelParameters(std::vector<double> const &params)
const {
    if (params.size() != this->_kernelList.size()) {
        throw lsst::mwi::exceptions::InvalidParameter(
            boost::format("params size is %d instead of %d\n") % params.size() % _kernelList.size());
    }
    this->_kernelParams = params;
}

//
// Private Member Functions
//

/**
 * Compute _kernelImagePtrList, the internal archive of kernel images.
 */
void lsst::fw::LinearCombinationKernel::_computeKernelImageList() {
    KernelListType::const_iterator kIter = _kernelList.begin();
    std::vector<double>::const_iterator kParIter = _kernelParams.begin();
    for ( ; kIter != _kernelList.end(); ++kIter) {
        PixelT kSum;
        boost::shared_ptr<lsst::fw::Image<PixelT> >
            kernelImagePtr(new lsst::fw::Image<PixelT>(this->getCols(), this->getRows()));
        (*kIter)->computeImage(*kernelImagePtr, kSum, 0, 0, false);
        _kernelImagePtrList.push_back(kernelImagePtr);
    }
}
