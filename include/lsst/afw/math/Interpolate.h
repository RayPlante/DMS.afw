// -*- LSST-C++ -*-
#if !defined(LSST_AFW_MATH_INTERPOLATE_H)
#define LSST_AFW_MATH_INTERPOLATE_H
/**
 * @file Interpolate.h
 * @brief Wrap GSL to interpolate values for a set of x,y vector<>s
 * @ingroup afw
 * @author Steve Bickerton
 */
#include <limits>
#include "gsl/gsl_interp.h"
#include "gsl/gsl_spline.h"

#include "boost/shared_ptr.hpp"

namespace lsst { namespace afw { namespace math {

enum Style {
    LINEAR = 0,
    NATURAL_SPLINE = 1,
    CUBIC_SPLINE = 2,
    CUBIC_SPLINE_PERIODIC = 3,
    AKIMA_SPLINE = 4,
    AKIMA_SPLINE_PERIODIC = 5
    
 };

namespace {
    ::gsl_interp_type const *gslInterpTypeList[6] = {::gsl_interp_linear,
                                                     ::gsl_interp_cspline,
                                                     ::gsl_interp_cspline,
                                                     ::gsl_interp_cspline_periodic,
                                                     ::gsl_interp_akima,
                                                     ::gsl_interp_akima_periodic };
}
            
class Interpolate {
public:
    
    Interpolate(std::vector<double> const &x, std::vector<double> const &y,
                   ::gsl_interp_type const *gslInterpType = ::gsl_interp_akima) :
        _x(x), _y(y) {
        InterpolateInit(_x, _y, gslInterpType);
    }

    Interpolate(std::vector<double> const &x, std::vector<double> const &y,
                Style const style) :
        _x(x), _y(y) {
        InterpolateInit(_x, _y, gslInterpTypeList[style]);
    }

    void InterpolateInit(std::vector<double> const &x, std::vector<double> const &y,
                         ::gsl_interp_type const *gslInterpType) {
        _acc    = ::gsl_interp_accel_alloc();
        _interp = ::gsl_interp_alloc(gslInterpType, y.size());
        ::gsl_interp_init(_interp, &x[0], &y[0], y.size());
    }
    
    ~Interpolate() {
        ::gsl_interp_free(_interp);
        ::gsl_interp_accel_free(_acc);
    }

    double interpolate(double const x) {
        return ::gsl_interp_eval(_interp, &_x[0], &_y[0], x, _acc);
    }
    
private:
    std::vector<double> const &_x;
    std::vector<double> const &_y;
    ::gsl_interp_accel *_acc;
    ::gsl_interp *_interp;
};

        
}}}
                     
#endif // LSST_AFW_MATH_INTERPOLATE_H
