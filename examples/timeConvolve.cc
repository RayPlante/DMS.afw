#include <iostream>
#include <sstream>
#include <ctime>

#include "lsst/afw/math/FunctionLibrary.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/MaskedImage.h"
#include "lsst/afw/math/Kernel.h"
#include "lsst/afw/math/KernelFunctions.h"

namespace afwImage = lsst::afw::image;
namespace afwMath = lsst::afw::math;

int main(int argc, char **argv) {
    typedef float imageType;
    typedef double kernelType;
    double sigma = 3;
    const int EdgeMaskBit = -1;
    const unsigned DefNIter = 10;
    const unsigned MinKernelSize = 5;
    const unsigned MaxKernelSize = 15;
    const unsigned DeltaKernelSize = 5;

    if (argc < 2) {
        std::cout << "Usage: timeConvolve fitsFile [nIter]" << std::endl;
        std::cout << "fitsFile excludes the \"_img.fits\" suffix" << std::endl;
        std::cout << "nIter (default " << DefNIter << ") is the number of iterations per kernel size" << std::endl;
        std::cout << "Kernel size ranges from " << MinKernelSize << " to " << MaxKernelSize
            << " in steps of " << DeltaKernelSize << " pixels on a side" << std::endl;
        return 1;
    }
    
    unsigned nIter = DefNIter;
    if (argc > 2) {
        std::istringstream(argv[2]) >> nIter;
    }
    
    // read in fits file
    afwImage::MaskedImage<imageType> mImage(argv[1]);
    
    unsigned imWidth = mImage.getWidth();
    unsigned imHeight = mImage.getHeight();
    
    std::cout << "Timing convolution for a " << imWidth << "x" << imHeight << " image." << std::endl;
    std::cout << std::endl;
    std::cout << "Columns:" << std::endl;
    std::cout << "* MOps: the number of operations of a kernel pixel on a masked pixel / 10e6." << std::endl;
    std::cout << "  One operation includes the all of the following:" << std::endl;
    std::cout << "  * two multiplies and two additions (one image, one for variance)," << std::endl;
    std::cout << "  * one OR (for the mask)" << std::endl;
    std::cout << "  * four pixel pointer increments (for image, variance, mask and kernel)" << std::endl;
    std::cout << "* CnvSec: time to perform one convolution (sec)" << std::endl;
    std::cout << std::endl;
    std::cout << "ImWidth\tImHeight\tKerWidth\tKerHeight\tMOps\tCnvSec\tMOpsPerSec" << std::endl;
    
    afwImage::MaskedImage<imageType> resMImage(mImage.dimensions());
    
    for (unsigned kSize = MinKernelSize; kSize <= MaxKernelSize; kSize += DeltaKernelSize) {
        // construct kernel
        afwMath::GaussianFunction2<kernelType> gaussFunc(sigma, sigma);
        afwMath::AnalyticKernel kernel(gaussFunc, kSize, kSize);
        
        clock_t startTime = clock();
        for (unsigned iter = 0; iter < nIter; ++iter) {
            // convolve
            afwMath::convolve(resMImage, mImage, kernel, EdgeMaskBit, true);
        }
        double secPerIter = (clock() - startTime) / static_cast<double> (nIter * CLOCKS_PER_SEC);
        
        double mOps = static_cast<double>((imHeight + 1 - kSize) * (imWidth + 1 - kSize) * kSize * kSize) / 1.0e6;
        double mOpsPerSec = mOps / secPerIter;
        std::cout << imWidth << "\t" << imHeight << "\t" << kSize << "\t" << kSize << "\t" << mOps
            << "\t" << secPerIter << "\t" << mOpsPerSec << std::endl;
    }
}
