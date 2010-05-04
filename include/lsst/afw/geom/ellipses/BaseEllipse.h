// -*- lsst-c++ -*-
#ifndef LSST_AFW_GEOM_ELLIPSES_BASEELLIPSE_H
#define LSST_AFW_GEOM_ELLIPSES_BASEELLIPSE_H

/**
 *  \file
 *  \brief Forward declarations, typedefs, and definitions for BaseEllipse and BaseCore.
 *
 *  \note Do not include directly; use the main ellipse header file.
 */

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <memory>

#include "lsst/afw/geom/Point.h"
#include "lsst/afw/geom/Extent.h"
#include "lsst/afw/geom/LinearTransform.h"
#include "lsst/afw/geom/AffineTransform.h"
#include "lsst/afw/geom/Box.h"

namespace lsst {
namespace afw {
namespace geom {
namespace ellipses {

class Quadrupole;
class Axes;
class Distortion;
class LogShear;

class BaseEllipse;
class BaseCore;

/**
 *  This typedef is expected to be used more often than the true class name, 
 *  and exists so that the Ellipse typedef within core classes merely hides
 *  another typedef instead of a true class name.
 */
typedef BaseEllipse Ellipse;

/**
 *  This typedef is expected to be used more often than the true class name, 
 *  and exists so that the Core typedef within ellipse classes merely hides
 *  another typedef instead of a true class name.
 */
typedef BaseCore Core;

/**
 *  \brief A base class for ellipse geometries.
 *
 *  An ellipse is composed of its center coordinate and its Core - a parametrization of the
 *  ellipticity and size of the ellipse.  A subclass of BaseEllipse is defined for each concrete
 *  subclass of BaseCore, and is typedef'd as Core within the core class.  As a result, setting
 *  the core of an ellipse never changes the type of the contained core, it merely sets the parameters
 *  of that core by converting the parametrization.
 *
 *  \ingroup EllipseGroup
 */
class BaseEllipse {
protected:
    class Transformer; ///< Proxy return type for BaseEllipse::transform().
public:

    typedef boost::shared_ptr<BaseEllipse> Ptr;
    typedef boost::shared_ptr<BaseEllipse const> ConstPtr;

    typedef BoxD Envelope; ///< Bounding box type.
    typedef Eigen::Matrix<double,5,1> ParameterVector; ///< Parameter vector type.

    typedef BaseEllipse Ellipse;
    typedef BaseCore Core;

    class RadialFraction;

    enum Parameters { X=0, Y=1 }; ///< Definitions for elements of an ellipse vector.

    /// \brief Deep copy the BaseEllipse.
    boost::shared_ptr<BaseEllipse> clone() const { return boost::shared_ptr<BaseEllipse>(_clone()); }

    Point2D const & getCenter() const { return _center; } ///< \brief Return the center point.
    Point2D & getCenter() { return _center; }             ///< \brief Return the center point.
    void setCenter(Point2D const & center) { _center = center; } ///< \brief Set the center point.

    BaseCore const & getCore() const; ///< \brief Return the core object.
    BaseCore & getCore();             ///< \brief Return the core object.
    void setCore(Core const & core); ///< \brief Set the core object.

    double & operator[](int n); ///< \brief Access the nth ellipse parameter.
    double operator[](int n) const; ///< \brief Access the nth ellipse parameter.

    /// \brief Put the parameters in a standard form, and return false if they are invalid.
    bool normalize();

    /// \brief Increase the major and minor radii of the ellipse by the given buffer.
    void grow(double buffer);

    /// \brief Scale the size of the ellipse by the given factor.
    void scale(double factor);

    /// \brief Move the ellipse center by the given offset.
    void shift(ExtentD const & offset);

    Transformer transform(AffineTransform const & transform);
    Transformer const transform(AffineTransform const & transform) const; 

    AffineTransform getGenerator() const;
    Envelope computeEnvelope() const;


    BaseEllipse & operator=(BaseEllipse const & other);
    ParameterVector const getVector() const;
    void setVector(ParameterVector const & vector);

    virtual ~BaseEllipse() {}

protected:

    virtual BaseEllipse * _clone() const = 0;

    explicit BaseEllipse(BaseCore const & core, Point2D const & center);

    explicit BaseEllipse(BaseCore * core, Point2D const & center);

    Point2D _center;
    boost::scoped_ptr<BaseCore> _core;
};

/**
 *  \brief A base class for parametrizations of the "core" of an ellipse - the ellipticity and size.
 *
 *  A subclass of BaseCore provides a particular interpretation of the three pointing point values that
 *  define an ellipse's size and ellipticity (including position angle).  All core subclasses
 *  are implicitly convertible and can be assigned to from any other core.
 *
 *  \ingroup EllipseGroup
 */
class BaseCore {
protected:
    class Transformer;
public:

    typedef boost::shared_ptr<BaseCore> Ptr;
    typedef boost::shared_ptr<BaseCore const> ConstPtr;

    typedef Eigen::Vector3d ParameterVector;  ///< Parameter vector type.
    typedef Eigen::Matrix3d Jacobian; ///< Parameter Jacobian matrix type.

    typedef BaseEllipse Ellipse;
    typedef BaseCore Core;

    class RadialFraction;

    /// \brief Return a string that identifies this parametrization.
    virtual char const * getName() const = 0;

    /// \brief Deep-copy the Core.
    boost::shared_ptr<BaseCore> clone() const { return boost::shared_ptr<BaseCore>(_clone()); }

    /// \brief Construct an ellipse of the appropriate subclass from this and the given center.
    boost::shared_ptr<BaseEllipse> makeEllipse(Point2D const & center = Point2D()) const {
        return boost::shared_ptr<BaseEllipse>(_makeEllipse(center));
    }

    double & operator[](int n) { return _vector[n]; } ///< \brief Access the nth core parameter.
    double operator[](int n) const { return _vector[n]; } ///< \brief Access the nth core parameter.

    /**
     *  \brief Put the parameters into a "standard form", if possible, and return
     *         false if they are entirely invalid.
     */
    virtual bool normalize() = 0;

    /// \brief Increase the major and minor radii of the ellipse core by the given buffer.
    virtual void grow(double buffer);

    /// \brief Scale the size of the ellipse core by the given factor.
    virtual void scale(double factor) = 0;

    Transformer transform(AffineTransform const & transform); 
    Transformer const transform(AffineTransform const & transform) const; 
    Transformer transform(LinearTransform const & Ltransform);
    Transformer const transform(LinearTransform const & transform) const;

    /// \brief Return the LinearTransform that transforms the unit circle into this.
    virtual LinearTransform getGenerator() const;

    /// \brief Return the size of the bounding box for the ellipse core.
    ExtentD computeDimensions() const;

    /// \brief Return the core parameters as a vector.
    ParameterVector const & getVector() const { return _vector; }

    /// \brief Set the core parameters from a vector.
    void setVector(ParameterVector const & vector) { _vector = vector; }

    /**
     *  \brief Set the parameters of this ellipse core from another.
     *
     *  This does not change the parametrization of the ellipse core.
     */
    virtual BaseCore & operator=(BaseCore const & other) = 0;
    
    /// \brief Assign other to this and return the derivative of the conversion, d(this)/d(other).
    virtual Jacobian dAssign(BaseCore const & other) = 0;

    virtual ~BaseCore() {}

protected:

    friend class BaseEllipse;

    virtual BaseCore * _clone() const = 0;
    virtual BaseEllipse * _makeEllipse(Point2D const & center) const = 0;

    explicit BaseCore(ParameterVector const & vector) : _vector(vector) {}

    explicit BaseCore(double v1=0, double v2=0, double v3=0) : _vector(v1,v2,v3) {}

    virtual void _assignTo(Quadrupole & other) const = 0;
    virtual void _assignTo(Axes & other) const = 0;
    virtual void _assignTo(Distortion & other) const = 0;
    virtual void _assignTo(LogShear & other) const = 0;

    virtual Jacobian _dAssignTo(Quadrupole & other) const = 0;
    virtual Jacobian _dAssignTo(Axes & other) const = 0;
    virtual Jacobian _dAssignTo(Distortion & other) const = 0;
    virtual Jacobian _dAssignTo(LogShear & other) const = 0;

    ParameterVector _vector;

    friend class Quadrupole;
    friend class Axes;
    friend class Distortion;
    friend class LogShear;
};

inline BaseCore const & BaseEllipse::getCore() const { return *_core; }
inline BaseCore & BaseEllipse::getCore() { return *_core; }
inline void BaseEllipse::setCore(BaseCore const & core) { *_core = core; }
inline bool BaseEllipse::normalize() { return _core->normalize(); }
inline void BaseEllipse::grow(double buffer) { getCore().grow(buffer); }
inline void BaseEllipse::scale(double factor) { getCore().scale(factor); }
inline void BaseEllipse::shift(ExtentD const & offset) { _center += offset; }

inline double & BaseEllipse::operator[](int i) { return (i<2) ? _center[i] : (*_core)[i-2]; }
inline double BaseEllipse::operator[](int i) const { return (i<2) ? _center[i] : (*_core)[i-2]; }

inline BaseEllipse::BaseEllipse(Core const & core, Point2D const & center) : 
    _center(center), _core(core._clone()) {}

inline BaseEllipse::BaseEllipse(Core * core, Point2D const & center) :
    _center(center), _core(core) {}

} // namespace lsst::afw::geom::ellipses
}}} // namespace lsst::afw::geom
#endif // !LSST_AFW_GEOM_ELLIPSES_BASEELLIPSE_H
