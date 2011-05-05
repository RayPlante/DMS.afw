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
 
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE testlsf1d
 
//The boost unit test header
#include "boost/test/unit_test.hpp"

using namespace std;

#include <vector>
#include <cmath>
#include <cstdio>
#include <cassert>
#include "boost/shared_ptr.hpp"
#include "Eigen/Core.h"

#include "lsst/afw/image/TanWcs.h"
#include "lsst/afw/geom/Point.h"
#include "lsst/afw/math/FunctionLibrary.h"
#include "lsst/afw/coord/Coord.h"
#include "lsst/afw/coord/Utils.h"

namespace math = lsst::afw::math;
namespace afwImg = lsst::afw::image;
namespace afwGeom = lsst::afw::geom;
namespace afwCoord = lsst::afw::coord;



double calculateDistortion( Eigen::MatrixXd sip, double u, double v)
{
    ///Computes all terms in the matrix, which the sip standard tells you not to do.
    ///At least the standard says they are not used. Does this mean set to zero or not
    ///computed?
    assert(sip.rows() == sip.cols());
    
    int i, j;
    int size = sip.rows();
    double distort=0;
    for(i=0; i<size; ++i)
    {
        for(j=0; j<size; ++j)
        {  distort += sip(i,j) * pow(u, i) * pow(v, j);
        }
    }
    
    return distort;
}


void testSip(afwImg::TanWcs &linWcs, afwImg::TanWcs &sipWcs, Eigen::MatrixXd sipA, Eigen::MatrixXd sipB)
{

    //Give sipA, and a position, calculate the expected distortion. Check the linearWcs
    //at position+distortion gives the same answer as sipWcs at position
    
    double range=1000;
    double step=1000;
    
    double u = -1*range;   //Relative pixel coord in x
    while(u<= range)
    {
        double v=-1*range;
        while(v<= range)
        {
            double distortX = calculateDistortion(sipA, u, v);
            double distortY = calculateDistortion(sipB, u, v);

            afwGeom::Point2D xy = sipWcs.getPixelOrigin();
            double x0 = xy[0];
            double y0 = xy[1];
            
            afwCoord::Fk5Coord lin = linWcs.pixelToSky(x0 + u +distortX, y0+v+distortY)->toFk5();
            afwCoord::Fk5Coord sip = sipWcs.pixelToSky(x0+u, y0+v)->toFk5();

            if(1) {
                printf("\n%.1f %.1f : %.3f\n", u, v, distortX);
                printf("%.7f %.7f \n", lin.getRa().asDegrees(), lin.getDec().asDegrees());
                printf("%.7f %.7f \n", sip.getRa().asDegrees(), sip.getDec().asDegrees());
                printf("Diff: %.7f %.7f \n", sip.getRa().asDegrees() - lin.getRa().asDegrees(), \
                        sip.getDec().asDegrees() - lin.getDec().asDegrees());
            }
                                
            BOOST_CHECK_CLOSE(lin.getRa().asDegrees(), sip.getRa().asDegrees(), 1e-7); 
            BOOST_CHECK_CLOSE(lin.getDec().asDegrees(), sip.getDec().asDegrees(), 1e-7); 
            
            v+=step;
        }
            
        u+=step;
    }
}




void testSipP(afwImg::TanWcs &linWcs, afwImg::TanWcs &sipWcs, Eigen::MatrixXd sipAp, Eigen::MatrixXd sipBp)
{
    //Test the reverse coefficients.
    //Given sipAp, sipBp, and a position, calculate the expected distortion. Check the linearWcs
    //at position+distortion gives the same answer as sipWcs at position
    
    double range=.25;
    double step=.0625;

    afwGeom::Point2D xy0 = linWcs.getPixelOrigin();
    
    afwCoord::Fk5Coord raDec0 = linWcs.getSkyOrigin()->toFk5();
    double ra = raDec0.getRa().asDegrees() - range;
    double raUpr = raDec0.getRa().asDegrees() + range;
    
    while(ra<= raUpr)
    {

        double dec = raDec0.getDec().asDegrees() - range;
        double decUpr = raDec0.getDec().asDegrees() + range;
        while(dec<= decUpr)
        {
            afwGeom::Point2D xy = linWcs.skyToPixel(ra, dec);
            afwGeom::Point2D xySip = sipWcs.skyToPixel(ra, dec);
            
            //Get pixel origin returns crpix in fits coords, so we convert to 
            //lsst coords before using (hence the -1)
            double u = xy[0] - xy0[0];
            double v = xy[1] - xy0[1];

            double distortX = calculateDistortion(sipAp, u, v);
            double distortY = calculateDistortion(sipBp, u, v);

            if(0)
            {
                printf("\n%.7f %.7f\n", ra, dec);
                printf("%.4f %.4f : %.4f %.4f\n", u, v, distortX, distortY);
                printf("%.4f %.4f \n", xy[0], xy[1]);
                printf("%.4f %.4f \n", xySip[0], xySip[1]);
                printf("Diff: %.4f %.4f \n", xySip[0]-xy[0], xySip[1]-xy[1]);
            }
            
            BOOST_CHECK_CLOSE(xy[0] + distortX, xySip[0], 1e-4); 
            BOOST_CHECK_CLOSE(xy[1] + distortY, xySip[1], 1e-4); 
            
            dec+=step;
            //return;
        }
            
        ra+=step;
    }
}


BOOST_AUTO_TEST_CASE(basic)
{
    afwGeom::Point2D crval = afwGeom::Point2D(45.,45.);
    afwGeom::Point2D crpix = afwGeom::Point2D(10,10);   
    Eigen::MatrixXd CD(2,2);
    
    double arcsecPerPixel = 0.000277777777777778;
    CD(0,0) = arcsecPerPixel;
    CD(0,1) = 0;
    CD(1,0) = 0;
    CD(1,1) = arcsecPerPixel;
    
    Eigen::Matrix3d sipA;
    Eigen::Matrix3d sipB;
    Eigen::Matrix3d sipAp;
    Eigen::Matrix3d sipBp;

    
    //zero the sips
    for(int i=0; i<3; ++i)
    {
        for(int j=0; j<3; ++j)
        {
            sipA(i, j) = 0;
            sipB(i, j) = 0;
            sipAp(i, j) = 0;
            sipBp(i, j) = 0;
        }
    }

    afwImg::TanWcs linWcs(crval, crpix, CD);

    //Test x direction
    //
    //quadratic in x direction            
    sipA(2,0) = 1e-4;
    afwImg::TanWcs sipWcs1(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSip(linWcs, sipWcs1, sipA, sipB);
    sipA(2,0) = 0;

    
    //Quadratic in y    
    sipA(0,2) = 1e-4;
    afwImg::TanWcs sipWcs2(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSip(linWcs, sipWcs2, sipA, sipB);
    sipA(0,2) = 0;
    
    //Cross term
    sipA(1,1) = 1e-4;
    afwImg::TanWcs sipWcs3(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSip(linWcs, sipWcs3, sipA, sipB);
    sipA(1,1) = 0;


    //test y direction
    //
    sipB(2,0) = 1e-4;
    afwImg::TanWcs sipWcs4(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSip(linWcs, sipWcs4, sipA, sipB);
    sipB(2,0) = 0;
    
    //Quadratic in y    
    sipA(0,2) = 1e-4;
    afwImg::TanWcs sipWcs5(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSip(linWcs, sipWcs5, sipA, sipB);
    sipB(0,2) = 0;
    
    //Cross term
    sipA(1,1) = 1e-4;
    afwImg::TanWcs sipWcs6(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSip(linWcs, sipWcs6, sipA, sipB);
    sipB(1,1) = 0;


    ///
    //Test reverse coeff.
    ///
    sipAp(2,0) = 1.e-4;
    afwImg::TanWcs sipWcs7(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSipP(linWcs, sipWcs7, sipAp, sipBp);
    sipA(2,0) = 0;

    //The linear term is allowed in the reverse matrix    
    sipAp(1,0) = 1.e-4;
    afwImg::TanWcs sipWcs8(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSipP(linWcs, sipWcs8, sipAp, sipBp);
    sipA(1,0) = 0;


    sipBp(2,0) = 1.e-4;
    afwImg::TanWcs sipWcs9(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSipP(linWcs, sipWcs9, sipAp, sipBp);
    sipA(2,0) = 0;

    //The linear term is allowed in the reverse matrix    
    sipBp(1,0) = 1.e-4;
    afwImg::TanWcs sipWcs10(crval, crpix, CD, sipA, sipB, sipAp, sipBp);
    testSipP(linWcs, sipWcs10, sipAp, sipBp);
    sipA(1,0) = 0;
    
}

//
// ******************************************************************************************************
//


void createSipTests(afwImg::TanWcs &wcs1, afwImg::TanWcs &wcs2)
{
    afwGeom::Point2D xy = wcs1.getPixelOrigin();
    double x0 = xy[0]-1;
    double y0 = xy[1]-1;

    
    double range=1000;
    double step=1000;
    
    double u = -1*range;   //Relative pixel coord in x
    while(u<= range)
    {
        double v=-1*range;
        while(v<= range)
        {
            
            afwCoord::Fk5Coord pos1 = wcs1.pixelToSky(x0+u, y0+v)->toFk5();
            afwCoord::Fk5Coord pos2 = wcs2.pixelToSky(x0+u, y0+v)->toFk5();

            printf("\n%.1f %.1f \n", u, v);
            printf("%.7f %.7f \n", pos1.getRa().asDegrees(), pos1.getDec().asDegrees());
            printf("%.7f %.7f \n", pos2.getRa().asDegrees(), pos2.getDec().asDegrees());
            
                                
            BOOST_CHECK_CLOSE(pos1.getRa().asDegrees(), pos2.getRa().asDegrees(), 1e-7); 
            BOOST_CHECK_CLOSE(pos1.getDec().asDegrees(), pos2.getDec().asDegrees(), 1e-7); 
            
            v+=step;
        }
            
        u+=step;
    }
}

