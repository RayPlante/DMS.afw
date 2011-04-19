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

#ifndef LSST_AFW_GEOM_ELLIPSES_GridTransform_h_INCLUDED
#define LSST_AFW_GEOM_ELLIPSES_GridTransform_h_INCLUDED

/**
 *  @file
 *  @brief Definitions for Ellipse::GridTransform and BaseCore::GridTransform.
 *
 *  @note Do not include directly; use the main ellipse header file.
 */

#include <boost/tuple/tuple.hpp>

#include "lsst/afw/geom/ellipses/Ellipse.h"
#include "lsst/afw/geom/AffineTransform.h"

namespace lsst { namespace afw { namespace geom { namespace ellipses {

/**
 *  @brief A temporary-only expression object representing a LinearTransform that
 *         maps the ellipse core to a unit circle.
 */
class BaseCore::GridTransform {
public:

    /// Matrix type for derivative with respect to ellipse parameters.
    typedef Eigen::Matrix<double,4,3> DerivativeMatrix;

    /// @brief Standard constructor.
    explicit GridTransform(BaseCore const & input) : _input(input) {}

    /// @brief Return the derivative of the transform with respect to input core.
    DerivativeMatrix d() const;
    
    /// @brief Convert the proxy to a LinearTransform.
    operator LinearTransform () const;

    /// @brief Return the determinant of the LinearTransform.
    double getDeterminant() const;

    /// @brief Return the inverse of the LinearTransform;
    LinearTransform invert() const { return LinearTransform(*this).invert(); }

private:

    BaseCore const & _input; ///< \internal input core to be transformed

};

/**
 *  @brief A temporary-only expression object representing an AffineTransform that
 *         maps the Ellipse to a unit circle at the origin.
 */
class Ellipse::GridTransform {
public:
    
    /// Matrix type for derivative with respect to input ellipse parameters.
    typedef Eigen::Matrix<double,6,5> DerivativeMatrix;

    /// @brief Standard constructor.
    explicit GridTransform(Ellipse const & input) : _input(input) {}
    
    /// @brief Return the derivative of transform with respect to input ellipse.
    DerivativeMatrix d() const;
    
    /// @brief Convert the proxy to a AffineTransform.
    operator AffineTransform () const;

    /// @brief Return the inverse of the AffineTransform.
    AffineTransform invert() const { return AffineTransform(*this).invert(); }

private:

    Ellipse const & _input; ///< \internal input ellipse to be transformed

};

inline BaseCore::GridTransform const BaseCore::getGridTransform() const{
    return BaseCore::GridTransform(*this);
}

inline Ellipse::GridTransform const Ellipse::getGridTransform() const {
    return Ellipse::GridTransform(*this);
}

}}}} // namespace lsst::afw::geom::ellipses

#endif // !LSST_AFW_GEOM_ELLIPSES_GridTransform_h_INCLUDED
