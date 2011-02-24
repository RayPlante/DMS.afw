// -*- LSST-C++ -*-

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
#include <cmath>
#include <vector>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Background

#include "boost/test/unit_test.hpp"
#include "boost/test/floating_point_comparison.hpp"

#include "lsst/afw/image/Image.h"
#include "lsst/afw/math/Interpolate.h"
#include "lsst/afw/math/Background.h"

using namespace std;
namespace image = lsst::afw::image;
namespace math = lsst::afw::math;
namespace geom = lsst::afw::geom;

typedef image::Image<float> Image;
typedef image::DecoratedImage<float> DecoratedImage;

BOOST_AUTO_TEST_CASE(BackgroundBasic) { /* parasoft-suppress  LsstDm-3-2a LsstDm-3-4a LsstDm-4-6 LsstDm-5-25 "Boost non-Std" */


    int nX = 40;
    int nY = 40;
    Image img(geom::ExtentI(nX, nY));
    Image::Pixel const pixVal = 10000;
    img = pixVal;

    {
        int xcen = nX/2;
        int ycen = nY/2;
        math::BackgroundControl bgCtrl("AKIMA_SPLINE");
        // test methods native BackgroundControl
        bgCtrl.setNxSample(5);
        bgCtrl.setNySample(5);
        // test methods for public stats objects in bgCtrl
        bgCtrl.getStatisticsControl()->setNumSigmaClip(3);
        bgCtrl.getStatisticsControl()->setNumIter(3);
        math::Background back = math::makeBackground(img, bgCtrl);
        double const TESTVAL = back.getPixel(xcen, ycen);
        
        image::Image<float>::Ptr bImage = back.getImage<float>();
        Image::Pixel const testFromImage = *(bImage->xy_at(xcen, ycen));
        
        BOOST_CHECK_EQUAL(TESTVAL, pixVal);
        BOOST_CHECK_EQUAL(TESTVAL, testFromImage);

    }

}

BOOST_AUTO_TEST_CASE(BackgroundTestImages) { /* parasoft-suppress  LsstDm-3-2a LsstDm-3-4a LsstDm-4-6 LsstDm-5-25 "Boost non-Std" */

    {
        vector<string> imgfiles;
        imgfiles.push_back("v1_i1_g_m400_s20_f.fits");
        imgfiles.push_back("v1_i1_g_m400_s20_u16.fits");
        //imgfiles.push_back("v1_i2_g_m400_s20_f.fits");
        //imgfiles.push_back("v1_i2_g_m400_s20_u16.fits");
        //imgfiles.push_back("v2_i1_p_m9_f.fits");
        //imgfiles.push_back("v2_i1_p_m9_u16.fits");
        //imgfiles.push_back("v2_i2_p_m9_f.fits");
        //imgfiles.push_back("v2_i2_p_m9_u16.fits");
        
        string afwdata_dir = getenv("AFWDATA_DIR");
        for (vector<string>::iterator imgfile = imgfiles.begin(); imgfile != imgfiles.end(); ++imgfile) {
            
            string img_path = afwdata_dir + "/Statistics/" + *imgfile;

            // get the image and header
            DecoratedImage dimg(img_path);
            Image::Ptr img = dimg.getImage();
            lsst::daf::base::PropertySet::Ptr fitsHdr = dimg.getMetadata(); // the FITS header

            // get the true values of the mean and stdev
            float reqMean = static_cast<float>(fitsHdr->getAsDouble("MEANREQ"));
            float reqStdev = static_cast<float>(fitsHdr->getAsDouble("SIGREQ"));

            int const width = img->getWidth();
            int const height = img->getHeight();
            
            // create a background control object
            math::BackgroundControl bctrl(math::Interpolate::AKIMA_SPLINE);
            bctrl.setNxSample(5);
            bctrl.setNySample(5);
            float stdevSubimg = reqStdev / sqrt(width*height/(bctrl.getNxSample()*bctrl.getNySample()));

            // run the background constructor and call the getPixel() and getImage() functions.
            math::Background backobj = math::makeBackground(*img, bctrl);

            // test getPixel()
            float testval = static_cast<float>(backobj.getPixel(width/2, height/2));
            BOOST_REQUIRE( fabs(testval - reqMean) < 2.0*stdevSubimg );

            // test getImage() by checking the center pixel
            image::Image<float>::Ptr bimg = backobj.getImage<float>();
            float testImgval = static_cast<float>(*(bimg->xy_at(width/2, height/2)));
            BOOST_REQUIRE( fabs(testImgval - reqMean) < 2.0*stdevSubimg );
            
        }
    }
        
}



BOOST_AUTO_TEST_CASE(BackgroundRamp) { /* parasoft-suppress  LsstDm-3-2a LsstDm-3-4a LsstDm-4-6 LsstDm-5-25 "Boost non-Std" */

    {
        
        // make a ramping image (spline should be exact for linear increasing image
        int const nX = 512;
        int const nY = 512;
        image::Image<double> rampimg = image::Image<double>(geom::ExtentI(nX, nY));
        double dzdx = 0.1;
        double dzdy = 0.2;
        double z0 = 10000.0;

        for (int i = 0; i < nX; ++i) {
            double x = static_cast<double>(i);
            for ( int j = 0; j < nY; ++j) {
                double y = static_cast<double>(j);
                *rampimg.xy_at(i, j) = dzdx*x + dzdy*y + z0;
            }
        }
        
        // check corner, edge, and center pixels
        math::BackgroundControl bctrl = math::BackgroundControl(math::Interpolate::AKIMA_SPLINE);
        bctrl.setNxSample(6);
        bctrl.setNySample(6);
        bctrl.getStatisticsControl()->setNumSigmaClip(20.0); //something large enough to avoid clipping entirely
        bctrl.getStatisticsControl()->setNumIter(1);
        math::Background backobj = math::Background(rampimg, bctrl);

        // test the values at the corners and in the middle
        int ntest = 3;
        for (int i = 0; i < ntest; ++i) {
            int xpix = i*(nX - 1)/(ntest - 1);
            for (int j = 0; j < ntest; ++j) {
                int ypix = j*(nY - 1)/(ntest - 1);
                double testval = backobj.getPixel(xpix, ypix);
                double realval = *rampimg.xy_at(xpix, ypix);
                BOOST_CHECK_CLOSE( testval, realval, 1.0e-10 );
            }
        }
                    
    }

}
BOOST_AUTO_TEST_CASE(BackgroundParabola) { /* parasoft-suppress  LsstDm-3-2a LsstDm-3-4a LsstDm-4-6 LsstDm-5-25 "Boost non-Std" */

    {
        
        // make an image which varies parabolicly (spline should be exact for 2rd order polynomial)
        int const nX = 512;
        int const nY = 512;
        image::Image<double> parabimg = image::Image<double>(geom::ExtentI(nX, nY));
        double d2zdx2 = -1.0e-4;
        double d2zdy2 = -1.0e-4;
        double dzdx   = 0.1;
        double dzdy   = 0.2;
        double z0 = 10000.0;  // no cross-terms

        for ( int i = 0; i < nX; ++i ) {
            for ( int j = 0; j < nY; ++j ) {
                *parabimg.xy_at(i, j) = d2zdx2*i*i + d2zdy2*j*j + dzdx*i + dzdy*j + z0;
            }
        }
        
        // check corner, edge, and center pixels
        math::BackgroundControl bctrl = math::BackgroundControl(math::Interpolate::CUBIC_SPLINE);
        bctrl.setNxSample(24);
        bctrl.setNySample(24);
        bctrl.getStatisticsControl()->setNumSigmaClip(10.0);
        bctrl.getStatisticsControl()->setNumIter(1);
        math::Background backobj = math::Background(parabimg, bctrl);

        // debug
        //bimg = backobj.getImageD()
        //ds9.mtv(parabimg)
        //ds9.mtv(bimg, frame=1)
        //parabimg.writeFits("a.fits")
        //bimg.writeFits("b.fits")

        // check the values at the corners and int he middle
        int const ntest = 3;
        for (int i = 0; i < ntest; ++i) {
            int xpix = i*(nX - 1)/(ntest - 1);
            for (int j = 0; j < ntest; ++j) {
                int ypix = j*(nY - 1)/(ntest - 1);
                double testval = backobj.getPixel(xpix, ypix);
                double realval = *parabimg.xy_at(xpix, ypix);
                //print xpix, ypix, testval, realval
                // quadratic terms skew the averages of the subimages and the clipped mean for
                // a subimage != value of center pixel.  1/20 counts on a 10000 count sky
                //  is a fair (if arbitrary) test.
                BOOST_CHECK_CLOSE( testval, realval, 0.05 );
            }
        }
    }
}

