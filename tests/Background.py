#!/usr/bin/env python
"""
Tests for Background

Run with:
   ./Background.py
or
   python
   >>> import Background; Background.run()
"""

import math
import os
import pdb  # we may want to say pdb.set_trace()
import sys
import unittest

import lsst.utils.tests as utilsTests
import lsst.pex.exceptions
import lsst.afw.image.imageLib as afwImage
import lsst.afw.math as afwMath
import lsst.afw.display.ds9 as ds9
import eups

try:
    type(display)
except NameError:
    display = False


# ==== summary to currently implemented tests ====
# getPixel: tests basic functionality of getPixel() method (floats)
# BackgroundTestImages: tests Laher's afwdata/Statistics/*.fits images (doubles)
# testRamp: make sure a constant slope is *exactly* reproduced by the spline model
# testParabola: make sure a quadratic map is *well* reproduced by the spline model

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class BackgroundTestCase(unittest.TestCase):
    
    """A test case for Background"""
    def setUp(self):
        self.val = 10
        self.image = afwImage.ImageF(100, 200); self.image.set(self.val)

    def tearDown(self):
        del self.image


        #self.assertAlmostEqual(mean[1], sd/math.sqrt(image2.getWidth()*image2.getHeight()), 10)

    def testgetPixel(self):
	"""Test the getPixel() function"""


	xcen, ycen = 50, 100
	bgCtrl = afwMath.BackgroundControl(afwMath.NATURAL_SPLINE)
	bgCtrl.setNxSample(3)
	bgCtrl.setNySample(3)
	bgCtrl.sctrl.setNumIter(3)
	bgCtrl.sctrl.setNumSigmaClip(3)
	back = afwMath.BackgroundF(self.image, bgCtrl)
	mid = back.getPixel(xcen,ycen)
	
        self.assertEqual(back.getPixel(xcen,ycen), self.val)


    def testBackgroundTestImages(self):

	imginfolist = []
        imginfolist.append( ["v1_i1_g_m400_s20_f.fits", 400.05551471441612] ) # cooked to known value
        #imgfiles.append("v1_i1_g_m400_s20_u16.fits")
        #imgfiles.append("v1_i2_g_m400_s20_f.fits"
        #imgfiles.append("v1_i2_g_m400_s20_u16.fits")
        #imgfiles.append("v2_i1_p_m9_f.fits")
        #imgfiles.append("v2_i1_p_m9_u16.fits")
        #imgfiles.append("v2_i2_p_m9_f.fits")
        #imgfiles.append("v2_i2_p_m9_u16.fits")
        
        afwdata_dir = eups.productDir("afwdata")
	for imginfo in imginfolist:

	    imgfile, center_value = imginfo

            img_path = afwdata_dir + "/Statistics/" + imgfile;

            # get the image and header
            dimg = afwImage.DecoratedImageD(img_path)
	    img = dimg.getImage()
            fitsHdr = dimg.getMetadata(); # the FITS header

            # get the True values of the mean and stdev
            req_mean  = fitsHdr.getAsDouble("MEANREQ")
            req_stdev = fitsHdr.getAsDouble("SIGREQ")
            naxis1 = img.getWidth()
            naxis2 = img.getHeight()
            
            # create a background control object
            bctrl = afwMath.BackgroundControl(afwMath.NATURAL_SPLINE);
            bctrl.setNxSample(3);
            bctrl.setNySample(3);
	    
            # run the background constructor and call the getPixel() and getImage() functions.
            backobj = afwMath.BackgroundD(img,bctrl)

	    pix_per_subimage = img.getWidth()*img.getHeight()/(bctrl.getNxSample()*bctrl.getNySample())
	    stdev_interp = req_stdev/math.sqrt(pix_per_subimage)
	    
            # test getPixel()
            testval = backobj.getPixel(naxis1/2, naxis2/2)
	    self.assertEqual( testval, center_value )
            self.assertTrue( abs(testval - req_mean) < 2*stdev_interp )

            # test getImage() by checking the center pixel
            bimg = backobj.getImageD()
            testImgval = bimg.get(naxis1/2,naxis2/2)
            self.assertTrue( abs(testImgval - req_mean) < 2*stdev_interp )
	    

    def testRamp(self):

	# make a ramping image (spline should be exact for linear increasing image
	nx = 512
	ny = 512
	rampimg = afwImage.ImageD(nx,ny)
	dzdx, dzdy, z0 = 0.1, 0.2, 10000.0

	for x in range(nx):
	    for y in range(ny):
		rampimg.set(x, y, dzdx*x + dzdy*y + z0)
	
	# check corner, edge, and center pixels
	bctrl = afwMath.BackgroundControl(afwMath.NATURAL_SPLINE);
	bctrl.setNxSample(6);
	bctrl.setNySample(6);
	bctrl.sctrl.setNumSigmaClip(20.0)  # something large enough to avoid clipping entirely
	bctrl.sctrl.setNumIter(1)
	backobj = afwMath.BackgroundD(rampimg,bctrl)

	xpixels = [0,nx/2,nx-1]
	ypixels = [0,ny/2,ny-1]
	for xpix in xpixels:
	    for ypix in ypixels:
		testval = backobj.getPixel(xpix, ypix)
		self.assertAlmostEqual( testval, rampimg.get(xpix,ypix),10 )
		

    def testParabola(self):

	# make an image which varies parabolicly (spline should be exact for 2rd order polynomial)
	nx = 512
	ny = 512
	parabimg = afwImage.ImageD(nx,ny)
	d2zdx2, d2zdy2, dzdx, dzdy, z0 = -1.0e-4, -1.0e-4, 0.1, 0.2, 10000.0  # no cross-terms

	for x in range(nx):
	    for y in range(ny):
		parabimg.set(x, y, d2zdx2*x*x + d2zdy2*y*y + dzdx*x + dzdy*y + z0)
	
	# check corner, edge, and center pixels
	bctrl = afwMath.BackgroundControl(afwMath.NATURAL_SPLINE);
	bctrl.setNxSample(16);
	bctrl.setNySample(16);
	bctrl.sctrl.setNumSigmaClip(10.0)  
	bctrl.sctrl.setNumIter(1)
	backobj = afwMath.BackgroundD(parabimg,bctrl)

	# debug
	#bimg = backobj.getImageD()
	#ds9.mtv(parabimg)
	#ds9.mtv(bimg, frame=1)
	#parabimg.writeFits("a.fits")
	#bimg.writeFits("b.fits")
	
	xpixels = [0,nx/2,nx-1]
	ypixels = [0,ny/2,ny-1]
	for xpix in xpixels:
	    for ypix in ypixels:
		testval = backobj.getPixel(xpix, ypix)
		realval = parabimg.get(xpix,ypix)
		#print xpix, ypix, testval, realval
		# quadratic terms skew the averages of the subimages and the clipped mean for
		# a subimage != value of center pixel.  1/20 counts on a 10000 count sky
		#  is a fair (if arbitrary) test.
		self.assertTrue( (testval - realval) < 0.05 )

    def testCFHT(self):
        """Test background subtraction on some real CFHT data"""

        mi = afwImage.MaskedImageF(os.path.join(eups.productDir("afwdata"), "CFHT", "D4", "cal-53535-i-797722_1"))
        mi = mi.Factory(mi, afwImage.BBox(afwImage.PointI(32, 2), afwImage.PointI(2079, 4609)))
        mi.setXY0(afwImage.PointI(0, 0))
        
	bctrl = afwMath.BackgroundControl(afwMath.NATURAL_SPLINE);
	bctrl.setNxSample(16);
	bctrl.setNySample(16);
	bctrl.sctrl.setNumSigmaClip(3.0)  
	bctrl.sctrl.setNumIter(2)
	backobj = afwMath.BackgroundF(mi.getImage(), bctrl)

        if display:
            ds9.mtv(mi, frame=0)

        im = mi.getImage(); im -= backobj.getImageF()

        if display:
            ds9.mtv(mi, frame=1)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(BackgroundTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(exit=False):
    """Run the tests"""
    utilsTests.run(suite(), exit)

if __name__ == "__main__":
    run(True)