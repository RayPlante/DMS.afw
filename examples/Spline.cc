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

#include "lsst/afw/math/detail/Spline.h"

namespace afwMath = lsst::afw::math;

double func(double const x)
{
    if (x < 2) {
        return 2 - x;
    } else {
        return 0;
    }
}

int main() {

    // create x,y vector<>s containing a sin() function
    int const nX = 20;
    std::vector<double> x(nX);
    std::vector<double> y(nX);
    double const xLo = 0;
    double const xHi = 2.0*M_PI;
    double const range = xHi - xLo;
    
    for (int i = 0; i < nX; ++i) {
        x[i] = xLo + i/(nX - 1.0)*range;
        y[i] = func(x[i]);
    }

    // create a new x vector<> on a different grid and extending beyond the bounds
    //   of the interpolation range to tests extrapolation properties
    int const nX2 = 100;
    std::vector<double> x2(nX2);
    for (int i = 0; i < nX2; ++i) {
        x2[i] = xLo + ((nX + 2.0)/nX*i/(nX2 - 1.0) - 1.0/nX)*range;
    }
    
    // declare an spline interpolate object.
    double gamma = 2.5;
    afwMath::detail::Spline *yinterp = new afwMath::detail::TautSpline(x, y, gamma);

    std::vector<double> y2(x2.size());
    yinterp->interpolate(x2, y2);
    
    // output the interpolated y values, 1st derivatives, and 2nd derivatives.
    for (unsigned int i = 0; i != x2.size(); ++i) {
        std::cout << i << " "
                  << x2[i] << " "
                  << func(x2[i]) << " "
                  << y2[i] << " "
                  << std::endl;
    }

    delete yinterp;
}
