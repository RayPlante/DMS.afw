#include <iostream>

#include "lsst/afw/geom.h"
#include "lsst/afw/image.h"
#include "lsst/afw/math.h"

namespace afwGeom = lsst::afw::geom;
namespace afwImage = lsst::afw::image;
namespace afwMath = lsst::afw::math;

int main(int argc, char **argv) {
    typedef float imageType;
    typedef double kernelType;
    double minSigma = 0.1;
    double maxSigma = 3.0;
    const unsigned int kSize = 9;

    // construct kernel
    afwMath::GaussianFunction2<kernelType> gaussFunc(1, 1, 0);
    unsigned int polyOrder = 1;
    afwMath::PolynomialFunction2<double> polyFunc(polyOrder);
    afwMath::AnalyticKernel::Ptr gaussSpVarKernelPtr(
        new afwMath::AnalyticKernel(kSize, kSize, gaussFunc, polyFunc));

    // get copy of spatial parameters (all zeros), set and feed back to the kernel
    std::vector<std::vector<double> > polyParams = gaussSpVarKernelPtr->getSpatialParameters();
    polyParams[0][0] = minSigma;
    polyParams[0][1] = (maxSigma - minSigma) / static_cast<double>(100);
    polyParams[0][2] = 0.0;
    polyParams[1][0] = minSigma;
    polyParams[1][1] = 0.0;
    polyParams[1][2] = (maxSigma - minSigma) / static_cast<double>(100);
    gaussSpVarKernelPtr->setSpatialParameters(polyParams);

    afwGeom::BoxI bbox(afwGeom::Point2I::make(10, 20), afwGeom::Extent2I::make(50, 75));
    
    afwMath::detail::KernelImagesForRegion kernelImageSet(gaussSpVarKernelPtr, bbox, false);

    afwMath::detail::KernelImagesForRegion::ImageConstPtr imPtr = 
        kernelImageSet.getImage(afwMath::detail::KernelImagesForRegion::CENTER);

    std::cout << "image size=" << imPtr->getWidth() << " x " << imPtr->getHeight() << std::endl;
}