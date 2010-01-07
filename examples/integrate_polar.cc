// -*- LSST-C++ -*-
/**
 * @file   rombergPolar.cc
 * @author S. Bickerton
 * @date   May 25, 2009
 *
 * This example demonstrates how to use the romberg2D()
 * integrator (afw::math Quadrature) with a polar function.
 *
 */
#include <iostream>
#include <vector>
#include <cmath>
#include <functional>

#include "lsst/afw/math/Integrate.h"

namespace math = lsst::afw::math;

/** =========================================================================
 * define a simple 2D function as a functor to be integrated.
 * I've chosen a paraboloid: f(x) = K - kr*r*r
 * as it's got an easy-to-check analytic answer.
 *
 * @note that we *have* to inherit from binary_function<>
 */
template<typename IntegrandT>
class Parab2D : public std::binary_function<IntegrandT, IntegrandT, IntegrandT> {
public:
    // declare coefficients at instantiation.
    Parab2D(IntegrandT const K, IntegrandT const kr) : _K(K), _kr(kr) {}
    
    // operator() must be overloaded to return the evaluation of the function
    // ** This is the function to be integrated **
    //
    // NOTE: extra 'r' term due to polar coords (ie. the 'r' in r*dr*dtheta)
    IntegrandT operator()(IntegrandT const r, IntegrandT const theta) const {
        return (_K - _kr*r*r)*r;
    }

    // for this example we have an analytic answer to check
    IntegrandT getAnalyticVolume(IntegrandT const r1, IntegrandT const r2,
                                 IntegrandT const theta1, IntegrandT const theta2) {
        return ((theta2 - theta1) *
                ( (0.5*_K*r2*r2 - (1.0/3.0)*_kr*r2*r2*r2) -
                  (0.5*_K*r1*r1 - (1.0/3.0)*_kr*r1*r1*r1) ));
    }

private:
    IntegrandT _K, _kr;
};



/** =============================================================================
 * Define a normal function that does the same thing as the above functor
 *
 */
double parabola2d(double const r, double const theta) {
    double const K = 1.0, kr = 0.0;
    return (K - kr*r*r)*r;
}



/** =====================================================================
 *  Main body of code
 *  ======================================================================
 */
int main() {

    // set limits of integration
    double const r1 = 0, r2 = 1, theta1 = 0, theta2 = 2.0*M_PI;
    // set the coefficients for the quadratic equation
    // (parabola f(r) = K - kr*r*r)
    double const K = 1.0, kr = 0.0;  // not really a parabola ... force the answer to be 'pi'

    
    // ==========  2D integrator ==========

    // instantiate a Parab2D
    Parab2D<double> parab2d(K, kr);

    // integrate the volume under the function, and then get the analytic result
    double const parab_volume_integrate  = math::integrate2d(parab2d, r1, r2, theta1, theta2);
    double const parab_volume_analytic = parab2d.getAnalyticVolume(r1, r2, theta1, theta2);

    // now run it on the 2d function (you *need* to wrap the function in ptr_fun())
    double const parab_volume_integrate_func =
        math::integrate2d(std::ptr_fun(parabola2d), r1, r2, theta1, theta2);

    // output
    std::cout << "2D integrate: functor = " << parab_volume_integrate <<
        "  function = " << parab_volume_integrate_func <<
        "  analytic = " << parab_volume_analytic << std::endl;
    
    return 0;
}
