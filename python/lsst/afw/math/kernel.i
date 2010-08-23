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
 
%{
#include "lsst/afw/math/FourierCutout.h"
#include "lsst/afw/math/LocalKernel.h"
#include "lsst/afw/math/Kernel.h"
#include "lsst/afw/math/KernelFunctions.h"
%}

%include "std_complex.i"

%ignore lsst::afw::math::FourierCutout::operator();
%ignore lsst::afw::math::FourierCutout::swap;
%ignore lsst::afw::math::FourierCutout::begin;
%ignore lsst::afw::math::FourierCutout::end;
%ignore lsst::afw::math::FourierCutout::at;
%extend lsst::afw::math::FourierCutout {
    /**
     * Set an image to the value val
     */
    void set(Real val) {
        *self = val;
    }
    
    Complex get(int x, int y) {
        return self->operator()(x,y);    
    }
};

SWIG_SHARED_PTR(FourierCutoutPtr, lsst::afw::math::FourierCutout);
SWIG_SHARED_PTR(FourierCutoutStackPtr, lsst::afw::math::FourierCutoutStack);


%include "lsst/afw/math/FourierCutout.h"


// I doubt newobject is needed; the code seems to work just as well without it.
%newobject lsst::afw::math::convolve;
%newobject lsst::afw::math::Kernel::getKernelParameters;
%newobject lsst::afw::math::Kernel::getSpatialParameters;
//
// Kernel classes (every template of a class must have a unique name)
//
// These definitions must go Before you include Kernel.h; the %templates must go After
//
%define %kernelPtr(TYPE...)
SWIG_SHARED_PTR_DERIVED(TYPE, lsst::afw::math::Kernel, lsst::afw::math::TYPE);
%lsst_persistable(lsst::afw::math::TYPE)
%enddef

SWIG_SHARED_PTR_DERIVED(Kernel, lsst::daf::data::LsstBase, lsst::afw::math::Kernel); // the base class
%lsst_persistable(lsst::afw::math::Kernel)

%kernelPtr(AnalyticKernel);
%kernelPtr(DeltaFunctionKernel);
%kernelPtr(FixedKernel);
%kernelPtr(LinearCombinationKernel);
%kernelPtr(SeparableKernel);

%include "lsst/afw/math/Kernel.h"

%include "lsst/afw/math/KernelFunctions.h"

%include "lsst/afw/math/ConvolveImage.h"
//
// Functions to convolve a MaskedImage or Image with a Kernel.
// There are a lot of these, so write a set of macros to do the instantiations
//
// First a couple of macros (%IMAGE and %MASKEDIMAGE) to provide MaskedImage's default arguments,
%define %IMAGE(PIXTYPE)
lsst::afw::image::Image<PIXTYPE>
%enddef

%define %MASKEDIMAGE(PIXTYPE)
lsst::afw::image::MaskedImage<PIXTYPE, lsst::afw::image::MaskPixel, lsst::afw::image::VariancePixel>
%enddef

// Next a macro to generate needed instantiations for IMAGE (e.g. %MASKEDIMAGE) and the specified pixel types
//
// @todo put convolveWith... functions in lsst.afw.math.detail instead of lsst.afw.math
//
// Note that IMAGE is a macro, not a class name
%define %templateKernelByType(IMAGE, PIXTYPE1, PIXTYPE2)
    %template(convolve) lsst::afw::math::convolve<
        IMAGE(PIXTYPE1), IMAGE(PIXTYPE2), lsst::afw::math::Kernel>;
    %template(convolve) lsst::afw::math::convolve<
        IMAGE(PIXTYPE1), IMAGE(PIXTYPE2), lsst::afw::math::AnalyticKernel>;
    %template(convolve) lsst::afw::math::convolve<
        IMAGE(PIXTYPE1), IMAGE(PIXTYPE2), lsst::afw::math::DeltaFunctionKernel>;
    %template(convolve) lsst::afw::math::convolve<
        IMAGE(PIXTYPE1), IMAGE(PIXTYPE2), lsst::afw::math::FixedKernel>;
    %template(convolve) lsst::afw::math::convolve<
        IMAGE(PIXTYPE1), IMAGE(PIXTYPE2), lsst::afw::math::LinearCombinationKernel>;
    %template(convolve) lsst::afw::math::convolve<
        IMAGE(PIXTYPE1), IMAGE(PIXTYPE2), lsst::afw::math::SeparableKernel>;
    %template(scaledPlus) lsst::afw::math::scaledPlus<IMAGE(PIXTYPE1), IMAGE(PIXTYPE2)>;
%enddef
//
// Now a macro to specify Image and MaskedImage
//
%define %templateKernel(PIXTYPE1, PIXTYPE2)
    %templateKernelByType(%IMAGE,       PIXTYPE1, PIXTYPE2);
    %templateKernelByType(%MASKEDIMAGE, PIXTYPE1, PIXTYPE2);
%enddef
//
// Finally, specify the functions we want
//
%templateKernel(double, double);
%templateKernel(double, float);
%templateKernel(double, int);
%templateKernel(double, boost::uint16_t);
%templateKernel(float, float);
%templateKernel(float, int);
%templateKernel(float, boost::uint16_t);
%templateKernel(int, int);
%templateKernel(boost::uint16_t, boost::uint16_t);
         
//
// When swig sees a Kernel it doesn't know about KERNEL_TYPE; all it knows is that it
// has a Kernel, and Kernels don't know about e.g. LinearCombinationKernel's getKernelParameters()
//
// We therefore provide a cast to KERNEL_TYPE* and swig can go from there
//
%define %dynamic_cast(KERNEL_TYPE)
%inline %{
    lsst::afw::math::KERNEL_TYPE *
        cast_##KERNEL_TYPE(lsst::afw::math::Kernel *candidate) {
        return dynamic_cast<lsst::afw::math::KERNEL_TYPE *>(candidate);
    }
%}
%enddef

%dynamic_cast(AnalyticKernel);
%dynamic_cast(DeltaFunctionKernel);
%dynamic_cast(FixedKernel);
%dynamic_cast(LinearCombinationKernel);
%dynamic_cast(SeparableKernel);

SWIG_SHARED_PTR_DERIVED(
    ImageLocalKernel, 
    lsst::afw::math::LocalKernel, 
    lsst::afw::math::ImageLocalKernel
)

SWIG_SHARED_PTR_DERIVED(
    FourierLocalKernel, 
    lsst::afw::math::LocalKernel, 
    lsst::afw::math::FourierLocalKernel
)
SWIG_SHARED_PTR_DERIVED(
    FftLocalKernel,
    lsst::afw::math::FourierLocalKernel,
    lsst::afw::math::FftLocalKernel
)

%include "lsst/afw/math/LocalKernel.h"
