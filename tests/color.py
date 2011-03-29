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
Tests for Color and Filter

Run with:
   color.py
or
   python
   >>> import color; color.run()
"""


import math, os, sys
import eups
import unittest
import lsst.utils.tests as tests
import lsst.daf.base as dafBase
import lsst.pex.logging as logging
import lsst.pex.exceptions as pexExcept
import lsst.pex.policy as pexPolicy
import lsst.afw.image as afwImage
import lsst.afw.image.utils as imageUtils
import lsst.afw.math as afwMath
import lsst.afw.detection as afwDetect
import lsst.afw.detection.utils as afwDetectUtils
import lsst.afw.display.ds9 as ds9

try:
    type(verbose)
except NameError:
    verbose = 0
    logging.Debug("afwDetect.Footprint", verbose)

try:
    type(display)
except NameError:
    display = False

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class CalibTestCase(unittest.TestCase):
    """A test case for Calib"""
    def setUp(self):
        self.calib = afwImage.Calib()

    def tearDown(self):
        del self.calib

    def testTime(self):
        """Test the exposure time information"""
        
        isoDate = "1995-01-26T07:32:00.000000000Z"
        self.calib.setMidTime(dafBase.DateTime(isoDate))
        self.assertEqual(isoDate, self.calib.getMidTime().toString())
        self.assertAlmostEqual(self.calib.getMidTime().get(), 49743.3142245)

        dt = 123.4
        self.calib.setExptime(dt)
        self.assertEqual(self.calib.getExptime(), dt)

    def testPhotom(self):
        """Test the zero-point information"""
        
        flux, fluxErr = 1000.0, 10.0
        flux0, flux0Err = 1e12, 1e10
        self.calib.setFluxMag0(flux0)

        self.assertEqual(flux0, self.calib.getFluxMag0()[0])
        self.assertEqual(0.0, self.calib.getFluxMag0()[1])
        self.assertEqual(22.5, self.calib.getMagnitude(flux))
        # Error just in flux
        self.assertAlmostEqual(self.calib.getMagnitude(flux, fluxErr)[1], 2.5/math.log(10)*fluxErr/flux)
        # Error just in flux0
        self.calib.setFluxMag0(flux0, flux0Err)
        self.assertEqual(flux0Err, self.calib.getFluxMag0()[1])
        self.assertAlmostEqual(self.calib.getMagnitude(flux, 0)[1], 2.5/math.log(10)*flux0Err/flux0)

    def testCtorFromMetadata(self):
        """Test building a Calib from metadata"""
        
        isoDate = "1995-01-26T07:32:00.000000000Z" 
        exptime = 123.4
        flux0, flux0Err = 1e12, 1e10
        flux, fluxErr = 1000.0, 10.0

        metadata = dafBase.PropertySet()
        metadata.add("TIME-MID", isoDate)
        metadata.add("EXPTIME", exptime)
        metadata.add("FLUXMAG0", flux0)
        metadata.add("FLUXMAG0ERR", flux0Err)

        self.calib = afwImage.Calib(metadata)

        self.assertEqual(isoDate, self.calib.getMidTime().toString())
        self.assertAlmostEqual(self.calib.getMidTime().get(), 49743.3142245)
        self.assertEqual(self.calib.getExptime(), exptime)
        
        self.assertEqual(flux0, self.calib.getFluxMag0()[0])
        self.assertEqual(flux0Err, self.calib.getFluxMag0()[1])
        self.assertEqual(22.5, self.calib.getMagnitude(flux))
        # Error just in flux
        self.calib.setFluxMag0(flux0, 0)
        
        self.assertAlmostEqual(self.calib.getMagnitude(flux, fluxErr)[1], 2.5/math.log(10)*fluxErr/flux)

        #
        # Check that we can clean up metadata
        #
        afwImage.stripCalibKeywords(metadata)
        self.assertEqual(len(metadata.names()), 0)

class ColorTestCase(unittest.TestCase):
    """A test case for Color"""
    def setUp(self):
        # Initialise our filters
        filterPolicy = pexPolicy.Policy.createPolicy(
            os.path.join(eups.productDir("afw"), "tests", "SdssFilters.paf"), True)

        imageUtils.defineFiltersFromPolicy(filterPolicy, reset=True)

    def tearDown(self):
        pass

    def testCtor(self):
        c = afwImage.Color()
        c = afwImage.Color(1.2)

    def testLambdaEff(self):
        f = afwImage.Filter("g")
        g_r = 1.2
        c = afwImage.Color(g_r)

        self.assertEqual(c.getLambdaEff(f), 1000*g_r) # XXX Not a real implementation!

    def testBool(self):
        """Test that a default-constructed Color tests False, but ones with a g-r value test True"""
        self.assertFalse(afwImage.Color())
        self.assertTrue(afwImage.Color(1.2))

class FilterTestCase(unittest.TestCase):
    """A test case for Filter"""

    def setUp(self):
        # Initialise our filters
        #
        # Start by forgetting that we may already have defined filters
        #
        filterPolicy = pexPolicy.Policy.createPolicy(
            os.path.join(eups.productDir("afw"), "tests", "SdssFilters.paf"), True)
        self.filters = tuple(sorted([f.get("name") for f in filterPolicy.getArray("Filter")]))

        imageUtils.defineFiltersFromPolicy(filterPolicy, reset=True)

        self.g_lambdaEff = [p.get("lambdaEff") for p in filterPolicy.getArray("Filter")
                            if p.get("name") == "g"][0] # used for tests

    def defineFilterProperty(self, name, lambdaEff, force=False):
        filterPolicy = pexPolicy.Policy()
        filterPolicy.add("lambdaEff", lambdaEff)

        return afwImage.FilterProperty(name, filterPolicy, force);

    def testListFilters(self):
        self.assertEqual(afwImage.Filter_getNames(), self.filters)

    def testCtor(self):
        """Test that we can construct a Filter"""
        # A filter of type 
        f = afwImage.Filter("g")

    def testCtorFromMetadata(self):
        """Test building a Filter from metadata"""
        
        metadata = dafBase.PropertySet()
        metadata.add("FILTER", "g")

        f = afwImage.Filter(metadata)
        self.assertEqual(f.getName(), "g")
        #
        # Check that we can clean up metadata
        #
        afwImage.stripFilterKeywords(metadata)
        self.assertEqual(len(metadata.names()), 0)

        badFilter = "rhl"               # an undefined filter
        metadata.add("FILTER", badFilter)
        # Not defined
        tests.assertRaisesLsstCpp(self, pexExcept.NotFoundException,
                                  lambda : afwImage.Filter(metadata))
        # Force definition
        f = afwImage.Filter(metadata, True)
        self.assertEqual(f.getName(), badFilter) # name is correctly defined

    def testFilterProperty(self):
        # a "g" filter
        f = afwImage.Filter("g")
        # The properties of a g filter
        g = afwImage.FilterProperty.lookup("g")

        if False:
            print "Filter: %s == %d lambda_{eff}=%g" % (f.getName(), f.getId(),
                                                        f.getFilterProperty().getLambdaEff())

        self.assertEqual(f.getName(), "g")
        self.assertEqual(f.getId(), 1)
        self.assertEqual(f.getFilterProperty().getLambdaEff(), self.g_lambdaEff)
        self.assertTrue(f.getFilterProperty() ==
                        self.defineFilterProperty("gX", self.g_lambdaEff, True))

        self.assertEqual(g.getLambdaEff(), self.g_lambdaEff)

    def testFilterAliases(self):
        """Test that we can provide an alias for a Filter"""
        f0 = afwImage.Filter("z")
        f1 = afwImage.Filter("zprime")
        f2 = afwImage.Filter("z'")

        self.assertEqual(f0.getFilterProperty().getLambdaEff(), f1.getFilterProperty().getLambdaEff())
        self.assertEqual(f0.getFilterProperty().getLambdaEff(), f2.getFilterProperty().getLambdaEff())

    def testReset(self):
        """Test that we can reset filter IDs and properties if needs be"""
        # The properties of a g filter
        g = afwImage.FilterProperty.lookup("g")
        #
        # First FilterProperty
        #
        def tst():
            gprime = self.defineFilterProperty("g", self.g_lambdaEff + 10)

        tests.assertRaisesLsstCpp(self, pexExcept.RuntimeErrorException, tst)
        gprime = self.defineFilterProperty("g", self.g_lambdaEff + 10, True) # should not raise
        gprime = self.defineFilterProperty("g", self.g_lambdaEff, True)
        #
        # Can redefine
        #
        def tst():
            self.defineFilterProperty("g", self.g_lambdaEff + 10) # changing definition is not allowed
        tests.assertRaisesLsstCpp(self, pexExcept.RuntimeErrorException, tst)

        self.defineFilterProperty("g", self.g_lambdaEff) # identical redefinition is allowed
        #
        # Now Filter
        #
        afwImage.Filter.define(g, afwImage.Filter("g").getId()) # OK if Id's the same
        afwImage.Filter.define(g, afwImage.Filter.AUTO)         # AUTO will assign the same ID

        def tst():
            afwImage.Filter.define(g, afwImage.Filter("g").getId() + 10) # different ID
            
        tests.assertRaisesLsstCpp(self, pexExcept.RuntimeErrorException, tst)

    def testUnknownFilter(self):
        """Test that we can define, but not use, an unknown filter"""
        badFilter = "rhl"               # an undefined filter
        # Not defined
        tests.assertRaisesLsstCpp(self, pexExcept.NotFoundException,
                                  lambda : afwImage.Filter(badFilter))
        # Force definition
        f = afwImage.Filter(badFilter, True)
        self.assertEqual(f.getName(), badFilter) # name is correctly defined
        
        tests.assertRaisesLsstCpp(self, pexExcept.NotFoundException,
                                  lambda : f.getFilterProperty().getLambdaEff()) # can't use Filter f
        #
        # Now define badFilter
        #
        lambdaEff = 666.0; self.defineFilterProperty(badFilter, lambdaEff)
        
        self.assertEqual(f.getFilterProperty().getLambdaEff(), lambdaEff) # but now we can
        #
        # Check that we didn't accidently define the unknown filter
        #
        tests.assertRaisesLsstCpp(self, pexExcept.NotFoundException,
                                  lambda : afwImage.Filter().getFilterProperty().getLambdaEff())

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""
    tests.init()

    suites = []
    suites += unittest.makeSuite(CalibTestCase)
    if not False:
        suites += unittest.makeSuite(ColorTestCase)
    else:
        print >> sys.stderr, "Skipping Color tests (wait until #1196 is merged)"
    suites += unittest.makeSuite(FilterTestCase)
    suites += unittest.makeSuite(tests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit=False):
    """Run the tests"""
    tests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
