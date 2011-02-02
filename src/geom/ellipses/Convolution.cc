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
#include "lsst/afw/geom/ellipses/Quadrupole.h"
#include "lsst/afw/geom/ellipses/Convolution.h"

namespace lsst { namespace afw { namespace geom { namespace ellipses {

BaseCore::Ptr BaseCore::Convolution::copy() const {
    BaseCore::Ptr r(_self.clone());
    apply(*r);
    return r;
}

void BaseCore::Convolution::inPlace() {
    apply(_self);
}

BaseCore::Convolution::DerivativeMatrix
BaseCore::Convolution::d() const {
    double ixx1, iyy1, ixy1;
    double ixx2, iyy2, ixy2;
    Jacobian rhs = _self._dAssignToQuadrupole(ixx1, iyy1, ixy1);
    _other._assignToQuadrupole(ixx2, iyy2, ixy2);
    BaseCore::Ptr convolved(_self.clone());
    Jacobian lhs = convolved->_dAssignFromQuadrupole(ixx1 + ixx2, iyy1 + iyy2, ixy1 + ixy2);
    return lhs * rhs;
}

void BaseCore::Convolution::apply(BaseCore & result) const {
    double ixx1, iyy1, ixy1;
    double ixx2, iyy2, ixy2;
    _self._assignToQuadrupole(ixx1, iyy1, ixy1);
    _other._assignToQuadrupole(ixx2, iyy2, ixy2);
    result._assignFromQuadrupole(ixx1 + ixx2, iyy1 + iyy2, ixy1 + ixy2);
}

}}}} // namespace lsst::afw::geom::ellipses
