#!/usr/bin/env python
"""
Test cases to test image I/O
"""
import os
import pdb                          # we may want to say pdb.set_trace()
import unittest

import eups
import lsst.afw.image as afwImage
import lsst.utils.tests as utilsTests
import lsst.afw.display.ds9 as ds9

try:
    type(verbose)
except NameError:
    verbose = 0

dataDir = eups.productDir("afwdata")
if not dataDir:
    raise RuntimeError("Must set up afwdata to run these tests")

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class ReadFitsTestCase(unittest.TestCase):
    """A test case for reading FITS images"""

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def testU16(self):
        """Test reading U16 image"""

        im = afwImage.ImageD(os.path.join(dataDir, "small_img.fits"))
        
        col, row, val = 0, 0, 1154
        self.assertEqual(im.get(col, row), val)

    def testS16(self):
        """Test reading S16 image"""
        im = afwImage.ImageD(os.path.join(dataDir, "871034p_1_img.fits"))

        if False:
            ds9.mtv(im)
        
        col, row, val = 32, 1, 62
        self.assertEqual(im.get(col, row), val)

    def testF32(self):
        """Test reading F32 image"""
        im = afwImage.ImageD(os.path.join(dataDir, "871034p_1_MI_var.fits"))
        
        col, row, val = 32, 1, 39.11672
        self.assertAlmostEqual(im.get(col, row), val, 5)

    def testF64(self):
        """Test reading a U16 file into a F64 image"""
        im = afwImage.ImageD(os.path.join(dataDir, "small_img.fits"))
        col, row, val = 0, 0, 1154
        self.assertEqual(im.get(col, row), val)
        
        #print "IM = ", im
    def testWriteReadF64(self):
        """Test writing then reading an F64 image"""

        im = afwImage.ImageD(100, 100); im.set(666)
        im.writeFits("smallD.fits")
        newIm = afwImage.ImageD("smallD.fits")

    def testSubimage(self):
        """Test reading a subimage image"""
        fileName, hdu = os.path.join(dataDir, "871034p_1_MI_var.fits"), 0
        im = afwImage.ImageF(fileName)

        bbox = afwImage.BBox(afwImage.PointI(110, 120), 20, 15)
        sim = im.Factory(im, bbox) 

        im2 = afwImage.ImageF(fileName, hdu, None, bbox)

        self.assertEqual(im2.getDimensions(), sim.getDimensions())
        self.assertEqual(im2.get(1,1), sim.get(1, 1))

        self.assertEqual(im2.getX0(), sim.getX0())
        self.assertEqual(im2.getY0(), sim.getY0())

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""
    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(ReadFitsTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)

    return unittest.TestSuite(suites)

def run(exit=False):
    """Run the tests"""
    utilsTests.run(suite(), exit)
 
if __name__ == "__main__":
    run(True)
