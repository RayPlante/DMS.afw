// -*- lsst-++ -*-

/* 
 * LSST Data Management System
 * Copyright 2008, 2009, 2010 LSST Corporation.
 * 
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the LSST License Statement and 
 * the GNU General Public License along with this program.  If not, 
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */
 
#define CAMERA_GEOM_LIB_I

%define cameraGeomLib_DOCSTRING
"
Python bindings for classes describing the the geometry of a mosaic camera
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.afw.cameraGeom", docstring=cameraGeomLib_DOCSTRING) cameraGeomLib

%{
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/Mask.h"
#include "lsst/afw/cameraGeom.h"
%}

%include "lsst/p_lsstSwig.i"
%include  "lsst/afw/utils.i" 
#if defined(IMPORT_IMAGE_I)
%import  "lsst/afw/image/imageLib.i" 
#endif

%lsst_exceptions();

SWIG_SHARED_PTR(DetectorPtr, lsst::afw::cameraGeom::Detector);

SWIG_SHARED_PTR_DERIVED(AmpPtr, lsst::afw::cameraGeom::Detector, lsst::afw::cameraGeom::Amp);
SWIG_SHARED_PTR(ElectronicParamsPtr, lsst::afw::cameraGeom::ElectronicParams);
SWIG_SHARED_PTR_DERIVED(DetectorMosaicPtr, lsst::afw::cameraGeom::Detector,
                        lsst::afw::cameraGeom::DetectorMosaic);
SWIG_SHARED_PTR_DERIVED(CcdPtr, lsst::afw::cameraGeom::Detector, lsst::afw::cameraGeom::Ccd);
SWIG_SHARED_PTR_DERIVED(RaftPtr, lsst::afw::cameraGeom::Detector, lsst::afw::cameraGeom::Raft);
SWIG_SHARED_PTR_DERIVED(CameraPtr, lsst::afw::cameraGeom::Detector, lsst::afw::cameraGeom::Camera);

%template(AmpSet) std::vector<boost::shared_ptr<lsst::afw::cameraGeom::Amp> >;
%template(DetectorSet) std::vector<boost::shared_ptr<lsst::afw::cameraGeom::Detector> >;

%include "lsst/afw/cameraGeom/Id.h"

%extend lsst::afw::cameraGeom::Id {
    %pythoncode {
        def __str__(self):
            return "%d, %s" % (self.getSerial(), self.getName())

        def __repr__(self):
            return "Id(%s)" % (str(self))
    }
}

%include "lsst/afw/cameraGeom/Orientation.h"
%include "lsst/afw/cameraGeom/Detector.h"
%include "lsst/afw/cameraGeom/Amp.h"
%include "lsst/afw/cameraGeom/DetectorMosaic.h"
%include "lsst/afw/cameraGeom/Ccd.h"
%include "lsst/afw/cameraGeom/Raft.h"
%include "lsst/afw/cameraGeom/Camera.h"

%inline %{
    lsst::afw::cameraGeom::Amp::Ptr
    cast_Amp(lsst::afw::cameraGeom::Detector::Ptr detector) {
        return boost::shared_dynamic_cast<lsst::afw::cameraGeom::Amp>(detector);
    }

    lsst::afw::cameraGeom::Ccd::Ptr
    cast_Ccd(lsst::afw::cameraGeom::Detector::Ptr detector) {
        return boost::shared_dynamic_cast<lsst::afw::cameraGeom::Ccd>(detector);
    }

    lsst::afw::cameraGeom::Raft::Ptr
    cast_Raft(lsst::afw::cameraGeom::Detector::Ptr detector) {
        return boost::shared_dynamic_cast<lsst::afw::cameraGeom::Raft>(detector);
    }
%}

%define Instantiate(PIXEL_TYPE...)
%template(prepareAmpData)
    lsst::afw::cameraGeom::Amp::prepareAmpData<lsst::afw::image::Image<PIXEL_TYPE> >;
%enddef

Instantiate(boost::uint16_t);
Instantiate(float);
Instantiate(double);
%template(prepareAmpData)
    lsst::afw::cameraGeom::Amp::prepareAmpData<lsst::afw::image::Mask<boost::uint16_t> >;

%definePythonIterator(lsst::afw::cameraGeom::Ccd);
%definePythonIterator(lsst::afw::cameraGeom::DetectorMosaic);

%pythoncode {
class ReadoutCorner(object):
    """A python object corresponding to Amp::ReadoutCorner"""
    def __init__(self, value):
        self.value = value

    def __repr__(self):
        return ["LLC", "LRC", "URC", "ULC"][self.value]
}
