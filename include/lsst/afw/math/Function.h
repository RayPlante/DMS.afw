// -*- LSST-C++ -*-

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
 
#ifndef LSST_AFW_MATH_FUNCTION_H
#define LSST_AFW_MATH_FUNCTION_H
/**
 * @file
 *
 * @brief Define the basic Function classes.
 *
 * @author Russell Owen
 *
 * @ingroup afw
 */
#include <stdexcept>
#include <sstream>
#include <vector>

#include "boost/format.hpp"
#include "boost/serialization/nvp.hpp"
#include "boost/serialization/vector.hpp"
#include "boost/serialization/void_cast.hpp"
#include "boost/serialization/export.hpp"

#include "lsst/daf/data/LsstBase.h"
#include "lsst/pex/exceptions.h"

namespace lsst {
namespace afw {
namespace math {

#ifndef SWIG
using boost::serialization::make_nvp;
#endif

    /**
     * @brief Basic Function class.
     *
     * Function objects are functions whose parameters may be read and changed using
     * getParameters and setParameters. They were designed for use with the Kernel class.
     *
     * These are simple functors with the restrictions that:
     * - Function arguments and parameters are double precision
     * - The return type is templated
     *
     * To create a function for a particular equation, subclass Function
     * or (much more likely) Function1 or Function2. Your subclass must:
     * - Have one or more constructors, all of which must initialize _params
     * - Define operator() with code to compute the function
     *   using this->_params or this->getParams() to reference the parameters
     * - If the function is a linear combination of parameters then override the function isLinearCombination.
     *
     * Design Notes:
     * The reason these functions exist (rather than using a pre-existing function class,
     * such as Functor in VisualWorkbench) is because the Kernel class requires function
     * objects with a standard interface for setting and getting function parameters.
     *
     * The reason isLinearCombination exists is to support refactoring LinearCombinationKernels.
     *
     * @ingroup afw
     */
    template<typename ReturnT>
    class Function : public lsst::daf::data::LsstBase {
    
    public:
        /**
         * @brief Construct a Function given the number of function parameters.
         *
         * The function parameters are initialized to 0.
         */
        explicit Function(
            unsigned int nParams)   ///< number of function parameters
        :
            lsst::daf::data::LsstBase(typeid(this)),
            _params(nParams)
        {}

        /**
         * @brief Construct a Function given the function parameters.
         */
        explicit Function(
            std::vector<double> const &params)
        :
            lsst::daf::data::LsstBase(typeid(this)),
            _params(params)   ///< function parameters
        {}
        
        virtual ~Function() {}
    
        /**
         * @brief Return the number of function parameters
         *
         * @return the number of function parameters
         */
        unsigned int getNParameters() const {
            return _params.size();
        }
        
        /**
         * @brief Get one function parameter without range checking
         *
         * @return the specified function parameter
         */
        virtual double getParameter(
            unsigned int ind)    ///< index of parameter
        const {
            return _params[ind];
        }
        
        /**
         * @brief Return all function parameters
         *
         * @return the function parameters as a vector
         */
        std::vector<double> const &getParameters() const {
            return _params;
        }

        /**
         * @brief Is the function a linear combination of its parameters?
         *
         * @return true if the function can be expressed as: sum over i of parameter_i * function_i(args)
         *
         * @warning: subclasses must override if true.
         */
        virtual bool isLinearCombination() const { return false; }
        
        /**
         * @brief Set one function parameter without range checking
         */
        void setParameter(
            unsigned int ind,   ///< index of parameter
            double newValue)    ///< new value for parameter
        {
            _params[ind] = newValue;
        }
        
        /**
         * @brief Set all function parameters
         *
         * @throw lsst::pex::exceptions::InvalidParameterException
         *        if the wrong number of parameters is supplied.
         */
        void setParameters(
            std::vector<double> const &params)   ///< vector of function parameters
        {
            if (_params.size() != params.size()) {
                throw LSST_EXCEPT(pexExcept::InvalidParameterException,
                    (boost::format("params has %d entries instead of %d") % \
                    params.size() % _params.size()).str());
            }
            _params = params;
        }
    
        /**
         * @brief Return a string representation of the function
         *
         * @return a string representation of the function
         */
        virtual std::string toString(std::string const&) const {
            std::stringstream os;
            os << "parameters: [ ";
            for (std::vector<double>::const_iterator i = _params.begin(); i != _params.end(); ++i) {
                if (i != _params.begin()) os << ", ";
                os << *i;
            }
            os << " ]";
            return os.str();
        }

    protected:
        std::vector<double> _params;

        /* Default constructor: intended only for serialization */
        explicit Function() : lsst::daf::data::LsstBase(typeid(this)), _params(0) {}   

    private: // serialization support
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, unsigned int const version) {
            ar & make_nvp("params", _params);
        }
    };

    
    /**
     * @brief A Function taking one argument.
     *
     * Subclass and override operator() to do useful work.
     *
     * @ingroup afw
     */
    template<typename ReturnT>
    class Function1 : public Function<ReturnT> {
    public:
        typedef boost::shared_ptr<Function1<ReturnT> > Ptr;

        /**
         * @brief Construct a Function1 given the number of function parameters.
         *
         * The function parameters are initialized to 0.
         */
        explicit Function1(
            unsigned int nParams)   ///< number of function parameters
        :
            Function<ReturnT>(nParams)
        {}

        /**
         * @brief Construct a Function1 given the function parameters.
         */
        explicit Function1(
            std::vector<double> const &params)   ///< function parameters
        :
            Function<ReturnT>(params)
        {}
        
        virtual ~Function1() {}
        
        /**
         * @brief Return a pointer to a deep copy of this function
         *
         * This function exists instead of a copy constructor
         * so one can obtain a copy of an actual function
         * instead of a useless copy of the base class.
         *
         * Every concrete subclass must override this method.
         *
         * @return a pointer to a deep copy of the function
         */
        virtual Ptr clone() const = 0; 
    
        virtual ReturnT operator() (double x) const = 0;
        
        virtual std::string toString(std::string const& prefix="") const {
            return std::string("Function1: ") + Function<ReturnT>::toString(prefix);
        }

        virtual void computeCache(int const n) {}

    protected:
        /* Default constructor: intended only for serialization */
        explicit Function1() : Function<ReturnT>() {}    
    
    private: // serialization
        friend class boost::serialization::access;
#ifndef SWIG // SWIG doesn't like base_object
        template <class Archive>
        void serialize(Archive& ar, unsigned const int version) {
            ar & make_nvp("fn", boost::serialization::base_object<Function<ReturnT> >(*this));
        }
#endif
    };    
    
    /**
     * @brief A Function taking two arguments.
     *
     * Subclass and override operator() to do useful work.
     *
     * @ingroup afw
     */
    template<typename ReturnT>
    class Function2 : public Function<ReturnT> {
    public:
        typedef boost::shared_ptr<Function2<ReturnT> > Ptr;

        /**
         * @brief Construct a Function2 given the number of function parameters.
         *
         * The function parameters are initialized to 0.
         */
        explicit Function2(
            unsigned int nParams)   ///< number of function parameters
        :
            Function<ReturnT>(nParams)
        {}

        /**
         * @brief Construct a Function2 given the function parameters.
         *
         * The number of function parameters is set to the length of params.
         */
        explicit Function2(
            std::vector<double> const &params)   ///< function parameters
        :
            Function<ReturnT>(params)
        {}
        
        virtual ~Function2() {}
        
        /**
         * @brief Return a pointer to a deep copy of this function
         *
         * This function exists instead of a copy constructor
         * so one can obtain a copy of an actual function
         * instead of a useless copy of the base class.
         *
         * Every non-virtual function must override this method.
         *
         * @return a pointer to a deep copy of the function
         */
        virtual Ptr clone() const = 0; 
    
        virtual ReturnT operator() (double x, double y) const = 0;

        virtual std::string toString(std::string const& prefix="") const {
            return std::string("Function2: ") + Function<ReturnT>::toString(prefix);
        }
        /**
         * Return the derivative of the Function with respect to its parameters
         */
        virtual std::vector<double> getDFuncDParameters(double, double) const {
            throw LSST_EXCEPT(lsst::pex::exceptions::NotFoundException,
                              "getDFuncDParameters is not implemented for this class");
        }

    protected:
        /* Default constructor: intended only for serialization */
        explicit Function2() : Function<ReturnT>() {}    
    
    private:
        friend class boost::serialization::access;
#ifndef SWIG // SWIG doesn't like base_object
        template <class Archive>
        void serialize(Archive& ar, unsigned const int version) {
            ar & make_nvp("fn", boost::serialization::base_object<Function<ReturnT> >(*this));
        }
#endif
    };


    /**
     * @brief Base class for 2-dimensional polynomials of the form:
     *
     * f(x,y) =   c0                                                    (0th order)
     *          + c1 f1(x) + c2 f1(y)                                   (1st order)
     *          + c2 f2(x) + c3 f1(x) f1(y) + c4 f2(y)                  (2nd order)
     *          + c5 f3(x) + c6 f2(x) f1(y) + c7 f1(x) f2(y) + f3(y)    (3rd order)
     *          + ...
     *
     * @ingroup afw
     */
    template<typename ReturnT>
    class BasePolynomialFunction2: public Function2<ReturnT> {
    public:
        typedef typename Function2<ReturnT>::Ptr Function2Ptr;

        /**
         * @brief Construct a polynomial function of specified order.
         *
         * The polynomial will have (order + 1) * (order + 2) / 2 coefficients
         *
         * The parameters are initialized to zero.
         */
        explicit BasePolynomialFunction2(
            unsigned int order) ///< order of polynomial (0 for constant)
        :
            Function2<ReturnT>(BasePolynomialFunction2::nParametersFromOrder(order)),
            _order(order)
        {}

        /**
         * @brief Construct a polynomial function with specified parameters.
         *
         * The order of the polynomial is determined from the length of the params vector
         * (see orderFromNParameters) and only certain lengths are suitable: 1, 3, 6, 10, 15...
         *
         * @throw lsst::pex::exceptions::InvalidParameterException if params length is unsuitable
         */
        explicit BasePolynomialFunction2(
            std::vector<double> params) ///< polynomial coefficients
        :
            Function2<ReturnT>(params),
            _order(BasePolynomialFunction2::orderFromNParameters(static_cast<int>(params.size())))
        {}
        
        virtual ~BasePolynomialFunction2() {}
        
        /**
         * @brief Get the polynomial order
         */
        int getOrder() const { return _order; }
       
        virtual bool isLinearCombination() const { return true; }
        
        /**
         * @brief Compute number of parameters from polynomial order.
         *
         * @throw lsst::pex::exceptions::InvalidParameterException if order < 0
         */
        static int nParametersFromOrder(int order) {
            if (order < 0) {
                std::ostringstream os;
                os << "order=" << order << " invalid: must be >= 0";
                throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterException, os.str());
            }
            return (order + 1) * (order + 2) / 2;
        }
        
        /**
         * @brief Compute polynomial order from the number of parameters
         *
         * Only certain values of nParameters are acceptable, including:
         * nParameters order
         *      1        0
         *      3        1
         *      6        2
         *     10        3
         *     15        4
         *    ...
         *
         * @throw lsst::pex::exceptions::InvalidParameterException if nParameters is invalid
         */
        static int orderFromNParameters(int nParameters) {
            int order = static_cast<int>(
                0.5 + ((-3.0 + (std::sqrt(1.0 + (8.0 * static_cast<double>(nParameters))))) / 2.0));
            if (nParameters != BasePolynomialFunction2::nParametersFromOrder(order)) {
                std::ostringstream os;
                os << "nParameters=" << nParameters << " invalid: order is not an integer";
                throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterException, os.str());
            }
            return order;
        }

    protected:
        int _order; ///< order of polynomial

        /* Default constructor: intended only for serialization */
        explicit BasePolynomialFunction2() : Function2<ReturnT>(1), _order(0) {}    

    private:
        friend class boost::serialization::access;
#ifndef SWIG // SWIG doesn't like base_object
        template <class Archive>
        void serialize(Archive& ar, unsigned const int version) {
            ar & make_nvp("fn2", boost::serialization::base_object<Function2<ReturnT> >(*this));
            ar & make_nvp("order", _order);
        }
#endif
    };


    /**
     * @brief a class used in function calls to indicate that no Function1 is being provided
     */
    template<typename ReturnT>
    class NullFunction1 : public Function1<ReturnT> {
    public:
        explicit NullFunction1() : Function1<ReturnT>(0) {}
        typename Function1<ReturnT>::Ptr clone() const {
            return typename Function1<ReturnT>::Ptr(new NullFunction1()); }

    private:
        ReturnT operator() (double) const { return static_cast<ReturnT>(0); }

    private:
        friend class boost::serialization::access;
#ifndef SWIG // SWIG doesn't like base_object
        template <class Archive>
        void serialize(Archive& ar, unsigned int const version) {
            ar & make_nvp("fn1", boost::serialization::base_object<Function1<ReturnT> >(*this));
        }
#endif
    };

    /**
     * @brief a class used in function calls to indicate that no Function2 is being provided
     */
    template<typename ReturnT>
    class NullFunction2 : public Function2<ReturnT> {
    public:
        explicit NullFunction2() : Function2<ReturnT>(0) {}
        typename Function2<ReturnT>::Ptr clone() const {
            return typename Function2<ReturnT>::Ptr(new NullFunction2()); }

    private:
        ReturnT operator() (double, double) const { return static_cast<ReturnT>(0); }

    private:
        friend class boost::serialization::access;
#ifndef SWIG // SWIG doesn't like base_object
        template <class Archive>
        void serialize(Archive& ar, unsigned int const version) {
            ar & make_nvp("fn2", boost::serialization::base_object<Function2<ReturnT> >(*this));
        }
#endif
    };


}}}   // lsst::afw::math

#endif // #ifndef LSST_AFW_MATH_FUNCTION_H
