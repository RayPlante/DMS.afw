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

import os
import math
import pdb                          # we may want to say pdb.set_trace()
import unittest

import eups
import lsst.afw.image as afwImage
import lsst.afw.geom as afwGeom
import lsst.afw.coord as afwCoord
import lsst.utils.tests as utilsTests
import lsst.afw.display.ds9 as ds9
import lsst.pex.exceptions.exceptionsLib as exceptions
import lsst.daf.base as dafBase

import lsst

try:
    type(verbose)
except NameError:
    verbose = 0



class MakeWcsTestCase(unittest.TestCase):
    """Test that makeWcs correctly returns a Wcs or TanWcs object
       as appropriate based on the contents of a fits header
    """
    
    def setUp(self):
        #metadata taken from CFHT data
        #v695856-e0/v695856-e0-c000-a00.sci_img.fits

        self.metadata = dafBase.PropertySet()

        self.metadata.set("SIMPLE",                    "T") 
        self.metadata.set("BITPIX",                  -32) 
        self.metadata.set("NAXIS",                    2) 
        self.metadata.set("NAXIS1",                 1024) 
        self.metadata.set("NAXIS2",                 1153) 
        self.metadata.set("RADECSYS", 'FK5')
        self.metadata.set("EQUINOX",                2000.)


        self.metadata.setDouble("CRVAL1",     215.604025685476)
        self.metadata.setDouble("CRVAL2",     53.1595451514076)
        self.metadata.setDouble("CRPIX1",     1109.99981456774)
        self.metadata.setDouble("CRPIX2",     560.018167811613)
        self.metadata.set("CTYPE1", 'RA---SIN')
        self.metadata.set("CTYPE2", 'DEC--SIN')

        self.metadata.setDouble("CD1_1", 5.10808596133527E-05)
        self.metadata.setDouble("CD1_2", 1.85579539217196E-07)
        self.metadata.setDouble("CD2_2", -5.10281493481982E-05)
        self.metadata.setDouble("CD2_1", -8.27440751733828E-07)

    def tearDown(self):
        del self.metadata
        
    def testCreateBaseWcs(self):
        """Check that a non-TAN projection in the header creates a base Wcs object"""
        
        wcs = afwImage.makeWcs(self.metadata)
        strRepresentation = str(wcs)
        self.assertNotEqual( strRepresentation.find("image::Wcs"), -1, "non Wcs object returned")

    def testNoCreateTanWcs(self):
        """Test than an exception is thrown if we try to upcast to a TanWcs inappropriately"""
        wcs = afwImage.makeWcs(self.metadata)
        
        excpt = lsst.pex.exceptions.exceptionsLib.LsstCppException
        self.assertRaises(excpt, afwImage.cast_TanWcs, wcs)
        
    def testCreateTanWcs(self):
        """Check that a non-TAN projection in the header creates a base Wcs object"""
        
        self.metadata.set("CTYPE1", "RA---TAN")
        self.metadata.set("CTYPE2", "DEC--TAN")
        
        wcs0 = afwImage.makeWcs(self.metadata)
        wcs = afwImage.cast_TanWcs(afwImage.makeWcs(self.metadata))
        strRepresentation = str(wcs)
        self.assertNotEqual( strRepresentation.find("image::TanWcs"), -1, "non TanWcs object returned")

    def testCreateTanSipWcs(self):
                
        self.metadata.set("CTYPE1", "RA---TAN")
        self.metadata.set("CTYPE2", "DEC--TAN")
        
        wcs = afwImage.cast_TanWcs(afwImage.makeWcs(self.metadata))
        strRepresentation = str(wcs)
        self.assertNotEqual( strRepresentation.find("image::TanWcs"), -1, "non TanWcs object returned")


    def testPythonLevelMakeWcs(self):
        """Verify that we can make a Wcs by providing the CD matrix elements in python."""
        
        m = self.metadata
        crval = afwGeom.makePointD(m.getDouble("CRVAL1"), m.getDouble("CRVAL2"))
        crpix = afwGeom.makePointD(m.getDouble("CRPIX1"), m.getDouble("CRPIX2"))
        cd11, cd12 = m.getDouble("CD1_1"), m.getDouble("CD1_2")
        cd21, cd22 = m.getDouble("CD2_1"), m.getDouble("CD2_2")
        
        # this is defined at the c++ level in src/image/makeWcs.cc
        wcsMade = afwImage.makeWcs(crval, crpix, cd11, cd12, cd21, cd22)
        
        # this is defined at the python/swig level in python/lsst/afw/image/imageLib.i
        # createWcs can probably be depricated
        wcsCreated = afwImage.createWcs(crval, crpix, cd11, cd12, cd21, cd22)


        # trivial test ... verify that we get back what we put in.
        for wcs in [wcsMade, wcsCreated]:
            crvalTest = wcs.getSkyOrigin().getPosition(afwCoord.DEGREES)
            crpixTest = wcs.getPixelOrigin()
            CD = wcs.getCDMatrix()
            
            self.assertAlmostEqual(crvalTest[0], crval[0])
            self.assertAlmostEqual(crvalTest[1], crval[1])
            self.assertAlmostEqual(crpixTest[0], crpix[0])
            self.assertAlmostEqual(crpixTest[1], crpix[1])
            self.assertAlmostEqual(CD[0,0], cd11)
            self.assertAlmostEqual(CD[0,1], cd12)
            self.assertAlmostEqual(CD[1,0], cd21)
            self.assertAlmostEqual(CD[1,1], cd22)

        
        
#####

def suite():
    """Returns a suite containing all the test cases in this module."""
    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(MakeWcsTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit=False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
