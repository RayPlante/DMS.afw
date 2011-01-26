// -*- lsst-c++ -*-

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
 
%define testSpatialCellLib_DOCSTRING
"
Various swigged-up C++ classes for testing
"
%enddef

%feature("autodoc", "1");
%module(package="testSpatialCellLib", docstring=testSpatialCellLib_DOCSTRING) testSpatialCellLib

%pythonnondynamic;
%naturalvar;  // use const reference typemaps

%include "lsst/p_lsstSwig.i"

%lsst_exceptions()

%{
#include "lsst/pex/policy.h"
#include "lsst/afw/geom.h"
#include "lsst/afw/math.h"
#include "testSpatialCell.h"
%}

%import "lsst/afw/image/imageLib.i"
%import "lsst/afw/math/mathLib.i"

SWIG_SHARED_PTR_DERIVED(ExampleCandidate,
                        lsst::afw::math::SpatialCellImageCandidate<lsst::afw::image::Image<float> >,
                        ExampleCandidate);

%include "testSpatialCell.h"

%inline %{
    ExampleCandidate *cast_ExampleCandidate(lsst::afw::math::SpatialCellCandidate* candidate) {
        return dynamic_cast<ExampleCandidate *>(candidate);
    }
%}
