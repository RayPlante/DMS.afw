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
Tests for geom.Box2I, geom.Box2D

Run with:
   ./Box.py
or
   python
   >>> import box; box.run()
"""

import os
import sys
import unittest
import numpy

import lsst.utils.tests as utilsTests
import lsst.pex.exceptions
import lsst.afw.geom as geom

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class Box2ITestCase(unittest.TestCase):

    def testEmpty(self):
        box = geom.Box2I()
        self.assert_(box.isEmpty())
        self.assertEqual(box.getWidth(), 0)
        self.assertEqual(box.getHeight(), 0)
        for x in (-1,0,1):
            for y in (-1,0,1):
                point = geom.Point2I(x,y)
                self.assertFalse(box.contains(point))
                box.include(point)
                self.assert_(box.contains(point))
                box = geom.Box2I()
        box.grow(3)
        self.assert_(box.isEmpty())

    def testConstruction(self):
        for n in range(10):
            xmin, xmax, ymin, ymax = [int(i) for i in numpy.random.randint(low=-5, high=5, size=4)]
            if xmin > xmax: xmin, xmax = xmax, xmin
            if ymin > ymax: ymin, ymax = ymax, ymin
            pmin = geom.Point2I(xmin,ymin)
            pmax = geom.Point2I(xmax,ymax)
            # min/max constructor
            box = geom.Box2I(pmin,pmax)
            self.assertEqual(box.getMin(), pmin)
            self.assertEqual(box.getMax(), pmax)
            box = geom.Box2I(pmax,pmin)
            self.assertEqual(box.getMin(), pmin)
            self.assertEqual(box.getMax(), pmax)
            box = geom.Box2I(pmin,pmax,False)
            self.assertEqual(box.getMin(), pmin)
            self.assertEqual(box.getMax(), pmax)
            box = geom.Box2I(pmax,pmin,False)
            self.assert_(box.isEmpty() or pmax == pmin)
            # min/dim constructor
            dim = geom.Extent2I(1) + pmax - pmin
            if any(dim.eq(0)):
                box = geom.Box2I(pmin,dim)
                self.assert_(box.isEmpty())
                box = geom.Box2I(pmin,dim,False)
                self.assert_(box.isEmpty())
            else:
                box = geom.Box2I(pmin,dim)
                self.assertEqual(box.getMin(), pmin)
                self.assertEqual(box.getDimensions(), dim)
                box = geom.Box2I(pmin,dim,False)
                self.assertEqual(box.getMin(), pmin)
                self.assertEqual(box.getDimensions(), dim)
                dim = -dim
                box = geom.Box2I(pmin,dim)
                self.assertEqual(box.getMin(), pmin + dim + geom.Extent2I(1))
                self.assertEqual(box.getDimensions(),
                                 geom.Extent2I(abs(dim.getX()),abs(dim.getY())))
    def testSwap(self):
        x00, y00, x01, y01 = (0,1,2,3)
        x10, y10, x11, y11 = (4,5,6,7)

        box0 = geom.Box2I(geom.PointI(x00, y00), geom.PointI(x01, y01))
        box1 = geom.Box2I(geom.PointI(x10, y10), geom.PointI(x11, y11))
        box0.swap(box1)

        self.assertEqual(box0.getMinX(), x10);
        self.assertEqual(box0.getMinY(), y10);
        self.assertEqual(box0.getMaxX(), x11);
        self.assertEqual(box0.getMaxY(), y11);

        self.assertEqual(box1.getMinX(), x00);
        self.assertEqual(box1.getMinY(), y00);
        self.assertEqual(box1.getMaxX(), x01);
        self.assertEqual(box1.getMaxY(), y01);

    def testConversion(self):
        for n in range(10):
            xmin, xmax, ymin, ymax = numpy.random.uniform(low=-10, high=10, size=4)
            if xmin > xmax: xmin, xmax = xmax, xmin
            if ymin > ymax: ymin, ymax = ymax, ymin
            fpMin = geom.Point2D(xmin,ymin)
            fpMax = geom.Point2D(xmax,ymax)
            if any((fpMax-fpMin).lt(3)): continue  # avoid empty boxes
            fpBox = geom.Box2D(fpMin, fpMax)
            intBoxBig = geom.Box2I(fpBox, geom.Box2I.EXPAND)
            fpBoxBig = geom.Box2D(intBoxBig)
            intBoxSmall = geom.Box2I(fpBox, geom.Box2I.SHRINK)
            fpBoxSmall = geom.Box2D(intBoxSmall)
            self.assert_(fpBoxBig.contains(fpBox))
            self.assert_(fpBox.contains(fpBoxSmall))
            self.assert_(intBoxBig.contains(intBoxSmall))
            self.assert_(geom.Box2D(intBoxBig))
            self.assertEqual(geom.Box2I(fpBoxBig, geom.Box2I.EXPAND), intBoxBig)
            self.assertEqual(geom.Box2I(fpBoxSmall, geom.Box2I.SHRINK), intBoxSmall)

    def testAccessors(self):
        xmin, xmax, ymin, ymax = [int(i) for i in numpy.random.randint(low=-5, high=5, size=4)]
        if xmin > xmax: xmin, xmax = xmax, xmin
        if ymin > ymax: ymin, ymax = ymax, ymin
        pmin = geom.Point2I(xmin,ymin)
        pmax = geom.Point2I(xmax,ymax)
        box = geom.Box2I(pmin, pmax, True)
        self.assertEqual(pmin, box.getMin())
        self.assertEqual(pmax, box.getMax())
        self.assertEqual(box.getMinX(), xmin)
        self.assertEqual(box.getMinY(), ymin)
        self.assertEqual(box.getMaxX(), xmax)
        self.assertEqual(box.getMaxY(), ymax)
        self.assertEqual(box.getBegin(), pmin)
        self.assertEqual(box.getEnd(), (pmax + geom.Extent2I(1)))
        self.assertEqual(box.getBeginX(), xmin)
        self.assertEqual(box.getBeginY(), ymin)
        self.assertEqual(box.getEndX(), xmax + 1)
        self.assertEqual(box.getEndY(), ymax + 1)
        self.assertEqual(box.getDimensions(), (pmax - pmin + geom.Extent2I(1)))
        self.assertEqual(box.getWidth(), (xmax - xmin  + 1))
        self.assertEqual(box.getHeight(), (ymax - ymin  + 1))
        self.assertAlmostEqual(box.getArea(), box.getWidth() * box.getHeight(),
                places=14)
        
    def testRelations(self):
        box = geom.Box2I(geom.Point2I(-2,-3), geom.Point2I(2,1), True)
        self.assert_(box.contains(geom.Point2I(0,0)))
        self.assert_(box.contains(geom.Point2I(-2,-3)))
        self.assert_(box.contains(geom.Point2I(2,-3)))
        self.assert_(box.contains(geom.Point2I(2,1)))
        self.assert_(box.contains(geom.Point2I(-2,1)))
        self.assertFalse(box.contains(geom.Point2I(-2,-4)))
        self.assertFalse(box.contains(geom.Point2I(-3,-3)))
        self.assertFalse(box.contains(geom.Point2I(2,-4)))
        self.assertFalse(box.contains(geom.Point2I(3,-3)))
        self.assertFalse(box.contains(geom.Point2I(3,1)))
        self.assertFalse(box.contains(geom.Point2I(2,2)))
        self.assertFalse(box.contains(geom.Point2I(-3,1)))
        self.assertFalse(box.contains(geom.Point2I(-2,2)))
        self.assert_(box.contains(geom.Box2I(geom.Point2I(-1,-2), geom.Point2I(1,0))))
        self.assert_(box.contains(box))
        self.assertFalse(box.contains(geom.Box2I(geom.Point2I(-2,-3), geom.Point2I(2,2))))
        self.assertFalse(box.contains(geom.Box2I(geom.Point2I(-2,-3), geom.Point2I(3,1))))
        self.assertFalse(box.contains(geom.Box2I(geom.Point2I(-3,-3), geom.Point2I(2,1))))
        self.assertFalse(box.contains(geom.Box2I(geom.Point2I(-3,-4), geom.Point2I(2,1))))
        self.assert_(box.overlaps(geom.Box2I(geom.Point2I(-2,-3), geom.Point2I(2,2))))
        self.assert_(box.overlaps(geom.Box2I(geom.Point2I(-2,-3), geom.Point2I(3,1))))
        self.assert_(box.overlaps(geom.Box2I(geom.Point2I(-3,-3), geom.Point2I(2,1))))
        self.assert_(box.overlaps(geom.Box2I(geom.Point2I(-3,-4), geom.Point2I(2,1))))
        self.assert_(box.overlaps(geom.Box2I(geom.Point2I(-1,-2), geom.Point2I(1,0))))
        self.assert_(box.overlaps(box))
        self.assertFalse(box.overlaps(geom.Box2I(geom.Point2I(-5,-3), geom.Point2I(-3,1))))
        self.assertFalse(box.overlaps(geom.Box2I(geom.Point2I(-2,-6), geom.Point2I(2,-4))))
        self.assertFalse(box.overlaps(geom.Box2I(geom.Point2I(3,-3), geom.Point2I(4,1))))
        self.assertFalse(box.overlaps(geom.Box2I(geom.Point2I(-2,2), geom.Point2I(2,2))))

    def testMutators(self):
        box = geom.Box2I(geom.Point2I(-2,-3), geom.Point2I(2,1), True)
        box.grow(1)
        self.assertEqual(box, geom.Box2I(geom.Point2I(-3,-4), geom.Point2I(3,2), True))
        box.grow(geom.Extent2I(2,3))
        self.assertEqual(box, geom.Box2I(geom.Point2I(-5,-7), geom.Point2I(5,5), True))
        box.shift(geom.Extent2I(3,2))
        self.assertEqual(box, geom.Box2I(geom.Point2I(-2,-5), geom.Point2I(8,7), True))
        box.include(geom.Point2I(-4,2))
        self.assertEqual(box, geom.Box2I(geom.Point2I(-4,-5), geom.Point2I(8,7), True))
        box.include(geom.Point2I(0,-6))
        self.assertEqual(box, geom.Box2I(geom.Point2I(-4,-6), geom.Point2I(8,7), True))
        box.include(geom.Box2I(geom.Point2I(0,0), geom.Point2I(10,11), True))
        self.assertEqual(box, geom.Box2I(geom.Point2I(-4,-6), geom.Point2I(10,11), True))
        box.clip(geom.Box2I(geom.Point2I(0,0), geom.Point2I(11,12), True))
        self.assertEqual(box, geom.Box2I(geom.Point2I(0,0), geom.Point2I(10,11), True))
        box.clip(geom.Box2I(geom.Point2I(-1,-2), geom.Point2I(5,4), True))
        self.assertEqual(box, geom.Box2I(geom.Point2I(0,0), geom.Point2I(5,4), True))

class Box2DTestCase(unittest.TestCase):

    def testEmpty(self):
        box = geom.Box2D()
        self.assert_(box.isEmpty())
        self.assertEqual(box.getWidth(), 0.0)
        self.assertEqual(box.getHeight(), 0.0)
        for x in (-1,0,1):
            for y in (-1,0,1):
                point = geom.Point2D(x,y)
                self.assertFalse(box.contains(point))
                box.include(point)
                self.assert_(box.contains(point))
                box = geom.Box2D()
        box.grow(3)
        self.assert_(box.isEmpty())

    def testConstruction(self):
        for n in range(10):
            xmin, xmax, ymin, ymax = numpy.random.uniform(low=-5, high=5, size=4)
            if xmin > xmax: xmin, xmax = xmax, xmin
            if ymin > ymax: ymin, ymax = ymax, ymin
            pmin = geom.Point2D(xmin,ymin)
            pmax = geom.Point2D(xmax,ymax)
            # min/max constructor
            box = geom.Box2D(pmin,pmax)
            self.assertEqual(box.getMin(), pmin)
            self.assertEqual(box.getMax(), pmax)
            box = geom.Box2D(pmax,pmin)
            self.assertEqual(box.getMin(), pmin)
            self.assertEqual(box.getMax(), pmax)
            box = geom.Box2D(pmin,pmax,False)
            self.assertEqual(box.getMin(), pmin)
            self.assertEqual(box.getMax(), pmax)
            box = geom.Box2D(pmax,pmin,False)
            self.assert_(box.isEmpty())
            # min/dim constructor
            dim = pmax - pmin
            if any(dim.eq(0)):
                box = geom.Box2D(pmin,dim)
                self.assert_(box.isEmpty())
                box = geom.Box2D(pmin,dim,False)
                self.assert_(box.isEmpty())
            else:
                box = geom.Box2D(pmin,dim)
                self.assertEqual(box.getMin(), pmin)
                self.assertEqual(box.getDimensions(), dim)
                box = geom.Box2D(pmin,dim,False)
                self.assertEqual(box.getMin(), pmin)
                self.assertEqual(box.getDimensions(), dim)
                dim = -dim
                box = geom.Box2D(pmin,dim)
                self.assertEqual(box.getMin(), pmin + dim)
                self.assert_(numpy.allclose(box.getDimensions(),
                                            geom.Extent2D(abs(dim.getX()),abs(dim.getY()))))

    def testSwap(self):
        x00, y00, x01, y01 = (0.,1.,2.,3.)
        x10, y10, x11, y11 = (4.,5.,6.,7.)

        box0 = geom.Box2D(geom.PointD(x00, y00), geom.PointD(x01, y01))
        box1 = geom.Box2D(geom.PointD(x10, y10), geom.PointD(x11, y11))
        box0.swap(box1)

        self.assertEqual(box0.getMinX(), x10);
        self.assertEqual(box0.getMinY(), y10);
        self.assertEqual(box0.getMaxX(), x11);
        self.assertEqual(box0.getMaxY(), y11);

        self.assertEqual(box1.getMinX(), x00);
        self.assertEqual(box1.getMinY(), y00);
        self.assertEqual(box1.getMaxX(), x01);
        self.assertEqual(box1.getMaxY(), y01);

    def testAccessors(self):
        xmin, xmax, ymin, ymax = numpy.random.uniform(low=-5, high=5, size=4)
        if xmin > xmax: xmin, xmax = xmax, xmin
        if ymin > ymax: ymin, ymax = ymax, ymin
        pmin = geom.Point2D(xmin,ymin)
        pmax = geom.Point2D(xmax,ymax)
        box = geom.Box2D(pmin, pmax, True)
        self.assertEqual(pmin, box.getMin())
        self.assertEqual(pmax, box.getMax())
        self.assertEqual(box.getMinX(), xmin)
        self.assertEqual(box.getMinY(), ymin)
        self.assertEqual(box.getMaxX(), xmax)
        self.assertEqual(box.getMaxY(), ymax)
        self.assertEqual(box.getDimensions(), (pmax - pmin))
        self.assertEqual(box.getWidth(), (xmax - xmin))
        self.assertEqual(box.getHeight(), (ymax - ymin))
        self.assertEqual(box.getArea(), box.getWidth() * box.getHeight())
        self.assertEqual(box.getCenterX(), 0.5*(pmax.getX() + pmin.getX()))
        self.assertEqual(box.getCenterY(), 0.5*(pmax.getY() + pmin.getY()))
        self.assertEqual(box.getCenter().getX(), box.getCenterX())
        self.assertEqual(box.getCenter().getY(), box.getCenterY())
        
    def testRelations(self):
        box = geom.Box2D(geom.Point2D(-2,-3), geom.Point2D(2,1), True)
        self.assert_(box.contains(geom.Point2D(0,0)))
        self.assert_(box.contains(geom.Point2D(-2,-3)))
        self.assertFalse(box.contains(geom.Point2D(2,-3)))
        self.assertFalse(box.contains(geom.Point2D(2,1)))
        self.assertFalse(box.contains(geom.Point2D(-2,1)))
        self.assert_(box.contains(geom.Box2D(geom.Point2D(-1,-2), geom.Point2D(1,0))))
        self.assert_(box.contains(box))
        self.assertFalse(box.contains(geom.Box2D(geom.Point2D(-2,-3), geom.Point2D(2,2))))
        self.assertFalse(box.contains(geom.Box2D(geom.Point2D(-2,-3), geom.Point2D(3,1))))
        self.assertFalse(box.contains(geom.Box2D(geom.Point2D(-3,-3), geom.Point2D(2,1))))
        self.assertFalse(box.contains(geom.Box2D(geom.Point2D(-3,-4), geom.Point2D(2,1))))
        self.assert_(box.overlaps(geom.Box2D(geom.Point2D(-2,-3), geom.Point2D(2,2))))
        self.assert_(box.overlaps(geom.Box2D(geom.Point2D(-2,-3), geom.Point2D(3,1))))
        self.assert_(box.overlaps(geom.Box2D(geom.Point2D(-3,-3), geom.Point2D(2,1))))
        self.assert_(box.overlaps(geom.Box2D(geom.Point2D(-3,-4), geom.Point2D(2,1))))
        self.assert_(box.overlaps(geom.Box2D(geom.Point2D(-1,-2), geom.Point2D(1,0))))
        self.assert_(box.overlaps(box))
        self.assertFalse(box.overlaps(geom.Box2D(geom.Point2D(-5,-3), geom.Point2D(-3,1))))
        self.assertFalse(box.overlaps(geom.Box2D(geom.Point2D(-2,-6), geom.Point2D(2,-4))))
        self.assertFalse(box.overlaps(geom.Box2D(geom.Point2D(3,-3), geom.Point2D(4,1))))
        self.assertFalse(box.overlaps(geom.Box2D(geom.Point2D(-2,2), geom.Point2D(2,2))))
        self.assertFalse(box.overlaps(geom.Box2D(geom.Point2D(-2,-5), geom.Point2D(2,-3))))
        self.assertFalse(box.overlaps(geom.Box2D(geom.Point2D(-4,-3), geom.Point2D(-2,1))))
        self.assertFalse(box.contains(geom.Box2D(geom.Point2D(-2,1), geom.Point2D(2,3))))
        self.assertFalse(box.contains(geom.Box2D(geom.Point2D(2,-3), geom.Point2D(4,1))))

    def testMutators(self):
        box = geom.Box2D(geom.Point2D(-2,-3), geom.Point2D(2,1), True)
        box.grow(1)
        self.assertEqual(box, geom.Box2D(geom.Point2D(-3,-4), geom.Point2D(3,2), True))
        box.grow(geom.Extent2D(2,3))
        self.assertEqual(box, geom.Box2D(geom.Point2D(-5,-7), geom.Point2D(5,5), True))
        box.shift(geom.Extent2D(3,2))
        self.assertEqual(box, geom.Box2D(geom.Point2D(-2,-5), geom.Point2D(8,7), True))
        box.include(geom.Point2D(-4,2))
        self.assertEqual(box, geom.Box2D(geom.Point2D(-4,-5), geom.Point2D(8,7), True))
        self.assert_(box.contains(geom.Point2D(-4,2)))
        box.include(geom.Point2D(0,-6))
        self.assertEqual(box, geom.Box2D(geom.Point2D(-4,-6), geom.Point2D(8,7), True))
        box.include(geom.Box2D(geom.Point2D(0,0), geom.Point2D(10,11), True))
        self.assertEqual(box, geom.Box2D(geom.Point2D(-4,-6), geom.Point2D(10,11), True))
        box.clip(geom.Box2D(geom.Point2D(0,0), geom.Point2D(11,12), True))
        self.assertEqual(box, geom.Box2D(geom.Point2D(0,0), geom.Point2D(10,11), True))
        box.clip(geom.Box2D(geom.Point2D(-1,-2), geom.Point2D(5,4), True))
        self.assertEqual(box, geom.Box2D(geom.Point2D(0,0), geom.Point2D(5,4), True))


#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(Box2ITestCase)
    suites += unittest.makeSuite(Box2DTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(exit=False):
    """Run the tests"""
    utilsTests.run(suite(), exit)

if __name__ == "__main__":
    run(True)
