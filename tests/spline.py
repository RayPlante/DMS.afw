#!/usr/bin/env python

# 
# LSST Data Management System
# Copyright 2008, 2009, 2010 LSST Corporation.
# 
# This product includes software developed by the
# LSST Project (http://www.lsst.org/).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the LSST License Statement and 
# the GNU General Public License along with this program.  If not, 
# see <http://www.lsstcorp.org/LegalNotices/>.
#

"""
Tests for Splines

Run with:
   ./spline.py
or
   python
   >>> import spline; spline.run()
"""

import math, os, sys
import unittest

import lsst.utils.tests as utilsTests
import lsst.pex.exceptions
import lsst.daf.base
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import eups
import lsst.afw.display.ds9 as ds9

try:
    type(display)
except NameError:
    display = False

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class SplineTestCase(unittest.TestCase):
    """A test case for Image"""
    def smooth(self, x, differentiate=False):
        if differentiate:
            return math.cos(x)
        else:
            return math.sin(x)

    def noDerivative(self, x):
        if False:
            return math.sin(x)
        else:
            if x < 1:
                return 1 - x
            else:
                return 0

    def setUp(self):
        x, x2, ySin, yND = [], [], [], []
        for i in range(0,40):
            x.append(0.1*i)
            for j in range(4):
                x2.append(0.1*(i + 0.25*j))
                
            ySin.append(self.smooth(x[i]))
            yND.append(self.noDerivative(x[i]))

        self.x = x
        self.x2 = x2
        self.yND = yND
        self.ySin = ySin

    def tearDown(self):
        del self.x
        del self.x2
        del self.ySin
        del self.yND

    def testNaturalSpline1(self):
        """Test fitting a natural spline to a smooth function"""
        gamma = 0
        sp = afwMath.TautSpline(self.x, self.ySin, gamma)

        y2 = afwMath.vectorD()
        sp.interpolate(self.x2, y2)

        for x, y in zip(self.x2, y2):
            self.assertAlmostEqual(y, self.smooth(x), 1) # fails at 2 places!

    def testNaturalSplineDerivative1(self):
        """Test fitting a natural spline to a smooth function and finding its derivative"""
        sp = afwMath.TautSpline(self.x, self.ySin)

        y2 = afwMath.vectorD()
        sp.derivative(self.x2, y2)

        for x, y in zip(self.x2, y2):
            self.assertTrue(abs(y - self.smooth(x, True)) < 1.5e-3)

    def testNaturalSpline2(self):
        """Test fitting a natural spline to a non-differentiable function (we basically fail)"""
        gamma = 0
        sp = afwMath.TautSpline(self.x, self.yND, gamma)

        y2 = afwMath.vectorD()
        sp.interpolate(self.x2, y2)

        for x, y in zip(self.x2, y2):
            self.assertAlmostEqual(y, self.noDerivative(x), 1) # fails at 2 places!

    def testTautSpline1(self):
        """Test fitting a taut spline to a smooth function"""
        gamma = 2.5
        sp = afwMath.TautSpline(self.x, self.ySin, gamma)

        y2 = afwMath.vectorD()
        sp.interpolate(self.x2, y2)

        for x, y in zip(self.x2, y2):
            self.assertAlmostEqual(y, self.smooth(x), 4)

    def testTautSpline2(self):
        """Test fitting a taut spline to a non-differentiable function"""
        gamma = 2.5
        sp = afwMath.TautSpline(self.x, self.yND, gamma)

        y2 = afwMath.vectorD()
        sp.interpolate(self.x2, y2)

        for x, y in zip(self.x2, y2):
            self.assertAlmostEqual(y, self.noDerivative(x))

    def testRootFinding(self):
        """Test finding roots of Spline = value"""

        gamma = 2.5
        sp = afwMath.TautSpline(self.x, self.yND, gamma)

        for value in (0.1, 0.5):
            self.assertEqual(sp.roots(value, self.x[0], self.x[-1])[0], 1 - value)

        if False:
            y = afwMath.vectorD()
            sp.interpolate(self.x, y)
            for x, y in zip(self.x, y):
                print x, y
        #
        # Solve sin(x) = 0.5
        #
        sp = afwMath.TautSpline(self.x, self.ySin)
        roots = [math.degrees(x) for x in sp.roots(0.5, self.x[0], self.x[-1])]
        self.assertAlmostEqual(roots[0], 30, 5)
        self.assertAlmostEqual(roots[1], 150, 5)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(SplineTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit=False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
