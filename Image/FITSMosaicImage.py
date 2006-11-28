#######################################################
#
# MosaicImage.py
# Python implementation of the Class MosaicImage
# Generated by Enterprise Architect
# Created on:      23-Aug-2005 2:36:50 PM
#
#######################################################
__all__ = ["FITSMosaicImage"]

from lsst.apps.fw.Policy import Policy
from Image import Image
from CCDImage import CCDImage
from CCDGeom import CCDGeom

class FITSMosaicImage(Image):

    #------------------------------------------------------
    def __init__(self, arg, mosaicPolicyFile=None, ccdPolicyFile=None, **kws):
        """
        __init__  MosiacImage initialization

        Input
            arg             Argument passed directly to Image initialization.  Most commonly this is a file name
                            Format:     ; Default: none
            mosaicPolicyFile      Filename for class Policy specification;
                            Format: string
                            Default:
                                if none provided, use "MosaicImage.conf"
            ccdPolicyFile   Filename for class Policy specification;
                            Format: string
                            Default:
                                if none provided, use "CCDImage.conf"
            kws             Keyword dictionary which is used, along with
                            Policy, to manage processing; Format: dictionary
        Return
            none

        Side Effect
            also initializes an Image instance
        """
        Image.__init__(self, arg)

        # Fetch the Mosaic Policy file
        if ( not mosaicPolicyFile ):
            mosaicConfFile = "MosaicImage.conf"
        else:
            mosaicConfFile = mosaicPolicyFile

        # Fetch the Mosaic's CCD Policy file
        if ( not ccdPolicyFile ):
            ccdConfFile = "CCDImage.conf"
        else:
            ccdConfFile = ccdPolicyFile

        self.policy = Policy(mosaicConfFile, kws)

        #
        # Get out ccdgeom info from policy
        #
        ccdGeomStrArr = self.policy.Get('ccdGeom')
        #
        # Build a (xOff, yOff, rot, flipx, flipy) array, converting strings to appropriate types
        #
        if (isinstance(ccdGeomStrArr, list) and len(ccdGeomStrArr) == self.NumCCDS()):
            self.ccdGeom = []
            for ccdGeom in ccdGeomStrArr:
                ccdGeomItems = ccdGeom.split(',')
                xOff = float(ccdGeomItems[0])
                yOff = float(ccdGeomItems[1])
                rot = float(ccdGeomItems[2])
                flipx = (int(ccdGeomItems[3])==1)
                flipy = (int(ccdGeomItems[4])==1)
                # self.geomList.append(CCDGeom(xOff, yOff, rot, flipx, flipy))
                self.ccdGeom.append((xOff, yOff, rot, flipx, flipy))
        else:
            print "Warning:  MosaicImage does not have valid ccdGeom"
            self.ccdGeom = None

        self.CCDs = []
        #
        # Build individual CCDImages, adding any geometry keywords
        #
        for i in range(self.NumCCDS()):
            if self.ccdGeom:
                initGeom = self.ccdGeom[i]
            else:
                initGeom = None

            ccdim = CCDImage(self.GetCCDHDU(i+1), ccdConfFile, geom = initGeom)
            self.CCDs.append(ccdim)

    #------------------------------------------------------
    def __del__(self):
        """
        __del__     system interface to deletes current instance

        Side Effect
            delete Image instance
        """
        Image.__del__(self)

    #------------------------------------------------------
    def CalculateSkyRegions(self, fuzzDegrees=None):
        """
        CalculateSkyRegions   - foreach CCD, calculate the SkyRegion
        including an extra fuzzDegrees border

        Input
            fuzzDegrees: width of border around mosaic to include when calculating SkyRegion
                         Units: degrees
        Return
            none
        """
        for self.i in range(self.NumCCDS()):
            self.CCDs[self.i].BuildSkyRegion(fuzzDegrees)
        pass

    #------------------------------------------------------
    def GetCCDImage(self, which):
        """
        GetCCDImage ....

        Input
            which       id of CCD image of interest

        Return
            CCDImage
        """
        if which in range(self.NumCCDS()) :
            return self.CCDs[which]
        raise LookupError, "Requested HDU is out of range of Image's HDUs"
