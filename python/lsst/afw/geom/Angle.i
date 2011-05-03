// -*- lsst-c++ -*-
%{
    #include <iostream>
    #include <sstream>
    #include "lsst/afw/geom/Angle.h"

    using namespace lsst::afw::geom;
%}

%ignore operator<<(std::ostream &s, Angle const a);

%rename(__float__) lsst::afw::geom::Angle::operator double() const;
%rename(Angle_add) lsst::afw::geom::operator+;
%rename(Angle_sub) lsst::afw::geom::operator-;
%rename(Angle_mul) lsst::afw::geom::operator*;
%rename(Angle_div) lsst::afw::geom::operator/;

%inline %{
    lsst::afw::geom::Angle AngleUnit_mul(lsst::afw::geom::AngleUnit const& lhs, double rhs) {
        return rhs*lhs;
    }
    lsst::afw::geom::Angle AngleUnit_mul(double lhs, lsst::afw::geom::AngleUnit const& rhs) {
        return lhs*rhs;
    }
%}

%extend lsst::afw::geom::Angle {
    const char *__str__() {
        std::ostringstream ss;
        ss << *self;

        static char buff[41];
        return strncpy(buff, ss.str().c_str(), sizeof(buff) - 1);
    }
            
    %pythoncode %{
         def __add__(self, rhs):
             return Angle_add(self, rhs)
         def __radd__(self, lhs):
             return Angle_add(lhs, self)
         def __sub__(self, rhs):
             return Angle_sub(self, rhs)
         def __rsub__(self, lhs):
             return Angle_sub(lhs, self)
         def __mul__(self, rhs):
             return Angle_mul(self, rhs)
         def __rmul__(self, lhs):
             return Angle_mul(lhs, self)
         def __div__(self, rhs):
             return Angle_div(self, rhs)
         def __rdiv__(self, lhs):
             return Angle_div(lhs, self)
         def __eq__(self, rhs):
             return float(self) == float(rhs)
    %}
}

%extend lsst::afw::geom::AngleUnit {
    %pythoncode %{
         def __mul__(self, rhs):
             return AngleUnit_mul(self, rhs)
         def __rmul__(self, lhs):
             return AngleUnit_mul(lhs, self)
    %}
}

%include "lsst/afw/geom/Angle.h"

%template(isAngle) lsst::afw::geom::isAngle<double>;
