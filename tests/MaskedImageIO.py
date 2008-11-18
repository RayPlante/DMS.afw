#!/usr/bin/env python
"""
Tests for MaskedImages

Run with:
   python MaskedImageIO.py
or
   python
   >>> import MaskedImageIO; MaskedImageIO.run()
"""

import pdb                              # we may want to say pdb.set_trace()
import os
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
            self.baseName = os.path.join(dataDir,"CFHT", "D4", "cal-53535-i-797722_1")
        self.mi = afwImage.MaskedImageF(self.baseName)

    def tearDown(self):
        del self.mi

    def testFitsRead(self):
        """Check if we read MaskedImages"""

        image = self.mi.getImage()
        mask = self.mi.getMask()

        if display:
            ds9.mtv(self.mi)

        self.assertEqual(image.get(32, 1), 3728);
        self.assertEqual(mask.get(0,0), 2); # == BAD
            
    def testFitsReadConform(self):
        """Check if we read MaskedImages and make them replace Mask's plane dictionary"""

        hdu, metadata, conformMasks = 0, None, True
        self.mi = afwImage.MaskedImageF(self.baseName, hdu, metadata, conformMasks)

        image = self.mi.getImage()
        mask = self.mi.getMask()

        self.assertEqual(image.get(32, 1), 3728);
        self.assertEqual(mask.get(0,0), 1); # i.e. not shifted 1 place to the right

        self.assertEqual(mask.getMaskPlane("CR"), 3, "Plane CR has value specified in FITS file")

    def testFitsReadNoConform2(self):
        """Check that reading a mask doesn't invalidate the plane dictionary"""

        testMask = afwImage.MaskU(afwImage.MaskedImageF_maskFileName(self.baseName))

        mask = self.mi.getMask()
        mask |= testMask

    def testFitsReadConform2(self):
        """Check that conforming a mask invalidates the plane dictionary"""

        hdu, metadata, conformMasks = 0, None, True
        testMask = afwImage.MaskU(afwImage.MaskedImageF_maskFileName(self.baseName), hdu, metadata, conformMasks)

        mask = self.mi.getMask()
        def tst(mask=mask):
            mask |= testMask

        self.assertRaises(pexEx.LsstRuntime, tst)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(MaskedImageTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(exit=False):
    """Run the tests"""
    utilsTests.run(suite(), exit)

if __name__ == "__main__":
    run(True)
