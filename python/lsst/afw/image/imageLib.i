// -*- lsst-c++ -*-
%define imageLib_DOCSTRING
"
Basic routines to talk to lsst::afw::image classes
"
%enddef

%feature("autodoc", "1");
%module(docstring=imageLib_DOCSTRING) imageLib

// Suppress swig complaints
// I had trouble getting %warnfilter to work; hence the pragmas
#pragma SWIG nowarn=314                 // print is a python keyword (--> _print)
 //#pragma SWIG nowarn=317                 // specialization of non-template
#pragma SWIG nowarn=362                 // operator=  ignored
 //#pragma SWIG nowarn=389                 // operator[] ignored
 //#pragma SWIG nowarn=503                 // Can't wrap 'operator unspecified_bool_type'

%{
#   include <fstream>
#   include <exception>
#   include <map>
#   include <boost/cstdint.hpp>
#   include <boost/static_assert.hpp>
#   include <boost/shared_ptr.hpp>
#   include <boost/any.hpp>
#   include <boost/array.hpp>
#   include <lsst/utils/Utils.h>
#   include <lsst/daf/base.h>
#   include <lsst/daf/data.h>
#   include <lsst/daf/persistence.h>
#   include <lsst/pex/exceptions.h>
#   include <lsst/pex/logging/Trace.h>
#   include <lsst/pex/policy/Policy.h>
#   include <lsst/afw/image.h>
%}

%inline %{
namespace lsst { namespace afw { namespace image { } } }
namespace lsst { namespace daf { namespace data { } } }
namespace boost {
    typedef unsigned short uint16_t;
    namespace filesystem {}
    class bad_any_cast;                 // for lsst/pex/policy/Policy.h
    namespace filesystem {}
    namespace mpl {}
}
    
using namespace lsst;
using namespace lsst::afw::image;
using namespace lsst::daf::data;
%}

%init %{
%}

%include "lsst/p_lsstSwig.i"
%include "lsstImageTypes.i"     // Image/Mask types and typedefs

%pythoncode %{
import lsst.daf.data
import lsst.utils

def version(HeadURL = r"$HeadURL$"):
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

/******************************************************************************/

%include "lsst/daf/base/Citizen.h"
%import "lsst/daf/base/Persistable.h"
%import "lsst/daf/base/DataProperty.h"
%include "lsst/daf/data/LsstData.h"
%include "lsst/daf/data/LsstImpl_DC3.h"
%include "lsst/daf/data/LsstBase.h"
%import "lsst/daf/data.h"
%import "lsst/daf/persistence/Persistence.h"
%import "lsst/pex/exceptions.h"
%import "lsst/pex/logging/Trace.h"
%import "lsst/pex/policy/Policy.h"

/******************************************************************************/
%template(pairIntInt)    std::pair<int,int>;
%template(mapStringInt) std::map<std::string, int>;
//%template(pairIntString) std::pair<int,std::string>;
//%template(mapIntString)  std::map<std::string, int>;

/************************************************************************************************************/
// Masks and MaskedImages
// Images, Masks, and MaskedImages
%ignore lsst::afw::image::Filter::operator int;
%include "lsst/afw/image/Filter.h"

%include "image.i"
%include "mask.i"
%include "maskedImage.i"

%template(PointD) lsst::afw::image::Point<double>;
%template(PointI) lsst::afw::image::Point<int>;

%apply double &OUTPUT { double & };
%rename(positionToIndexAndResidual) lsst::afw::image::positionToIndex(double &, double);
%clear double &OUTPUT;

%include "lsst/afw/image/ImageUtils.h"

/************************************************************************************************************/

%{
#include "lsst/afw/image/Exposure.h"
%}

%include "lsst/afw/image/Exposure.h"

%template(ExposureU)    lsst::afw::image::Exposure<boost::uint16_t, lsst::afw::image::maskPixelType>;
%template(ExposureI)    lsst::afw::image::Exposure<int, lsst::afw::image::maskPixelType>;
%template(ExposureF)    lsst::afw::image::Exposure<float, lsst::afw::image::maskPixelType>;
%template(ExposureD)    lsst::afw::image::Exposure<double, lsst::afw::image::maskPixelType>;

/************************************************************************************************************/

%{
    #include "lsst/afw/image/Wcs.h"
%}

%rename(isValid) operator bool;

%include "lsst/afw/image/Wcs.h"

/******************************************************************************/
// Local Variables: ***
// eval: (setq indent-tabs-mode nil) ***
// End: ***
