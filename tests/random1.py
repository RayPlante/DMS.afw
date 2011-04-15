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
Tests for the lsst.afw.math.Random Python wrapper

Run with:
   python Random_1.py
or
   python
   >>> import unittest; T=load("Random_1"); unittest.TextTestRunner(verbosity=1).run(T.suite())
"""

import time
import unittest

import lsst.pex.policy as pexPolicy
import lsst.utils.tests as utilsTests
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.geom as afwGeom

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def checkRngEquivalence(rng1, rng2):
    for i in xrange(1000):
        assert rng1.uniform() == rng2.uniform()

def getSeed():
    return int(time.time() * 1000000.0) % 1000000

class RandomTestCase(unittest.TestCase):
    """A test case for lsst.afw.math.Random"""

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def testCreate(self):
        rngs = []
        for name in afwMath.Random.getAlgorithmNames():
            rngs.append(afwMath.Random(name))
        for r in rngs:
            assert afwMath.Random(r.getAlgorithmName()).uniform() == r.uniform()
            r2 = afwMath.Random(r.getAlgorithm())
            r2.uniform()
            assert r2.uniform() == r.uniform()

    def testCopy(self):
        """
        Test that the generator returned by deepCopy() produces an
        identical random number sequence to its prototype
        """
        rng1 = afwMath.Random(afwMath.Random.MT19937, getSeed())
        rng2 = rng1.deepCopy()
        checkRngEquivalence(rng1, rng2)

    def testPolicy(self):
        """
        Tests that policy files and environment variables can override
        user specified algorithms and seed values.
        """
        pol = pexPolicy.Policy()
        seed = getSeed()
        pol.set("rngSeed", str(seed))
        pol.set("rngAlgorithm", afwMath.Random.getAlgorithmNames()[afwMath.Random.RANLXD2])
        r1 = afwMath.Random(afwMath.Random.RANLXD2, seed)
        r2 = afwMath.Random(pol)
        checkRngEquivalence(r1, r2)

class RandomImageTestCase(unittest.TestCase):
    """A test case for lsst.afw.math.Random applied to Images"""

    def setUp(self):
        self.rand = afwMath.Random()
        self.image = afwImage.ImageF(afwGeom.Extent2I(1000, 1000))

    def tearDown(self):
        del self.image

    def testRandomUniformImage(self):
        afwMath.randomUniformImage(self.image, self.rand)
        #stats = afwMath.makeStatistics(self.image, afwMath.MEAN | afwMath.STDEV)

    def testRandomGaussianImage(self):
        afwMath.randomGaussianImage(self.image, self.rand)
        #stats = afwMath.makeStatistics(self.image, afwMath.MEAN | afwMath.STDEV)
        
    def testRandomChisqImage(self):
        nu = 10
        afwMath.randomChisqImage(self.image, self.rand, nu)
        stats = afwMath.makeStatistics(self.image, afwMath.MEAN | afwMath.VARIANCE)
        if False:
            print "nu = %g.  mean = %g, variance = %g" % \
                  (nu, stats.getValue(afwMath.MEAN), stats.getValue(afwMath.VARIANCE))
        self.assertAlmostEqual(stats.getValue(afwMath.MEAN), nu, 1)
        self.assertAlmostEqual(stats.getValue(afwMath.VARIANCE), 2*nu, 1)
        
    def testRandomPoissonImage(self):
        mu = 10
        afwMath.randomPoissonImage(self.image, self.rand, mu)
        stats = afwMath.makeStatistics(self.image, afwMath.MEAN | afwMath.VARIANCE)
        if False:
            print "mu = %g.  mean = %g, variance = %g" % \
                  (mu, stats.getValue(afwMath.MEAN), stats.getValue(afwMath.VARIANCE))
        self.assertAlmostEqual(stats.getValue(afwMath.MEAN), mu, 1)
        self.assertAlmostEqual(stats.getValue(afwMath.VARIANCE), mu, 1)
        
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(RandomTestCase)
    suites += unittest.makeSuite(RandomImageTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(shouldExit=False):
    """Run the tests"""
    utilsTests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)

