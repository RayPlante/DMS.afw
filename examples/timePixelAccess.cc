#include <iostream>
#include <sstream>
#include <ctime>

#include "lsst/afw/image.h"

namespace afwImage = lsst::afw::image;

template<class ImageT>
void timePixelAccess(ImageT const &image, int nIter) {
    std::cout << "Accessor Type\tCols\tRows\tMPix\tSecPerIter\tSecPerIterPerMPix" << std::endl;
    
    const int nCols = image.getWidth();
    const int nRows = image.getHeight();
    
    const typename ImageT::SinglePixel pix(1.0);

    clock_t startTime = clock();
    for (int iter = 0; iter < nIter; ++iter) {
        for (int y = 0; y < image.getHeight(); ++y) {
            for (typename ImageT::x_iterator ptr = image.row_begin(y), end = image.row_end(y); ptr != end; ++ptr) {
                *ptr += pix;
            }
        }
    }
    double secPerIter = (clock() - startTime) / static_cast<double> (nIter * CLOCKS_PER_SEC);
    double megaPix = static_cast<double>(nCols * nRows) / 1.0e6;
    double secPerMPixPerIter = secPerIter / static_cast<double>(megaPix);
    std::cout << "Pixel Iterator\t" << nCols << "\t" << nRows << "\t" << megaPix << "\t" << secPerIter << "\t\t" << secPerMPixPerIter << std::endl;

    startTime = clock();
    for (int iter = 0; iter < nIter; ++iter) {
        for (int y = 0; y < image.getHeight(); ++y) {
            for (typename ImageT::xy_locator ptr = image.xy_at(0, y), end = image.xy_at(nCols, y); ptr != end; ++ptr.x()) {
                *ptr += pix;
            }
        }
    }
    secPerIter = (clock() - startTime) / static_cast<double> (nIter * CLOCKS_PER_SEC);
    secPerMPixPerIter = secPerIter / static_cast<double>(megaPix);
    std::cout << "Pixel Locator\t" << nCols << "\t" << nRows << "\t" << megaPix << "\t" << secPerIter << "\t\t" << secPerMPixPerIter << std::endl;
}

int main(int argc, char **argv) {
    typedef float imageType;

    int const DefNIter = 100;
    int const DefNCols = 1024;

    if ((argc == 2) && (argv[1][0] == '-')) {
        std::cout << "Usage: timePixelAccess [nIter [nCols [nRows]]]" << std::endl;
        std::cout << "nIter (default " << DefNIter << ") is the number of iterations" << std::endl;
        std::cout << "nCols (default " << DefNCols << ") is the number of columns" << std::endl;
        std::cout << "nRows (default = nCols) is the number of rows" << std::endl;
        return 1;
    }
    
    int nIter = DefNIter;
    if (argc > 1) {
        std::istringstream(argv[1]) >> nIter;
    }
    int nCols = DefNCols;
    if (argc > 2) {
        std::istringstream(argv[2]) >> nCols;
    }
    int nRows = nCols;
    if (argc > 3) {
        std::istringstream(argv[3]) >> nRows;
    }
    
      
    std::cout << "Image(" << nCols << ", " << nRows << ")" << std::endl;
    afwImage::Image<imageType> image(nCols, nRows);
    timePixelAccess(image, nIter);
    
    std::cout << "MaskedImage(" << nCols << ", " << nRows << ")" << std::endl;
    afwImage::MaskedImage<imageType> maskedImage(nCols, nRows);
    timePixelAccess(maskedImage, nIter);
}
