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
 
%define mathLib_DOCSTRING
"
Python interface to lsst::afw::math classes
"
%enddef

%feature("autodoc", "1");
%module(package="lsst.afw.math",docstring=mathLib_DOCSTRING) mathLib

%{
#   include "lsst/daf/base.h"
#   include "lsst/pex/policy.h"
#   include "lsst/afw/image.h"
#   include "lsst/afw/geom.h"
#   include "lsst/afw/math.h"
%}


%include "lsst/p_lsstSwig.i"



%pythoncode %{
import lsst.utils

def version(HeadURL = r"$HeadURL: svn+ssh://svn.lsstcorp.org/DMS/afw/trunk/python/lsst/afw/math/mathLib.i $"):
    """Return a version given a HeadURL string. If a different version is setup, return that too"""

    version_svn = lsst.utils.guessSvnVersion(HeadURL)

    try:
        import eups
    except ImportError:
        return version_svn
    else:
        try:
            version_eups = eups.setup("afw")
        except AttributeError:
            return version_svn

    if version_eups == version_svn:
        return version_svn
    else:
        return "%s (setup: %s)" % (version_svn, version_eups)

%}

// vectors of plain old types; template vectors of more complex types in objectVectors.i
%template(vectorF) std::vector<float>;
%template(vectorD) std::vector<double>;
%template(vectorI) std::vector<int>;
%template(vectorVectorF) std::vector<std::vector<float> >;
%template(vectorVectorD) std::vector<std::vector<double> >;
%template(vectorVectorI) std::vector<std::vector<int> >;

%import "lsst/afw/image/imageLib.i"

%lsst_exceptions();

%include "function.i"
%include "kernel.i"
%include "minimize.i"
%include "statistics.i"
%include "interpolate.i"
%include "background.i"
%include "warpExposure.i"
%include "spatialCell.i"
%include "random.i"
%include "stack.i"
%include "objectVectors.i" // must come last

%inline %{
    struct InitGsl {
        InitGsl() {
            static int first = true;

            if (first) {
                (void)gsl_set_error_handler_off();
            }
        }
    };

    InitGsl _initGsl;                   // created at import time, to initialise the GSL library
%}
