#!/usr/bin/env python
import pdb                          # we may want to say pdb.set_trace()
import unittest

import numpy

import lsst.utils.tests as utilsTests
import lsst.pex.exceptions as pexExcept
import lsst.pex.logging as pexLog
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.image.testUtils as imTestUtils

VERBOSITY = 0 # increase to see trace

pexLog.Debug("lsst.afw", VERBOSITY)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class KernelTestCase(unittest.TestCase):
    """A test case for Kernels"""
    def testAnalyticKernel(self):
        """Test AnalyticKernel using a Gaussian function
        """
        kWidth = 5
        kHeight = 8

        gaussFunc = afwMath.GaussianFunction2D(1.0, 1.0, 0.0)
        kernel = afwMath.AnalyticKernel(kWidth, kHeight, gaussFunc)
        self.basicTests(kernel, 2)
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
                
                kernel.setKernelParameters((xsigma, ysigma))
                kImage = afwImage.ImageD(kernel.getDimensions())
                kernel.computeImage(kImage, True)
                
                kArr = imTestUtils.arrayFromImage(kImage)
                if not numpy.allclose(fArr, kArr):
                    self.fail("%s = %s != %s for xsigma=%s, ysigma=%s" % \
                        (kernel.__class__.__name__, kArr, fArr, xsigma, ysigma))

        kernel.setKernelParameters((0.5, 1.1))
        kernelClone = kernel.clone()
        errStr = self.compareKernels(kernel, kernelClone)
        if errStr:
            self.fail(errStr)
        
        kernel.setKernelParameters((1.5, 0.2))
        errStr = self.compareKernels(kernel, kernelClone)
        if not errStr:
            self.fail("Clone was modified by changing original's kernel parameters")

        self.checkComputeImage(kernel)
    
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
        basisImArrList = []
        basisKernelList = afwMath.KernelList()
        for row in range(kHeight):
            for col in range(kWidth):
                basisKernel = afwMath.DeltaFunctionKernel(kWidth, kHeight, afwImage.PointI(col, row))
                basisImage = afwImage.ImageD(basisKernel.getDimensions())
                basisKernel.computeImage(basisImage, True)
                basisImArrList.append(imTestUtils.arrayFromImage(basisImage))
                basisKernelList.append(basisKernel)

        kParams = [0.0]*len(basisKernelList)
        kernel = afwMath.LinearCombinationKernel(basisKernelList, kParams)
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
        
        for numKernelParams in (1, 3):
            spFuncList = afwMath.Function2DList()
            for ii in range(numKernelParams):
                spFuncList.append(spFunc.clone())
            try:
                afwMath.AnalyticKernel(kWidth, kHeight, gaussFunc2, spFuncList)
                self.fail("Should have failed with wrong # of spatial functions")
            except pexExcept.LsstCppException:
                pass
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

    def compareKernels(self, kernel1, kernel2, newCtr1=(0, 0)):
        """Compare two kernels; return None if they match, else return a string describing a difference.
        
        kernel1: one kernel to test
        kernel2: the other kernel to test
        newCtr: if not None then set the center of kernel1 and see if it changes the center of kernel2
        """
        retStrs = []
        if kernel1.getDimensions() != kernel2.getDimensions():
            retStrs.append("dimensions differ: %s != %s" % (kernel1.getDimensions(), kernel2.getDimensions()))
        ctr1 = kernel1.getCtrX(), kernel1.getCtrY()
        ctr2 = kernel2.getCtrX(), kernel2.getCtrY()
        if ctr1 != ctr2:
            retStrs.append("centers differ: %s != %s" % (ctr1, ctr2))
        if kernel1.getSpatialParameters() != kernel2.getSpatialParameters():
            retStrs.append("spatial parameters differs: %s != %s" % \
                (kernel1.getSpatialParameters(), kernel2.getSpatialParameters()))
        if kernel1.isSpatiallyVarying() != kernel2.isSpatiallyVarying():
            retStrs.append("isSpatiallyVarying differs: %s != %s" % \
                (kernel1.isSpatiallyVarying(), kernel2.isSpatiallyVarying()))
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
