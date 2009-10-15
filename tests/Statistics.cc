#include <iostream>
#include <limits>
#include <cmath>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Statistics

#include "boost/test/unit_test.hpp"
#include "boost/test/floating_point_comparison.hpp"

#include "lsst/afw/image/Image.h"
#include "lsst/afw/math/Statistics.h"

using namespace std;

namespace image = lsst::afw::image;
namespace math = lsst::afw::math;

typedef image::Image<float> Image;
typedef image::DecoratedImage<float> DecoratedImage;

BOOST_AUTO_TEST_CASE(StatisticsBasic) {
    Image img(10, 40);
    Image::Pixel const pixval = 10000;
    img = pixval;

    {
        math::Statistics stats = math::makeStatistics(img, math::NPOINT | math::STDEV | math::MEAN);
        double const mean = stats.getValue(math::MEAN);
        double const dmean = stats.getError(math::MEAN);
        double const sd = stats.getValue(math::STDEV);
        
        BOOST_CHECK_EQUAL(stats.getValue(math::NPOINT), img.getWidth()*img.getHeight());
        BOOST_CHECK_EQUAL(mean, img(0, 0));
        BOOST_CHECK(std::isnan(dmean)); // we didn't ask for the error, so it's a NaN
        BOOST_CHECK_EQUAL(sd, 0);
    }

    {
        math::Statistics stats = math::makeStatistics(img, math::STDEV | math::MEAN | math::ERRORS);
        std::pair<double, double> const mean = stats.getResult(math::MEAN);
        double const sd = stats.getValue(math::STDEV);
        
        BOOST_CHECK_EQUAL(mean.first,  img(0, 0));
        BOOST_CHECK_EQUAL(mean.second, sd/sqrt(img.getWidth()*img.getHeight()));
    }

    {
        math::Statistics stats = math::makeStatistics(img, math::NPOINT);
        BOOST_CHECK_THROW(stats.getValue(math::MEAN), lsst::pex::exceptions::InvalidParameterException);
    }

    // ===============================================================================
    // sjb code for percentiles and clipped stats
    {
        math::Statistics stats = math::makeStatistics(img, math::MEDIAN);
        BOOST_CHECK_EQUAL(pixval, stats.getValue(math::MEDIAN));
    }
    {
        math::Statistics stats = math::makeStatistics(img, math::IQRANGE);
        BOOST_CHECK_EQUAL(0.0, stats.getValue(math::IQRANGE));
    }
    {
        math::Statistics stats = math::makeStatistics(img, math::MEANCLIP);
        BOOST_CHECK_EQUAL(pixval, stats.getValue(math::MEANCLIP));
    }
    {
        math::Statistics stats = math::makeStatistics(img, math::VARIANCECLIP);
        BOOST_CHECK_EQUAL(0.0, stats.getValue(math::VARIANCECLIP));
    }

    
    
    {
        Image img2(img);
        //
        // Add 1 to every other row, so the variance is 1/4
        //
        BOOST_REQUIRE(img.getHeight()%2 == 0);
        for (int y = 1; y < img.getHeight(); y += 2) {
            for (Image::x_iterator ptr = img.row_begin(y), end = ptr + img.getWidth(); ptr != end; ++ptr) {
                *ptr += 1;
            }
        }

        math::Statistics stats =
            math::makeStatistics(img2, math::NPOINT | math::STDEV | math::MEAN | math::ERRORS);
        std::pair<double, double> const mean = stats.getResult(math::MEAN);
        double const n = stats.getValue(math::NPOINT);
        double const sd = stats.getValue(math::STDEV);
        
        BOOST_CHECK_EQUAL(mean.first,  img(0, 0) + 0.5);
        BOOST_CHECK_EQUAL(sd, 1/sqrt(4.0)*sqrt(n/(n - 1)));
        BOOST_CHECK_CLOSE(mean.second, sd/sqrt(img.getWidth()*img.getHeight()), 1e-10);
    }

}




BOOST_AUTO_TEST_CASE(StatisticsRamp) {

    int nx = 101;
    int ny = 64;
    Image img(nx, ny);
    
    double z0 = 10.0;
    double dzdx = 1.0;
    double mean = z0 + (nx/2)*dzdx;
    double stdev = 0.0;
    for (int i_y=0; i_y < ny; ++i_y) {
        double x = 0;
        for (Image::x_iterator ptr=img.row_begin(i_y); ptr != img.row_end(i_y); ++ptr) {
            *ptr = z0 + dzdx*x;
            x += 1.0;
            stdev += (*ptr - mean)*(*ptr - mean);
        }
    }
    stdev = sqrt(stdev/(nx*ny - 1));
    
    {
        math::Statistics stats = math::makeStatistics(img, math::NPOINT | math::STDEV | math::MEAN);
        double const testmean = stats.getValue(math::MEAN);
        double const teststdev = stats.getValue(math::STDEV);
        
        BOOST_CHECK_EQUAL(stats.getValue(math::NPOINT), nx*ny);
        BOOST_CHECK_EQUAL(testmean, mean);
        BOOST_CHECK_EQUAL(teststdev, stdev );
    }

    {
        math::Statistics stats = math::makeStatistics(img, math::STDEV | math::MEAN | math::ERRORS);
        std::pair<double, double> const mean = stats.getResult(math::MEAN);
        double const sd = stats.getValue(math::STDEV);
        
        BOOST_CHECK_EQUAL(mean.first,  img(nx/2, ny/2));
        BOOST_CHECK_EQUAL(mean.second, sd/sqrt(img.getWidth()*img.getHeight()));
    }

    // ===============================================================================
    // sjb code for percentiles and clipped stats
    {
        math::Statistics stats = math::makeStatistics(img, math::MEDIAN);
        BOOST_CHECK_EQUAL(z0 + dzdx*(nx - 1)/2.0, stats.getValue(math::MEDIAN));
    }
    {
        math::Statistics stats = math::makeStatistics(img, math::IQRANGE);
        BOOST_CHECK_EQUAL(dzdx*(nx - 1)/2.0, stats.getValue(math::IQRANGE));
    }
    {
        math::Statistics stats = math::makeStatistics(img, math::MEANCLIP);
        BOOST_CHECK_EQUAL(z0 + dzdx*(nx - 1)/2.0, stats.getValue(math::MEANCLIP));
    }
    //{
    //    math::Statistics stats = math::makeStatistics(img, math::VARIANCECLIP);
    //    BOOST_CHECK_EQUAL(0.0, stats.getValue(math::VARIANCECLIP));
    //}

    
    
}


BOOST_AUTO_TEST_CASE(StatisticsTestAllNanButOne) {

    /*
     * The mean/stddev/min/max are computed in a single pass, but there's a pre-pass
     *   to get a crude mean with a stride of 10.  If there are no valid points
     *   in the pre-pass, 'crude_mean' is set to zero, and min/max are initialized
     *   to +/- MaxDouble.
     * This test verifies that when the only valid numbers present aren't on a 10-stride,
     *   the mean,min,max are set correctly.
     * The problem was apparent when parasoft found a possible div-by-zero error
     *   for crude_mean = sum/n with no valid points.
     */
    
    double const NaN = std::numeric_limits<double>::quiet_NaN();

    int nx = 101;
    int ny = 64;
    Image img(nx, ny);
    img = NaN;
    double z0 = 10.0;

    // set two pixels to non-nan ... neither on stride 10
    img(4, 4) = z0;
    img(3, 3) = z0 + 1.0;
    
    double const mean = z0 + 0.5;
    double const stdev = std::sqrt( (0.5*0.5 + 0.5*0.5)/(2.0 - 1.0) );
    double const min = z0;
    double const max = z0 + 1.0;
    
    {
        math::Statistics stats = math::makeStatistics(img, math::NPOINT | math::STDEV | math::MEAN |
                                                      math::MIN | math::MAX);
        double const testmean = stats.getValue(math::MEAN);
        double const teststdev = stats.getValue(math::STDEV);
        double const testmin = stats.getValue(math::MIN);
        double const testmax = stats.getValue(math::MAX);
        
        BOOST_CHECK_EQUAL(stats.getValue(math::NPOINT), 2);
        BOOST_CHECK_EQUAL(testmean, mean);
        BOOST_CHECK_EQUAL(teststdev, stdev );
        BOOST_CHECK_EQUAL(testmin, min);
        BOOST_CHECK_EQUAL(testmax, max);
    }

}

BOOST_AUTO_TEST_CASE(StatisticsTestImages) {
    
    /* =============================================================================
     * Tests of mean and standard deviation for Russ Laher's noise images.
     * - only one for now (time consuming)
     */
    {
        vector<string> imgfiles;
        imgfiles.push_back("v1_i1_g_m400_s20_f.fits");
        imgfiles.push_back("v1_i1_g_m400_s20_u16.fits");
        imgfiles.push_back("v1_i2_g_m400_s20_f.fits");
        imgfiles.push_back("v1_i2_g_m400_s20_u16.fits");
        imgfiles.push_back("v2_i1_p_m9_f.fits");
        imgfiles.push_back("v2_i1_p_m9_u16.fits");
        imgfiles.push_back("v2_i2_p_m9_f.fits");
        imgfiles.push_back("v2_i2_p_m9_u16.fits");

        string afwdata_dir = getenv("AFWDATA_DIR");
        for (vector<string>::iterator imgfile = imgfiles.begin(); imgfile != imgfiles.end(); ++imgfile) {
            
            string img_path = afwdata_dir + "/Statistics/" + *imgfile;

            // get the image and header
            DecoratedImage dimg(img_path);
            lsst::daf::base::PropertySet::Ptr fitsHdr = dimg.getMetadata(); // the FITS header

            // get the true values of the mean and stdev
            double const true_mean = fitsHdr->getAsDouble("MEANCOMP");
            double const true_stdev = fitsHdr->getAsDouble("SIGCOMP");

            // measure the mean and stdev with the Statistics class
            Image::Ptr img = dimg.getImage();
            math::Statistics statobj = math::makeStatistics(*img, math::MEAN | math::STDEV);
            //int n = img->getWidth() * img->getHeight();
            //double sampleToPop = 1.0; //sqrt( n/static_cast<double>(n - 1) );
            double const mean  = statobj.getValue(math::MEAN);
            double const stdev = statobj.getValue(math::STDEV);
            

            BOOST_CHECK_CLOSE(mean, true_mean, 1e-8);
            BOOST_CHECK_CLOSE(stdev, true_stdev, 1e-8);
        }
    }
}
