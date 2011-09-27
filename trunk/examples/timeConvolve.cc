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

typedef float ImageType;
typedef double KernelType;

const double Sigma = 3;
const unsigned DefNIter = 10;
const unsigned MinKernelSize = 5;
const unsigned MaxKernelSize = 15;
const unsigned DeltaKernelSize = 5;

template <class ImageClass>
void timeConvolution(ImageClass &image, unsigned int nIter) {
    typedef double KernelType;

    unsigned imWidth = image.getWidth();
    unsigned imHeight = image.getHeight();

    ImageClass resImage(image.getDimensions());

    std::cout << std::endl << "Analytic Kernel" << std::endl;
    std::cout << "ImWid\tImHt\tKerWid\tKerHt\tMOps\tCnvSec\tMOpsPerSec" << std::endl;
    
    for (unsigned kSize = MinKernelSize; kSize <= MaxKernelSize; kSize += DeltaKernelSize) {
        // construct kernel
        afwMath::GaussianFunction2<KernelType> gaussFunc(Sigma, Sigma, 0);
        afwMath::AnalyticKernel analyticKernel(kSize, kSize, gaussFunc);
        
        clock_t startTime = clock();
        for (unsigned int iter = 0; iter < nIter; ++iter) {
            // convolve
            afwMath::convolve(resImage, image, analyticKernel, true);
        }
        double secPerIter = (clock() - startTime) / static_cast<double> (nIter * CLOCKS_PER_SEC);
        
        double mOps = static_cast<double>(
            (imHeight + 1 - kSize) * (imWidth + 1 - kSize) * kSize * kSize) / 1.0e6;
        double mOpsPerSec = mOps / secPerIter;
        std::cout << imWidth << "\t" << imHeight << "\t" << kSize << "\t" << kSize << "\t" << mOps
            << "\t" << secPerIter << "\t" << mOpsPerSec << std::endl;
    }

    std::cout << std::endl << "Separable Kernel" << std::endl;
    std::cout << "ImWid\tImHt\tKerWid\tKerHt\tMOps\tCnvSec\tMOpsPerSec" << std::endl;
    
    for (unsigned kSize = MinKernelSize; kSize <= MaxKernelSize; kSize += DeltaKernelSize) {
        // construct kernel
        afwMath::GaussianFunction1<KernelType> gaussFunc(Sigma);
        afwMath::SeparableKernel separableKernel(kSize, kSize, gaussFunc, gaussFunc);
        
        clock_t startTime = clock();
        for (unsigned int iter = 0; iter < nIter; ++iter) {
            // convolve
            afwMath::convolve(resImage, image, separableKernel, true);
        }
        double secPerIter = (clock() - startTime) / static_cast<double> (nIter * CLOCKS_PER_SEC);
        
        double mOps = static_cast<double>((
            imHeight + 1 - kSize) * (imWidth + 1 - kSize) * kSize * kSize) / 1.0e6;
        double mOpsPerSec = mOps / secPerIter;
        std::cout << imWidth << "\t" << imHeight << "\t" << kSize << "\t" << kSize << "\t" << mOps
            << "\t" << secPerIter << "\t" << mOpsPerSec << std::endl;
    }
}

int main(int argc, char **argv) {


    std::string mimg;
    if (argc < 2) {
        std::string afwdata = getenv("AFWDATA_DIR");
        if (afwdata.empty()) {
            std::cout << "Time convolution with a spatially invariant kernel" << std::endl << std::endl;
            std::cout << "Usage: timeConvolve fitsFile [nIter]" << std::endl;
            std::cout << "fitsFile excludes the \"_img.fits\" suffix" << std::endl;
            std::cout << "nIter (default " << DefNIter
                      << ") is the number of iterations per kernel size" << std::endl;
            std::cout << "Kernel size ranges from " << MinKernelSize << " to " << MaxKernelSize
                      << " in steps of " << DeltaKernelSize << " pixels on a side" << std::endl;
            std::cerr << "I can take a default file from AFWDATA_DIR, but it's not defined." << std::endl;
            std::cerr << "Is afwdata set up?\n" << std::endl;
            exit(EXIT_FAILURE);
        } else {
            mimg = afwdata + "/small_MI";
            std::cerr << "Using " << mimg << std::endl;
        }
    } else {
        mimg = std::string(argv[1]);
    }

    unsigned int nIter = DefNIter;
    if (argc > 2) {
        std::istringstream(argv[2]) >> nIter;
    }

    std::cout << "Timing convolution with a spatially invariant kernel" << std::endl;
    std::cout << "Columns:" << std::endl;
    std::cout << "* MOps: the number of operations of a kernel pixel on a masked pixel / 10e6." << std::endl;
    std::cout << "  One operation includes the all of the following:" << std::endl;
    std::cout << "  * two multiplies and two additions (one image, one for variance)," << std::endl;
    std::cout << "  * one OR (for the mask)" << std::endl;
    std::cout << "  * four pixel pointer increments (for image, variance, mask and kernel)" << std::endl;
    std::cout << "* CnvSec: time to perform one convolution (sec)" << std::endl;

    std::string maskedImagePath(mimg);
    std::string imagePath = maskedImagePath + "_img.fits";

    std::cout << std::endl << "Image " << imagePath << std::endl;
    afwImage::Image<ImageType> image(imagePath);
    timeConvolution(image, nIter);
    
    std::cout << std::endl << "MaskedImage " << maskedImagePath << std::endl;
    afwImage::MaskedImage<ImageType> mImage(mimg);
    timeConvolution(mImage, nIter);
}
