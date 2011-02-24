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
Tests for MaskedImages

Run with:
   python MaskedImageIO.py
or
   python
   >>> import MaskedImageIO; MaskedImageIO.run()
"""


import os, re
import unittest

import eups

import lsst.utils.tests as utilsTests
import lsst.afw.image as afwImage
import lsst.afw.display.ds9 as ds9
import lsst.pex.exceptions as pexEx

dataDir = eups.productDir("afwdata")
if not dataDir:
    raise RuntimeError("You must set up afwdata to run these tests")

try:
    type(display)
except NameError:
    display = False

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class MaskedImageTestCase(unittest.TestCase):
    """A test case for MaskedImage"""
    def setUp(self):
        # Set a (non-standard) initial Mask plane definition
        #
        # Ideally we'd use the standard dictionary and a non-standard file, but
        # a standard file's what we have
        #
        mask = afwImage.MaskU()

        mask.clearMaskPlaneDict()
        for p in ("ZERO", "BAD", "SAT", "INTRP", "CR", "EDGE"):
            mask.addMaskPlane(p)

        if False:
            self.baseName = os.path.join(dataDir, "Small_MI")
        else:
            self.baseName = os.path.join(dataDir, "CFHT", "D4", "cal-53535-i-797722_1")
        self.mi = afwImage.MaskedImageF(self.baseName)

    def tearDown(self):
        del self.mi

    def testFitsRead(self):
        """Check if we read MaskedImages"""

        image = self.mi.getImage()
        mask = self.mi.getMask()

        if display:
            ds9.mtv(self.mi)

        self.assertEqual(image.get(32, 1), 3728)
        self.assertEqual(mask.get(0, 0), 2) # == BAD
            
    def testFitsReadConform(self):
        """Check if we read MaskedImages and make them replace Mask's plane dictionary"""

        hdu, metadata, bbox, conformMasks = 0, None, afwGeom.BoxI(), True
        self.mi = afwImage.MaskedImageF(self.baseName, hdu, metadata, bbox, conformMasks)

        image = self.mi.getImage()
        mask = self.mi.getMask()

        self.assertEqual(image.get(32, 1), 3728)
        self.assertEqual(mask.get(0, 0), 1) # i.e. not shifted 1 place to the right

        self.assertEqual(mask.getMaskPlane("CR"), 3, "Plane CR has value specified in FITS file")

    def testFitsReadNoConform2(self):
        """Check that reading a mask doesn't invalidate the plane dictionary"""

        testMask = afwImage.MaskU(afwImage.MaskedImageF_maskFileName(self.baseName))

        mask = self.mi.getMask()
        mask |= testMask

    def testFitsReadConform2(self):
        """Check that conforming a mask invalidates the plane dictionary"""

        hdu, metadata, bbox, conformMasks = 0, None, afwGeom.BoxI(), True
        testMask = afwImage.MaskU(afwImage.MaskedImageF_maskFileName(self.baseName),
                                  hdu, metadata, bbox, conformMasks)

        mask = self.mi.getMask()
        def tst(mask=mask):
            mask |= testMask

        utilsTests.assertRaisesLsstCpp(self, pexEx.RuntimeErrorException, tst)

    def testTicket617(self):
        """Test reading an F64 image and converting it to a MaskedImage"""
        im = afwImage.ImageD(100, 100)
        im.set(666)
        mi = afwImage.MaskedImageD(im)

    def testReadWriteMEF(self):
        """Test that we read and write MEF and non-MEF representions of MaskedImages correctly"""
        im = afwImage.MaskedImageF(10, 20)
        im.set(666, 0x10, 10)

        x0, y0 = 1, 2
        im.setXY0(x0, y0)

        for tmpFile, writeMef in [("foo", False), ("foo", True),
                                  ("foo.fits", False)]:
            im.writeFits(tmpFile, None, "w", writeMef)

            im2 = im.Factory(tmpFile)
            self.assertEqual(im2.getX0(), x0)
            self.assertEqual(im2.getY0(), y0)

            self.assertEqual(im2.getImage().getX0(), x0)
            self.assertEqual(im2.getImage().getY0(), y0)

            self.assertEqual(im2.getMask().getX0(), x0)
            self.assertEqual(im2.getMask().getY0(), y0)

            self.assertEqual(im2.getVariance().getX0(), x0)
            self.assertEqual(im2.getVariance().getY0(), y0)

            if writeMef or re.search(r"\.fits(\.gz)?$", tmpFile):
                os.remove(tmpFile)
            else:
                os.remove(afwImage.MaskedImageF.imageFileName(tmpFile))
                os.remove(afwImage.MaskedImageF.maskFileName(tmpFile))
                os.remove(afwImage.MaskedImageF.varianceFileName(tmpFile))

        def tst():
            """We can't write a compressed MEF as we write 3 HDUs"""
            im.writeFits("foo.fits.gz", None, "w", writeMef)

        utilsTests.assertRaisesLsstCpp(self, pexEx.IoErrorException, tst)

    def testReadWriteXY0(self):
        """Test that we read and write (X0, Y0) correctly"""
        im = afwImage.MaskedImageF(10, 20)

        x0, y0 = 1, 2
        im.setXY0(x0, y0)
        tmpFile = "foo"
        im.writeFits(tmpFile)

        im2 = im.Factory(tmpFile)
        self.assertEqual(im2.getX0(), x0)
        self.assertEqual(im2.getY0(), y0)

        self.assertEqual(im2.getImage().getX0(), x0)
        self.assertEqual(im2.getImage().getY0(), y0)

        self.assertEqual(im2.getMask().getX0(), x0)
        self.assertEqual(im2.getMask().getY0(), y0)
        
        self.assertEqual(im2.getVariance().getX0(), x0)
        self.assertEqual(im2.getVariance().getY0(), y0)
        
        os.remove(afwImage.MaskedImageF.imageFileName(tmpFile))
        os.remove(afwImage.MaskedImageF.maskFileName(tmpFile))
        os.remove(afwImage.MaskedImageF.varianceFileName(tmpFile))

    def testWcs(self):
        """Test round-tripping an empty Wcs"""
        mi = afwImage.MaskedImageF(10, 20)
        wcs = afwImage.Wcs()

        exp = afwImage.makeExposure(mi, wcs)

        tmpFile = "foo.fits"
        exp.writeFits(tmpFile)

        exp2 = type(exp)(tmpFile)
        self.assertFalse(exp2.getWcs())

        os.remove(tmpFile)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(MaskedImageTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit=False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
