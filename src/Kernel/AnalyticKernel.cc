// -*- LSST-C++ -*-
/**
 * \file
 *
 * \brief Definitions of AnalyticKernel member functions and explicit instantiations of the class.
 *
 * \author Russell Owen
 *
 * \ingroup fw
 */
#include <vw/Image.h>

#include <lsst/mwi/exceptions.h>
#include <lsst/fw/Kernel.h>

// This file is meant to be included by lsst/fw/Kernel.h

/**
 * \brief Construct an empty spatially invariant AnalyticKernel of size 0x0
 */
lsst::fw::AnalyticKernel::AnalyticKernel()
:
    Kernel(),
    _kernelFunctionPtr()
{}

/**
 * \brief Construct a spatially invariant AnalyticKernel
 */
lsst::fw::AnalyticKernel::AnalyticKernel(
    Kernel::KernelFunctionPtrType kernelFunction,
    unsigned int cols,
    unsigned int rows)
:
    Kernel(cols, rows, kernelFunction->getNParameters()),
    _kernelFunctionPtr(kernelFunction)
{}

/**
 * \brief Construct a spatially varying AnalyticKernel with spatial coefficients initialized to 0
 */
lsst::fw::AnalyticKernel::AnalyticKernel(
    Kernel::KernelFunctionPtrType kernelFunction,
    unsigned int cols,
    unsigned int rows,
    Kernel::SpatialFunctionPtrType spatialFunction)
:
    Kernel(cols, rows, kernelFunction->getNParameters(), spatialFunction),
    _kernelFunctionPtr(kernelFunction)
{}

/**
 * \brief Construct a spatially varying AnalyticKernel with the spatially varying parameters specified
 *
 * See setSpatialParameters for the form of the spatial parameters.
 */
lsst::fw::AnalyticKernel::AnalyticKernel(
    Kernel::KernelFunctionPtrType kernelFunction,
    unsigned int cols,
    unsigned int rows,
    Kernel::SpatialFunctionPtrType spatialFunction,
    std::vector<std::vector<double> > const &spatialParameters)
:
    Kernel(cols, rows, kernelFunction->getNParameters(), spatialFunction, spatialParameters),
    _kernelFunctionPtr(kernelFunction)
{}

void lsst::fw::AnalyticKernel::computeImage(
    Image<PixelT> &image,
    PixelT &imSum,
    double x,
    double y,
    bool doNormalize
) const {
    typedef Image<PixelT>::pixel_accessor pixelAccessor;
    if ((image.getCols() != this->getCols()) || (image.getRows() != this->getRows())) {
        throw lsst::mwi::exceptions::InvalidParameter("image is the wrong size");
    }
    if (this->isSpatiallyVarying()) {
        std::vector<double> kernelParams(this->getNKernelParameters());
        this->computeKernelParametersFromSpatialModel(kernelParams, x, y);
        this->basicSetKernelParameters(kernelParams);
    }
    pixelAccessor imRow = image.origin();
    double xOffset = - static_cast<double>(this->getCtrCol());
    double yOffset = - static_cast<double>(this->getCtrRow());
    imSum = 0;
    for (unsigned int row = 0; row < this->getRows(); ++row, imRow.next_row()) {
        double y = static_cast<double>(row) + yOffset;
        pixelAccessor imCol = imRow;
        for (unsigned int col = 0; col < this->getCols(); ++col, imCol.next_col()) {
            double x = static_cast<double>(col) + xOffset;
            PixelT pixelVal = (*_kernelFunctionPtr)(x, y);
            *imCol = pixelVal;
            imSum += pixelVal;
        }
    }
    if (doNormalize) {
        image /= imSum;
        imSum = 1;
    }
}

/**
 * \brief Get the kernel function
 */
lsst::fw::Kernel::KernelFunctionPtrType lsst::fw::AnalyticKernel::getKernelFunction(
) const {
    return _kernelFunctionPtr;
}

std::vector<double> lsst::fw::AnalyticKernel::getCurrentKernelParameters() const {
    return _kernelFunctionPtr->getParameters();
}

//
// Protected Member Functions
//

void lsst::fw::AnalyticKernel::basicSetKernelParameters(std::vector<double> const &params) const {
    _kernelFunctionPtr->setParameters(params);
}
