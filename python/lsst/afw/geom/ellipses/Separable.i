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
#include "lsst/afw/geom/ellipses/radii.h"
#include "lsst/afw/geom/ellipses/ConformalShear.h"
#include "lsst/afw/geom/ellipses/ReducedShear.h"
#include "lsst/afw/geom/ellipses/Distortion.h"
#include "lsst/afw/geom/ellipses/Separable.h"
%}

%include "std_complex.i"

%declareNumPyConverters(lsst::afw::geom::ellipses::EllipticityBase::Jacobian);

%rename(assign) lsst::afw::geom::ellipses::DeterminantRadius::operator=;
%rename(assign) lsst::afw::geom::ellipses::TraceRadius::operator=;
%rename(assign) lsst::afw::geom::ellipses::LogDeterminantRadius::operator=;
%rename(assign) lsst::afw::geom::ellipses::LogTraceRadius::operator=;

%rename(assign) lsst::afw::geom::ellipses::Distortion::operator=;
%rename(assign) lsst::afw::geom::ellipses::ConformalShear::operator=;
%rename(assign) lsst::afw::geom::ellipses::ReducedShear::operator=;
%ignore lsst::afw::geom::ellipses::detail::EllipticityBase::getComplex;

%define %Radius_POSTINCLUDE(RADIUS)
%extend lsst::afw::geom::ellipses::RADIUS {
    double __float__() const {
        return static_cast<double const &>(*self);
    }
    double getValue() const {
        return static_cast<double const &>(*self);
    }
    void setValue(double value) {
        static_cast<double &>(*self) = value;
    }
    %pythoncode {
    def __str__(self):
        return float(self)
    def __repr__(self):
        return "%s(%d)" % (self.getName(), float(self))
    }
}
%enddef

%define %Ellipticity_POSTINCLUDE(ELLIPTICITY)
%extend lsst::afw::geom::ellipses::ELLIPTICITY {
    std::complex<double> _getComplex() const {
        return self->getComplex();
    }
    void setComplex(std::complex<double> other) {
        self->getComplex() = other;
    }
    %pythoncode {
    def getComplex(self):
        return self._getComplex();
    def __str__(self):
        return "(%d, %d)" % (self.getE1(), self.getE2())
    def __repr__(self):
        return "%s(%d, %d)" % (self.getName(), self.getE1(), self.getE2())
    }
}
%enddef

%include "lsst/afw/geom/ellipses/radii.h"
%include "lsst/afw/geom/ellipses/EllipticityBase.h"
%include "lsst/afw/geom/ellipses/Distortion.h"
%include "lsst/afw/geom/ellipses/ConformalShear.h"
%include "lsst/afw/geom/ellipses/ReducedShear.h"

%Ellipticity_POSTINCLUDE(Distortion);
%Ellipticity_POSTINCLUDE(ConformalShear);
%Ellipticity_POSTINCLUDE(ReducedShear);

%Radius_POSTINCLUDE(TraceRadius);
%Radius_POSTINCLUDE(DeterminantRadius);
%Radius_POSTINCLUDE(LogTraceRadius);
%Radius_POSTINCLUDE(LogDeterminantRadius);

%ignore lsst::afw::geom::ellipses::Separable::writeParameters;
%ignore lsst::afw::geom::ellipses::Separable::readParameters;
%rename(assign) lsst::afw::geom::ellipses::Separable::operator=;

%define %Separable_PREINCLUDE(ELLIPTICITY, RADIUS)
SWIG_SHARED_PTR_DERIVED(
    Separable ## ELLIPTICITY ## RADIUS ## Ptr,
    lsst::afw::geom::ellipses::BaseCore,
    lsst::afw::geom::ellipses::Separable<
        lsst::afw::geom::ellipses::ELLIPTICITY, 
        lsst::afw::geom::ellipses::RADIUS
    > 
);
%rename(assign) lsst::afw::geom::ellipses::Separable<
        lsst::afw::geom::ellipses::ELLIPTICITY,
        lsst::afw::geom::ellipses::RADIUS
    >::operator=;
%enddef


%define %Separable_POSTINCLUDE(ELLIPTICITY, RADIUS)
%template(Separable ## ELLIPTICITY ## RADIUS) 
    lsst::afw::geom::ellipses::Separable<
        lsst::afw::geom::ellipses::ELLIPTICITY, 
        lsst::afw::geom::ellipses::RADIUS
    >;
%enddef


%Separable_PREINCLUDE(Distortion, DeterminantRadius);
%Separable_PREINCLUDE(Distortion, TraceRadius);
%Separable_PREINCLUDE(Distortion, LogDeterminantRadius);
%Separable_PREINCLUDE(Distortion, LogTraceRadius);

%Separable_PREINCLUDE(ConformalShear, DeterminantRadius);
%Separable_PREINCLUDE(ConformalShear, TraceRadius);
%Separable_PREINCLUDE(ConformalShear, LogDeterminantRadius);
%Separable_PREINCLUDE(ConformalShear, LogTraceRadius);

%Separable_PREINCLUDE(ReducedShear, DeterminantRadius);
%Separable_PREINCLUDE(ReducedShear, TraceRadius);
%Separable_PREINCLUDE(ReducedShear, LogDeterminantRadius);
%Separable_PREINCLUDE(ReducedShear, LogTraceRadius);

%include "lsst/afw/geom/ellipses/Separable.h"

%extend lsst::afw::geom::ellipses::Separable {
    lsst::afw::geom::ellipses::Separable<Ellipticity_, Radius_>::Ptr _transform(
            lsst::afw::geom::LinearTransform const & t
    ) {
        return boost::static_pointer_cast<lsst::afw::geom::ellipses::Separable<Ellipticity_, Radius_> >(
            self->transform(t).copy()
        );
    }
    void _transformInPlace(lsst::afw::geom::LinearTransform const & t) {
       self->transform(t).inPlace();
    }
    lsst::afw::geom::ellipses::Separable<Ellipticity_, Radius_>::Ptr _convolve(
            lsst::afw::geom::ellipses::BaseCore const & other
    ) {
        return boost::static_pointer_cast<lsst::afw::geom::ellipses::Separable<Ellipticity_, Radius_> >(
            self->convolve(other).copy()
        );
    }
    static lsst::afw::geom::ellipses::Separable<Ellipticity_, Radius_>::Ptr cast(
        lsst::afw::geom::ellipses::BaseCore::Ptr const & p
    ) {
        return boost::dynamic_pointer_cast<lsst::afw::geom::ellipses::Separable<Ellipticity_, Radius_> >(p);
    }
    %pythoncode {
    def transform(self, t): return self._transform(t)
    def transformInPlace(self, t): self._transformInPlace(t)
    def convolve(self, t): return self._convolve(t)
    def __repr__(self):
        return "Separable(%r, %r)" % (self.getEllipticity(), self.getRadius())
    def __str__(self):
        return "(%s, %s)" % (self.getEllipticity(), self.getRadius())
    }
}

%Separable_POSTINCLUDE(Distortion, DeterminantRadius);
%Separable_POSTINCLUDE(Distortion, TraceRadius);
%Separable_POSTINCLUDE(Distortion, LogDeterminantRadius);
%Separable_POSTINCLUDE(Distortion, LogTraceRadius);

%Separable_POSTINCLUDE(ConformalShear, DeterminantRadius);
%Separable_POSTINCLUDE(ConformalShear, TraceRadius);
%Separable_POSTINCLUDE(ConformalShear, LogDeterminantRadius);
%Separable_POSTINCLUDE(ConformalShear, LogTraceRadius);

%Separable_POSTINCLUDE(ReducedShear, DeterminantRadius);
%Separable_POSTINCLUDE(ReducedShear, TraceRadius);
%Separable_POSTINCLUDE(ReducedShear, LogDeterminantRadius);
%Separable_POSTINCLUDE(ReducedShear, LogTraceRadius);
