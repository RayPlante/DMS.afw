// -*- LSST-C++ -*-
#ifndef LSST_AFW_MATH_KERNEL_H
#define LSST_AFW_MATH_KERNEL_H
/**
 * @file
 *
 * @brief Declare the Kernel class and subclasses.
 *
 * @author Russell Owen
 *
 * @ingroup afw
 */
#include <vector>

#include "boost/mpl/or.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/static_assert.hpp"
#include "boost/type_traits/is_same.hpp"
#include "boost/type_traits/is_base_and_derived.hpp"

#include "boost/serialization/shared_ptr.hpp"
#include "boost/serialization/vector.hpp"
#include "boost/serialization/export.hpp"

#include "lsst/pex/logging/Trace.h"
#include "lsst/daf/base/Persistable.h"
#include "lsst/daf/data/LsstBase.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/math/Function.h"
#include "lsst/afw/math/traits.h"

namespace lsst {
namespace afw {

namespace formatters {
class KernelFormatter;
}

namespace math {

#ifndef SWIG
using boost::serialization::make_nvp;
#endif


    /**
     * @brief Kernels are used for convolution with MaskedImages and (eventually) Images
     *
     * Kernel is a virtual base class; it cannot be instantiated. The following notes apply to
     * Kernel and to its subclasses.
     *
     * The template type should usually be float or double; integer kernels
     * should be used with caution because they do not normalize well.
     *
     * The center pixel of a Kernel is at index: (width-1)/2, (height-1)/2. Thus it is centered along
     * columns/rows if the kernel has an odd number of columns/rows and shifted 1/2 pixel towards 0 otherwise.
     * A kernel should have an odd number of columns and rows unless it is intended to shift an image.
     *
     * <b>Spatially Varying Kernels</b>
     *
     * Kernels may optionally vary spatially (so long as they have any kernel parameters).
     * To make a spatially varying kernel, specify a spatial function at construction
     * (you cannot change your mind once the kernel is constructed).
     * You must also specify a set of spatial parameters, and you may do this at construction
     * and/or later by calling setSpatialParameters. The spatial parameters are a vector
     * (one per kernel function parameter) of spatial function parameters. In other words
     * the spatial parameters are a vector of vectors indexed as [kernel parameter][spatial parameter].
     * The one spatial function is used to compute the kernel parameters at a given spatial position
     * by computing each kernel parameter using its associated vector of spatial function parameters.
     *
     * One may only specify a single spatial function to describe the variation of all kernel parameters
     * (though the spatial parameters can, and usually will, be different for each kernel parameter).
     * The only use cases we had used the same spatial model for all parameters, and I felt it
     * would be a burden to the user to have to create one function per kernel parameter.
     * (I would gladly change this design if wanted, since it would simplify the internal code.)
     * You still can assign a different spatial model to each kernel parameter by defining
     * one function that is the sum of all the desired spatial models. Then for each kernel parameter,
     * then zero all but the few spatial parameters that control the spatial model for that kernel parameter.
     *
     * When determining the parameters for a spatial function or a kernel function,
     * please keep the LSST convention for pixel position vs. pixel index in mind.
     * See lsst/afw/image/ImageUtils.h for the convention.
     *
     * Note that if a kernel is spatially varying then you may not set the kernel parameters directly;
     * that is the job of the spatial function! However, you may change the spatial parameters at any time.
     *
     * <b>Design Notes</b>
     *
     * The basic design is to use the same kernel class for both spatially varying and spatially invariant
     * kernels. The user either does or does not supply a function describing the spatial variation 
     * at creation time. In addition, analytic kernels are described by a user-supplied function
     * of the same basic type as the spatial variation function.
     * 
     * Several other designs were considered, including:
     * A) Use different classes for spatially varying and spatially invariant versions of each kernel.
     * Thus instead of three basic kernel classes (FixedKernel, AnalyticKernel and LinearCombinationKernel)
     * we would have five (since FixedKernel cannot be spatially varying). Robert Lupton argued that
     * was a needless expansion of the class hiearchy and I agreed.
     * B) Construct analytic kernels by defining a subclass of AnalyticKernel that is specific to the
     * desired functional (e.g. GaussianAnalyticKernel). If spatial models are handled the same way
     * then this creates a serious proliferation of kernel classes (even if we only have two different
     * spatial models, e.g. polynomial and Chebyshev polynomial). I felt it made more sense to define
     * the spatial model by some kind of function class (often called a "functor"), and since we needed
     * such a class, I chose to use it for the analytic kernel as well.
     *
     * However, having a separate function class does introduce some potential inefficiencies.
     * If a function is part of the class it can potentially be evaluated more quickly than calling
     * a function for each pixel or spatial position.
     *
     * A possible variant on the current design is to define the spatial model and analytic kernel
     * by specifying the functions as template parameters. This has the potential to regain some efficiency
     * in evaluating the functions. However, it would be difficult or impossible to pre-instantiate
     * the desired template classes, a requirement of the LSST coding standards.
     *
     * @ingroup afw
     */
    class Kernel : public lsst::daf::data::LsstBase, public lsst::daf::base::Persistable {
    
    public:
        typedef double PixelT;
        typedef lsst::afw::image::Image<PixelT>::Pixel Pixel;
        typedef boost::shared_ptr<Kernel> PtrT;
        typedef boost::shared_ptr<lsst::afw::math::Function2<double> > SpatialFunctionPtr;
        typedef lsst::afw::math::Function2<double> SpatialFunction;
        typedef lsst::afw::math::NullFunction2<double> NullSpatialFunction;
        
        // Traits values for this class of Kernel
        typedef generic_kernel_tag kernel_fill_factor;

        explicit Kernel(int width=0, int height=0, unsigned int nKernelParams=0,
                        SpatialFunction const &spatialFunction=NullSpatialFunction());
        explicit Kernel(int width, int height, const std::vector<SpatialFunctionPtr> spatialFunctionList);

        virtual ~Kernel() {};
        
        const std::pair<int, int> getDimensions() const { return std::pair<int, int>(getWidth(), getHeight()); }

        /**
         * @brief Compute an image (pixellized representation of the kernel) in place
         *
         * x, y are ignored if there is no spatial function.
         *
         * @note computeNewImage has been retired; it doesn't need to be a member
         *
         * @throw lsst::pex::exceptions::InvalidParameterException if the image is the wrong size
         */
        virtual double computeImage(
            lsst::afw::image::Image<PixelT> &image,   ///< image whose pixels are to be set (output)
            bool doNormalize,   ///< normalize the image (so sum is 1)?
            double x = 0.0, ///< x (column position) at which to compute spatial function
            double y = 0.0  ///< y (row position) at which to compute spatial function
        ) const = 0;
    
        /**
         * @brief Return the Kernel's width
         */
        inline int getWidth() const {
            return _width;
        };
        
        /**
         * @brief Return the Kernel's height
         */
        inline int getHeight() const {
            return _height;
        };
        
        /**
         * @brief Return index of the center column
         */
        inline int getCtrX() const {
            return _ctrX;
        };

        /**
         * @brief Return index of the center row
         */
        inline int getCtrY() const {
            return _ctrY;
        };
        
        /**
         * @brief Return the number of kernel parameters (0 if none)
         */
        inline unsigned int getNKernelParameters() const {
            return _nKernelParams;
        };
    
        /**
         * @brief Return the number of spatial parameters (0 if not spatially varying)
         */
        inline int getNSpatialParameters() const {
            return this->isSpatiallyVarying() ? _spatialFunctionList[0]->getNParameters() : 0;
        };
        
        SpatialFunctionPtr getSpatialFunction(unsigned int index) const;

        virtual std::vector<double> getKernelParameters() const;
        
        inline void setCtrX(int ctrX) {
            _ctrX = ctrX;
        };
        
        inline void setCtrY(int ctrY) {
            _ctrY = ctrY;
        };
    
        /**
         * @brief Return the spatial parameters parameters (an empty vector if not spatially varying)
         */
        inline std::vector<std::vector<double> > getSpatialParameters() const {
            std::vector<std::vector<double> > spatialParams;
            std::vector<SpatialFunctionPtr>::const_iterator spFuncIter = _spatialFunctionList.begin();
            for ( ; spFuncIter != _spatialFunctionList.end(); ++spFuncIter) {
                spatialParams.push_back((*spFuncIter)->getParameters());
            }
            return spatialParams;
        };
            
        /**
         * @brief Return true iff the kernel is spatially varying (has a spatial function)
         */
        inline bool isSpatiallyVarying() const {
            return _spatialFunctionList.size() != 0;
        };
    
        /**
         * @brief Set the kernel parameters of a spatially invariant kernel.
         *
         * @throw lsst::pex::exceptions::RuntimeErrorException if the kernel has a spatial function
         * @throw lsst::pex::exceptions::InvalidParameterException if the params vector is the wrong length
         */
        inline void setKernelParameters(std::vector<double> const &params) {
            if (this->isSpatiallyVarying()) {
                throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Kernel is spatially varying");
            }
            const unsigned int nParams = this->getNKernelParameters();
            if (nParams != params.size()) {
                throw LSST_EXCEPT(lsst::pex::exceptions::InvalidParameterException,
                                  (boost::format("Number of parameters is wrong, saw %d expected %d") %
                                   nParams % params.size()).str());                                   
            }
            for (unsigned int ii = 0; ii < nParams; ++ii) {
                this->setKernelParameter(ii, params[ii]);
            }
        };
        
        /**
         * @brief Set the kernel parameters of a 2-component spatially invariant kernel.
         *
         * @note no checking of the kernel;  use the std::vector<double> form if you want that
         */
        inline void setKernelParameters(std::pair<double, double> const& params) {
            this->setKernelParameter(0, params.first);
            this->setKernelParameter(1, params.second);
        };
        
        void setSpatialParameters(const std::vector<std::vector<double> > params);

        void computeKernelParametersFromSpatialModel(std::vector<double> &kernelParams, double x, double y) const;
    
        virtual std::string toString(std::string prefix = "") const;

        virtual void toFile(std::string fileName) const;

    protected:
        virtual void setKernelParameter(unsigned int ind, double value) const;

        void setKernelParametersFromSpatialModel(double x, double y) const;
           
    private:
        LSST_PERSIST_FORMATTER(lsst::afw::formatters::KernelFormatter);

        int _width;
        int _height;
        int _ctrX;
        int _ctrY;
        unsigned int _nKernelParams;
        std::vector<SpatialFunctionPtr> _spatialFunctionList;
    };

    /**
     * @brief A list of Kernels
     *
     * This is basically a wrapper for an stl container, but defines
     * a conversion from KernelList<K1> to KernelList<K2> providing
     * that K1 is derived from K2 (or that K1 == K2)
     *
     * @ingroup afw
     */
    template<typename _KernelT=Kernel>
    class KernelList : public std::vector<typename _KernelT::PtrT> {
    public:
        typedef _KernelT KernelT;

        KernelList() { }
        
        template<typename Kernel2T>
        KernelList(const KernelList<Kernel2T>& k2l) :
            std::vector<typename KernelT::PtrT>(k2l.size())
            {
#if !defined(SWIG)
                BOOST_STATIC_ASSERT((
                                     boost::mpl::or_<
                                     boost::is_same<KernelT, Kernel2T>,
                                     boost::is_base_and_derived<KernelT, Kernel2T>
                                     >::value
                                    ));
#endif
                copy(k2l.begin(), k2l.end(), this->begin());
            }

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, unsigned int const version) {
            lsst::pex::logging::TTrace<5>("afw.serialize", "KernelList");
            ar & make_nvp("list",
                          boost::serialization::base_object<
                              std::vector<typename _KernelT::PtrT>
                          >(*this));
            lsst::pex::logging::TTrace<5>("afw.serialize", "KernelList end");
        };

    };

    /**
     * @brief A kernel created from an Image
     *
     * It has no adjustable parameters and so cannot be spatially varying.
     *
     * @ingroup afw
     */
    class FixedKernel : public Kernel {
    public:
        typedef boost::shared_ptr<FixedKernel> PtrT;

        explicit FixedKernel();

        explicit FixedKernel(
            lsst::afw::image::Image<PixelT> const &image
        );
        
        virtual ~FixedKernel() {};
    
        virtual double computeImage(
            lsst::afw::image::Image<PixelT> &image,
            bool doNormalize,
            double x = 0.0,
            double y = 0.0
        ) const;
            
        virtual std::string toString(std::string prefix = "") const;

    private:
        lsst::afw::image::Image<PixelT> _image;
        PixelT _sum;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, unsigned int const version) {
            lsst::pex::logging::TTrace<5>("afw.serialize", "FixedKernel");
            lsst::pex::logging::TTrace<6>("afw.serialize", "k");
            ar & make_nvp("k",
                          boost::serialization::base_object<Kernel>(*this));
            lsst::pex::logging::TTrace<6>("afw.serialize", "img");
            ar & make_nvp("img", _image);
            lsst::pex::logging::TTrace<6>("afw.serialize", "sum");
            ar & make_nvp("sum", _sum);
            lsst::pex::logging::TTrace<5>("afw.serialize", "FixedKernel done");
        };
    };
    
    
    /**
     * @brief A kernel described by a function.
     *
     * The function's x, y arguments are as follows:
     * * -getCtrX(), -getCtrY() for the lower left corner pixel
     * * 0, 0 for the center pixel
     * * (getWidth() - 1) - getCtrX(), (getHeight() - 1) - getCtrY() for the upper right pixel
     *
     * Note: each pixel is set to the value of the kernel function at the center of the pixel
     * (rather than averaging the function over the area of the pixel).
     *
     * @ingroup afw
     */
    class AnalyticKernel : public Kernel {
    public:
        typedef boost::shared_ptr<AnalyticKernel> PtrT;
        typedef lsst::afw::math::Function2<PixelT> KernelFunction;
        typedef lsst::afw::math::NullFunction2<PixelT> NullKernelFunction;
        typedef boost::shared_ptr<lsst::afw::math::Function2<PixelT> > KernelFunctionPtr;
        
        explicit AnalyticKernel();

        explicit AnalyticKernel(
            int width,
            int height,
            KernelFunction const &kernelFunction
        );
        
        explicit AnalyticKernel(
            int width,
            int height,
            KernelFunction const &kernelFunction,
            Kernel::SpatialFunction const &spatialFunction
        );
        
        explicit AnalyticKernel(
            int width,
            int height,
            KernelFunction const &kernelFunction,
            std::vector<Kernel::SpatialFunctionPtr> const &spatialFunctionList
        );
        
        virtual ~AnalyticKernel() {};
    
        virtual double computeImage(
            lsst::afw::image::Image<PixelT> &image,
            bool doNormalize,
            double x = 0.0,
            double y = 0.0
        ) const;

        virtual std::vector<double> getKernelParameters() const;
    
        virtual KernelFunctionPtr getKernelFunction() const;
            
        virtual std::string toString(std::string prefix = "") const;

    protected:
        virtual void setKernelParameter(unsigned int ind, double value) const;
    
    private:
        KernelFunctionPtr _kernelFunctionPtr;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, unsigned int const version) {
            lsst::pex::logging::TTrace<5>("afw.serialize", "AnalyticKernel");
            lsst::pex::logging::TTrace<6>("afw.serialize", "k");
            ar & make_nvp("k",
                          boost::serialization::base_object<Kernel>(*this));
            lsst::pex::logging::TTrace<6>("afw.serialize", "fn");
            ar & make_nvp("fn", _kernelFunctionPtr);
            lsst::pex::logging::TTrace<5>("afw.serialize", "AnalyticKernel end");
        };
    };
    
    
    /**
     * @brief A kernel that has only one non-zero pixel
     *
     * @ingroup afw
     */
    class DeltaFunctionKernel : public Kernel {
    public:
        typedef boost::shared_ptr<DeltaFunctionKernel> PtrT;
        // Traits values for this class of Kernel
        typedef deltafunction_kernel_tag kernel_fill_factor;

        explicit DeltaFunctionKernel(
            int width,
            int height,
            lsst::afw::image::PointI point
        );

        virtual double computeImage(
            lsst::afw::image::Image<PixelT> &image,
            bool doNormalize,
            double x = 0.0,
            double y = 0.0
        ) const;

        std::pair<int, int> getPixel() const { return _pixel; }

        virtual std::string toString(std::string prefix = "") const;

    private:
        std::pair<int, int> _pixel;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, unsigned int const version) {
            lsst::pex::logging::TTrace<5>("afw.serialize", "DeltaFunctionKernel");
            boost::serialization::void_cast_register<
                DeltaFunctionKernel, Kernel>(
                    static_cast<DeltaFunctionKernel*>(0),
                    static_cast<Kernel*>(0));
            lsst::pex::logging::TTrace<5>("afw.serialize", "DeltaFunctionKernel end");
        };
    };


    /**
     * @brief A kernel that is a linear combination of fixed basis kernels.
     * 
     * Convolution may be performed by first convolving the image
     * with each fixed kernel, then adding the resulting images using the (possibly
     * spatially varying) kernel coefficients.
     *
     * Warnings:
     * - This class does not normalize the individual basis kernels; they are used "as is".
     * - The kernels are assumed to be invariant; do not try to modify the basis kernels
     *   while using LinearCombinationKernel.
     *
     * @ingroup afw
     */
    class LinearCombinationKernel : public Kernel {
    public:
        typedef boost::shared_ptr<LinearCombinationKernel> PtrT;
        typedef lsst::afw::math::KernelList<Kernel> KernelList;

        explicit LinearCombinationKernel();

        explicit LinearCombinationKernel(
            KernelList const &kernelList,
            std::vector<double> const &kernelParameters
        );
        
        explicit LinearCombinationKernel(
            KernelList const &kernelList,
            Kernel::SpatialFunction const &spatialFunction
        );
        
        explicit LinearCombinationKernel(
            KernelList const &kernelList,
            std::vector<Kernel::SpatialFunctionPtr> const &spatialFunctionList
        );
        
        virtual ~LinearCombinationKernel() {};
    
        virtual double computeImage(
            lsst::afw::image::Image<PixelT> &image,
            bool doNormalize,
            double x = 0.0,
            double y = 0.0
        ) const;

        virtual std::vector<double> getKernelParameters() const;
                
        virtual KernelList const &getKernelList() const;
        
        void checkKernelList(const KernelList &kernelList) const;
        
        virtual std::string toString(std::string prefix = "") const;

    protected:
        virtual void setKernelParameter(unsigned int ind, double value) const;
    
    private:
        void _computeKernelImageList();
        KernelList _kernelList;
        std::vector<boost::shared_ptr<lsst::afw::image::Image<PixelT> > > _kernelImagePtrList;
        mutable std::vector<double> _kernelParams;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, unsigned int const version) {
            lsst::pex::logging::TTrace<5>("afw.serialize", "LinearCombinationKernel");
            lsst::pex::logging::TTrace<6>("afw.serialize", "k");
            ar & make_nvp("k",
                          boost::serialization::base_object<Kernel>(*this));
            lsst::pex::logging::TTrace<6>("afw.serialize", "klist");
            ar & make_nvp("klist", _kernelList);
            lsst::pex::logging::TTrace<6>("afw.serialize", "kimglist");
            ar & make_nvp("kimglist", _kernelImagePtrList);
            lsst::pex::logging::TTrace<6>("afw.serialize", "params");
            ar & make_nvp("params", _kernelParams);
            lsst::pex::logging::TTrace<5>("afw.serialize", "LinearCombinationKernel end");
        };
    };

    
    /**
     * @brief A kernel described by a pair of functions: func(x, y) = colFunc(x) * rowFunc(y)
     *
     * The function's x, y arguments are as follows:
     * * -getCtrX(), -getCtrY() for the lower left corner pixel
     * * 0, 0 for the center pixel
     * * (getWidth() - 1) - getCtrX(), (getHeight() - 1) - getCtrY() for the upper right pixel
     *
     * Note: each pixel is set to the value of the kernel function at the center of the pixel
     * (rather than averaging the function over the area of the pixel).
     *
     * @ingroup afw
     */
    class SeparableKernel : public Kernel {
    public:
        typedef boost::shared_ptr<SeparableKernel> Ptr;
        typedef boost::shared_ptr<SeparableKernel> PtrT;
        typedef lsst::afw::math::Function1<PixelT> KernelFunction;
        typedef lsst::afw::math::NullFunction1<PixelT> NullKernelFunction;
        typedef boost::shared_ptr<KernelFunction> KernelFunctionPtr;
        
        explicit SeparableKernel(
            int width=0, int height=0,
            KernelFunction const& kernelColFunction=NullKernelFunction(),
            KernelFunction const& kernelRowFunction=NullKernelFunction(),
            Kernel::SpatialFunction const& spatialFunction=NullSpatialFunction()
        );
        
        explicit SeparableKernel(int width, int height,
                                 KernelFunction const& kernelColFunction,
                                 KernelFunction const& kernelRowFunction,
                                 std::vector<Kernel::SpatialFunctionPtr> const& spatialFunctionList);
        virtual ~SeparableKernel() {};
    
        virtual double computeImage(
            lsst::afw::image::Image<PixelT> &image,
            bool doNormalize,
            double x = 0.0,
            double y = 0.0
        ) const;

        double computeVectors(
            std::vector<PixelT> &colList,
            std::vector<PixelT> &rowList,
            bool doNormalize,
            double x = 0.0,
            double y = 0.0
        ) const;
        
        virtual std::vector<double> getKernelParameters() const;
    
        KernelFunctionPtr getKernelColFunction() const;

        KernelFunctionPtr getKernelRowFunction() const;

        virtual std::string toString(std::string prefix = "") const;

    protected:
        virtual void setKernelParameter(unsigned int ind, double value) const;
    
    private:
        double basicComputeVectors(
            std::vector<PixelT> &colList,
            std::vector<PixelT> &rowList,
            bool doNormalize
        ) const;

        KernelFunctionPtr _kernelColFunctionPtr;
        KernelFunctionPtr _kernelRowFunctionPtr;
        mutable std::vector<PixelT> _localColList;  // used by computeImage
        mutable std::vector<PixelT> _localRowList;

    private:
        friend class boost::serialization::access;
        template <class Archive>
        void serialize(Archive& ar, unsigned int const version) {
            lsst::pex::logging::TTrace<5>("afw.serialize", "SeparableKernel");
            lsst::pex::logging::TTrace<6>("afw.serialize", "k");
            ar & make_nvp("k",
                          boost::serialization::base_object<Kernel>(*this));
            lsst::pex::logging::TTrace<6>("afw.serialize", "colfn");
            ar & make_nvp("colfn", _kernelColFunctionPtr);
            lsst::pex::logging::TTrace<6>("afw.serialize", "rowfn");
            ar & make_nvp("rowfn", _kernelRowFunctionPtr);
            lsst::pex::logging::TTrace<6>("afw.serialize", "cols");
            ar & make_nvp("cols", _localColList);
            lsst::pex::logging::TTrace<6>("afw.serialize", "rows");
            ar & make_nvp("rows", _localRowList);
            lsst::pex::logging::TTrace<5>("afw.serialize", "SeparableKernel end");
            };
    };
    
}}}   // lsst:afw::math

namespace boost {
namespace serialization {

template <class Archive>
inline void save_construct_data(
    Archive& ar, lsst::afw::math::DeltaFunctionKernel const* k,
    unsigned int const file_version) {
    lsst::pex::logging::TTrace<5>("afw.serialize", "DeltaFunctionKernel scd");
    int width = k->getWidth();
    int height = k->getHeight();
    int x = k->getPixel().first;
    int y = k->getPixel().second;
    lsst::pex::logging::TTrace<6>("afw.serialize", "width");
    ar << make_nvp("width", width);
    lsst::pex::logging::TTrace<6>("afw.serialize", "height");
    ar << make_nvp("height", height);
    lsst::pex::logging::TTrace<6>("afw.serialize", "pixX");
    ar << make_nvp("pixX", x);
    lsst::pex::logging::TTrace<6>("afw.serialize", "pixY");
    ar << make_nvp("pixY", y);
    lsst::pex::logging::TTrace<5>("afw.serialize", "DeltaFunctionKernel scd end");
};

template <class Archive>
inline void load_construct_data(
    Archive& ar, lsst::afw::math::DeltaFunctionKernel* k,
    unsigned int const file_version) {
    lsst::pex::logging::TTrace<5>("afw.serialize", "DeltaFunctionKernel lcd");
    int width;
    int height;
    int x;
    int y;
    lsst::pex::logging::TTrace<6>("afw.serialize", "width");
    ar >> make_nvp("width", width);
    lsst::pex::logging::TTrace<6>("afw.serialize", "height");
    ar >> make_nvp("height", height);
    lsst::pex::logging::TTrace<6>("afw.serialize", "pixX");
    ar >> make_nvp("pixX", x);
    lsst::pex::logging::TTrace<6>("afw.serialize", "pixY");
    ar >> make_nvp("pixY", y);
    lsst::pex::logging::TTrace<6>("afw.serialize", "placement new");
    ::new(k) lsst::afw::math::DeltaFunctionKernel(
        width, height, lsst::afw::image::PointI(x, y));
    lsst::pex::logging::TTrace<5>("afw.serialize", "DeltaFunctionKernel lcd end");
};

}}

#endif // !defined(LSST_AFW_MATH_KERNEL_H)
