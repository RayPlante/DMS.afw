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
import math
import unittest

import numpy

import lsst.utils.tests as utilsTests
import lsst.pex.exceptions as pexExcept
import lsst.pex.logging as pexLog
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.image.testUtils as imTestUtils

VERBOSITY = 0 # increase to see trace

pexLog.Debug("lsst.afw", VERBOSITY)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def makeGaussianKernelList(kWidth, kHeight, gaussParamsList):
    """Create a list of gaussian kernels.

    This is useful for constructing a LinearCombinationKernel.
    
    Inputs:
    - kWidth, kHeight: width and height of kernel
    - gaussParamsList: a list of parameters for GaussianFunction2D (each a 3-tuple of floats)
    """
    gaussParamsList = [
        (1.5, 1.5, 0.0),
        (2.5, 1.5, 0.0),
        (2.5, 1.5, math.pi / 2.0),
    ]
    kVec = afwMath.KernelList()
    for majorSigma, minorSigma, angle in gaussParamsList:
        kFunc = afwMath.GaussianFunction2D(majorSigma, minorSigma, angle)
        kVec.append(afwMath.AnalyticKernel(kWidth, kHeight, kFunc))
    return kVec

def makeDeltaFunctionKernelList(kWidth, kHeight):
    """Create a list of delta function kernels

    This is useful for constructing a LinearCombinationKernel.
    """
    kVec = afwMath.KernelList()
    for activeCol in range(kWidth):
        for activeRow in range(kHeight):
            kVec.append(afwMath.DeltaFunctionKernel(kWidth, kHeight, afwImage.PointI(activeCol, activeRow)))
    return kVec

class KernelTestCase(unittest.TestCase):
    """A test case for Kernels"""
    def testAnalyticKernel(self):
        """Test AnalyticKernel using a Gaussian function
        """
        kWidth = 5
        kHeight = 8

        gaussFunc = afwMath.GaussianFunction2D(1.0, 1.0, 0.0)
        kernel = afwMath.AnalyticKernel(kWidth, kHeight, gaussFunc)
        self.basicTests(kernel, 3)
        fArr = numpy.zeros(shape=[kernel.getWidth(), kernel.getHeight()], dtype=float)
        for xsigma in (0.1, 1.0, 3.0):
            for ysigma in (0.1, 1.0, 3.0):
                gaussFunc.setParameters((xsigma, ysigma, 0.0))
                # compute array of function values and normalize
                for row in range(kernel.getHeight()):
                    y = row - kernel.getCtrY()
                    for col in range(kernel.getWidth()):
                        x = col - kernel.getCtrX()
                        fArr[col, row] = gaussFunc(x, y)
                fArr /= fArr.sum()
                
                kernel.setKernelParameters((xsigma, ysigma, 0.0))
                kImage = afwImage.ImageD(kernel.getDimensions())
                kernel.computeImage(kImage, True)
                
                kArr = imTestUtils.arrayFromImage(kImage)
                if not numpy.allclose(fArr, kArr):
                    self.fail("%s = %s != %s for xsigma=%s, ysigma=%s" % \
                        (kernel.__class__.__name__, kArr, fArr, xsigma, ysigma))

        kernel.setKernelParameters((0.5, 1.1, 0.3))
        kernelClone = kernel.clone()
        errStr = self.compareKernels(kernel, kernelClone)
        if errStr:
            self.fail(errStr)
        
        kernel.setKernelParameters((1.5, 0.2, 0.7))
        errStr = self.compareKernels(kernel, kernelClone)
        if not errStr:
            self.fail("Clone was modified by changing original's kernel parameters")

        self.checkComputeImage(kernel)

    def testShrinkGrowBBox(self):
        """Test Kernel methods shrinkBBox and growBBox
        """
        boxStart = afwGeom.makePointI(3, -3)
        for kWidth in (1, 2, 6):
            for kHeight in (1, 2, 5):
                for deltaWidth in (-1, 0, 1, 20):
                    fullWidth = kWidth + deltaWidth
                    for deltaHeight in (-1, 0, 1, 20):
                        fullHeight = kHeight + deltaHeight
                        kernel = afwMath.DeltaFunctionKernel(kWidth, kHeight, afwImage.PointI(0, 0))
                        fullBBox = afwGeom.BoxI(boxStart, afwGeom.makeExtentI(fullWidth, fullHeight))
                        if (fullWidth < kWidth) or (fullHeight < kHeight):
                            self.assertRaises(Exception, kernel.shrinkBBox, fullBBox)
                            continue

                        shrunkBBox = kernel.shrinkBBox(fullBBox)
                        self.assert_((shrunkBBox.getWidth() == fullWidth + 1 - kWidth) and
                            (shrunkBBox.getHeight() == fullHeight + 1 - kHeight),
                            "shrinkBBox returned box of wrong size")
                        self.assert_((shrunkBBox.getMinX() == boxStart[0] + kernel.getCtrX()) and
                            (shrunkBBox.getMinY() == boxStart[1] + kernel.getCtrY()),
                            "shrinkBBox returned box with wrong minimum")
                        newFullBBox = kernel.growBBox(shrunkBBox)
                        self.assert_(newFullBBox == fullBBox, "growBBox(shrinkBBox(x)) != x")
    
    def testDeltaFunctionKernel(self):
        """Test DeltaFunctionKernel
        """
        for kWidth in range(1, 4):
            for kHeight in range(1, 4):
                for activeCol in range(kWidth):
                    for activeRow in range(kHeight):
                        kernel = afwMath.DeltaFunctionKernel(kWidth, kHeight,
                                                             afwImage.PointI(activeCol, activeRow))
                        kImage = afwImage.ImageD(kernel.getDimensions())
                        kSum = kernel.computeImage(kImage, False)
                        self.assertEqual(kSum, 1.0)
                        kArr = imTestUtils.arrayFromImage(kImage)
                        self.assertEqual(kArr[activeCol, activeRow], 1.0)
                        kArr[activeCol, activeRow] = 0.0
                        self.assertEqual(kArr.sum(), 0.0)

                        errStr = self.compareKernels(kernel, kernel.clone())
                        if errStr:
                            self.fail(errStr)

                utilsTests.assertRaisesLsstCpp(self, pexExcept.InvalidParameterException,
                    afwMath.DeltaFunctionKernel, 0, kHeight, afwImage.PointI(kWidth, kHeight))
                utilsTests.assertRaisesLsstCpp(self, pexExcept.InvalidParameterException,
                    afwMath.DeltaFunctionKernel, kWidth, 0, afwImage.PointI(kWidth, kHeight))
                            
        kernel = afwMath.DeltaFunctionKernel(5, 6, afwImage.PointI(1, 1))
        self.basicTests(kernel, 0)

        self.checkComputeImage(kernel)

    def testFixedKernel(self):
        """Test FixedKernel using a ramp function
        """
        kWidth = 5
        kHeight = 6
        
        inArr = numpy.arange(kWidth * kHeight, dtype=float)
        inArr.shape = [kWidth, kHeight]

        inImage = afwImage.ImageD(kWidth, kHeight)
        for row in range(inImage.getHeight()):
            for col in range(inImage.getWidth()):
                inImage.set(col, row, inArr[col, row])
        
        kernel = afwMath.FixedKernel(inImage)
        self.basicTests(kernel, 0)
        outImage = afwImage.ImageD(kernel.getDimensions())
        kernel.computeImage(outImage, False)
        
        outArr = imTestUtils.arrayFromImage(outImage)
        if not numpy.allclose(inArr, outArr):
            self.fail("%s = %s != %s (not normalized)" % \
                (kernel.__class__.__name__, inArr, outArr))
        
        normInArr = inArr / inArr.sum()
        normOutImage = afwImage.ImageD(kernel.getDimensions())
        kernel.computeImage(normOutImage, True)
        normOutArr = imTestUtils.arrayFromImage(normOutImage)
        if not numpy.allclose(normOutArr, normInArr):
            self.fail("%s = %s != %s (normalized)" % \
                (kernel.__class__.__name__, normInArr, normOutArr))

        errStr = self.compareKernels(kernel, kernel.clone())
        if errStr:
            self.fail(errStr)

        self.checkComputeImage(kernel)
    
    def testLinearCombinationKernelDelta(self):
        """Test LinearCombinationKernel using a set of delta basis functions
        """
        kWidth = 3
        kHeight = 2
        
        # create list of kernels
        basisKernelList = makeDeltaFunctionKernelList(kWidth, kHeight)
        basisImArrList = []
        for basisKernel in basisKernelList:
            basisImage = afwImage.ImageD(basisKernel.getDimensions())
            basisKernel.computeImage(basisImage, True)
            basisImArrList.append(imTestUtils.arrayFromImage(basisImage))

        kParams = [0.0]*len(basisKernelList)
        kernel = afwMath.LinearCombinationKernel(basisKernelList, kParams)
        self.assert_(kernel.isDeltaFunctionBasis())
        self.basicTests(kernel, len(kParams))
        for ii in range(len(basisKernelList)):
            kParams = [0.0]*len(basisKernelList)
            kParams[ii] = 1.0
            kernel.setKernelParameters(kParams)
            kIm = afwImage.ImageD(kernel.getDimensions())
            kernel.computeImage(kIm, True)
            kImArr = imTestUtils.arrayFromImage(kIm)
            if not numpy.allclose(kImArr, basisImArrList[ii]):
                self.fail("%s = %s != %s for the %s'th basis kernel" % \
                    (kernel.__class__.__name__, kImArr, basisImArrList[ii], ii))
        
        kernelClone = kernel.clone()
        errStr = self.compareKernels(kernel, kernelClone)
        if errStr:
            self.fail(errStr)

        self.checkComputeImage(kernel)
    
    def testComputeImageRaise(self):
        """Test Kernel.computeImage raises OverflowException iff doNormalize True and kernel sum exactly 0
        """
        kWidth = 4
        kHeight = 3
        
        polyFunc1 = afwMath.PolynomialFunction1D(0)
        polyFunc2 = afwMath.PolynomialFunction2D(0)
        analKernel = afwMath.AnalyticKernel(kWidth, kHeight, polyFunc2)
        kImage = afwImage.ImageD(analKernel.getDimensions())
        for coeff in (-1, -1e-99, 0, 1e99, 1):
            analKernel.setKernelParameters([coeff])

            analKernel.computeImage(kImage, False)
            fixedKernel = afwMath.FixedKernel(kImage)

            sepKernel = afwMath.SeparableKernel(kWidth, kHeight, polyFunc1, polyFunc1)
            sepKernel.setKernelParameters([coeff, coeff])

            kernelList = afwMath.KernelList()
            kernelList.append(analKernel)
            lcKernel = afwMath.LinearCombinationKernel(kernelList, [1])
            self.assert_(not lcKernel.isDeltaFunctionBasis())

            doRaise = (coeff == 0)
            self.basicTestComputeImageRaise(analKernel,  doRaise, "AnalyticKernel")
            self.basicTestComputeImageRaise(fixedKernel, doRaise, "FixedKernel")
            self.basicTestComputeImageRaise(sepKernel,   doRaise, "SeparableKernel")
            self.basicTestComputeImageRaise(lcKernel,    doRaise, "LinearCombinationKernel")
        
        lcKernel.setKernelParameters([0])
        self.basicTestComputeImageRaise(lcKernel, True, "LinearCombinationKernel")

    def testLinearCombinationKernelAnalytic(self):
        """Test LinearCombinationKernel using analytic basis kernels.
        
        The basis kernels are mutable so that we can verify that the
        LinearCombinationKernel has private copies of the basis kernels.
        """
        kWidth = 5
        kHeight = 8
        
        # create list of kernels
        basisImArrList = []
        basisKernelList = afwMath.KernelList()
        for basisKernelParams in [(1.2, 0.3, 1.570796), (1.0, 0.2, 0.0)]:
            basisKernelFunction = afwMath.GaussianFunction2D(*basisKernelParams)
            basisKernel = afwMath.AnalyticKernel(kWidth, kHeight, basisKernelFunction)
            basisImage = afwImage.ImageD(basisKernel.getDimensions())
            basisKernel.computeImage(basisImage, True)
            basisImArrList.append(imTestUtils.arrayFromImage(basisImage))
            basisKernelList.append(basisKernel)

        kParams = [0.0]*len(basisKernelList)
        kernel = afwMath.LinearCombinationKernel(basisKernelList, kParams)
        self.assert_(not kernel.isDeltaFunctionBasis())
        self.basicTests(kernel, len(kParams))
        
        # make sure the linear combination kernel has private copies of its basis kernels
        # by altering the local basis kernels and making sure the new images do NOT match
        modBasisImArrList = []
        for basisKernel in basisKernelList:
            basisKernel.setKernelParameters((0.4, 0.5, 0.6))
            modBasisImage = afwImage.ImageD(basisKernel.getDimensions())
            basisKernel.computeImage(modBasisImage, True)
            modBasisImArrList.append(imTestUtils.arrayFromImage(modBasisImage))
        
        for ii in range(len(basisKernelList)):
            kParams = [0.0]*len(basisKernelList)
            kParams[ii] = 1.0
            kernel.setKernelParameters(kParams)
            kIm = afwImage.ImageD(kernel.getDimensions())
            kernel.computeImage(kIm, True)
            kImArr = imTestUtils.arrayFromImage(kIm)
            if not numpy.allclose(kImArr, basisImArrList[ii]):
                self.fail("%s = %s != %s for the %s'th basis kernel" % \
                    (kernel.__class__.__name__, kImArr, basisImArrList[ii], ii))
            if numpy.allclose(kImArr, modBasisImArrList[ii]):
                self.fail("%s = %s == %s for *modified* %s'th basis kernel" % \
                    (kernel.__class__.__name__, kImArr, modBasisImArrList[ii], ii))
        
        kernelClone = kernel.clone()
        errStr = self.compareKernels(kernel, kernelClone)
        if errStr:
            self.fail(errStr)

    def testSeparableKernel(self):
        """Test SeparableKernel using a Gaussian function
        """
        kWidth = 5
        kHeight = 8

        gaussFunc1 = afwMath.GaussianFunction1D(1.0)
        kernel = afwMath.SeparableKernel(kWidth, kHeight, gaussFunc1, gaussFunc1)
        self.basicTests(kernel, 2)
        fArr = numpy.zeros(shape=[kernel.getWidth(), kernel.getHeight()], dtype=float)
        gaussFunc = afwMath.GaussianFunction2D(1.0, 1.0, 0.0)
        for xsigma in (0.1, 1.0, 3.0):
            gaussFunc1.setParameters((xsigma,))
            for ysigma in (0.1, 1.0, 3.0):
                gaussFunc.setParameters((xsigma, ysigma, 0.0))
                # compute array of function values and normalize
                for row in range(kernel.getHeight()):
                    y = row - kernel.getCtrY()
                    for col in range(kernel.getWidth()):
                        x = col - kernel.getCtrX()
                        fArr[col, row] = gaussFunc(x, y)
                fArr /= fArr.sum()
                
                kernel.setKernelParameters((xsigma, ysigma))
                kImage = afwImage.ImageD(kernel.getDimensions())
                kernel.computeImage(kImage, True)
                kArr = imTestUtils.arrayFromImage(kImage)
                if not numpy.allclose(fArr, kArr):
                    self.fail("%s = %s != %s for xsigma=%s, ysigma=%s" % \
                        (kernel.__class__.__name__, kArr, fArr, xsigma, ysigma))
        kernelClone = kernel.clone()
        errStr = self.compareKernels(kernel, kernelClone)
        if errStr:
            self.fail(errStr)
        
        kernel.setKernelParameters((1.2, 0.6))
        errStr = self.compareKernels(kernel, kernelClone)
        if not errStr:
            self.fail("Clone was modified by changing original's kernel parameters")

        self.checkComputeImage(kernel)
        
    def testMakeBadKernels(self):
        """Attempt to make various invalid kernels; make sure the constructor shows an exception
        """
        kWidth = 4
        kHeight = 3
        
        gaussFunc1 = afwMath.GaussianFunction1D(1.0)
        gaussFunc2 = afwMath.GaussianFunction2D(1.0, 1.0, 0.0)
        spFunc = afwMath.PolynomialFunction2D(1)
        kernelList = afwMath.KernelList()
        kernelList.append(afwMath.FixedKernel(afwImage.ImageD(kWidth, kHeight, 0.1)))
        kernelList.append(afwMath.FixedKernel(afwImage.ImageD(kWidth, kHeight, 0.2)))

        for numKernelParams in (2, 4):
            spFuncList = afwMath.Function2DList()
            for ii in range(numKernelParams):
                spFuncList.append(spFunc.clone())
            try:
                afwMath.AnalyticKernel(kWidth, kHeight, gaussFunc2, spFuncList)
                self.fail("Should have failed with wrong # of spatial functions")
            except pexExcept.LsstCppException:
                pass
        
        for numKernelParams in (1, 3):
            spFuncList = afwMath.Function2DList()
            for ii in range(numKernelParams):
                spFuncList.append(spFunc.clone())
            try:
                afwMath.LinearCombinationKernel(kernelList, spFuncList)
                self.fail("Should have failed with wrong # of spatial functions")
            except pexExcept.LsstCppException:
                pass
            kParamList = [0.2]*numKernelParams
            try:
                afwMath.LinearCombinationKernel(kernelList, kParamList)
                self.fail("Should have failed with wrong # of kernel parameters")
            except pexExcept.LsstCppException:
                pass
            try:
                afwMath.SeparableKernel(kWidth, kHeight, gaussFunc1, gaussFunc1, spFuncList)
                self.fail("Should have failed with wrong # of spatial functions")
            except pexExcept.LsstCppException:
                pass

        for pointX in range(-1, kWidth+2):
            for pointY in range(-1, kHeight+2):
                if (0 <= pointX < kWidth) and (0 <= pointY < kHeight):
                    continue
                try:
                    afwMath.DeltaFunctionKernel(kWidth, kHeight, afwImage.PointI(pointX, pointY))
                    self.fail("Should have failed with point not on kernel")
                except pexExcept.LsstCppException, e:
                    pass
                    

    def testSVAnalyticKernel(self):
        """Test spatially varying AnalyticKernel using a Gaussian function
        
        Just tests cloning.
        """
        kWidth = 5
        kHeight = 8

        # spatial model
        spFunc = afwMath.PolynomialFunction2D(1)
        
        # spatial parameters are a list of entries, one per kernel parameter;
        # each entry is a list of spatial parameters
        sParams = (
            (1.0, 1.0, 0.0),
            (1.0, 0.0, 1.0),
            (0.5, 0.5, 0.5),
        )

        gaussFunc = afwMath.GaussianFunction2D(1.0, 1.0, 0.0)
        kernel = afwMath.AnalyticKernel(kWidth, kHeight, gaussFunc, spFunc)
        kernel.setSpatialParameters(sParams)
        
        kernelClone = kernel.clone()
        errStr = self.compareKernels(kernel, kernelClone)
        if errStr:
            self.fail(errStr)

        newSParams = (
            (0.1, 0.2, 0.5),
            (0.1, 0.5, 0.2),
            (0.2, 0.3, 0.3),
        )
        kernel.setSpatialParameters(newSParams)
        errStr = self.compareKernels(kernel, kernelClone)
        if not errStr:
            self.fail("Clone was modified by changing original's spatial parameters")

    def testSVLinearCombinationKernelFixed(self):
        """Test a spatially varying LinearCombinationKernel
        """
        kWidth = 3
        kHeight = 2

        # create image arrays for the basis kernels
        basisImArrList = []
        imArr = numpy.zeros((kWidth, kHeight), dtype=float)
        imArr += 0.1
        imArr[kWidth//2, :] = 0.9
        basisImArrList.append(imArr)
        imArr = numpy.zeros((kWidth, kHeight), dtype=float)
        imArr += 0.2
        imArr[:, kHeight//2] = 0.8
        basisImArrList.append(imArr)
        
        # create a list of basis kernels from the images
        basisKernelList = afwMath.KernelList()
        for basisImArr in basisImArrList:
            basisImage = imTestUtils.imageFromArray(basisImArr, retType=afwImage.ImageD)
            kernel = afwMath.FixedKernel(basisImage)
            basisKernelList.append(kernel)

        # create spatially varying linear combination kernel
        spFunc = afwMath.PolynomialFunction2D(1)
        
        # spatial parameters are a list of entries, one per kernel parameter;
        # each entry is a list of spatial parameters
        sParams = (
            (0.0, 1.0, 0.0),
            (0.0, 0.0, 1.0),
        )
        
        kernel = afwMath.LinearCombinationKernel(basisKernelList, spFunc)
        self.assert_(not kernel.isDeltaFunctionBasis())
        self.basicTests(kernel, 2, 3)
        kernel.setSpatialParameters(sParams)
        kImage = afwImage.ImageD(kWidth, kHeight)
        for colPos, rowPos, coeff0, coeff1 in [
            (0.0, 0.0, 0.0, 0.0),
            (1.0, 0.0, 1.0, 0.0),
            (0.0, 1.0, 0.0, 1.0),
            (1.0, 1.0, 1.0, 1.0),
            (0.5, 0.5, 0.5, 0.5),
        ]:
            kernel.computeImage(kImage, False, colPos, rowPos)
            kImArr = imTestUtils.arrayFromImage(kImage)
            refKImArr = (basisImArrList[0] * coeff0) + (basisImArrList[1] * coeff1)
            if not numpy.allclose(kImArr, refKImArr):
                self.fail("%s = %s != %s at colPos=%s, rowPos=%s" % \
                    (kernel.__class__.__name__, kImArr, refKImArr, colPos, rowPos))

        sParams = (
            (0.1, 1.0, 0.0),
            (0.1, 0.0, 1.0),
        )
        kernel.setSpatialParameters(sParams)
        kernelClone = kernel.clone()
        errStr = self.compareKernels(kernel, kernelClone)
        if errStr:
            self.fail(errStr)

        newSParams = (
            (0.1, 0.2, 0.5),
            (0.1, 0.5, 0.2),
        )
        kernel.setSpatialParameters(newSParams)
        errStr = self.compareKernels(kernel, kernelClone)
        if not errStr:
            self.fail("Clone was modified by changing original's spatial parameters")

    def testSVSeparableKernel(self):
        """Test spatially varying SeparableKernel using a Gaussian function
        
        Just tests cloning.
        """
        kWidth = 5
        kHeight = 8

        # spatial model
        spFunc = afwMath.PolynomialFunction2D(1)
        
        # spatial parameters are a list of entries, one per kernel parameter;
        # each entry is a list of spatial parameters
        sParams = (
            (1.0, 1.0, 0.0),
            (1.0, 0.0, 1.0),
        )

        gaussFunc = afwMath.GaussianFunction1D(1.0)
        kernel = afwMath.SeparableKernel(kWidth, kHeight, gaussFunc, gaussFunc, spFunc)
        kernel.setSpatialParameters(sParams)
        
        kernelClone = kernel.clone()
        errStr = self.compareKernels(kernel, kernelClone)
        if errStr:
            self.fail(errStr)

        newSParams = (
            (0.1, 0.2, 0.5),
            (0.1, 0.5, 0.2),
        )
        kernel.setSpatialParameters(newSParams)
        errStr = self.compareKernels(kernel, kernelClone)
        if not errStr:
            self.fail("Clone was modified by changing original's spatial parameters")
    
    def testSetCtr(self):
        """Test setCtrCol/Row"""
        kWidth = 3
        kHeight = 4

        gaussFunc = afwMath.GaussianFunction2D(1.0, 1.0, 0.0)
        kernel = afwMath.AnalyticKernel(kWidth, kHeight, gaussFunc)
        for xCtr in range(kWidth):
            kernel.setCtrX(xCtr)
            for yCtr in range(kHeight):
                kernel.setCtrY(yCtr)
                self.assertEqual(kernel.getCtrX(), xCtr)
                self.assertEqual(kernel.getCtrY(), yCtr)

    def testZeroSizeKernel(self):
        """Creating a kernel with width or height < 1 should raise an exception.
        
        Note: this ignores the default constructors, which produce kernels with height = width = 0.
        The default constructors are only intended to support persistence, not to produce useful kernels.
        """
        emptyImage = afwImage.ImageF(0, 0)
        gaussFunc2D = afwMath.GaussianFunction2D(1.0, 1.0, 0.0)
        gaussFunc1D = afwMath.GaussianFunction1D(1.0)
        zeroPoint = afwImage.PointI(0, 0)
        for kWidth in (-1, 0, 1):
            for kHeight in (-1, 0, 1):
                if (kHeight > 0) and (kWidth > 0):
                    continue
                if (kHeight >= 0) and (kWidth >= 0):
                    # don't try to create an image with negative dimensions
                    blankImage = afwImage.ImageF(kWidth, kHeight)
                    self.assertRaises(Exception, afwMath.FixedKernel, blankImage)
                self.assertRaises(Exception, afwMath.AnalyticKernel, kWidth, kHeight, gaussFunc2D)
                self.assertRaises(Exception, afwMath.SeparableKernel, kWidth, kHeight, gaussFunc1D, gaussFunc1D)
                self.assertRaises(Exception, afwMath.DeltaFunctionKernel, kWidth, kHeight, zeroPoint)

    def testRefactorDeltaLinearCombinationKernel(self):
        """Test LinearCombinationKernel.refactor with delta function basis kernels
        """
        kWidth = 4
        kHeight = 3

        for spOrder in (0, 1, 2):
            spFunc = afwMath.PolynomialFunction2D(spOrder)
            numSpParams = spFunc.getNParameters()

            basisKernelList = makeDeltaFunctionKernelList(kWidth, kHeight)
            kernel = afwMath.LinearCombinationKernel(basisKernelList, spFunc)

            numBasisKernels = kernel.getNKernelParameters()
            maxVal = 1.01 + ((numSpParams - 1) * 0.1)
            sParamList = [numpy.arange(kInd + 1.0, kInd + maxVal, 0.1) for kInd in range(numBasisKernels)]
            kernel.setSpatialParameters(sParamList)

            refKernel = kernel.refactor()
            self.assert_(refKernel)
            errStr = self.compareKernels(kernel, refKernel, compareParams=False)
            if errStr:
                self.fail("failed with %s for spOrder=%s (numSpCoeff=%s)" % (errStr, spOrder, numSpParams))

    def testRefactorGaussianLinearCombinationKernel(self):
        """Test LinearCombinationKernel.refactor with Gaussian basis kernels
        """
        kWidth = 4
        kHeight = 3

        for spOrder in (0, 1, 2):
            spFunc = afwMath.PolynomialFunction2D(spOrder)
            numSpParams = spFunc.getNParameters()

            gaussParamsList = [
                (1.5, 1.5, 0.0),
                (2.5, 1.5, 0.0),
                (2.5, 1.5, math.pi / 2.0),
            ]
            gaussBasisKernelList = makeGaussianKernelList(kWidth, kHeight, gaussParamsList)
            kernel = afwMath.LinearCombinationKernel(gaussBasisKernelList, spFunc)

            numBasisKernels = kernel.getNKernelParameters()
            maxVal = 1.01 + ((numSpParams - 1) * 0.1)
            sParamList = [numpy.arange(kInd + 1.0, kInd + maxVal, 0.1) for kInd in range(numBasisKernels)]
            kernel.setSpatialParameters(sParamList)

            refKernel = kernel.refactor()
            self.assert_(refKernel)
            errStr = self.compareKernels(kernel, refKernel, compareParams=False)
            if errStr:
                self.fail("failed with %s for spOrder=%s; numSpCoeff=%s" % (errStr, spOrder, numSpParams))


    def basicTests(self, kernel, nKernelParams, nSpatialParams=0):
        """Basic tests of a kernel"""
        self.assert_(kernel.getNSpatialParameters() == nSpatialParams)
        self.assert_(kernel.getNKernelParameters() == nKernelParams)
        if nSpatialParams == 0:
            self.assert_(not kernel.isSpatiallyVarying())
            for ii in range(nKernelParams+5):
                utilsTests.assertRaisesLsstCpp(self, pexExcept.InvalidParameterException,
                    kernel.getSpatialFunction, ii)
        else:
            self.assert_(kernel.isSpatiallyVarying())
            for ii in range(nKernelParams):
                kernel.getSpatialFunction(ii)
            for ii in range(nKernelParams, nKernelParams+5):
                utilsTests.assertRaisesLsstCpp(self, pexExcept.InvalidParameterException,
                    kernel.getSpatialFunction, ii)
        for nsp in range(nSpatialParams + 2):
            spatialParamsForOneKernel = (1.0,)*nsp
            for nkp in range(nKernelParams + 2):
                spatialParams = (spatialParamsForOneKernel,)*nkp
                if ((nkp == nKernelParams) and ((nsp == nSpatialParams) or (nkp == 0))):
                    kernel.setSpatialParameters(spatialParams)
                    self.assert_(numpy.alltrue(numpy.equal(kernel.getSpatialParameters(), spatialParams)))
                else:
                    utilsTests.assertRaisesLsstCpp(self, pexExcept.InvalidParameterException,
                        kernel.setSpatialParameters, spatialParams)

    def basicTestComputeImageRaise(self, kernel, doRaise, kernelDescr=""):
        """Test that computeImage either does or does not raise an exception, as appropriate
        """
        kImage = afwImage.ImageD(kernel.getDimensions())
        try:
            kernel.computeImage(kImage, True)
            if doRaise:
                self.fail(kernelDescr + ".computeImage should have raised an exception")
        except pexExcept.LsstCppException, e:
            if not doRaise:
                self.fail(kernelDescr + ".computeImage should not have raised an exception")

    def checkComputeImage(self, kernel):
        """Verify that one cannot compute a kernel image of the wrong size
        """
        kWidth = kernel.getWidth()
        kHeight = kernel.getHeight()

        for doNormalize in (False, True):
            for width in (0, 1, kWidth-1, kWidth, kWidth+1):
                for height in (0, 1, kHeight-1, kHeight, kHeight+1):
                    if (width, height) == (kWidth, kHeight):
                        continue
                    outImage = afwImage.ImageD(width, height)
                    try:
                        kernel.computeImage(outImage, doNormalize)
                        self.fail("computeImage accepted wrong-sized image; kernel=%s; image size=(%s, %s)" %
                            (kernel, width, height))
                    except pexExcept.LsstCppException:
                        pass

    def compareKernels(self, kernel1, kernel2, compareParams=True, newCtr1=(0, 0)):
        """Compare two kernels; return None if they match, else return a string kernelDescribing a difference.
        
        kernel1: one kernel to test
        kernel2: the other kernel to test
        compareParams: compare spatial parameters and kernel parameters if they exist
        newCtr: if not None then set the center of kernel1 and see if it changes the center of kernel2
        """
        retStrs = []
        if kernel1.getDimensions() != kernel2.getDimensions():
            retStrs.append("dimensions differ: %s != %s" % (kernel1.getDimensions(), kernel2.getDimensions()))
        ctr1 = kernel1.getCtrX(), kernel1.getCtrY()
        ctr2 = kernel2.getCtrX(), kernel2.getCtrY()
        if ctr1 != ctr2:
            retStrs.append("centers differ: %s != %s" % (ctr1, ctr2))
        if kernel1.isSpatiallyVarying() != kernel2.isSpatiallyVarying():
            retStrs.append("isSpatiallyVarying differs: %s != %s" % \
                (kernel1.isSpatiallyVarying(), kernel2.isSpatiallyVarying()))

        if compareParams:
            if kernel1.getSpatialParameters() != kernel2.getSpatialParameters():
                retStrs.append("spatial parameters differ: %s != %s" % \
                    (kernel1.getSpatialParameters(), kernel2.getSpatialParameters()))
            if kernel1.getNSpatialParameters() != kernel2.getNSpatialParameters():
                retStrs.append("# spatial parameters differs: %s != %s" % \
                    (kernel1.getNSpatialParameters(), kernel2.getNSpatialParameters()))
            if not kernel1.isSpatiallyVarying() and hasattr(kernel1, "getKernelParameters"):
                if kernel1.getKernelParameters() != kernel2.getKernelParameters():
                    retStrs.append("kernel parameters differs: %s != %s" % \
                        (kernel1.getKernelParameters(), kernel2.getKernelParameters()))
        if retStrs:
            return "; ".join(retStrs)
        
        im1 = afwImage.ImageD(kernel1.getDimensions())
        im2 = afwImage.ImageD(kernel2.getDimensions())
        if kernel1.isSpatiallyVarying():
            posList = [(0, 0), (200, 0), (0, 200), (200, 200)]
        else:
            posList = [(0, 0)]

        for doNormalize in (False, True):
            for pos in posList:
                kernel1.computeImage(im1, pos[0], pos[1], doNormalize)
                kernel2.computeImage(im2, pos[0], pos[1], doNormalize)
                im1Arr = imTestUtils.arrayFromImage(im1)
                im2Arr = imTestUtils.arrayFromImage(im2)
                if not numpy.allclose(im1Arr, im2Arr):
                    print "im1Arr =", im1Arr
                    print "im2Arr =", im2Arr
                    return "kernel images do not match at %s with doNormalize=%s" % (pos, doNormalize)

        if newCtr1 != None:
            kernel1.setCtrX(newCtr1[0])
            kernel1.setCtrY(newCtr1[1])
            newCtr2 = kernel2.getCtrX(), kernel2.getCtrY()
            if ctr2 != newCtr2:
                return "changing center of kernel1 to %s changed the center of kernel2 from %s to %s" % \
                    (newCtr1, ctr2, newCtr2)
        
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""
    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(KernelTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)

    return unittest.TestSuite(suites)

def run(doExit=False):
    """Run the tests"""
    utilsTests.run(suite(), doExit)

if __name__ == "__main__":
    run(True)
