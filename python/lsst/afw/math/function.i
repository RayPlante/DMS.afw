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
#include <vector>

#include "boost/shared_ptr.hpp"

#include "lsst/afw/math/Function.h"
#include "lsst/afw/math/FunctionLibrary.h"
%}

// Must be used before %include
%define %baseFunctionPtrs(TYPE, CTYPE)
SWIG_SHARED_PTR_DERIVED(Function##TYPE, lsst::daf::base::Citizen, lsst::afw::math::Function<CTYPE>);
SWIG_SHARED_PTR_DERIVED(Function1##TYPE, lsst::afw::math::Function<CTYPE>, lsst::afw::math::Function1<CTYPE>);
SWIG_SHARED_PTR_DERIVED(Function2##TYPE, lsst::afw::math::Function<CTYPE>, lsst::afw::math::Function2<CTYPE>);
SWIG_SHARED_PTR_DERIVED(BasePolynomialFunction2##TYPE, lsst::afw::math::Function2<CTYPE>,
    lsst::afw::math::BasePolynomialFunction2<CTYPE>);
%enddef

%define %functionPtr(NAME, N, TYPE, CTYPE)
SWIG_SHARED_PTR_DERIVED(NAME##N##TYPE, lsst::afw::math::Function##N<CTYPE>, lsst::afw::math::NAME##N<CTYPE>);
%enddef

// Must be used after %include
%define %baseFunctions(TYPE, CTYPE)
%template(Function##TYPE) lsst::afw::math::Function<CTYPE>;
%template(Function1##TYPE) lsst::afw::math::Function1<CTYPE>;
%template(Function2##TYPE) lsst::afw::math::Function2<CTYPE>;
%template(BasePolynomialFunction2##TYPE) lsst::afw::math::BasePolynomialFunction2<CTYPE>;
%enddef

%define %function(NAME, N, TYPE, CTYPE)
%template(NAME##N##TYPE) lsst::afw::math::NAME##N<CTYPE>;
%enddef
//
// Macros to define float or double versions of things
//
%define %definePointers(NAME, TYPE)
    // Must be called BEFORE %include
    %baseFunctionPtrs(NAME, TYPE);

    %functionPtr(Chebyshev1Function, 1, NAME, TYPE);
    %functionPtr(Chebyshev1Function, 2, NAME, TYPE);
    %functionPtr(DoubleGaussianFunction, 2, NAME, TYPE);
    %functionPtr(GaussianFunction, 1, NAME, TYPE);
    %functionPtr(GaussianFunction, 2, NAME, TYPE);
    %functionPtr(IntegerDeltaFunction, 2, NAME, TYPE);
    %functionPtr(LanczosFunction, 1, NAME, TYPE);
    %functionPtr(LanczosFunction, 2, NAME, TYPE);
    %functionPtr(NullFunction, 1, NAME, TYPE);
    %functionPtr(NullFunction, 2, NAME, TYPE);
    %functionPtr(PolynomialFunction, 1, NAME, TYPE);
    %functionPtr(PolynomialFunction, 2, NAME, TYPE);
%enddef

%define %defineTemplates(NAME, TYPE)
    // Must be called AFTER %include
    %baseFunctions(NAME, TYPE);

    %function(Chebyshev1Function, 1, NAME, TYPE);
    %function(Chebyshev1Function, 2, NAME, TYPE);
    %function(DoubleGaussianFunction, 2, NAME, TYPE);
    %function(GaussianFunction, 1, NAME, TYPE);
    %function(GaussianFunction, 2, NAME, TYPE);
    %function(IntegerDeltaFunction, 2, NAME, TYPE);
    %function(LanczosFunction, 1, NAME, TYPE);
    %function(LanczosFunction, 2, NAME, TYPE);
    %function(NullFunction, 1, NAME, TYPE);
    %function(NullFunction, 2, NAME, TYPE);
    %function(PolynomialFunction, 1, NAME, TYPE);
    %function(PolynomialFunction, 2, NAME, TYPE);
%enddef

/************************************************************************************************************/

%definePointers(D, double);
%definePointers(F, float);

%include "lsst/afw/math/Function.h"
%include "lsst/afw/math/FunctionLibrary.h"

%defineTemplates(D, double)
%defineTemplates(F, float)
