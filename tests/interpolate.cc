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
#define BOOST_TEST_MODULE Interpolate

#include "boost/test/unit_test.hpp"
#include "lsst/afw/math/Interpolate.h"

using namespace std;
namespace math = lsst::afw::math;

typedef math::Interpolate Interp;

BOOST_AUTO_TEST_CASE(LinearInterpolateRamp) { /* parasoft-suppress  LsstDm-3-2a LsstDm-3-4a LsstDm-4-6 LsstDm-5-25 "Boost non-Std" */

    int n = 10;
    vector<double> x(n);
    vector<double> y(n);
    for (int i = 0; i < n; ++i) {
        x[i] = static_cast<double>(i);
        y[i] = static_cast<double>(i);
    }
    double xtest = 4.5;

    {
        // === test the Linear interpolator ============================
        //math::InterpControl ictrl1(math::LINEAR, NaN, NaN);
        Interp yinterpL(x, y, ::gsl_interp_linear);
        double youtL = yinterpL.interpolate(xtest);

        BOOST_CHECK_EQUAL(youtL, xtest);
    }
}

BOOST_AUTO_TEST_CASE(SplineInterpolateRamp) { /* parasoft-suppress  LsstDm-3-2a LsstDm-3-4a LsstDm-4-6 LsstDm-5-25 "Boost non-Std" */

    int n = 10;
    vector<double> x(n);
    vector<double> y(n);
    //double const NaN = std::numeric_limits<double>::quiet_NaN();
    for (int i = 0; i < n; ++i) {
        x[i] = static_cast<double>(i);
        y[i] = static_cast<double>(i);
    }
    double xtest = 4.5;

    {
        // === test the Spline interpolator =======================
        //math::InterpControl ictrl2(math::NATURAL_SPLINE, NaN, NaN);
        Interp yinterpS(x, y, ::gsl_interp_cspline);
        double youtS = yinterpS.interpolate(xtest);
        
        BOOST_CHECK_EQUAL(youtS, xtest);
    }
}


BOOST_AUTO_TEST_CASE(SplineInterpolateParabola) { /* parasoft-suppress  LsstDm-3-2a LsstDm-3-4a LsstDm-4-6 LsstDm-5-25 "Boost non-Std" */

    int const n = 20;
    vector<double> x(n);
    vector<double> y(n);
    double dydx = 1.0;
    double d2ydx2 = 0.5;
    double y0 = 10.0;
    
    //double const NaN = std::numeric_limits<double>::quiet_NaN();
    for (int i = 0; i < n; ++i) {
        x[i] = static_cast<double>(i);
        y[i] = d2ydx2*x[i]*x[i] + dydx*x[i] + y0;
    }
    double xtest = 9.5;
    double ytest = d2ydx2*xtest*xtest + dydx*xtest + y0;
    
    {
        // === test the Spline interpolator =======================
        Interp yinterpS(x, y, ::gsl_interp_akima);
        double youtS = yinterpS.interpolate(xtest);
        
        BOOST_CHECK_CLOSE(youtS, ytest, 1.0e-8);
    }
}


