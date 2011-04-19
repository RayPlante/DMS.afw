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


##test1079
##\brief Test that the wcs of sub-images are written and read from disk correctly
##$Id$
##\author Fergal Mullally

import math, os
import unittest

import eups
import lsst.afw.image as afwImage
import lsst.afw.geom as afwGeom
import lsst.utils.tests as utilsTests
import lsst.pex.exceptions.exceptionsLib as exceptions
import lsst.afw.display.ds9 as ds9

try:
    type(verbose)
except NameError:
    verbose = 0

class SavingSubImagesTest(unittest.TestCase):
    """
    Tests for changes made for ticket #1079. In the LSST wcs transformations are done in terms
    of pixel position, which is measured from the lower left hand corner of the parent image from
    which this sub-image is drawn. However, when saving a sub-image to disk, the fits standards 
    has no concept of parent- and sub- images, and specifies that the wcs is measured relative to 
    the pixel index (i.e the lower left hand corner of the sub-image). This test makes sure 
    we're saving and reading wcs headers from sub-images correctly.
    """
    
    def setUp(self):
        path = eups.productDir("afw")
        self.parentFile = os.path.join(path, "tests", "data", "parent.fits")
        
        self.parent = afwImage.ExposureF(self.parentFile)
        self.llcParent = self.parent.getMaskedImage().getXY0()
        self.oParent = self.parent.getWcs().getPixelOrigin()
        
        #A list of pixel positions to test
        self.testPositions = []
        self.testPositions.append(afwGeom.Point2D(128, 128))
        self.testPositions.append(afwGeom.Point2D(0,0))        
        self.testPositions.append(afwGeom.Point2D(20,30))        
        self.testPositions.append(afwGeom.Point2D(60,50))        
        self.testPositions.append(afwGeom.Point2D(80, 80))        
        self.testPositions.append(afwGeom.Point2D(255,255))

        self.parent.getMaskedImage().set(0)
        for p in self.testPositions:
            self.parent.getMaskedImage().set(int(p[0]), int(p[1]), (10 + p[0],))

    def tearDown(self):
        del self.parent
        del self.oParent
        del self.testPositions

    def testInvarianceOfCrpix1(self):
        """Test that crpix is the same for parent and sub-image. Also tests that llc of sub-image
        saved correctly"""
        
        llc = afwGeom.Point2I(20, 30)
        bbox = afwGeom.Box2I(llc, afwGeom.Extent2I(60, 50))
        subImg = afwImage.ExposureF(self.parent, bbox, afwImage.LOCAL)

        subImgLlc = subImg.getMaskedImage().getXY0()
        oSubImage = subImg.getWcs().getPixelOrigin()
        
        #Useful for debugging
        if False:
            print self.parent.getMaskedImage().getXY0()
            print subImg.getMaskedImage().getXY0()
            print self.parent.getWcs().getFitsMetadata().toString()
            print subImg.getWcs().getFitsMetadata().toString()
            print self.oParent, oSubImage
        
        for i in range(2):
            self.assertEqual(llc[i], subImgLlc[i], "Corner of sub-image not correct")
            self.assertAlmostEqual(self.oParent[i], oSubImage[i], 6, "Crpix of sub-image not correct")


    def testInvarianceOfCrpix2(self):
        """For sub-images loaded from disk, test that crpix is the same for parent and sub-image. 
        Also tests that llc of sub-image saved correctly"""
        
        #Load sub-image directly off of disk
        llc = afwGeom.Point2I(20, 30)
        bbox = afwGeom.Box2I(llc, afwGeom.Extent2I(60, 50))
        hdu=0
        subImg = afwImage.ExposureF(self.parentFile, hdu, bbox, afwImage.LOCAL)
        oSubImage = subImg.getWcs().getPixelOrigin()
        subImgLlc = subImg.getMaskedImage().getXY0()
       
        #Useful for debugging
        if False:
            print self.parent.getMaskedImage().getXY0()
            print subImg.getMaskedImage().getXY0()
            print self.parent.getWcs().getFitsMetadata().toString()
            print subImg.getWcs().getFitsMetadata().toString()
            print self.oParent, oSubImage
        
        for i in range(2):
            self.assertEqual(llc[i], subImgLlc[i], "Corner of sub-image not correct")
            self.assertAlmostEqual(self.oParent[i], oSubImage[i], 6, "Crpix of sub-image not correct")
    
    
    def testInvarianceOfPixelToSky(self):

        for deep in (True, False):
            llc = afwGeom.Point2I(20, 30)
            bbox = afwGeom.Box2I(llc, afwGeom.Extent2I(60, 50))
            subImg = afwImage.ExposureF(self.parent, bbox, afwImage.LOCAL, deep)

            xy0 = subImg.getMaskedImage().getXY0()

            if False:
                ds9.mtv(self.parent, frame=0)
                ds9.mtv(subImg, frame=1)

            for p in self.testPositions:
                subP = p - afwGeom.Extent2D(llc[0], llc[1]) # pixel in subImg

                if \
                       subP[0] < 0 or subP[0] >= bbox.getWidth() or \
                       subP[1] < 0 or subP[1] >= bbox.getHeight():
                    continue

                adParent = self.parent.getWcs().pixelToSky(p)
                adSub = subImg.getWcs().pixelToSky(subP + afwGeom.Extent2D(xy0[0], xy0[1]))
                #
                # Check that we're talking about the same pixel
                #            
                self.assertEqual(self.parent.getMaskedImage().get(int(p[0]), int(p[1])),
                                 subImg.getMaskedImage().get(int(subP[0]), int(subP[1])))

                self.assertEqual(adParent[0], adSub[0], "RAs are equal; deep = %s" % deep)
                self.assertEqual(adParent[1], adSub[1], "DECs are equal; deep = %s" % deep)

    def testSubSubImage(self):
        """Check that a sub-image of a sub-image is equivalent to a sub image, i.e
        that the parent is an invarient"""                
        
        llc1 = afwGeom.Point2I(20, 30)
        bbox = afwGeom.Box2I(llc1, afwGeom.Extent2I(60, 50))
        hdu=0
        subImg = afwImage.ExposureF(self.parentFile, hdu, bbox, afwImage.LOCAL)


        llc2 = afwGeom.Point2I(22, 23)

        #This subsub image should fail. Although it's big enough to fit in the parent image
        #it's too small for the sub-image
        bbox = afwGeom.Box2I(llc2, afwGeom.Extent2I(100, 110))
        self.assertRaises(exceptions.LsstCppException, afwImage.ExposureF, subImg, bbox, afwImage.LOCAL)
        
        bbox = afwGeom.Box2I(llc2, afwGeom.Extent2I(10, 11))
        subSubImg = afwImage.ExposureF(subImg, bbox, afwImage.LOCAL)
        
        sub0 = subImg.getMaskedImage().getXY0()
        subsub0= subSubImg.getMaskedImage().getXY0()
        
        
        if False:
            print sub0
            print subsub0
            
            
        for i in range(2):
            self.assertEqual(llc1[i], sub0[i], "XY0 don't match (1)")
            self.assertEqual(llc1[i] + llc2[i], subsub0[i], "XY0 don't match (2)")
        
        subCrpix = subImg.getWcs().getPixelOrigin()
        subsubCrpix = subSubImg.getWcs().getPixelOrigin()

        for i in range(2):
            self.assertAlmostEqual(subCrpix[i], subsubCrpix[i], 6, "crpix don't match")
        
     
    def testRoundTrip(self):
        """Test that saving and retrieving an image doesn't alter the metadata"""
        llc = afwGeom.Point2I(20, 30)
        bbox = afwGeom.Box2I(llc, afwGeom.Extent2I(60, 50))
        for deep in (False, True):
            subImg = afwImage.ExposureF(self.parent, bbox, afwImage.LOCAL, deep)

            outFile = "tmp2.fits"
            subImg.writeFits(outFile)
            newImg = afwImage.ExposureF(outFile)
            os.system("cp %s tmp-%s.fits" % (outFile, deep))
            os.remove(outFile)

            subXY0 = subImg.getMaskedImage().getXY0()
            newXY0 = newImg.getMaskedImage().getXY0()

            parentCrpix = self.parent.getWcs().getPixelOrigin()
            subCrpix = subImg.getWcs().getPixelOrigin()
            newCrpix = newImg.getWcs().getPixelOrigin()

            if False:
                print self.parent.getWcs().getFitsMetadata().toString()
                print subImg.getWcs().getFitsMetadata().toString()
                print newImg.getWcs().getFitsMetadata().toString()

            for i in range(2):
                self.assertEqual(subXY0[i], newXY0[i], "Origin has changed; deep = %s" % deep)
                self.assertAlmostEqual(subCrpix[i], newCrpix[i], 6,"crpix has changed; deep = %s" % deep)

    def testFitsHeader(self):
        """Test that XY0 and crpix are written to the header as expected"""

        #getPixelOrigin() returns origin in lsst coordinates, so need to add 1 to 
        #compare to values stored in fits headers        
        parentCrpix = self.parent.getWcs().getPixelOrigin()
        
        #Make a sub-image
        x0, y0 = 20, 30
        llc = afwGeom.Point2I(x0, y0)
        bbox = afwGeom.Box2I(llc, afwGeom.Extent2I(60, 50))
        deep = False
        subImg = afwImage.ExposureF(self.parent, bbox, afwImage.LOCAL, deep)
        
        outFile = "tmp.fits"
        subImg.writeFits(outFile)
        hdr = afwImage.readMetadata(outFile)
        os.remove(outFile)
        
        self.assertTrue( hdr.exists("LTV1"), "LTV1 not saved to fits header")
        self.assertTrue( hdr.exists("LTV2"), "LTV2 not saved to fits header")
        self.assertEqual(hdr.get("LTV1"), -1*x0, "LTV1 has wrong value")
        self.assertEqual(hdr.get("LTV2"), -1*y0, "LTV1 has wrong value")


        self.assertTrue( hdr.exists("CRPIX1"), "CRPIX1 not saved to fits header")
        self.assertTrue( hdr.exists("CRPIX2"), "CRPIX2 not saved to fits header")
        
        fitsCrpix = [hdr.get("CRPIX1"), hdr.get("CRPIX2")]
        self.assertAlmostEqual(fitsCrpix[0] - hdr.get("LTV1"), parentCrpix[0]+1, 6, "CRPIX1 saved wrong")
        self.assertAlmostEqual(fitsCrpix[1] - hdr.get("LTV2"), parentCrpix[1]+1, 6, "CRPIX2 saved wrong")
        
#####

def suite():
    """Returns a suite containing all the test cases in this module."""
    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(SavingSubImagesTest)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit=False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
