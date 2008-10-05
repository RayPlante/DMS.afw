// -*- lsst-c++ -*-
#include <typeinfo>

#include "lsst/daf/base.h"
#include "lsst/daf/data/FitsFormatter.h"
#include "lsst/pex/exceptions.h"
#include "lsst/pex/logging/Trace.h"
#include "lsst/afw/image.h"

using namespace std;
using lsst::pex::logging::Trace;
using lsst::daf::data::FitsFormatter;
namespace pexEx = lsst::pex::exceptions;
namespace image = lsst::afw::image;

int test(int argc, char**argv) {
    if (argc < 5) {
       cerr << "Usage: inputBaseName1 inputBaseName2 outputBaseName1  outputBaseName2" << endl;
       return 1;
    }
    
    Trace::setDestination(cout);
    Trace::setVerbosity(".", 0);
    
    typedef image::MaskedImage<float> MaskedImage;

    //
    // We want to construct the MaskedImage within a try block, so declare a pointer outside
    //
    MaskedImage::Ptr testMaskedImage1;
    try {
        testMaskedImage1 = MaskedImage::Ptr(new MaskedImage(argv[1]));
    } catch (pexEx::ExceptionStack &e) {
        cerr << "Failed to open " << argv[1] << ": " << e.what() << endl;
        return 1;
    }

    *testMaskedImage1->getVariance() = 10.0;
    testMaskedImage1->getMask()->addMaskPlane("CR");
    
    // verify that copy constructor and operator= build and do not leak
    MaskedImage::Image testImage(100, 100);
    MaskedImage::Image imageCopy(testImage);
    imageCopy = testImage;

    MaskedImage testMaskedImage2(testMaskedImage1->dimensions()); // n.b. could just do a deep copy
    testMaskedImage2 = *testMaskedImage1;

    MaskedImage::Ptr testFlat;
    try {
        testFlat = MaskedImage::Ptr(new MaskedImage(argv[2]));
    } catch (pexEx::ExceptionStack &e) {
        cerr << "Failed to open " << argv[2] << ": " << e.what() << endl;
        return 1;
    }
    *testFlat->getVariance() = 20.0;

    *testFlat /= 20000.0;

    testMaskedImage2 *= *testFlat;

    // test of fits write

    testMaskedImage2.writeFits(argv[3]);

    // test of subImage

    image::Bbox region(image::PointI(100, 600), 200, 300);
    MaskedImage subMaskedImage1 = MaskedImage(*testMaskedImage1, region);
    subMaskedImage1 *= 0.5;
    subMaskedImage1.writeFits(argv[4]);

    // Check whether offsets have been correctly saved
    image::Bbox region2(image::PointI(80, 110), 20, 30);
    MaskedImage subMaskedImage2 = MaskedImage(subMaskedImage1, region2);

    cout << "Offsets: " << subMaskedImage2.getX0() << " " << subMaskedImage2.getY0() << endl;

    testMaskedImage1->writeFits(argv[3]);
    
    return 0;
}

int main(int argc, char **argv) {
    try {
       try {
           test(argc, argv);
       } catch (pexEx::ExceptionStack &e) {
           throw pexEx::Runtime(std::string("In handler\n") + e.what());
       }
    } catch (pexEx::ExceptionStack &e) {
       clog << e.what() << endl;
    }

    //
    // Check for memory leaks
    //
    if (lsst::daf::base::Citizen::census(0) == 0) {
        cerr << "No leaks detected" << endl;
    } else {
        cerr << "Leaked memory blocks:" << endl;
        lsst::daf::base::Citizen::census(cerr);
    }
}
