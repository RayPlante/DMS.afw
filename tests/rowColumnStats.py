#!/usr/bin/env python
# -*- python -*-
"""
Tests for statisticsStack row/column statistics

Run with:
   ./rowColumnStats.py
or
   python
   >>> import rowColumnStats; rowColumnStats.run()
"""

##########################
# rowColumnStats.py
# Steve Bickerton
# An python test to check the row/column statistics from statisticsStack

import unittest
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.utils.tests as utilsTests


class RowColumnStatisticsTestCase(unittest.TestCase):

    def setUp(self):

        # fill an image with a gradient
        self.n      = 8
        self.img    = afwImage.ImageF(self.n, self.n, 0)

        # these are the known answers for comparison
        def nVector(n, v):
            return [v for i in range(n)]
        self.column = nVector(self.n, 0.0)
        self.row    = nVector(self.n, 0.0)
        self.colPlus  = nVector(self.n, 0.0)
        self.colMinus = nVector(self.n, 0.0)
        self.colMult  = nVector(self.n, 0.0)
        self.colDiv   = nVector(self.n, 0.0)
        self.rowPlus  = nVector(self.n, 0.0)
        self.rowMinus = nVector(self.n, 0.0)
        self.rowMult  = nVector(self.n, 0.0)
        self.rowDiv   = nVector(self.n, 0.0)

        # set the values in the image, and keep track of the stats to verify things
        for y in range(self.n):
            for x in range(self.n):
                val = 1.0*x + 2.0*y
                self.img.set(x, y, val)
                self.column[y] += val
                self.row[x]    += val

        for i in range(self.n):
            self.row[i]    /= self.n
            self.column[i] /= self.n
            self.colPlus[i] = self.img.get(0, i) + self.column[i]
            
        # get stats on the columns and rows
        self.imgProjectCol = afwMath.statisticsStack(self.img, afwMath.MEAN, 'x')
        self.imgProjectRow = afwMath.statisticsStack(self.img, afwMath.MEAN, 'y')

        
    def tearDown(self):
        del self.img

    def testColumnStats(self):
        """Test the column statistics """
        for i in range(self.n):
            self.assertEqual(self.imgProjectCol.get(0, i)[0], self.column[i])

    def testRowStats(self):
        """Test the row statistics """
        for i in range(self.n):
            self.assertEqual(self.imgProjectRow.get(i, 0)[0], self.row[i])

            
    def testColumnOperators(self):
        """ Test operator overloading on columns """

        columnSlice = afwImage.ImageSliceF(self.imgProjectCol.getImage())
        
        imgAdd = self.img + columnSlice
        imgAdd2 = columnSlice + self.img
        imgSub = self.img - columnSlice
        imgMul = self.img * columnSlice
        imgMul2 = columnSlice * self.img
        imgDiv = self.img / columnSlice

        for i in range(self.n):
            self.assertAlmostEqual(imgAdd.get(0, i),  self.img.get(0, i) + columnSlice.get(0, i))
            self.assertAlmostEqual(imgAdd2.get(0, i),  imgAdd.get(0, i))
            self.assertAlmostEqual(imgSub.get(0, i),  self.img.get(0, i) - columnSlice.get(0, i))
            self.assertAlmostEqual(imgMul.get(0, i),  self.img.get(0, i) * columnSlice.get(0, i))
            self.assertAlmostEqual(imgMul2.get(0, i),  imgMul.get(0, i))
            self.assertAlmostEqual(imgDiv.get(0, i),  self.img.get(0, i) / columnSlice.get(0, i))


#################################################################
# Test suite boiler plate
#################################################################
def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(RowColumnStatisticsTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit = False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)

