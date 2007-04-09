// -*- lsst-c++ -*-
%define fwLib_DOCSTRING
"
Basic routines to talk to FW's classes (including visionWorkbench) and ds9
"
%enddef

%feature("autodoc", "1");
%module(docstring=fwLib_DOCSTRING, naturalvar=1) fwLib

%{
#   include <boost/cstdint.hpp>
#   include "lsst/fw/Citizen.h"
#   include "lsst/fw/DiskImageResourceFITS.h"
#   include "simpleFits.h"
#   include "lsst/fw/Mask.h"
#   include "lsst/fw/MaskedImage.h"
#   include "lsst/fw/Trace.h"

using namespace lsst;
using namespace lsst::fw;
using namespace vw;
%}

// Suppress swig complaints from vw
// 317: Specialization of non-template
// 389: operator[] ignored
// 362: operator=  ignored
// 503: Can't wrap 'operator unspecified_bool_type()'
// I had trouble getting %warnfilter to work; hence the pragmas
#pragma SWIG nowarn=317
#pragma SWIG nowarn=362
#pragma SWIG nowarn=389
%warnfilter(503) vw;

%include "../Core/p_lsstSwig.i"
%import "lsst/fw/Utils.h"

%import <vw/Image/ImageResource.h>
%import <vw/FileIO/DiskImageResource.h>

%import "lsst/fw/DiskImageResourceFITS.h"

/******************************************************************************/
// Talk to DS9

%include "simpleFits.h"

using namespace lsst::fw;

%import "lsst/fw/Mask.h"
%import "lsst/fw/Image.h"
%import "lsst/fw/MaskedImage.h"

%template(MaskD)          lsst::Mask<MaskPixelType>;
%template(MaskDPtr)       boost::shared_ptr<lsst::Mask<MaskPixelType> >;
%template(ImageD)         lsst::Image<ImagePixelType>;
%template(ImageDPtr)      boost::shared_ptr<lsst::Image<ImagePixelType> >;
%template(MaskedImageD)	  lsst::MaskedImage<ImagePixelType, MaskPixelType>;
%template(MaskedImageDPtr) boost::shared_ptr<lsst::MaskedImage<ImagePixelType, MaskPixelType> >;

%template(readMask) read<MaskPixelType>;
%template(writeFitsMask) writeFits<MaskPixelType>;
%template(writeFitsFileMask) writeFitsFile<MaskPixelType>;

%template(readImage) read<ImagePixelType>;
%template(writeFitsImage) writeFits<ImagePixelType>;
%template(writeFitsFileImage) writeFitsFile<ImagePixelType>;

/******************************************************************************/
// Local Variables: ***
// eval: (setq indent-tabs-mode nil) ***
// End: ***
