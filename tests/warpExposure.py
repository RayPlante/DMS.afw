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

"""Test warpExposure
"""
import os

import unittest

import numpy

import eups
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.image.testUtils as imageTestUtils
import lsst.utils.tests as utilsTests
import lsst.pex.logging as logging

import lsst.afw.display.ds9 as ds9
try:
    display
except:
    display = False
    VERBOSITY = 0                       # increase to see trace
    # set True to save afw-warped images as FITS files
    SAVE_FITS_FILES = False
    # set True to save failed afw-warped images as FITS files even if SAVE_FITS_FILES is False
    SAVE_FAILED_FITS_FILES = False

logging.Debug("lsst.afw.math", VERBOSITY)

dataDir = eups.productDir("afwdata")
if not dataDir:
    raise RuntimeError("Must set up afwdata to run these tests")

originalExposureName = "med"
originalExposurePath = os.path.join(dataDir, originalExposureName)
subExposureName = "medsub"
subExposurePath = os.path.join(dataDir, originalExposureName)
originalFullExposureName = os.path.join("CFHT", "D4", "cal-53535-i-797722_1")
originalFullExposurePath = os.path.join(dataDir, originalFullExposureName)

class WarpExposureTestCase(unittest.TestCase):
    """Test case for warpExposure
    """
    def testNullWarpExposure(self, interpLength=10):
        """Test that warpExposure maps an image onto itself.
        
        Note:
        - edge pixels must be ignored
        - bad mask pixels get smeared out so we have to excluded all bad mask pixels
          from the output image when comparing masks.
        """
        originalExposure = afwImage.ExposureF(originalExposurePath)
        afwWarpedExposure = afwImage.ExposureF(originalExposurePath)
        warpingKernel = afwMath.LanczosWarpingKernel(4)
        afwMath.warpExposure(afwWarpedExposure, originalExposure, warpingKernel, interpLength)
        if SAVE_FITS_FILES:
            afwWarpedExposure.writeFits("afwWarpedExposureNull")
        afwWarpedMaskedImage = afwWarpedExposure.getMaskedImage()
        afwWarpedMask = afwWarpedMaskedImage.getMask()
        edgeBitMask = afwWarpedMask.getPlaneBitMask("EDGE")
        if edgeBitMask == 0:
            self.fail("warped mask has no EDGE bit")
        afwWarpedMaskedImageArrSet = imageTestUtils.arraysFromMaskedImage(afwWarpedMaskedImage)
        afwWarpedMaskArr = afwWarpedMaskedImageArrSet[1]
        
        # compare all non-edge pixels of image and variance, but relax specs a bit
        # because of minor noise introduced by bad pixels
        edgeMaskArr = afwWarpedMaskArr & edgeBitMask
        originalMaskedImageArrSet = imageTestUtils.arraysFromMaskedImage(originalExposure.getMaskedImage())
        errStr = imageTestUtils.maskedImagesDiffer(afwWarpedMaskedImageArrSet, originalMaskedImageArrSet,
            doMask=False, skipMaskArr=edgeMaskArr, atol=1e-5)
        if errStr:
            self.fail("afw null-warped MaskedImage (all pixels, relaxed tolerance): %s" % (errStr,))
        
        # compare good pixels of image, mask and variance using full tolerance
        errStr = imageTestUtils.maskedImagesDiffer(afwWarpedMaskedImageArrSet, originalMaskedImageArrSet,
            doImage=False, doVariance=False, skipMaskArr=afwWarpedMaskArr)
        if errStr:
            self.fail("afw null-warped MaskedImage (good pixels, max tolerance): %s" % (errStr,))

    def testNullWarpImage(self, interpLength=10):
        """Test that warpImage maps an image onto itself.
        """
        originalExposure = afwImage.ExposureF(originalExposurePath)
        afwWarpedExposure = afwImage.ExposureF(originalExposurePath)
        originalImage = originalExposure.getMaskedImage().getImage()
        afwWarpedImage = afwWarpedExposure.getMaskedImage().getImage()
        originalWcs = originalExposure.getWcs()
        afwWarpedWcs = afwWarpedExposure.getWcs()
        warpingKernel = afwMath.LanczosWarpingKernel(4)
        afwMath.warpImage(afwWarpedImage, afwWarpedWcs, originalImage,
                          originalWcs, warpingKernel, interpLength)
        if SAVE_FITS_FILES:
            afwWarpedImage.writeFits("afwWarpedImageNull.fits")
        afwWarpedImageArr = imageTestUtils.arrayFromImage(afwWarpedImage)
        edgeMaskArr = numpy.isnan(afwWarpedImageArr)
        originalImageArr = imageTestUtils.arrayFromImage(originalImage)
        # relax specs a bit because of minor noise introduced by bad pixels
        errStr = imageTestUtils.imagesDiffer(originalImageArr, originalImageArr,
            skipMaskArr=edgeMaskArr)
        if errStr:
            self.fail("afw null-warped Image: %s" % (errStr,))

    def testNullWcs(self, interpLength=10):
        """Cannot warp from or into an exposure without a Wcs.
        """
        exposureWithWcs = afwImage.ExposureF(originalExposurePath)
        mi = exposureWithWcs.getMaskedImage()
        exposureWithoutWcs = afwImage.ExposureF(mi.getWidth(), mi.getHeight())
        warpingKernel = afwMath.BilinearWarpingKernel()
        try:
            afwMath.warpExposure(exposureWithWcs, exposureWithoutWcs, warpingKernel, interpLength)
            self.fail("warping from a source Exception with no Wcs should fail")
        except Exception:
            pass
        try:
            afwMath.warpExposure(exposureWithoutWcs, exposureWithWcs, warpingKernel, interpLength)
            self.fail("warping into a destination Exception with no Wcs should fail")
        except Exception:
            pass
    
    def testWarpIntoSelf(self, interpLength=10):
        """Cannot warp in-place
        """
        originalExposure = afwImage.ExposureF(100, 100)
        warpingKernel = afwMath.BilinearWarpingKernel()
        try:
            afwMath.warpExposure(originalExposure, originalExposure, warpingKernel, interpLength)
            self.fail("warpExposure in place (dest is src) should fail")
        except Exception:
            pass
        try:
            afwMath.warpImage(originalExposure.getMaskedImage(), originalExposure.getWcs(),
                originalExposure.getMaskedImage(), originalExposure.getWcs(), warpingKernel, interpLength)
            self.fail("warpImage<MaskedImage> in place (dest is src) should fail")
        except Exception:
            pass
        try:
            afwMath.warpImage(originalExposure.getImage(), originalExposure.getWcs(),
                originalExposure.getImage(), originalExposure.getWcs(), warpingKernel, interpLength)
            self.fail("warpImage<Image> in place (dest is src) should fail")
        except Exception:
            pass

    def testMatchSwarpBilinearImage(self):
        """Test that warpExposure matches swarp using a bilinear warping kernel
        """
        self.compareToSwarp("bilinear", useWarpExposure=False, atol=0.15)

    def testMatchSwarpBilinearExposure(self):
        """Test that warpExposure matches swarp using a bilinear warping kernel
        """
        self.compareToSwarp("bilinear", useWarpExposure=True, useSubregion=False, useDeepCopy=True)

    def testMatchSwarpLanczos2Image(self):
        """Test that warpExposure matches swarp using a lanczos2 warping kernel
        """
        self.compareToSwarp("lanczos2", useWarpExposure=False)

    def testMatchSwarpLanczos2Exposure(self):
        """Test that warpExposure matches swarp using a lanczos2 warping kernel.
        """
        self.compareToSwarp("lanczos2", useWarpExposure=True)

    def testMatchSwarpLanczos2SubExposure(self):
        """Test that warpExposure matches swarp using a lanczos2 warping kernel with a subexposure
        """
        for useDeepCopy in (False, True):
            self.compareToSwarp("lanczos2", useWarpExposure=True, useSubregion=True, useDeepCopy=useDeepCopy)

    def testMatchSwarpLanczos3Image(self):
        """Test that warpExposure matches swarp using a lanczos2 warping kernel
        """
        self.compareToSwarp("lanczos3", useWarpExposure=False)

    def testMatchSwarpLanczos3(self):
        """Test that warpExposure matches swarp using a lanczos4 warping kernel.
        """
        self.compareToSwarp("lanczos3", useWarpExposure=True)

    def testMatchSwarpLanczos4Image(self):
        """Test that warpExposure matches swarp using a lanczos2 warping kernel
        """
        self.compareToSwarp("lanczos4", useWarpExposure=False)

    def testMatchSwarpLanczos4(self):
        """Test that warpExposure matches swarp using a lanczos4 warping kernel.
        """
        self.compareToSwarp("lanczos4", useWarpExposure=True)

    def testMatchSwarpNearestExposure(self):
        """Test that warpExposure matches swarp using a nearest neighbor warping kernel
        """
        self.compareToSwarp("nearest", useWarpExposure=True, atol=60)

    def compareToSwarp(self, kernelName, 
                       useWarpExposure=True, useSubregion=False, useDeepCopy=False,
                       interpLength=10, cacheSize=100000,
                       rtol=4e-05, atol=1e-2):
        """Compare warpExposure to swarp for given warping kernel.
        
        Note that swarp only warps the image plane, so only test that plane.
        
        Inputs:
        - kernelName: name of kernel in the form used by afwImage.makeKernel
        - useWarpExposure: if True, call warpExposure to warp an ExposureF,
            else call warpImage to warp an ImageF
        - useSubregion: if True then the original source exposure (from which the usual
            test exposure was extracted) is read and the correct subregion extracted
        - useDeepCopy: if True then the copy of the subimage is a deep copy,
            else it is a shallow copy; ignored if useSubregion is False
        - interpLength: interpLength argument for lsst.afw.math.warpExposure
        - cacheSize: cacheSize argument for lsst.afw.math.SeparableKernel.computeCache;
            0 disables the cache
            10000 gives some speed improvement but less accurate results (atol must be increased)
            100000 gives better accuracy but no speed improvement in this test
        - rtol: relative tolerance as used by numpy.allclose
        - atol: absolute tolerance as used by numpy.allclose
        """
        warpingKernel = afwMath.makeWarpingKernel(kernelName)
        warpingKernel.computeCache(cacheSize)

        if useSubregion:
            originalFullExposure = afwImage.ExposureF(originalExposurePath)
            # "medsub" is a subregion of med starting at 0-indexed pixel (40, 150) of size 145 x 200
            bbox = afwImage.BBox(afwImage.PointI(40, 150), 145, 200)
            originalExposure = afwImage.ExposureF(originalFullExposure, bbox, useDeepCopy)
            swarpedImageName = "medsubswarp1%s.fits" % (kernelName,)
        else:
            originalExposure = afwImage.ExposureF(originalExposurePath)
            swarpedImageName = "medswarp1%s.fits" % (kernelName,)

        swarpedImagePath = os.path.join(dataDir, swarpedImageName)
        swarpedDecoratedImage = afwImage.DecoratedImageF(swarpedImagePath)
        swarpedImage = swarpedDecoratedImage.getImage()
        swarpedMetadata = swarpedDecoratedImage.getMetadata()
        warpedWcs = afwImage.makeWcs(swarpedMetadata)
        destWidth = swarpedImage.getWidth()
        destHeight = swarpedImage.getHeight()
        
        if useWarpExposure:
            # path for saved afw-warped image
            afwWarpedImagePath = "afwWarpedExposure1%s" % (kernelName,)
    
            afwWarpedMaskedImage = afwImage.MaskedImageF(destWidth, destHeight)
            afwWarpedExposure = afwImage.ExposureF(afwWarpedMaskedImage, warpedWcs)
            afwMath.warpExposure(afwWarpedExposure, originalExposure, warpingKernel, interpLength)
            if SAVE_FITS_FILES:
                afwWarpedExposure.writeFits(afwWarpedImagePath)
            if display:
                ds9.mtv(afwWarpedExposure, frame=1, title="Warped")
    
            afwWarpedMask = afwWarpedMaskedImage.getMask()
            edgeBitMask = afwWarpedMask.getPlaneBitMask("EDGE")
            if edgeBitMask == 0:
                self.fail("warped mask has no EDGE bit")
            afwWarpedMaskedImageArrSet = imageTestUtils.arraysFromMaskedImage(afwWarpedMaskedImage)
            afwWarpedMaskArr = afwWarpedMaskedImageArrSet[1]
    
            swarpedMaskedImage = afwImage.MaskedImageF(swarpedImage)
            swarpedMaskedImageArrSet = imageTestUtils.arraysFromMaskedImage(swarpedMaskedImage)

            if display:
                ds9.mtv(swarpedMaskedImage, frame=2, title="SWarped")
            
            errStr = imageTestUtils.maskedImagesDiffer(afwWarpedMaskedImageArrSet, swarpedMaskedImageArrSet,
                doImage=True, doMask=False, doVariance=False, skipMaskArr=afwWarpedMaskArr,
                rtol=rtol, atol=atol)
            if errStr:
                if SAVE_FAILED_FITS_FILES:
                    afwWarpedExposure.writeFits(afwWarpedImagePath)
                    print "Saved failed afw-warped exposure as: %s" % (afwWarpedImagePath,)
                self.fail("afw and swarp %s-warped %s (ignoring bad pixels)" % (kernelName, errStr))
        else:
            # path for saved afw-warped image
            afwWarpedImagePath = "afwWarpedImage1%s.fits" % (kernelName,)
    
            afwWarpedImage = afwImage.ImageF(destWidth, destHeight)
            originalImage = originalExposure.getMaskedImage().getImage()
            originalWcs = originalExposure.getWcs()
            afwMath.warpImage(afwWarpedImage, warpedWcs, originalImage,
                              originalWcs, warpingKernel, interpLength)
            if display:
                ds9.mtv(afwWarpedImage, frame=1, title="Warped")
                ds9.mtv(swarpedImage, frame=2, title="SWarped")
                diff = swarpedImage.Factory(swarpedImage, True)
                diff -= afwWarpedImage
                ds9.mtv(diff, frame=3, title="swarp - afw")
            if SAVE_FITS_FILES:
                afwWarpedImage.writeFits(afwWarpedImagePath)
            
            afwWarpedImageArr = imageTestUtils.arrayFromImage(afwWarpedImage)
            swarpedImageArr = imageTestUtils.arrayFromImage(swarpedImage)
            edgeMaskArr = numpy.isnan(afwWarpedImageArr)
            errStr = imageTestUtils.imagesDiffer(afwWarpedImageArr, swarpedImageArr,
                skipMaskArr=edgeMaskArr, rtol=rtol, atol=atol)
            if errStr:
                if SAVE_FAILED_FITS_FILES:
                    # save the image anyway
                    afwWarpedImage.writeFits(afwWarpedImagePath)
                    print "Saved failed afw-warped image as: %s" % (afwWarpedImagePath,)
                self.fail("afw and swarp %s-warped images do not match (ignoring NaN pixels): %s" % \
                    (kernelName, errStr))
    
    def compareMaskedImages(self, maskedImage1, maskedImage2, descr="",
        doImage=True, doMask=True, doVariance=True, skipMaskArr=None, rtol=1.0e-05, atol=1e-08):
        """Compare pixels from two masked images
        
        Inputs:
        - maskedImage1: first masked image to compare
        - maskedImage2: second masked image to compare
        - descr: a short description of the inputs
        - doImage: compare image planes if True
        - doMask: compare mask planes if True
        - doVariance: compare variance planes if True
        - skipMaskArr: pixels to ingore on the image, mask and variance arrays; nonzero values are skipped
        
        Returns None if all is well, or an error string if any plane did not match.
        The error string is one of:
        <plane> plane does not match
        (<plane1>, <plane2>,...) planes do not match
        """
        badPlanes = []
        for (doPlane, planeName) in ((doImage, "image"), (doMask, "mask"), (doVariance, "variance")):
            if not doPlane:
                continue
            funcName = "get%s" % (planeName.title(),)
            imageDescr = "%s: %s plane" % (descr, planeName)
            image1 = getattr(maskedImage1, funcName)()
            image2 = getattr(maskedImage2, funcName)()
            imageOK = self.compareImages(image1, image2, descr=imageDescr, skipMaskArr=skipMaskArr,
                                         rtol=rtol, atol=atol)
            if not imageOK:
                badPlanes.append(planeName)
        if not badPlanes:
            return None
        if len(badPlanes) > 1:
            return "%s planes do not match" % (badPlanes,)
        return "%s plane does not match" % (badPlanes[0],)
    
    def compareImages(self, image1, image2, descr="", skipMaskArr=None, rtol=1.0e-05, atol=1e-08):
        """Return True if two images are nearly equal, False otherwise
        """
        arr1 = imageTestUtils.arrayFromImage(image1)
        arr2 = imageTestUtils.arrayFromImage(image2)

        if skipMaskArr != None:
            maskedArr1 = numpy.ma.array(arr1, copy=False, mask = skipMaskArr)
            maskedArr2 = numpy.ma.array(arr2, copy=False, mask = skipMaskArr)
            filledArr1 = maskedArr1.filled(0.0)
            filledArr2 = maskedArr2.filled(0.0)
        else:
            filledArr1 = arr1
            filledArr2 = arr2
        
        if not numpy.allclose(filledArr1, filledArr2, rtol=rtol, atol=atol):
            errArr =  numpy.abs(filledArr1 - filledArr2) - (filledArr2 * rtol) + atol
            maxErr = errArr.max()
            maxPosInd = numpy.where(errArr==maxErr)
            maxPosTuple = (maxPosInd[0][0], maxPosInd[1][0])
            relErr = numpy.abs(filledArr1 - filledArr2) / (filledArr1 + filledArr2)
            print "%s: maxErr=%s at position %s; value=%s vs. %s; relErr=%s" % \
                (descr, maxErr, maxPosTuple, filledArr1[maxPosInd][0], filledArr2[maxPosInd][0], relErr)
            return False
        return True
        
        
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """
    Returns a suite containing all the test cases in this module.
    """
    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(WarpExposureTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)

    return unittest.TestSuite(suites)

def run(doExit=False):
    """Run the tests"""
    utilsTests.run(suite(), doExit)

if __name__ == "__main__":
    run(True)
