#include <iostream>
#include <sstream>
#include <string>

#include "lsst/daf/base.h"
#include "lsst/pex/logging/Trace.h"
#include "lsst/afw/image.h"
#include "lsst/afw/math.h"

using namespace std;
const std::string outFile("clOut");
const std::string altOutFile("clAltOut");

int main(int argc, char **argv) {
    lsst::pex::logging::Trace::setDestination(std::cout);
    lsst::pex::logging::Trace::setVerbosity("lsst.afw.kernel", 5);

    typedef float imagePixelType;
    unsigned int KernelCols = 5;
    unsigned int KernelRows = 8;
    double MinSigma = 1.5;
    double MaxSigma = 2.5;
    
    if (argc < 2) {
        std::cerr << "Usage: linearConvolve fitsFile [doBothWays]" << std::endl;
        std::cerr << "fitsFile excludes the \"_img.fits\" suffix" << std::endl;
        std::cerr << "doBothWays (default 0); if 1 then also compute using the normal convolve function"
            << std::endl;
        return 1;
    }
    
    { // block in which to allocate and deallocate memory
    
        bool doBothWays = 0;
        if (argc > 3) {
            std::istringstream(argv[3]) >> doBothWays;
        }
        
        // read in fits file
        lsst::afw::image::MaskedImage<imagePixelType> mImage(argv[1]);
        
        // construct basis kernels
        lsst::afw::math::KernelList<> kernelVec;
        for (int ii = 0; ii < 3; ++ii) {
            double xSigma = (ii == 1) ? MaxSigma : MinSigma;
            double ySigma = (ii == 2) ? MinSigma : MaxSigma;
            lsst::afw::math::GaussianFunction2<lsst::afw::math::Kernel::Pixel> gaussFunc(xSigma, ySigma);
            lsst::afw::math::Kernel::Ptr basisKernelPtr(
			new lsst::afw::math::AnalyticKernel(KernelCols, KernelRows, gaussFunc)
            );
            kernelVec.push_back(basisKernelPtr);
        }
        
        // construct spatially varying linear combination kernel
        unsigned int polyOrder = 1;
        lsst::afw::math::PolynomialFunction2<double> polyFunc(polyOrder);
        lsst::afw::math::LinearCombinationKernel lcSpVarKernel(kernelVec, polyFunc);
    
        // Get copy of spatial parameters (all zeros), set and feed back to the kernel
        vector<vector<double> > polyParams = lcSpVarKernel.getSpatialParameters();
        // Set spatial parameters for basis kernel 0
        polyParams[0][0] =  1.0;
        polyParams[0][1] = -0.5 / static_cast<double>(mImage.getWidth());
        polyParams[0][2] = -0.5 / static_cast<double>(mImage.getHeight());
        // Set spatial function parameters for basis kernel 1
        polyParams[1][0] = 0.0;
        polyParams[1][1] = 1.0 / static_cast<double>(mImage.getWidth());
        polyParams[1][2] = 0.0;
        // Set spatial function parameters for basis kernel 2
        polyParams[2][0] = 0.0;
        polyParams[2][1] = 0.0;
        polyParams[2][2] = 1.0 / static_cast<double>(mImage.getHeight());
        // Set spatial function parameters for kernel parameter 1
        lcSpVarKernel.setSpatialParameters(polyParams);
    
        // convolve
        lsst::afw::image::MaskedImage<imagePixelType> resMaskedImage(mImage.getDimensions());
        lsst::afw::math::convolve(resMaskedImage, mImage, lcSpVarKernel, false);
        
        // write results
        resMaskedImage.writeFits(outFile);
        std::cout << "Wrote " << outFile << "_img.fits, etc." << std::endl;

        if (doBothWays) {
            lsst::afw::image::MaskedImage<imagePixelType> altResMaskedImage(mImage.getDimensions());
            lsst::afw::math::convolve(altResMaskedImage, mImage, lcSpVarKernel, false);
            altResMaskedImage.writeFits(altOutFile);
            std::cout << "Wrote " << altOutFile << "_img.fits, etc. (using lsst::afw::math::convolve)" << std::endl;
        }
    }

     //
     // Check for memory leaks
     //
     if (lsst::daf::base::Citizen::census(0) != 0) {
         std::cerr << "Leaked memory blocks:" << std::endl;
         lsst::daf::base::Citizen::census(std::cerr);
     }
    
}
