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
Tests for SpatialCell

Run with:
   python SpatialCell.py
or
   python
   >>> import SpatialCell; SpatialCell.run()
"""

import math
import os
import sys
import unittest
import pyfits

import lsst.pex.policy as pexPolicy
import lsst.afw.geom as afwGeom
import lsst.afw.image as afwImage
import lsst.afw.math as afwMath
import lsst.afw.cameraGeom as cameraGeom

import lsst.afw.display.ds9 as ds9
import lsst.afw.display.utils as displayUtils

try:
    type(display)
except NameError:
    display = False
    force = False

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class GetCcdImage(object):
    """A class to return an Image of a given Ccd"""

    def __init__(self, imageFile=None):
        self.imageFile = imageFile
        self.isTrimmed = False
        self.isRaw = True

    def getImage(self, ccd, amp=None, imageFactory=afwImage.ImageU):
        """Return the image of the chip with cameraGeom.Id == id; if provided only read the given BBox"""

        return self.getImageFromFilename(self.imageFile, ccd, amp, imageFactory=imageFactory)

    def getImageFromFilename(self, fileName, ccd, amp=None, hdu=0, imageFactory=afwImage.ImageU,
                             oneAmpPerFile=False):
        """Return the image of the chip with cameraGeom.Id == id; if provided only read the given BBox"""

        if amp:
            if self.isTrimmed:
                bbox = amp.getDiskDataSec()
            else:
                bbox = amp.getDiskAllPixels()
        else:
            bbox = ccd.getAllPixels()

        md = None
        return imageFactory(fileName, hdu, md, bbox)

    def setTrimmed(self, doTrim):
        self.isTrimmed = doTrim

class SynthesizeCcdImage(GetCcdImage):
    """A class to return an Image of a given Ccd based on its cameraGeometry"""
    
    def __init__(self, isTrimmed=True):
        """Initialise"""
        super(SynthesizeCcdImage, self).__init__()
        self.isTrimmed = isTrimmed
        self.isRaw = True               # we're always pretending to generate data straight from the DAQ

    def getImage(self, ccd, amp, imageFactory=afwImage.ImageU):
        """Return an image of the specified amp in the specified ccd"""
        
        bbox = amp.getAllPixels(self.isTrimmed)
        im = imageFactory(bbox.getDimensions())
        xy0 = afwGeom.Extent2I(bbox.getMin())

        im += int(amp.getElectronicParams().getReadNoise())
        bbox = afwGeom.Box2I(amp.getDataSec(self.isTrimmed))
        bbox.shift(-xy0)
        sim = imageFactory(im, bbox, afwImage.LOCAL)
        sim += int(1 + 100*amp.getElectronicParams().getGain() + 0.5)
        # Mark the amplifier
        dataSec = amp.getDataSec(self.isTrimmed)
        if amp.getReadoutCorner() == cameraGeom.Amp.LLC:
            xy = dataSec.getMin()
        elif amp.getReadoutCorner() == cameraGeom.Amp.LRC:
            xy = afwGeom.Point2I(dataSec.getMaxX() - 2, dataSec.getMinY())
        elif amp.getReadoutCorner() == cameraGeom.Amp.ULC:
            xy = afwGeom.Point2I(dataSec.getMinX()    , dataSec.getMaxY() - 2)
        elif amp.getReadoutCorner() == cameraGeom.Amp.URC:
            xy = afwGeom.Point2I(dataSec.getMaxX() - 2, dataSec.getMaxY() - 2)
        else:
            assert(not "Possible readoutCorner")

        imageFactory(im, afwGeom.Box2I(xy - xy0, afwGeom.Extent2I(3, 3)), afwImage.LOCAL).set(0)

        return im

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def mergeGeomDefaults(cameraGeomPolicy):
   policyFile = pexPolicy.DefaultPolicyFile("afw", "CameraGeomDictionary.paf", "policy")
   defPolicy = pexPolicy.Policy.createPolicy(policyFile, policyFile.getRepositoryPath(), True)

   cameraGeomPolicy.mergeDefaults(defPolicy.getDictionary())
   
#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def getGeomPolicy(cameraGeomPolicy):
    """Return a Policy describing a Camera's geometry given a filename; the Policy will be validated using the
    dictionary, and defaults will be supplied.  If you pass a Policy, it will be validated and completed.
"""

    policyFile = pexPolicy.DefaultPolicyFile("afw", "CameraGeomDictionary.paf", "policy")
    defPolicy = pexPolicy.Policy.createPolicy(policyFile, policyFile.getRepositoryPath(), True)

    if isinstance(cameraGeomPolicy, pexPolicy.Policy):
        geomPolicy = cameraGeomPolicy
    else:
        if os.path.exists(cameraGeomPolicy):
            geomPolicy = pexPolicy.Policy.createPolicy(cameraGeomPolicy)
        else:
            policyFile = pexPolicy.DefaultPolicyFile("afw", cameraGeomPolicy, "examples")
            geomPolicy = pexPolicy.Policy.createPolicy(policyFile, policyFile.getRepositoryPath(), True)

    geomPolicy.mergeDefaults(defPolicy.getDictionary())

    return geomPolicy

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def makeCcd(geomPolicy, ccdId=None, ccdInfo=None, defectDict={}):
    """Build a Ccd from a set of amplifiers given a suitable pex::Policy

If ccdInfo is provided it's set to various facts about the CCDs which are used in unit tests.  Note
in particular that it has an entry ampSerial which is a single-element list, the amplifier serial counter
    """
    ccdPol = geomPolicy.get("Ccd")

    pixelSize = ccdPol.get("pixelSize")

    nCol = ccdPol.get("nCol")
    nRow = ccdPol.get("nRow")
    if not ccdId:
        try:
            ccdId = cameraGeom.Id(ccdPol.get("serial"), ccdPol.get("name"))
        except Exception, e:
            ccdId = cameraGeom.Id(0, "unknown")
    #
    # Find the proper electronic parameters.  The Ccd name may be specified as "*" to match all detectors,
    # a feature that's probably mostly useful for testing
    #
    electronicPol = geomPolicy.get("Electronic")
    electronics = {}
    for pol in electronicPol.getArray("Raft"):
        for pol in pol.getArray("Ccd"):
            electronicCcdName = pol.get("name")
            if electronicCcdName in ("*", ccdId.getName()):
                electronics["ccdName"] = electronicCcdName
                for p in pol.getArray("Amp"):
                    electronics[tuple(p.getArray("index"))] = p
                break
    #
    # Actually build the Ccd
    #
    ccd = cameraGeom.Ccd(ccdId, pixelSize)
    
    for k in defectDict.keys():
        if ccdId == k:
            ccd.setDefects(defectDict[k])
        else:
            pass

    if nCol*nRow != len(ccdPol.getArray("Amp")):
        msg = "Expected location of %d amplifiers, got %d" % (nCol*nRow, len(ccdPol.getArray("Amp")))
        
        if force:
            print >> sys.stderr, msg
        else:
            raise RuntimeError, msg

    if ccdInfo is None:
        ampSerial = [0]
    else:
        ampSerial = ccdInfo.get("ampSerial", [0])
    ampSerial0 = None                   # used in testing
        
    readoutCorners = dict(LLC = cameraGeom.Amp.LLC,
                          LRC = cameraGeom.Amp.LRC,
                          ULC = cameraGeom.Amp.ULC,
                          URC = cameraGeom.Amp.URC)
    for ampPol in ccdPol.getArray("Amp"):
        if ampPol.exists("serial"):
            serial = ampPol.get("serial")
            ampSerial[0] = serial
        else:
            serial = ampSerial[0]
        ampSerial[0] += 1

        if ampSerial0 is None:
            ampSerial0 = serial

        Col, Row = index = tuple(ampPol.getArray("index"))
        c =  ampPol.get("readoutCorner")

        if Col not in range(nCol) or Row not in range(nRow):
            msg = "Amp location %d, %d is not in 0..%d, 0..%d" % (Col, Row, nCol, nRow)
            if force:
                print >> sys.stderr, msg
                continue
            else:
                raise RuntimeError, msg

        try:
            ePol = electronics[index]
            gain = ePol.get("gain")
            readNoise = ePol.get("readNoise")
            saturationLevel = ePol.get("saturationLevel")
        except KeyError:
            if electronics.get("ccdName") != "*":
                raise RuntimeError, ("Unable to find electronic info for Ccd \"%s\", Amp %s" %
                                     (ccd.getId(), serial))
            gain, readNoise, saturationLevel = 0, 0, 0
        #
        # Now lookup properties common to all the CCD's amps
        #
        ampPol = geomPolicy.get("Amp")
        width = ampPol.get("width")
        height = ampPol.get("height")

        extended = ampPol.get("extended")
        preRows = ampPol.get("preRows")
        overclockH = ampPol.get("overclockH")
        overclockV = ampPol.get("overclockV")

        eWidth = extended + width + overclockH
        eHeight = preRows + height + overclockV

        allPixels = afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Extent2I(eWidth, eHeight))

        try:
            c = readoutCorners[c]
        except IndexError:
            raise RuntimeError, ("Unknown readoutCorner %s" % c)
        
        if c in (cameraGeom.Amp.LLC, cameraGeom.Amp.ULC):
            biasSec = afwGeom.Box2I(afwGeom.Point2I(extended + width, preRows), afwGeom.Extent2I(overclockH, height))
            dataSec = afwGeom.Box2I(afwGeom.Point2I(extended, preRows), afwGeom.Extent2I(width, height))
        elif c in (cameraGeom.Amp.LRC, cameraGeom.Amp.URC):
            biasSec = afwGeom.Box2I(afwGeom.Point2I(0, preRows), afwGeom.Extent2I(overclockH, height))
            dataSec = afwGeom.Box2I(afwGeom.Point2I(overclockH, preRows), afwGeom.Extent2I(width, height))

        eParams = cameraGeom.ElectronicParams(gain, readNoise, saturationLevel)
        amp = cameraGeom.Amp(cameraGeom.Id(serial, "ID%d" % serial, Col, Row),
                             allPixels, biasSec, dataSec, c, eParams)
        #
        # Actually add amp to the Ccd
        #
        ccd.addAmp(Col, Row, amp)
    #
    # Information for the test code
    #
    if ccdInfo is not None:
        ccdInfo.clear()
        ccdInfo["ampSerial"] = ampSerial
        ccdInfo["name"] = ccd.getId().getName()
        ccdInfo["ampWidth"], ccdInfo["ampHeight"] = width, height
        ccdInfo["width"], ccdInfo["height"] = nCol*eWidth, nRow*eHeight
        ccdInfo["trimmedWidth"], ccdInfo["trimmedHeight"] = nCol*width, nRow*height
        ccdInfo["pixelSize"] = pixelSize
        ccdInfo["ampIdMin"] = ampSerial0
        ccdInfo["ampIdMax"] = ampSerial[0] - 1

    return ccd

def makeRaft(geomPolicy, raftId=None, raftInfo=None, defectDict={}):
    """Build a Raft from a set of CCDs given a suitable pex::Policy
    
If raftInfo is provided it's set to various facts about the Rafts which are used in unit tests.  Note in
particular that it has an entry ampSerial which is a single-element list, the amplifier serial counter
"""

    if raftInfo is None:
        ccdInfo = None
    else:
        ccdInfo = {"ampSerial" : raftInfo.get("ampSerial", [0])}

    if raftId and geomPolicy.isArray("Raft"):
        raftPol = None
        for p in geomPolicy.getArray("Raft"):
            if p.exists("name"):        # Build an Id from available information
                if p.exists("serial"):
                    rid = cameraGeom.Id(p.get("serial"), p.get("name"))
                else:
                    rid = cameraGeom.Id(p.get("name"))
            elif p.exists("serial"):
                rid = cameraGeom.Id(p.get("serial"))
            else:
                raise RuntimeError, "Please provide a raft name, a raft serial, or both"
                    
            if rid == raftId:
                raftPol = p
                break

        if not raftPol:
            raise RuntimeError, ("I can't find Raft %s" % raftId)            
    else:
        raftPol = geomPolicy.get("Raft")
        
    nCol = raftPol.get("nCol")
    nRow = raftPol.get("nRow")
    if not raftId:
        try:
            raftId = cameraGeom.Id(raftPol.get("serial"), raftPol.get("name"))
        except Exception, e:
            raftId = cameraGeom.Id(0, "unknown")
    #
    # Discover how the per-amp data is laid out on disk; it's common for the data acquisition system to
    # put together a single image for an entire CCD, but it isn't mandatory
    #
    diskFormatPol = geomPolicy.get("CcdDiskLayout")
    hduPerAmp = diskFormatPol.get("HduPerAmp")
    ampDiskLayout = {}
    if hduPerAmp:
        for p in diskFormatPol.getArray("Amp"):
            ampDiskLayout[p.get("serial")] = (p.get("hdu"), p.get("flipLR"), p.get("flipTB"))
    #
    # Build the Raft
    #
    raft = cameraGeom.Raft(raftId, nCol, nRow)

    if nCol*nRow != len(raftPol.getArray("Ccd")):
        if False:                       # many cameras don't use filled rafts at the edge (e.g. HSC)
            msg = "Expected location of %d Ccds, got %d" % (nCol*nRow, len(raftPol.getArray("Ccd")))
            
            if force:
                print >> sys.stderr, msg
            else:
                raise RuntimeError, msg

    for ccdPol in raftPol.getArray("Ccd"):
        Col, Row = ccdPol.getArray("index")
        xc, yc = ccdPol.getArray("offset")

        nQuarter = ccdPol.get("nQuarter")
        pitch, roll, yaw = [float(math.radians(a)) for a in ccdPol.getArray("orientation")]

        if Col not in range(nCol) or Row not in range(nRow):
            msg = "Ccd location %d, %d is not in 0..%d, 0..%d" % (Col, Row, nCol, nRow)
            if force:
                print >> sys.stderr, msg
                continue
            else:
                raise RuntimeError, msg

        ccdId = cameraGeom.Id(ccdPol.get("serial"), ccdPol.get("name"))
        ccd = makeCcd(geomPolicy, ccdId, ccdInfo=ccdInfo, defectDict=defectDict)

        raft.addDetector(afwGeom.Point2I(Col, Row),
                         afwGeom.Point2D(xc, yc),
                         cameraGeom.Orientation(nQuarter, pitch, roll, yaw), ccd)

        #
        # Set the on-disk layout parameters now that we've possibly rotated the Ccd to fit the raft
        #
        if hduPerAmp:
            for amp in ccd:
                hdu, flipLR, flipTB = ampDiskLayout[amp.getId().getSerial()]
                nQuarterAmp = 0         # XXX For now; needs to come from policy
                amp.setDiskLayout(amp.getAllPixels().getMin(), nQuarterAmp, flipLR, flipTB)

        if raftInfo is not None:
            # Guess the gutter between detectors
            if (Col, Row) == (0, 0):
                xGutter, yGutter = xc, yc
            elif (Col, Row) == (nCol - 1, nRow - 1):
                if nCol == 1:
                    xGutter = 0.0
                else:
                    xGutter = (xc - xGutter)/float(nCol - 1) - ccd.getSize()[0]

                if nRow == 1:
                    yGutter = 0.0
                else:
                    yGutter = (yc - yGutter)/float(nRow - 1) - ccd.getSize()[1]

    if raftInfo is not None:
        raftInfo.clear()
        raftInfo["ampSerial"] = ccdInfo["ampSerial"]
        raftInfo["name"] = raft.getId().getName()
        raftInfo["pixelSize"] = ccd.getPixelSize()
        raftInfo["width"] =  nCol*ccd.getAllPixels(True).getWidth()
        raftInfo["height"] = nRow*ccd.getAllPixels(True).getHeight()
        raftInfo["widthMm"] =  nCol*ccd.getSize()[0] + (nCol - 1)*xGutter
        raftInfo["heightMm"] = nRow*ccd.getSize()[1] + (nRow - 1)*yGutter

    return raft

def makeCamera(geomPolicy, cameraId=None, cameraInfo=None):
    """Build a Camera from a set of Rafts given a suitable pex::Policy
    
If cameraInfo is provided it's set to various facts about the Camera which are used in unit tests.  Note in
particular that it has an entry ampSerial which is a single-element list, the amplifier serial counter
"""
    if cameraInfo is None:
        raftInfo = None
    else:
        raftInfo = {"ampSerial" : cameraInfo.get("ampSerial", [0])}

    cameraPol = geomPolicy.get("Camera")
    nCol = cameraPol.get("nCol")
    nRow = cameraPol.get("nRow")

    if not cameraId:
        cameraId = cameraGeom.Id(cameraPol.get("serial"), cameraPol.get("name"))
    camera = cameraGeom.Camera(cameraId, nCol, nRow)
   
    if geomPolicy.get("Defects").get("Raft").get("Ccd").isPolicy("Defect"):
        defDict = makeDefects(geomPolicy)
    else:
        defDict = {}

    for raftPol in cameraPol.getArray("Raft"):
        Col, Row = raftPol.getArray("index")
        xc, yc = raftPol.getArray("offset")
        raftId = cameraGeom.Id(raftPol.get("serial"), raftPol.get("name"))
        raft = makeRaft(geomPolicy, raftId, raftInfo, defectDict=defDict)
        camera.addDetector(afwGeom.Point2I(Col, Row),
                           afwGeom.Point2D(xc, yc), cameraGeom.Orientation(), raft)

        if cameraInfo is not None:
            # Guess the gutter between detectors
            if (Col, Row) == (0, 0):
                xGutter, yGutter = xc, yc
            elif (Col, Row) == (nCol - 1, nRow - 1):
                if nCol == 1:
                    xGutter = 0.0
                else:
                    xGutter = (xc - xGutter)/float(nCol - 1) - raft.getSize()[0]
                    
                if nRow == 1:
                    yGutter = 0.0
                else:
                    yGutter = (yc - yGutter)/float(nRow - 1) - raft.getSize()[1]


    #######################
    # get the distortion
    distortPolicy = cameraPol.get('Distortion')
    distortActive = distortPolicy.get('active')
    activePolicy  = distortPolicy.get(distortActive)

    distort = None
    if distortActive == "NullDistortion":
	distort = cameraGeom.NullDistortion()
    elif distortActive == "RadialPolyDistortion":
	coeffs = activePolicy.getArray('coeffs')
	distort = cameraGeom.RadialPolyDistortion(coeffs)
    camera.setDistortion(distort)


    if cameraInfo is not None:
        cameraInfo.clear()
        cameraInfo["ampSerial"] = raftInfo["ampSerial"]
        cameraInfo["name"] = camera.getId().getName()
        cameraInfo["width"] =  nCol*raft.getAllPixels().getWidth()
        cameraInfo["height"] = nRow*raft.getAllPixels().getHeight()
        cameraInfo["pixelSize"] = raft.getPixelSize()
        cameraInfo["widthMm"] =  nCol*raft.getSize()[0] + (nCol - 1)*xGutter
        cameraInfo["heightMm"] = nRow*raft.getSize()[1] + (nRow - 1)*yGutter

    return camera

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def makeAmpImageFromCcd(amp, imageSource=SynthesizeCcdImage(), isTrimmed=None, imageFactory=afwImage.ImageU):
    """Make an Image of an Amp"""

    return imageSource.getImage(amp, imageFactory=imageFactory)

def makeImageFromCcd(ccd, imageSource=SynthesizeCcdImage(), amp=None,
                     isTrimmed=None, imageFactory=afwImage.ImageU, bin=1, natural=False):
    """Make an Image of a Ccd (or just a single amp)

    If natural is True, return the CCD image without worrying about whether it's rotated when
    placed into the camera
    """

    if isTrimmed is None:
        isTrimmed = ccd.isTrimmed()
    imageSource.setTrimmed(isTrimmed)

    if amp:
        ampImage = imageFactory(amp.getAllPixels(isTrimmed).getDimensions())
        ampImage <<= imageSource.getImage(ccd, amp, imageFactory=imageFactory)

        if bin > 1:
            ampImage = afwMath.binImage(ampImage, bin)
            
        return ampImage
    #
    # Start by building the image in "Natural" (non-rotated) orientation
    # (unless it's 'raw', in which case it's just easier to prepareAmpData)
    #
    if imageSource.isRaw:
        ccdImage = imageFactory(ccd.getAllPixelsNoRotation(isTrimmed))

        for a in ccd:
            im = ccdImage.Factory(ccdImage, a.getAllPixels(isTrimmed), afwImage.LOCAL)
            im <<= a.prepareAmpData(imageSource.getImage(ccd, a, imageFactory=imageFactory))
    else:
        ccdImage = imageSource.getImage(ccd, imageFactory=imageFactory)

    if bin > 1:
        ccdImage = afwMath.binImage(ccdImage, bin)
    #
    # Now rotate to the as-installed orientation
    #
    if not natural and not imageSource.isRaw:
        ccdImage = afwMath.rotateImageBy90(ccdImage, ccd.getOrientation().getNQuarter())

    return ccdImage

def trimExposure(ccdImage, ccd=None):
    """Trim a raw CCD Exposure"""

    if not ccd:
        ccd = cameraGeom.cast_Ccd(ccdImage.getDetector())
    
    dim = ccd.getAllPixelsNoRotation(True).getDimensions()
    trimmedImage = ccdImage.Factory(dim)
    for a in ccd:
        data = ccdImage.Factory(ccdImage, a.getDataSec(False), afwImage.LOCAL).getMaskedImage()
        tdata = trimmedImage.Factory(trimmedImage, a.getDataSec(True), afwImage.LOCAL).getMaskedImage()
        tdata <<= data

    ccd.setTrimmed(True)
    return trimmedImage

def showCcd(ccd, ccdImage="", amp=None, ccdOrigin=None, isTrimmed=None, frame=None, overlay=True, bin=1):
    """Show a CCD on ds9.  If cameraImage is "", an image will be created based on the properties
of the detectors"""
    
    if isTrimmed is None:
        isTrimmed = ccd.isTrimmed()

    if ccdImage == "":
        ccdImage = makeImageFromCcd(ccd, bin=bin)

    if ccdImage:
        title = ccd.getId().getName()
        if amp:
            title += ":%d" % amp.getId().getSerial()
        if isTrimmed:
            title += "(trimmed)"
        ds9.mtv(ccdImage, frame=frame, title=title)

    if not overlay:
        return

    if amp:
        bboxes = [(amp.getAllPixels(isTrimmed), 0.49, None),]
        xy0 = bboxes[0][0].getMin()
        if not isTrimmed:
            bboxes.append((amp.getBiasSec(), 0.49, ds9.RED)) 
            bboxes.append((amp.getDataSec(), 0.49, ds9.BLUE))

        for bbox, borderWidth, ctype in bboxes:
            bbox = bbox.clone()
            bbox.shift(-afwGeom.ExtentI(xy0))
            displayUtils.drawBBox(bbox, borderWidth=borderWidth, ctype=ctype, frame=frame, bin=bin)

        return

    nQuarter = ccd.getOrientation().getNQuarter()
    ccdDim = ccd.getAllPixels(isTrimmed).getDimensions()
    for a in ccd:
        bbox = a.getAllPixels(isTrimmed)
        if nQuarter != 0:
            bbox = cameraGeom.rotateBBoxBy90(bbox, nQuarter, ccdDim)

        displayUtils.drawBBox(bbox, origin=ccdOrigin, borderWidth=0.49,
                              frame=frame, bin=bin)

        if not isTrimmed:
            for bbox, ctype in ((a.getBiasSec(), ds9.RED), (a.getDataSec(), ds9.BLUE)):
                if nQuarter != 0:
                    bbox = cameraGeom.rotateBBoxBy90(bbox, nQuarter, ccdDim)
                displayUtils.drawBBox(bbox, origin=ccdOrigin,
                                      borderWidth=0.49, ctype=ctype, frame=frame, bin=bin)
        # Label each Amp
        ap = a.getAllPixels(isTrimmed)
        xc, yc = (ap.getX0() + ap.getX1())//2, (ap.getY0() + ap.getY1())//2
        cen = afwGeom.makePointI(xc, yc)
        #
        # Rotate the amp labels too
        #
        if nQuarter == 0:
            c, s = 1, 0
        elif nQuarter == 1:
            c, s = 0, -1
        elif nQuarter == 2:
            c, s = -1, 0
        elif nQuarter == 3:
            c, s = 0, 1

        xc -= 0.5*ccdHeight
        yc -= 0.5*ccdWidth
            
        xc, yc = 0.5*ccdHeight + c*xc + s*yc, 0.5*ccdWidth + -s*xc + c*yc

        if ccdOrigin:
            xc += ccdOrigin[0]
            yc += ccdOrigin[1]

        ds9.dot(str(ccd.findAmp(cen).getId().getSerial()), xc/bin, yc/bin, frame=frame)

    displayUtils.drawBBox(ccd.getAllPixels(isTrimmed), origin=ccdOrigin,
                          borderWidth=0.49, ctype=ds9.MAGENTA, frame=frame, bin=bin)

def makeImageFromRaft(raft, imageSource=SynthesizeCcdImage(), raftCenter=None,
                      imageFactory=afwImage.ImageU, bin=1):
    """Make an Image of a Raft"""

    if raftCenter is None:
        raftCenter = afwGeom.Point2I(raft.getAllPixels().getDimensions()//2)

    raftImage = imageFactory(raft.getAllPixels().Dimensions()//bin)

    for det in raft:
        ccd = cameraGeom.cast_Ccd(det)
        
        bbox = ccd.getAllPixels(True)
        cen = getCenterPixel()
        origin = afwGeom.PointI(cen[0], cen[1]) - \
                bbox.getDimensions()/2 + \
                afwGeom.Extent2I(raftCenter[0], raftCenter[1])

        bbox = afwGeom.Box2I(afwGeom.Point2I((origin.getX() + bbox.getMinX())//bin,
                                             (origin.getY() + bbox.getMinY())//bin),
                             bbox.getDimensions()//bin)

        ccdImage = raftImage.Factory(raftImage, bbox, afwImage.LOCAL)
        ccdImage <<= makeImageFromCcd(ccd, imageSource, imageFactory=imageFactory, isTrimmed=True, bin=bin)

    return raftImage

def showRaft(raft, imageSource=SynthesizeCcdImage(), raftOrigin=None, frame=None, overlay=True, bin=1):
    """Show a Raft on ds9.

If imageSource isn't None, create an image using the images specified by imageSource"""

    raftCenter = afwGeom.Point2I(raft.getAllPixels().getDimensions()/2)
    if raftOrigin:
        raftCenter += afwGeom.ExtentI(int(raftOrigin[0]), int(raftOrigin[1]))

    if imageSource is None:
        raftImage = None
    elif isinstance(imageSource, GetCcdImage):
        raftImage = makeImageFromRaft(raft, imageSource=imageSource, raftCenter=raftCenter, bin=bin)
    else:
        raftImage = imageSource

    if raftImage:
        ds9.mtv(raftImage, frame=frame, title=raft.getId().getName())

    if not raftImage and not overlay:
        return

    for det in raft:
        ccd = cameraGeom.cast_Ccd(det)
        
        bbox = ccd.getAllPixels(True)
        origin = ccd.getCenterPixel() - \
                afwGeom.ExtentD(bbox.getWidth()/2 - raftCenter.getX(), 
                                bbox.getHeight()/2 - raftCenter.getY())
            
        if True:
            name = ccd.getId().getName()
        else:
            name = str(ccd.getCenter())

        ds9.dot(name, (origin[0] + 0.5*bbox.getWidth())/bin,
                      (origin[1] + 0.4*bbox.getHeight())/bin, frame=frame)

        showCcd(ccd, None, isTrimmed=True, frame=frame, ccdOrigin=origin, overlay=overlay, bin=bin)

def makeImageFromCamera(camera, imageSource=None, imageFactory=afwImage.ImageU, bin=1):
    """Make an Image of a Camera"""

    cameraImage = imageFactory(camera.getAllPixels().getDimensions()/bin)
    for det in camera:
        raft = cameraGeom.cast_Raft(det);
        bbox = raft.getAllPixels()
        origin = camera.getCenterPixel() + afwGeom.Extent2D(raft.getCenterPixel()) - \
                 afwGeom.Extent2D(bbox.getWidth()/2, bbox.getHeight()/2)               
        bbox = afwGeom.Box2I(afwGeom.Point2I((origin.getX() + bbox.getMinX())//bin,
                                           (origin.getY() + bbox.getMinY())//bin),
                             bbox.getDimensions()//bin)

        im = cameraImage.Factory(cameraImage, bbox, afwImage.LOCAL)

        im <<= makeImageFromRaft(raft, imageSource,
                                 raftCenter=None,
                                 imageFactory=imageFactory, bin=bin)
        serial = raft.getId().getSerial()
        im += serial if serial > 0 else 0

    return cameraImage

def showCamera(camera, imageSource=SynthesizeCcdImage(), imageFactory=afwImage.ImageU,
               frame=None, overlay=True, bin=1):
    """Show a Camera on ds9 (with the specified frame); if overlay show the IDs and amplifier boundaries

If imageSource is provided its getImage method will be called to return a CCD image (e.g. a
cameraGeom.GetCcdImage object); if it is "", an image will be created based on the properties
of the detectors"""

    if imageSource is None:
        cameraImage = None
    elif isinstance(imageSource, GetCcdImage):
        cameraImage = makeImageFromCamera(camera, imageSource, bin=bin, imageFactory=imageFactory)
    else:
        cameraImage = imageSource

    if cameraImage:
        ds9.mtv(cameraImage, frame=frame, title=camera.getId().getName())

    for det in camera:
        raft = cameraGeom.cast_Raft(det)
        
        center = camera.getCenterPixel() + afwGeom.Extent2D(raft.getCenterPixel())

        if overlay:
            bbox = raft.getAllPixels()
            ds9.dot(raft.getId().getName(), center[0]/bin, center[1]/bin, frame=frame)

        showRaft(raft, None, frame=frame, overlay=overlay,
                 raftOrigin=center - afwGeom.makeExtentD(raft.getAllPixels().getWidth()/2,
                                                         raft.getAllPixels().getHeight()/2), 
                 bin=bin)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def showMosaic(fileName, geomPolicy=None, camera=None,
               display=True, what=cameraGeom.Camera, id=None, overlay=False, describe=False, doTrim=False,
               imageFactory=afwImage.ImageU, bin=1, frame=None):
    """Return a mosaic for a given snapshot of the sky; if display is true, also show it on ds9

The camera geometry is defined by cameraGeomPolicyFile;  raft IDs etc. are drawn on ds9 if overlay is True;
The camera (or raft) is described if describe is True

You may set what to a type (e.g. cameraGeom.Ccd) to display that type; if provided id will be obeyed

If relevant (for e.g. a Ccd) doTrim is applied to the Detector.
    """

    if isinstance(fileName, GetCcdImage):
        imageSource = fileName
    elif isinstance(fileName, str):
        imageSource = GetCcdImage(fileName) # object that understands the CCD <--> HDU mapping
    else:
        imageSource = None

    if imageSource:
        imageSource.setTrimmed(doTrim)
    
    if not camera:
        camera = makeCamera(geomPolicy)

    if what == cameraGeom.Amp:
        if id is None:
            ccd = makeCcd(geomPolicy)
        else:
            ccd = findCcd(camera, id[0])
        amp = [a for a in ccd if a.getId() == id[1]][0]

        if not amp:
            raise RuntimeError, "Failed to find Amp %s" % id

        ccd.setTrimmed(doTrim)

        outImage = makeImageFromCcd(ccd, imageSource, amp=amp, imageFactory=imageFactory, bin=bin)
        if display:
            showCcd(ccd, outImage, amp=amp, overlay=overlay, frame=frame, bin=bin)
    elif what == cameraGeom.Ccd:
        if id is None:
            ccd = makeCcd(geomPolicy)
        else:
            ccd = findCcd(camera, id)

        if not ccd:
            raise RuntimeError, "Failed to find Ccd %s" % id

        ccd.setTrimmed(doTrim)

        outImage = makeImageFromCcd(ccd, imageSource, imageFactory=imageFactory, bin=bin)
        if display:
            showCcd(ccd, outImage, overlay=overlay, frame=frame, bin=bin)
    elif what == cameraGeom.Raft:
        if id:
            raft = findRaft(camera, id)
        else:
            raft = makeRaft(geomPolicy)
        if not raft:
            raise RuntimeError, "Failed to find Raft %s" % id

        outImage = makeImageFromRaft(raft, imageSource, imageFactory=imageFactory, bin=bin)
        if display:
            showRaft(raft, outImage, overlay=overlay, frame=frame, bin=bin)

        if describe:
            print describeRaft(raft)
    elif what == cameraGeom.Camera:
        outImage = makeImageFromCamera(camera, imageSource, imageFactory=imageFactory, bin=bin)
        if display:
            showCamera(camera, outImage, overlay=overlay, frame=frame, bin=bin)

        if describe:
            print describeCamera(camera)
    else:
        raise RuntimeError, ("I don't know how to display %s" % what)

    return outImage

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def describeRaft(raft, indent=""):
    """Describe an entire Raft"""
    descrip = []

    size = raft.getSize()
    descrip.append("%sRaft \"%s\",  %gx%g  BBox %s" % (indent, raft.getId(),
                                                        size[0], size[1], raft.getAllPixels()))
    
    for d in cameraGeom.cast_Raft(raft):
        cenPixel = d.getCenterPixel()
        cen = d.getCenter()

        descrip.append("%sCcd: %s (%5d, %5d) %s  (%7.1f, %7.1f)" % \
                       ((indent + "    "),
                        d.getAllPixels(True), cenPixel[0], cenPixel[1],
                        cameraGeom.ReadoutCorner(d.getOrientation().getNQuarter()), cen[0], cen[1]))
            
    return "\n".join(descrip)

def describeCamera(camera):
    """Describe an entire Camera"""
    descrip = []

    size = camera.getSize()
    descrip.append("Camera \"%s\",  %gx%g  BBox %s" % \
                   (camera.getId(), size[0], size[1], camera.getAllPixels()))

    for raft in camera:
        descrip.append(describeRaft(raft, "    "))

    return "\n".join(descrip)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def findAmp(parent, ccdId, ix, iy):
    """Find the Amp with the specified Id within the composite"""

    ccd = findCcd(parent, ccdId)
    for amp in ccd:
        if amp.getId().getIndex() == (ix, iy):
            return amp

    return None

def findCcd(parent, id):
    """Find the Ccd with the specified Id within the composite"""

    if isinstance(parent, cameraGeom.Camera):
        for d in parent:
            ccd = findCcd(cameraGeom.cast_Raft(d), id)
            if ccd:
                return ccd
    elif isinstance(parent, cameraGeom.Raft):
        try:
            return cameraGeom.cast_Ccd(parent.findDetector(id))
        except:
            pass
    else:
        if parent.getId() == id:
            return cameraGeom.cast_Ccd(parent)
        
    return None

def findRaft(parent, id):
    """Find the Raft with the specified Id within the composite"""

    if isinstance(parent, cameraGeom.Camera):
        d = parent.findDetector(id)
        if d:
            return cameraGeom.cast_Raft(d)
    else:
        if parent.getId() == id:
            return raft

    return None

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def makeDefectsFromFits(filename):
    """Create a dictionary of DefectSets from a fits file with one ccd worth
    of defects per extension.

       The dictionay is indexed by an Id object --- remember to compare by str(id) not object identity
    """
    hdulist = pyfits.open(filename)
    defects = {}
    for hdu in hdulist[1:]:
        id = cameraGeom.Id(hdu.header['serial'], hdu.header['name'])
        data = hdu.data
        defectList = []
        for i in range(len(data)):
            bbox = afwGeom.Box2I(
                        afwGeom.Point2I(int(data[i]['x0']), int(data[i]['y0'])),\
		        afwGeom.Extent2I(int(data[i]['width']), int(data[i]['height'])))
            defectList.append(afwImage.DefectBase(bbox))
        defects[id] = defectList
    return defects

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def makeDefects(geomPolicy):
    """Create a dictionary of DefectSets from a pexPolicy::Policy

The dictionay is indexed by an Id object --- remember to compare by str(id) not object identity
    """

    defectsDict = {}
    defectListPol = geomPolicy.get("Defects")
    for raftPol in defectListPol.getArray("Raft"):
        for defectPol in raftPol.getArray("Ccd"):
            defects = afwImage.DefectSet()
            ccdId = cameraGeom.Id(defectPol.get("serial"), defectPol.get("name"))
            defectsDict[ccdId] = defects

            for defect in defectPol.getArray("Defect"):
                x0 = defect.get("x0")
                y0 = defect.get("y0")

                x1 = y1 = width = height = None
                if defect.exists("x1"):
                    x1 = defect.get("x1")
                if defect.exists("y1"):
                    y1 = defect.get("y1")
                if defect.exists("width"):
                    width = defect.get("width")
                if defect.exists("height"):
                    height = defect.get("height")

                if x1 is None:
                    if width:
                        x1 = x0 + width - 1
                    else:
                        raise RuntimeError, ("Defect at (%d,%d) for CCD (%s) has no x1/width" % (x0, y0, ccdId))
                else:
                    if width:
                        if x1 != x0 + width - 1:
                            raise RuntimeError, \
                                  ("Defect at (%d,%d) for CCD (%s) has inconsistent x1/width = %d,%d" % \
                                   (x0, y0, ccdId, x1, width))

                if y1 is None:
                    if height:
                        y1 = y0 + height - 1
                    else:
                        raise RuntimeError, ("Defect at (%d,%d) for CCD (%s) has no y1/height" % (x0, y0, ccdId))
                else:
                    if height:
                        if y1 != y0 + height - 1:
                            raise RuntimeError, \
                                  ("Defect at (%d,%d) for CCD (%s) has inconsistent y1/height = %d,%d" % \
                                   (x0, y0, ccdId, y1, height))

                bbox = afwGeom.Box2I(afwGeom.Point2I(x0, y0), afwGeom.Point2I(x1, y1))
                defects.push_back(afwImage.DefectBase(bbox))

    return defectsDict

