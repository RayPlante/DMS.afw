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

#ifndef LSST_AFW_GEOM_ELLIPSES_Quadrupole_h_INCLUDED
#define LSST_AFW_GEOM_ELLIPSES_Quadrupole_h_INCLUDED

/**
 *  \file
 *  @brief Definitions and inlines for Quadrupole.
 *
 *  \note Do not include directly; use the main ellipse header file.
 */

#include "lsst/afw/geom/ellipses/BaseCore.h"
#include "lsst/afw/geom/ellipses/Convolution.h"
#include "lsst/afw/geom/ellipses/Transformer.h"
#include "lsst/afw/geom/ellipses/GridTransform.h"

namespace lsst { namespace afw { namespace geom { namespace ellipses {

/**
 *  @brief An ellipse core with quadrupole moments as parameters.
 */
class Quadrupole : public BaseCore {
public:

    typedef boost::shared_ptr<Quadrupole> Ptr;
    typedef boost::shared_ptr<Quadrupole const> ConstPtr;

    enum ParameterEnum { IXX=0, IYY=1, IXY=2 }; ///< Definitions for elements of a core vector.

    /// Matrix type for the matrix representation of Quadrupole parameters.
    typedef Eigen::Matrix<double,2,2,Eigen::DontAlign> Matrix;

    double const getIXX() const { return _matrix(0, 0); }
    void setIXX(double ixx) { _matrix(0, 0) = ixx; }

    double const getIYY() const { return _matrix(1, 1); }
    void setIYY(double iyy) { _matrix(1, 1) = iyy; }

    double const getIXY() const { return _matrix(1, 0); }
    void setIXY(double ixy) { _matrix(0, 1) = _matrix(1, 0) = ixy; }

    /// @brief Deep copy the ellipse core.
    Ptr clone() const { return boost::static_pointer_cast<Quadrupole>(_clone()); }

    /// Return a string that identifies this parametrization.
    virtual std::string getName() const;

    /**
     *  @brief Put the parameters into a "standard form", and throw InvalidEllipseParameters
     *         if they cannot be normalized.
     */
    virtual void normalize();

    virtual void readParameters(double const * iter);

    virtual void writeParameters(double * iter) const;

    /// @brief Return a 2x2 symmetric matrix of the parameters.
    Matrix const & getMatrix() const { return _matrix; }

    /// @brief Return the determinant of the matrix representation.
    double getDeterminant() const { return getIXX() * getIYY() - getIXY() * getIXY(); }

    /// @brief Standard assignment.
    Quadrupole & operator=(Quadrupole const & other) { _matrix = other._matrix; return *this; }

    /// @brief Converting assignment.
    Quadrupole & operator=(BaseCore const & other) { BaseCore::operator=(other); return *this; }

    /// @brief Construct from parameter values.
    explicit Quadrupole(double ixx=1.0, double iyy=1.0, double ixy=0.0, bool normalize=false);

    /// @brief Construct from a parameter vector.
    explicit Quadrupole(BaseCore::ParameterVector const & vector, bool normalize=false);

    /// @brief Construct from a 2x2 matrix.
    explicit Quadrupole(Matrix const & matrix, bool normalize=true);

    /// @brief Copy constructor.
    Quadrupole(Quadrupole const & other) : _matrix(other._matrix) {}

    /// @brief Converting copy constructor.
    Quadrupole(BaseCore const & other) { *this = other; }
#ifndef SWIG
    /// @brief Converting copy constructor.
    Quadrupole(BaseCore::Transformer const & transformer) {
        transformer.apply(*this);
    }

    /// @brief Converting copy constructor.
    Quadrupole(BaseCore::Convolution const & convolution) {
        convolution.apply(*this);
    }
#endif
protected:

    virtual BaseCore::Ptr _clone() const { return boost::make_shared<Quadrupole>(*this); }

    virtual void _assignToQuadrupole(double & ixx, double & iyy, double & ixy) const;
    virtual void _assignFromQuadrupole(double ixx, double iyy, double ixy);

    virtual void _assignToAxes(double & a, double & b, double & theta) const;
    virtual void _assignFromAxes(double a, double b, double theta);

    virtual Jacobian _dAssignToQuadrupole(double & ixx, double & iyy, double & ixy) const;
    virtual Jacobian _dAssignFromQuadrupole(double ixx, double iyy, double ixy);

    virtual Jacobian _dAssignToAxes(double & a, double & b, double & theta) const;
    virtual Jacobian _dAssignFromAxes(double a, double b, double theta);

private:
    static Registrar<Quadrupole> registrar;

    Matrix _matrix;
};

}}}} // namespace lsst::afw::geom::ellipses

#endif // !LSST_AFW_GEOM_ELLIPSES_Quadrupole_h_INCLUDED
