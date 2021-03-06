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
Tests for Footprints, and FootprintSets

Run with:
   footprint1.py
or
   python
   >>> import footprint1; footprint1.run()
"""


import math, sys
import unittest
import lsst.utils.tests as tests
import lsst.pex.logging as logging
import lsst.afw.geom as afwGeom
import lsst.afw.geom.ellipses as afwGeomEllipses
import lsst.afw.image as afwImage
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

def toString(*args):
    """toString written in python"""
    if len(args) == 1:
        args = args[0]

    y, x0, x1 = args
    return "%d: %d..%d" % (y, x0, x1)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class Object(object):
    def __init__(self, val, spans):            
        self.val = val
        self.spans = spans

    def __str__(self):
        return ", ".join([str(s) for s in self.spans])

    def insert(self, im):
        """Insert self into an image"""
        for sp in self.spans:
            y, x0, x1 = sp
            for x in range(x0, x1+1):
                im.set(x, y, self.val)

    def __eq__(self, other):
        for osp, sp in zip(other.getSpans(), self.spans):
            if osp.toString() != toString(sp):
                return False

        return True

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class ThresholdTestCase(unittest.TestCase):
    def testThresholdFactory(self):
        """
        Test the creation of a Threshold object

        This is a white-box test.
        -tests missing parameters
        -tests mal-formed parameters
        """
        try:
            afwDetect.createThreshold(3.4)
        except:
            self.fail("Failed to build Threshold with proper parameters")
        
        try:
            afwDetect.createThreshold(3.4, "foo bar")
        except:
            pass
        else:
            self.fail("Threhold parameters not properly validated")

        try:
            afwDetect.createThreshold(3.4, "variance")
        except:
            self.fail("Failed to build Threshold with proper parameters")

        try:
            afwDetect.createThreshold(3.4, "stdev")
        except:
            self.fail("Failed to build Threshold with proper parameters")

        try:
            afwDetect.createThreshold(3.4, "value")
        except:
            self.fail("Failed to build Threshold with proper parameters")
        
        try:
            afwDetect.createThreshold(3.4, "value", False)
        except:
            self.fail("Failed to build Threshold with VALUE, False parameters")

        try:
            afwDetect.createThreshold(0x4, "bitmask")
        except:
            self.fail("Failed to build Threshold with BITMASK parameters")
        
        try:
            afwDetect.createThreshold(5, "pixel_stdev")
        except:
            self.fail("Failed to build Threshold with PIXEL_STDEV parameters")
        
class FootprintTestCase(unittest.TestCase):
    """A test case for Footprint"""
    def setUp(self):
        self.foot = afwDetect.Footprint()

    def tearDown(self):
        del self.foot

    def testToString(self):
        y, x0, x1 = 10, 100, 101
        s = afwDetect.Span(y, x0, x1)
        self.assertEqual(s.toString(), toString(y, x0, x1))

    def testGC(self):
        """Check that Footprints are automatically garbage collected (when MemoryTestCase runs)"""
        
        f = afwDetect.Footprint()

    def testId(self):
        """Test uniqueness of IDs"""
        
        self.assertNotEqual(self.foot.getId(), afwDetect.Footprint().getId())

    def testIntersectMask(self):
        bbox = afwGeom.BoxI(afwGeom.PointI(0,0), afwGeom.ExtentI(10))
        fp = afwDetect.Footprint(bbox)
        maskBBox = afwGeom.BoxI(bbox)
        maskBBox.grow(-2)
        mask = afwImage.MaskU(maskBBox)
        innerBBox = afwGeom.BoxI(maskBBox)
        innerBBox.grow(-2)
        subMask = mask.Factory(mask, innerBBox, afwImage.PARENT)
        subMask.set(1)

        fp.intersectMask(mask)
        fpBBox = fp.getBBox()
        self.assertEqual(fpBBox.getMinX(), maskBBox.getMinX())
        self.assertEqual(fpBBox.getMinY(), maskBBox.getMinY())
        self.assertEqual(fpBBox.getMaxX(), maskBBox.getMaxX())
        self.assertEqual(fpBBox.getMaxY(), maskBBox.getMaxY())

        self.assertEqual(fp.getArea(), maskBBox.getArea() - innerBBox.getArea())


    def testAddSpans(self):
        """Add spans to a Footprint"""
        for y, x0, x1 in [(10, 100, 105), (11, 99, 104)]:
            self.foot.addSpan(y, x0, x1)

        sp = self.foot.getSpans()
        
        self.assertEqual(sp[-1].toString(), toString(y, x0, x1))

    def testBbox(self):
        """Add Spans and check bounding box"""
        foot = afwDetect.Footprint()
        for y, x0, x1 in [(10, 100, 105),
                          (11, 99, 104)]:
            foot.addSpan(y, x0, x1)

        bbox = foot.getBBox()
        self.assertEqual(bbox.getWidth(), 7)
        self.assertEqual(bbox.getHeight(), 2)
        self.assertEqual(bbox.getMinX(), 99)
        self.assertEqual(bbox.getMinY(), 10)
        self.assertEqual(bbox.getMaxX(), 105)
        self.assertEqual(bbox.getMaxY(), 11)

    def testSpanShift(self):
        """Test our ability to shift spans"""

        span = afwDetect.Span(10, 100, 105)
        foot = afwDetect.Footprint()

        foot.addSpan(span, 1, 2)

        bbox = foot.getBBox()
        self.assertEqual(bbox.getWidth(), 6)
        self.assertEqual(bbox.getHeight(), 1)
        self.assertEqual(bbox.getMinX(), 101)
        self.assertEqual(bbox.getMinY(), 12)
        #
        # Shift that span using Span.shift
        #
        foot = afwDetect.Footprint()
        span.shift(-1, -2)
        foot.addSpan(span)

        bbox = foot.getBBox()
        self.assertEqual(bbox.getWidth(), 6)
        self.assertEqual(bbox.getHeight(), 1)
        self.assertEqual(bbox.getMinX(), 99)
        self.assertEqual(bbox.getMinY(), 8)

    def testFootprintFromBBox1(self):
        """Create a rectangular Footprint"""
        x0, y0, w, h = 9, 10, 7, 4
        foot = afwDetect.Footprint(afwGeom.Box2I(afwGeom.Point2I(x0, y0), afwGeom.Extent2I(w, h)))

        bbox = foot.getBBox()

        self.assertEqual(bbox.getWidth(), w)
        self.assertEqual(bbox.getHeight(), h)
        self.assertEqual(bbox.getMinX(), x0)
        self.assertEqual(bbox.getMinY(), y0)
        self.assertEqual(bbox.getMaxX(), x0 + w - 1)
        self.assertEqual(bbox.getMaxY(), y0 + h - 1)



        if False:
            idImage = afwImage.ImageU(w, h)
            idImage.set(0)        
            foot.insertIntoImage(idImage, foot.getId(), bbox)
            ds9.mtv(idImage, frame=2)

    def testGetBBox(self):
        """Check that Footprint.getBBox() returns a copy"""
        
        x0, y0, w, h = 9, 10, 7, 4
        foot = afwDetect.Footprint(afwGeom.Box2I(afwGeom.Point2I(x0, y0), afwGeom.Extent2I(w, h)))
        bbox = foot.getBBox()

        dx, dy = 10, 20
        bbox.shift(afwGeom.Extent2I(dx, dy))

        self.assertEqual(bbox.getMinX(), x0 + dx)
        self.assertEqual(foot.getBBox().getMinX(), x0)

    def testFootprintFromEllipse(self):
        """Create a circular Footprint"""

	ellipse = afwGeomEllipses.Ellipse(afwGeomEllipses.Axes(6, 6, 0), 
                                          afwGeom.Point2D(9,15))
        foot = afwDetect.Footprint(
                ellipse, 
                afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Extent2I(20, 30)))

        idImage = afwImage.ImageU(afwGeom.Extent2I(foot.getRegion().getWidth(), foot.getRegion().getHeight()))
        idImage.set(0)
        
        foot.insertIntoImage(idImage, foot.getId())

        if False:
            ds9.mtv(idImage, frame=2)

    def testCopy(self):
        bbox = afwGeom.BoxI(afwGeom.PointI(0,2), afwGeom.PointI(5,6))

        fp = afwDetect.Footprint(bbox, bbox)

        #test copy construct
        fp2 = afwDetect.Footprint(fp)

        self.assertEqual(fp2.getBBox(), bbox)
        self.assertEqual(fp2.getRegion(), bbox)
        self.assertEqual(fp2.getArea(), bbox.getArea())
        self.assertEqual(fp2.isNormalized(), True)

        y = bbox.getMinY()
        for s in fp2.getSpans():
            self.assertEqual(s.getY(), y)
            self.assertEqual(s.getX0(), bbox.getMinX())
            self.assertEqual(s.getX1(), bbox.getMaxX())
            y+=1
        
        #test assignment
        fp3 = afwDetect.Footprint()
        fp3.assign(fp)
        self.assertEqual(fp3.getBBox(), bbox)
        self.assertEqual(fp3.getRegion(), bbox)
        self.assertEqual(fp3.getArea(), bbox.getArea())
        self.assertEqual(fp3.isNormalized(), True)

        y = bbox.getMinY()
        for s in fp3.getSpans():
            self.assertEqual(s.getY(), y)
            self.assertEqual(s.getX0(), bbox.getMinX())
            self.assertEqual(s.getX1(), bbox.getMaxX())
            y+=1


    def testGrow(self):
        """Test growing a footprint"""
        x0, y0 = 20, 20
        width, height = 20, 30
        foot1 = afwDetect.Footprint(afwGeom.Box2I(afwGeom.Point2I(x0, y0), afwGeom.Extent2I(width, height)),
                                    afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Extent2I(100, 100)))
        bbox1 = foot1.getBBox()

        self.assertEqual(bbox1.getMinX(), x0)
        self.assertEqual(bbox1.getMaxX(), x0 + width - 1)
        self.assertEqual(bbox1.getWidth(), width)

        self.assertEqual(bbox1.getMinY(), y0)
        self.assertEqual(bbox1.getMaxY(), y0 + height - 1)
        self.assertEqual(bbox1.getHeight(), height)

        ngrow = 5
        for isotropic in (True, False):
            foot2 = afwDetect.growFootprint(foot1, ngrow, isotropic)
            bbox2 = foot2.getBBox()

            if False and display:
                idImage = afwImage.ImageU(width, height)
                idImage.set(0)

                i = 1
                for foot in [foot1, foot2]:
                    foot.insertIntoImage(idImage, i)
                    i += 1

                metricImage = afwImage.ImageF("foo.fits")
                ds9.mtv(metricImage, frame=1)
                ds9.mtv(idImage)

            # check bbox2
            self.assertEqual(bbox2.getMinX(), x0 - ngrow)
            self.assertEqual(bbox2.getWidth(), width + 2*ngrow)

            self.assertEqual(bbox2.getMinY(), y0 - ngrow)
            self.assertEqual(bbox2.getHeight(), height + 2*ngrow)
            # Check that region was preserved
            self.assertEqual(foot1.getRegion(), foot2.getRegion())

    def testFootprintToBBoxList(self):
        """Test footprintToBBoxList"""
        region = afwGeom.Box2I(afwGeom.Point2I(0,0), afwGeom.Extent2I(12,10))
        foot = afwDetect.Footprint(0, region)
        for y, x0, x1 in [(3, 3, 5), (3, 7, 7),
                          (4, 2, 3), (4, 5, 7),
                          (5, 2, 3), (5, 5, 8),
                          (6, 3, 5), 
                          ]:
            foot.addSpan(y, x0, x1)
        
        idImage = afwImage.ImageU(region.getDimensions())
        idImage.set(0)

        foot.insertIntoImage(idImage, 1)
        if display:
            ds9.mtv(idImage)

        idImageFromBBox = idImage.Factory(idImage, True)
        idImageFromBBox.set(0)
        bboxes = afwDetect.footprintToBBoxList(foot)
        for bbox in bboxes:
            x0, y0, x1, y1 = bbox.getMinX(), bbox.getMinY(), bbox.getMaxX(), bbox.getMaxY()

            for y in range(y0, y1 + 1):
                for x in range(x0, x1 + 1):
                    idImageFromBBox.set(x, y, 1)

            if display:
                x0 -= 0.5
                y0 -= 0.5
                x1 += 0.5
                y1 += 0.5

                ds9.line([(x0, y0), (x1, y0), (x1, y1), (x0, y1), (x0, y0)], ctype=ds9.RED)

        idImageFromBBox -= idImage      # should be blank
        stats = afwMath.makeStatistics(idImageFromBBox, afwMath.MAX)

        self.assertEqual(stats.getValue(), 0)

    def testWriteDefect(self):
        """Write a Footprint as a set of Defects"""
        region = afwGeom.Box2I(afwGeom.Point2I(0,0), afwGeom.Extent2I(12,10))
        foot = afwDetect.Footprint(0, region)
        for y, x0, x1 in [(3, 3, 5), (3, 7, 7),
                          (4, 2, 3), (4, 5, 7),
                          (5, 2, 3), (5, 5, 8),
                          (6, 3, 5), 
                          ]:
            foot.addSpan(y, x0, x1)
        
        if True:
            fd = open("/dev/null", "w")
        else:
            fd = sys.stdout
            
        afwDetectUtils.writeFootprintAsDefects(fd, foot)


    def testNormalize(self):
        """Test Footprint.normalize"""


        w, h = 12, 10
        region = afwGeom.Box2I(afwGeom.Point2I(0,0), afwGeom.Extent2I(w,h))
        im = afwImage.ImageU(afwGeom.Extent2I(w, h))
        im.set(0)
        #
        # Create a footprint;  note that these Spans overlap
        #
        for spans, box in (([(3, 5, 6),
                             (4, 7, 7), ], afwGeom.Box2I(afwGeom.Point2I(5,3), afwGeom.Point2I(7,4))),
                           ([(3, 3, 5), (3, 6, 9),
                             (4, 2, 3), (4, 5, 7), (4, 8, 8),
                             (5, 2, 3), (5, 5, 8), (5, 6, 7),
                             (6, 3, 5), 
                             ], afwGeom.Box2I(afwGeom.Point2I(2,3), afwGeom.Point2I(9,6)))
                      ):

            foot = afwDetect.Footprint(0, region)
            for y, x0, x1 in spans:
                foot.addSpan(y, x0, x1)

                for x in range(x0, x1 + 1): # also insert into im
                    im.set(x, y, 1)

            idImage = afwImage.ImageU(afwGeom.Extent2I(w, h))
            idImage.set(0)

            foot.insertIntoImage(idImage, 1)
            if display:             # overlaping pixels will be > 1
                ds9.mtv(idImage)
            #
            # Normalise the Footprint, removing overlapping spans
            #
            foot.normalize()

            idImage.set(0)
            foot.insertIntoImage(idImage, 1)
            if display:
                ds9.mtv(idImage, frame=1)

            idImage -= im

            self.assertTrue(box == foot.getBBox())
            self.assertEqual(afwMath.makeStatistics(idImage, afwMath.MAX).getValue(), 0)

    def testSetFromFootprint(self):
        """Test setting mask/image pixels from a Footprint list"""
        
        mi = afwImage.MaskedImageF(afwGeom.Extent2I(12, 8))
        im = mi.getImage()
        #
        # Objects that we should detect
        #
        self.objects = []
        self.objects += [Object(10, [(1, 4, 4), (2, 3, 5), (3, 4, 4)])]
        self.objects += [Object(20, [(5, 7, 8), (5, 10, 10), (6, 8, 9)])]
        self.objects += [Object(20, [(6, 3, 3)])]

        im.set(0)                       # clear image
        for obj in self.objects:
            obj.insert(im)

        if False and display:
            ds9.mtv(mi, frame=0)

        ds = afwDetect.makeFootprintSet(mi, afwDetect.Threshold(15))

        objects = ds.getFootprints()
        afwDetect.setMaskFromFootprintList(mi.getMask(), objects, 0x1)

        self.assertEqual(mi.getMask().get(4, 2), 0x0)
        self.assertEqual(mi.getMask().get(3, 6), 0x1)
        
        self.assertEqual(mi.getImage().get(3, 6), 20)
        afwDetect.setImageFromFootprintList(mi.getImage(), objects, 5.0)
        self.assertEqual(mi.getImage().get(4, 2), 10)
        self.assertEqual(mi.getImage().get(3, 6), 5)
        
        if False and display:
            ds9.mtv(mi, frame=1)
        #
        # Check Footprint.contains() while we are about it
        #
        self.assertTrue(objects[0].contains(afwGeom.Point2I(7, 5)))
        self.assertFalse(objects[0].contains(afwGeom.Point2I(10, 6)))
        self.assertFalse(objects[0].contains(afwGeom.Point2I(7, 6)))
        self.assertFalse(objects[0].contains(afwGeom.Point2I(4, 2)))

        self.assertTrue(objects[1].contains(afwGeom.Point2I(3, 6)))

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class FootprintSetTestCase(unittest.TestCase):
    """A test case for FootprintSet"""

    def setUp(self):
        self.ms = afwImage.MaskedImageF(afwGeom.Extent2I(12, 8))
        im = self.ms.getImage()
        #
        # Objects that we should detect
        #
        self.objects = []
        self.objects += [Object(10, [(1, 4, 4), (2, 3, 5), (3, 4, 4)])]
        self.objects += [Object(20, [(5, 7, 8), (5, 10, 10), (6, 8, 9)])]
        self.objects += [Object(20, [(6, 3, 3)])]

        self.ms.set((0, 0x0, 4.0))      # clear image; set variance
        for obj in self.objects:
            obj.insert(im)

        if False and display:
            ds9.mtv(im, frame=0)
        
    def tearDown(self):
        del self.ms

    def testGC(self):
        """Check that FootprintSets are automatically garbage collected (when MemoryTestCase runs)"""
        
        ds = afwDetect.makeFootprintSet(afwImage.MaskedImageF(afwGeom.Extent2I(10, 20)), afwDetect.Threshold(10))

    def testFootprints(self):
        """Check that we found the correct number of objects and that they are correct"""
        ds = afwDetect.makeFootprintSet(self.ms, afwDetect.Threshold(10))

        objects = ds.getFootprints()

        self.assertEqual(len(objects), len(self.objects))
        for i in range(len(objects)):
            self.assertEqual(objects[i], self.objects[i])
            
    def testFootprints2(self):
        """Check that we found the correct number of objects using makeFootprintSet"""
        ds = afwDetect.makeFootprintSet(self.ms, afwDetect.Threshold(10))

        objects = ds.getFootprints()

        self.assertEqual(len(objects), len(self.objects))
        for i in range(len(objects)):
            self.assertEqual(objects[i], self.objects[i])
            
    def testFootprints3(self):
        """Check that we found the correct number of objects using makeFootprintSet and PIXEL_STDEV"""
        threshold = 4.5                 # in units of sigma

        self.ms.set(2, 4, (10, 0x0, 36)) # not detected (high variance)

        y, x = self.objects[2].spans[0][0:2]
        self.ms.set(x, y, (threshold, 0x0, 1.0))

        ds = afwDetect.makeFootprintSet(self.ms,
                                        afwDetect.createThreshold(threshold, "pixel_stdev"), "OBJECT")

        objects = ds.getFootprints()

        self.assertEqual(len(objects), len(self.objects))
        for i in range(len(objects)):
            self.assertEqual(objects[i], self.objects[i])
            
    def testFootprintsMasks(self):
        """Check that detectionSets have the proper mask bits set"""
        ds = afwDetect.makeFootprintSet(self.ms, afwDetect.Threshold(10), "OBJECT")
        objects = ds.getFootprints()

        if display:
            ds9.mtv(self.ms, frame=1)

        mask = self.ms.getMask()
        for i in range(len(objects)):
            for sp in objects[i].getSpans():
                for x in range(sp.getX0(), sp.getX1() + 1):
                    self.assertEqual(mask.get(x, sp.getY()), mask.getPlaneBitMask("OBJECT"))

    def testFootprintsImageId(self):
        """Check that we can insert footprints into an Image"""
        ds = afwDetect.makeFootprintSet(self.ms, afwDetect.Threshold(10))
        objects = ds.getFootprints()

        idImage = afwImage.ImageU(self.ms.getDimensions())
        idImage.set(0)
        
        for foot in objects:
            foot.insertIntoImage(idImage, foot.getId())

        if False:
            ds9.mtv(idImage, frame=2)

        for i in range(len(objects)):
            for sp in objects[i].getSpans():
                for x in range(sp.getX0(), sp.getX1() + 1):
                    self.assertEqual(idImage.get(x, sp.getY()), objects[i].getId())


    def testFootprintSetImageId(self):
        """Check that we can insert a FootprintSet into an Image, setting relative IDs"""
        ds = afwDetect.makeFootprintSet(self.ms, afwDetect.Threshold(10))
        objects = ds.getFootprints()

        idImage = ds.insertIntoImage(True)
        if display:
            ds9.mtv(idImage, frame=2)

        for i in range(len(objects)):
            for sp in objects[i].getSpans():
                for x in range(sp.getX0(), sp.getX1() + 1):
                    self.assertEqual(idImage.get(x, sp.getY()), i + 1)

    def testFootprintsImage(self):
        """Check that we can search Images as well as MaskedImages"""
        ds = afwDetect.makeFootprintSet(self.ms.getImage(), afwDetect.Threshold(10))

        objects = ds.getFootprints()

        self.assertEqual(len(objects), len(self.objects))
        for i in range(len(objects)):
            self.assertEqual(objects[i], self.objects[i])
            
    def testGrow2(self):
        """Grow some more interesting shaped Footprints.  Informative with display, but no numerical tests"""
        
        ds = afwDetect.makeFootprintSet(self.ms, afwDetect.Threshold(10), "OBJECT")

        idImage = afwImage.ImageU(self.ms.getDimensions())
        idImage.set(0)

        i = 1
        for foot in ds.getFootprints()[0:1]:
            gfoot = afwDetect.growFootprint(foot, 3, False)
            gfoot.insertIntoImage(idImage, i)
            i += 1

        if display:
            ds9.mtv(self.ms, frame=0)
            ds9.mtv(idImage, frame=1)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class MaskFootprintSetTestCase(unittest.TestCase):
    """A test case for generating FootprintSet from Masks"""

    def setUp(self):
        self.mim = afwImage.MaskedImageF(afwGeom.ExtentI(12, 8))
        #
        # Objects that we should detect
        #
        self.objects = []
        self.objects += [Object(0x2, [(1, 4, 4), (2, 3, 5), (3, 4, 4)])]
        self.objects += [Object(0x41, [(5, 7, 8), (6, 8, 8)])]
        self.objects += [Object(0x42, [(5, 10, 10)])]
        self.objects += [Object(0x82, [(6, 3, 3)])]

        self.mim.set((0, 0, 0))                 # clear image
        for obj in self.objects:
            obj.insert(self.mim.getImage())
            obj.insert(self.mim.getMask())

        if display:
            ds9.mtv(self.mim, frame=0)
        
    def tearDown(self):
        del self.mim

    def testFootprints(self):
        """Check that we found the correct number of objects using makeFootprintSet"""
        level = 0x2
        ds = afwDetect.makeFootprintSet(self.mim.getMask(), afwDetect.createThreshold(level, "bitmask"))

        objects = ds.getFootprints()

        if 0 and display:
            ds9.mtv(self.mim, frame=0)

        self.assertEqual(len(objects), len([o for o in self.objects if (o.val & level)]))

        i = 0
        for o in self.objects:
            if o.val & level:
                self.assertEqual(o, objects[i])
                i += 1
            

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

class NaNFootprintSetTestCase(unittest.TestCase):
    """A test case for FootprintSet when the image contains NaNs"""

    def setUp(self):
        self.ms = afwImage.MaskedImageF(afwGeom.Extent2I(12, 8))
        im = self.ms.getImage()
        #
        # Objects that we should detect
        #
        self.objects = []
        self.objects += [Object(10, [(1, 4, 4), (2, 3, 5), (3, 4, 4)])]
        self.objects += [Object(20, [(5, 7, 8), (6, 8, 8)])]
        self.objects += [Object(20, [(5, 10, 10)])]
        self.objects += [Object(30, [(6, 3, 3)])]

        im.set(0)                       # clear image
        for obj in self.objects:
            obj.insert(im)

        self.NaN = float("NaN")
        im.set(3, 7, self.NaN)
        im.set(0, 0, self.NaN)
        im.set(8, 2, self.NaN)

        im.set(9, 6, self.NaN)          # connects the two objects with value==20 together if NaN is detected

        if False and display:
            ds9.mtv(im, frame=0)
        
    def tearDown(self):
        del self.ms

    def testFootprints(self):
        """Check that we found the correct number of objects using makeFootprintSet"""
        ds = afwDetect.makeFootprintSet(self.ms, afwDetect.Threshold(10), "DETECTED")

        objects = ds.getFootprints()

        if display:
            ds9.mtv(self.ms, frame=0)

        self.assertEqual(len(objects), len(self.objects))
        for i in range(len(objects)):
            self.assertEqual(objects[i], self.objects[i])
            
    def testGrowEmpty(self):
        """Check that growing an empty Footprint doesn't fail an assertion; #1186"""
        fp = afwDetect.Footprint()
        fp.setRegion(afwGeom.Box2I(afwGeom.Point2I(0, 0), afwGeom.Extent2I(10, 10)))
        afwDetect.growFootprint(fp, 5)

#-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

def suite():
    """Returns a suite containing all the test cases in this module."""
    tests.init()

    suites = []
    suites += unittest.makeSuite(ThresholdTestCase)
    suites += unittest.makeSuite(FootprintTestCase)
    suites += unittest.makeSuite(FootprintSetTestCase)
    suites += unittest.makeSuite(NaNFootprintSetTestCase)
    suites += unittest.makeSuite(MaskFootprintSetTestCase)
    suites += unittest.makeSuite(tests.MemoryTestCase)
    return unittest.TestSuite(suites)


def run(shouldExit=False):
    """Run the tests"""
    tests.run(suite(), shouldExit)

if __name__ == "__main__":
    run(True)
