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

# -*- python -*-
"""
Check that coord and coordPtr are properly passed through swig

Run with:
   python coordptr.py
"""

import os
import unittest

import eups
import lsst.afw.image            as image
import lsst.afw.geom             as geom
import lsst.afw.geom             as afwGeom
import lsst.afw.coord.coordLib   as coord
import lsst.utils.tests          as utilsTests
import lsst.daf.base             as dafBase


class CoordPtrTestCase(unittest.TestCase):

    def setUp(self):
        pass
        
    def tearDown(self):
        pass
        
    def testMakeCoord(self):
        
        c = coord.Coord(1 * afwGeom.degrees,2 * afwGeom.degrees)
        print type(c)
        c = coord.makeCoord(coord.FK5, 1 * afwGeom.degrees, 2 * afwGeom.degrees)
        print type(c)
    def testMakeWcs(self):
        path= eups.productDir("afw")
        path = os.path.join(path, "tests", "data", "parent.fits")
        fitsHdr = image.readMetadata(path)
        
        wcs = image.makeWcs(fitsHdr)
        
        c = wcs.pixelToSky(0,0)
        print type(c)
        c.getPosition()
        
#################################################################
# Test suite boiler plate
#################################################################
def suite():
    """Returns a suite containing all the test cases in this module."""

    utilsTests.init()

    suites = []
    suites += unittest.makeSuite(CoordPtrTestCase)
    suites += unittest.makeSuite(utilsTests.MemoryTestCase)
    return unittest.TestSuite(suites)

def run(exit = False):
    """Run the tests"""
    utilsTests.run(suite(), exit)

if __name__ == "__main__":
    run(True)
