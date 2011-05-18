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
import sys
import os
import time

import eups

import lsst.daf.base as dafBase
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath

MaxIter = 20
MaxTime = 1.0 # seconds
SaveImages = False
DegPerRad = 180.0 / math.pi


DegreesFlag = afwGeom.degrees

def setDegreesFlag(newValue):
    """Set global DegreesFlag; avoids a syntax warning in makeWcs
    """
    global DegreesFlag
    DegreesFlag = newValue

dataDir = eups.productDir("afwdata")
if not dataDir:
    raise RuntimeError("Must set up afwdata")

InputExposurePath = os.path.join(dataDir, "ImSim/calexp/v85408556-fr/R23/S11.fits")

def timeWarp(destExposure, srcExposure, warpingKernel, interpLength):
    """Time warpExposure

    int warpExposure(
        DestExposureT &destExposure,
        SrcExposureT const &srcExposure,
        SeparableKernel &warpingKernel, int const interpLength=0);

    @param destExposure: output exposure (including a WCS)
    @param srcExposure: input exposure (including a WCS)
    @param warpingKernel: warping kernel
    @param interpLength: interpolation length (0 for no interpolation)

    @return (elapsed time in seconds, number of iterations)
    """
    startTime = time.time();
    for nIter in range(1, MaxIter + 1):
        goodPix = afwMath.warpExposure(destExposure, srcExposure, warpingKernel, interpLength)
        endTime = time.time()
        if endTime - startTime > MaxTime:
            break

    return (endTime - startTime, nIter, goodPix)

def makeWcs(projName, destCtrInd, skyOffset, rotAng, scaleFac, srcWcs, srcCtrInd):
    """Make an RA/Dec WCS from another RA/Dec WCS
    
    @param projName: projection, e,g, "TAN"
    @param destCtrInd: pixel index of center of WCS; used to compute CRPIX;
        typically the center of the destination exposure
    @param skyOffset: offset in sky coords (axis 1, axis 2 degrees):
        dest sky pos at destCtrInd = src sky pos at destCtrInd + skyOffset
    @param rotAng: change of orientation with respect to srcWcs, in degrees
    @param scaleFac: output resolution / input resolution
    @param srcWcs: reference WCS
    @param srcCtrInd: index of source pixel whose sky matches destCtrInd on new WCS
        typically the center of the source exposure
    """
    ps = dafBase.PropertySet()
    destCtrFitsPix = afwGeom.Point2D([ind + 1.0 for ind in destCtrInd])
    srcCtrFitsPix = afwGeom.Point2D([ind + 1.0 for ind in srcCtrInd])
    srcOffFitsPix = srcCtrFitsPix + afwGeom.Extent2D(1.0, 0.0) # offset 1 pixel in x to compute orient & scale
    try:
        srcCtrSkyPos = srcWcs.pixelToSky(srcCtrFitsPix).getPosition(DegreesFlag)
    except Exception:
        import lsst.afw.coord as afwCoord
        setDegreesFlag(afwCoord.DEGREES)
        srcCtrSkyPos = srcWcs.pixelToSky(srcCtrFitsPix).getPosition(DegreesFlag)
    srcOffSkyPos = srcWcs.pixelToSky(srcOffFitsPix).getPosition(DegreesFlag)
    srcSkyOff = srcOffSkyPos - srcCtrSkyPos
    srcAngleRad = math.atan2(srcSkyOff[1], srcSkyOff[0])
    destAngleRad = srcAngleRad + (rotAng / DegPerRad)
    srcScale = math.sqrt(srcSkyOff[0]**2 + srcSkyOff[1]**2) # in degrees/pixel
    destScale = srcScale / scaleFac
    for i in range(2):
        ip1 = i + 1
        ctypeStr = ("%-5s%3s" % (("RA", "DEC")[i], projName)).replace(" ", "-")
        ps.add("CTYPE%1d" % (ip1,), ctypeStr)
        ps.add("CRPIX%1d" % (ip1,), destCtrFitsPix[i])
        ps.add("CRVAL%1d" % (ip1,), srcCtrSkyPos[i] + skyOffset[i])
    ps.add("RADECSYS", "ICRS")
    ps.add("EQUINOX", 2000)
    ps.add("CD1_1", -destScale * math.cos(destAngleRad))
    ps.add("CD2_1", destScale * math.sin(destAngleRad))
    ps.add("CD1_2", destScale * math.sin(destAngleRad))
    ps.add("CD2_2", destScale * math.cos(destAngleRad))
    return afwImage.makeWcs(ps)
    

def run():
    if len(sys.argv) < 2:
        srcExposure = afwImage.ExposureF(InputExposurePath)
        if True:
            bbox = afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Extent2I(2000, 2000))
            srcExposure = afwImage.ExposureF(srcExposure, bbox, afwImage.LOCAL, False)
    else:
        srcExposure = afwImage.ExposureF(sys.argv[1])
    srcWcs = srcExposure.getWcs()
    srcDim = srcExposure.getDimensions()
    srcCtrInd = [int(d / 2) for d in srcDim]
    
    # make the destination exposure small enough that even after rotation and offset
    # (by reasonable amounts) there are no edge pixels
    destDim = afwGeom.Extent2I([int(sd * 0.5) for sd in srcDim])
    destExposure = afwImage.ExposureF(destDim)
    destCtrInd = [int(d / 2) for d in destDim]
    
    print "Warping", InputExposurePath
    print "Source (sub)image size:", srcDim
    print "Destination image size:", destDim
    print
    
    print "test# interp  scaleFac     skyOffset     rotAng   kernel   goodPix time/iter"
    print '       (pix)              (RA, Dec ")    (deg)                      (sec)'
    testNum = 1
    for interpLength in (0, 1, 5, 10):
        for scaleFac in (1.2,): # (1.0, 1.5):
            for skyOffsetArcSec in ((0.0, 0.0),): #  ((0.0, 0.0), (10.5, -5.5)):
                skyOffset = [so / 3600.0 for so in skyOffsetArcSec]
                for rotAng, kernelName in (
                    (0.0, "bilinear"),
                    (0.0, "lanczos2"),
                    (0.0, "lanczos3"),
                    (45.0, "lanczos3"),
                ):
                    destWcs = makeWcs(
                        projName = "TAN",
                        destCtrInd = destCtrInd,
                        skyOffset = skyOffset,
                        rotAng = rotAng,
                        scaleFac = scaleFac,
                        srcWcs = srcWcs,
                        srcCtrInd = srcCtrInd,
                    )
                    destExposure.setWcs(destWcs)
                    warpingKernel = afwMath.makeWarpingKernel(kernelName)
                    dTime, nIter, goodPix = timeWarp(destExposure, srcExposure, warpingKernel, interpLength)
                    print "%4d  %5d  %8.1f  %6.1f, %6.1f  %7.1f %10s %8d %6.2f" % (
                        testNum, interpLength, scaleFac, skyOffsetArcSec[0], skyOffsetArcSec[1],
                        rotAng, kernelName, goodPix, dTime/float(nIter))
                    
                    if SaveImages:
                        destExposure.writeFits("warpedExposure%03d.fits" % (testNum,))
                    testNum += 1

if __name__ == "__main__":
    run()
