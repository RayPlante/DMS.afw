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
Tests for geom.Point, geom.Extent, geom.CoordinateExpr

Run with:
   ./Coordinates.py
or
   python
   >>> import Coordinates; Coordinates.run()
"""

import unittest
import numpy

import lsst.utils.tests as utilsTests
import lsst.pex.exceptions
import lsst.afw.geom as geom

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class CoordinateTestCase(unittest.TestCase):
    
    def assertClose(self, a, b):
        if not numpy.allclose(a, b):
            return self.assertEqual(a, b)
        else:
            return self.assert_(True)

    def testAccessors(self):
        for dtype, cls, rnd in self.classes:
            vector1 = rnd()
            p = cls(*vector1)
            self.assertEqual(p.__class__, cls)
            self.assertEqual(tuple(p), tuple(vector1))
            self.assertEqual(tuple(p.clone()), tuple(p))
            self.assert_(p.clone() is not p)
            vector2 = rnd()
            for n in range(cls.dimensions):
                p[n] = vector2[n]
            self.assertEqual(tuple(p), tuple(vector2))

    def testComparison(self):
        for dtype, cls, rnd in self.classes:
            CoordinateExpr = geom.CoordinateExpr[cls.dimensions]
            vector1 = rnd()
            vector2 = rnd()
            p1 = cls(*vector1)
            p2 = cls(*vector2)

            self.assertEqual(tuple(p1.eq(p2)), tuple([v1 == v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(tuple(p1.ne(p2)), tuple([v1 != v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(tuple(p1.lt(p2)), tuple([v1 <  v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(tuple(p1.le(p2)), tuple([v1 <= v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(tuple(p1.gt(p2)), tuple([v1 >  v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(tuple(p1.ge(p2)), tuple([v1 >= v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(type(p1.eq(p2)), CoordinateExpr)
            self.assertEqual(type(p1.ne(p2)), CoordinateExpr)
            self.assertEqual(type(p1.lt(p2)), CoordinateExpr)
            self.assertEqual(type(p1.le(p2)), CoordinateExpr)
            self.assertEqual(type(p1.gt(p2)), CoordinateExpr)
            self.assertEqual(type(p1.ge(p2)), CoordinateExpr)
            scalar = dtype(rnd()[0])
            self.assertEqual(tuple(p1.eq(scalar)), tuple([v1 == scalar for v1 in vector1]))
            self.assertEqual(tuple(p1.ne(scalar)), tuple([v1 != scalar for v1 in vector1]))
            self.assertEqual(tuple(p1.lt(scalar)), tuple([v1 <  scalar for v1 in vector1]))
            self.assertEqual(tuple(p1.le(scalar)), tuple([v1 <= scalar for v1 in vector1]))
            self.assertEqual(tuple(p1.gt(scalar)), tuple([v1 >  scalar for v1 in vector1]))
            self.assertEqual(tuple(p1.ge(scalar)), tuple([v1 >= scalar for v1 in vector1]))
            self.assertEqual(type(p1.eq(scalar)), CoordinateExpr)
            self.assertEqual(type(p1.ne(scalar)), CoordinateExpr)
            self.assertEqual(type(p1.lt(scalar)), CoordinateExpr)
            self.assertEqual(type(p1.le(scalar)), CoordinateExpr)
            self.assertEqual(type(p1.gt(scalar)), CoordinateExpr)
            self.assertEqual(type(p1.ge(scalar)), CoordinateExpr)

class PointTestCase(CoordinateTestCase):
    """A test case for Point"""

    def setUp(self):
        self.classes = [
            (float, geom.Point2D, lambda: [float(x) for x in numpy.random.randn(2)]),
            (int, geom.Point2I, lambda: [int(x) for x in numpy.random.randint(-5, 5, 2)]),
            (float, geom.Point3D, lambda: [float(x) for x in numpy.random.randn(3)]),
            (int, geom.Point3I, lambda: [int(x) for x in numpy.random.randint(-5, 5, 3)]),
            ]

    def testArithmetic(self):
        for dtype, cls, rnd in self.classes:
            Extent = geom.Extent[dtype, cls.dimensions]
            vector1 = rnd()
            vector2 = rnd()
            p1 = cls(*vector1)
            p2 = cls(*vector2)
            self.assertClose(tuple(p1-p2), tuple([v1 - v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(type(p1-p2), Extent)
            self.assertClose(tuple(p1+Extent(p2)), tuple([v1 + v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(type(p1+Extent(p2)), cls)
            self.assertClose(tuple(p1-Extent(p2)), tuple([v1 - v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(type(p1-Extent(p2)), cls)
            p1 += Extent(p2)
            vector1 = [v1 + v2 for v1, v2 in zip(vector1, vector2)]
            self.assertEqual(tuple(p1), tuple(vector1))
            p1 -= Extent(p2)
            vector1 = [v1 - v2 for v1, v2 in zip(vector1, vector2)]
            self.assertClose(tuple(p1), tuple(vector1))
            p1.shift(Extent(p2))
            vector1 = [v1 + v2 for v1, v2 in zip(vector1, vector2)]
            self.assertClose(tuple(p1), tuple(vector1))

class ExtentTestCase(CoordinateTestCase):
    """A test case for Extent"""

    def setUp(self):
        self.classes = [
            (float, geom.Extent2D, lambda: [float(x) for x in numpy.random.randn(2)]),
            (int, geom.Extent2I, lambda: [int(x) for x in numpy.random.randint(-5, 5, 2)]),
            (float, geom.Extent3D, lambda: [float(x) for x in numpy.random.randn(3)]),
            (int, geom.Extent3I, lambda: [int(x) for x in numpy.random.randint(-5, 5, 3)]),
            ]

    def testArithmetic(self):
        for dtype, cls, rnd in self.classes:
            Point = geom.Point[dtype, cls.dimensions]
            vector1 = rnd()
            vector2 = rnd()
            p1 = cls(*vector1)
            p2 = cls(*vector2)
            self.assertClose(tuple(p1+Point(p2)), tuple([v1 + v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(type(p1+Point(p2)), Point)
            self.assertClose(tuple(p1+p2), tuple([v1 + v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(type(p1+p2), cls)
            self.assertClose(tuple(p1-p2), tuple([v1 - v2 for v1, v2 in zip(vector1, vector2)]))
            self.assertEqual(type(p1-p2), cls)
            self.assertClose(tuple(+p1), tuple(vector1))
            self.assertEqual(type(+p1), cls)
            self.assertClose(tuple(-p1), tuple([-v1 for v1 in vector1]))
            self.assertEqual(type(-p1), cls)
            p1 += p2
            vector1 = [v1 + v2 for v1, v2 in zip(vector1, vector2)]
            self.assertClose(tuple(p1), tuple(vector1))
            p1 -= p2
            vector1 = [v1 - v2 for v1, v2 in zip(vector1, vector2)]
            self.assertClose(tuple(p1), tuple(vector1))
            scalar = 2
            # Python handles integer division differently from C++ for negative numbers
            vector1 = [abs(x) for x in vector1]
            p1 = cls(*vector1)
            self.assertClose(tuple(p1*scalar), tuple([v1*scalar for v1 in vector1]))
            self.assertEqual(type(p1*scalar), cls)
            self.assertClose(tuple(p1/scalar), tuple([v1/scalar for v1 in vector1]))
            self.assertEqual(type(p1/scalar), cls)
            p1 *= scalar
            vector1 = [v1*scalar for v1 in vector1]
            self.assertClose(tuple(p1), tuple(vector1))
            p1 /= scalar
            vector1 = [v1/scalar for v1 in vector1]
            self.assertClose(tuple(p1), tuple(vector1))

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(PointTestCase)
    suites += unittest.makeSuite(ExtentTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit=False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
