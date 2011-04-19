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
 
/**
 * \file
 * \brief Implementation for ImageBase and Image
 */
#include <iostream>
#include "boost/mpl/vector.hpp"
#include "boost/lambda/lambda.hpp"
#include "boost/format.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/gil/gil_all.hpp"

#include "lsst/pex/exceptions.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/image/ImageAlgorithm.h"
#include "lsst/afw/image/Wcs.h"
#include "lsst/afw/image/fits/fits_io.h"
#include "lsst/afw/image/fits/fits_io_mpl.h"

namespace image = lsst::afw::image;
namespace geom = lsst::afw::geom;

/************************************************************************************************************/
template <typename PixelT>
typename image::ImageBase<PixelT>::_view_t image::ImageBase<PixelT>::_allocateView(
    geom::Extent2I const & dimensions,
    Manager::Ptr & manager
) {    
    std::pair<Manager::Ptr,PixelT*> r = ndarray::SimpleManager<PixelT>::allocate(
        dimensions.getX() * dimensions.getY()
    );
    manager = r.first;
    return boost::gil::interleaved_view(
        dimensions.getX(), dimensions.getY(), 
        (typename _view_t::value_type* )r.second, 
        dimensions.getX()*sizeof(PixelT)
    );
}
template <typename PixelT>
typename image::ImageBase<PixelT>::_view_t image::ImageBase<PixelT>::_makeSubView(
    geom::Extent2I const & dimensions, geom::Extent2I const & offset, const _view_t & view
) {
    if (offset.getX() < 0 || offset.getY() < 0 ||
        offset.getX() + dimensions.getX() > view.width() || 
        offset.getY() + dimensions.getY() > view.height()
    ) {
        throw LSST_EXCEPT(
            lsst::pex::exceptions::LengthErrorException,
            (boost::format("Box2I(Point2I(%d,%d),Extent2I(%d,%d)) doesn't fit in image %dx%d") %
                offset.getX() % offset.getY() % 
                dimensions.getX() % dimensions.getY() %
                view.width() % view.height()
            ).str()
        );
    }
    return boost::gil::subimage_view(
        view, 
        offset.getX(), offset.getY(),
        dimensions.getX(), dimensions.getY()
    );
}

/**
 * Allocator Constructor
 *
 * allocate a new image with the specified dimensions.
 * Sets origin at (0,0)
 */
template <typename PixelT>
image::ImageBase<PixelT>::ImageBase(
    geom::Extent2I const & dimensions
) : lsst::daf::data::LsstBase(typeid(this)),
    _origin(0,0), _manager(),
    _gilView(_allocateView(dimensions, _manager))
{}

/**
 * Allocator Constructor
 *
 * allocate a new image with the specified dimensions and origin
 */
template <typename PixelT>
image::ImageBase<PixelT>::ImageBase(
    geom::Box2I const & bbox
) : lsst::daf::data::LsstBase(typeid(this)),
    _origin(bbox.getMin()), _manager(),
    _gilView(_allocateView(bbox.getDimensions(), _manager))
{}

/**
 * Copy constructor.
 *
 * \note Unless \c deep is \c true, the new %image will share the old %image's pixels;
 * this may not be what you want.  See also operator<<=() to copy pixels between Image%s
 */
template<typename PixelT>
image::ImageBase<PixelT>::ImageBase(
    ImageBase const& rhs, ///< Right-hand-side %image
    bool const deep       ///< If false, new ImageBase shares storage with rhs;
                          ///< if true make a new, standalone, ImageBase
) :
    lsst::daf::data::LsstBase(typeid(this)),
    _origin(rhs._origin),
    _manager(rhs._manager),
    _gilView(rhs._gilView)
{
    if (deep) {
        ImageBase tmp(getBBox(PARENT));
        tmp <<= *this;                  // now copy the pixels
        swap(tmp);
    }
}

/**
 * Copy constructor to make a copy of part of an %image.
 *
 * The bbox ignores X0/Y0 if origin == LOCAL, and uses it if origin == PARENT.
 *
 * \note Unless \c deep is \c true, the new %image will share the old %image's pixels;
 * this is probably what you want 
 */
template<typename PixelT>
image::ImageBase<PixelT>::ImageBase(
    ImageBase const& rhs, ///< Right-hand-side %image
    geom::Box2I const& bbox,     ///< Specify desired region
    ImageOrigin const origin,   ///< Specify the coordinate system of the bbox
    bool const deep       ///< If false, new ImageBase shares storage with rhs;
                          ///< if true make a new, standalone, ImageBase
) :
    lsst::daf::data::LsstBase(typeid(this)),
    _origin((origin==PARENT) ? bbox.getMin(): rhs._origin + geom::Extent2I(bbox.getMin())),
    _manager(rhs._manager), // reference counted pointer, don't copy pixels
    _gilView(_makeSubView(bbox.getDimensions(), _origin - rhs._origin, rhs._gilView))
{
    
    if (deep) {
        ImageBase tmp(getBBox(PARENT));        
        tmp <<= *this;                  // now copy the pixels
        swap(tmp);
    }
}

/**
 *  Construction from ndarray::Array and NumPy.
 *
 *  \note ndarray and NumPy indexes are ordered (y,x), but Image indices are ordered (x,y).
 *
 *  Unless deep is true, the new image will share memory with the array if the the
 *  dimension is contiguous in memory.  If the last dimension is not contiguous, the array
 *  will be deep-copied in Python, but the constructor will fail to compile in pure C++.
 */
template<typename PixelT>
image::ImageBase<PixelT>::ImageBase(Array const & array, bool deep, geom::Point2I const & xy0) :
    lsst::daf::data::LsstBase(typeid(this)),
    _origin(xy0),
    _manager(array.getManager()),
    _gilView(
        boost::gil::interleaved_view(
            array.template getSize<1>(), array.template getSize<0>(), 
            (typename _view_t::value_type* )array.getData(), 
            array.template getStride<0>() * sizeof(PixelT)
        )
    )
{
    if (deep) {
        ImageBase tmp(*this, true);
        swap(tmp);
    }
}

/// Shallow assignment operator.
///
/// \note that this has the effect of making the lhs share pixels with the rhs which may
/// not be what you intended;  to copy the pixels, use operator<<=()
///
/// \note this behaviour is required to make the swig interface work, otherwise I'd
/// declare this function private
template<typename PixelT>
image::ImageBase<PixelT>& image::ImageBase<PixelT>::operator=(ImageBase const& rhs) {
    ImageBase tmp(rhs);
    swap(tmp);                          // See Meyers, Effective C++, Item 11
    
    return *this;
}

/// Set the lhs's %pixel values to equal the rhs's
template<typename PixelT>
void image::ImageBase<PixelT>::operator<<=(ImageBase const& rhs) {
    if (getDimensions() != rhs.getDimensions()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Dimension mismatch: %dx%d v. %dx%d") %
                              getWidth() % getHeight() % rhs.getWidth() % rhs.getHeight()).str());
    }
    copy_pixels(rhs._gilView, _gilView);
}

/// Return a reference to the pixel <tt>(x, y)</tt>
template<typename PixelT>
typename image::ImageBase<PixelT>::PixelReference image::ImageBase<PixelT>::operator()(int x, int y) {
    return const_cast<typename image::ImageBase<PixelT>::PixelReference>(
        static_cast<typename image::ImageBase<PixelT>::PixelConstReference>(_gilView(x, y)[0])
    );
}

/// Return a reference to the pixel <tt>(x, y)</tt> with bounds checking
template<typename PixelT>
typename image::ImageBase<PixelT>::PixelReference image::ImageBase<PixelT>::operator()(
        int x,
        int y,
        image::CheckIndices const& check
                                                                                      )
{
    if (check && (x < 0 || x >= getWidth() || y < 0 || y >= getHeight())) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Index (%d, %d) is out of range [0--%d], [0--%d]") %
                           x % y % (getWidth() - 1) % (getHeight() - 1)).str());
    }
                                          
    return const_cast<typename image::ImageBase<PixelT>::PixelReference>(
        static_cast<typename image::ImageBase<PixelT>::PixelConstReference>(_gilView(x, y)[0])
    );
}

/// Return a const reference to the pixel <tt>(x, y)</tt>
template<typename PixelT>
typename image::ImageBase<PixelT>::PixelConstReference
    image::ImageBase<PixelT>::operator()(int x, int y) const {
    return _gilView(x, y)[0];
}

/// Return a const reference to the pixel <tt>(x, y)</tt> with bounds checking
template<typename PixelT>
typename image::ImageBase<PixelT>::PixelConstReference
    image::ImageBase<PixelT>::operator()(int x, int y, image::CheckIndices const& check) const {
    if (check && (x < 0 || x >= getWidth() || y < 0 || y >= getHeight())) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Index (%d, %d) is out of range [0--%d], [0--%d]") %
                           x % y % (this->getWidth() - 1) % (this->getHeight() - 1)).str());
    }
                                          
    return _gilView(x, y)[0];
}

template<typename PixelT>
void image::ImageBase<PixelT>::swap(ImageBase &rhs) {
    using std::swap;                    // See Meyers, Effective C++, Item 25
    
    swap(_manager, rhs._manager);   // just swapping the pointers
    swap(_gilView, rhs._gilView);
    swap(_origin, rhs._origin);
}

template<typename PixelT>
void image::swap(ImageBase<PixelT>& a, ImageBase<PixelT>& b) {
    a.swap(b);
}

template <typename PixelT>
typename image::ImageBase<PixelT>::Array image::ImageBase<PixelT>::getArray() {
    int rowStride = reinterpret_cast<PixelT*>(row_begin(1)) - reinterpret_cast<PixelT*>(row_begin(0));
    typedef lsst::ndarray::detail::ArrayAccess<Array> ArrayAccess;
    typedef typename ArrayAccess::Core ArrayCore;
    return ArrayAccess::construct(
        reinterpret_cast<PixelT*>(row_begin(0)),
        ArrayCore::create(
            lsst::ndarray::makeVector(getHeight(), getWidth()),
            lsst::ndarray::makeVector(rowStride, 1),
            this->_manager
        )
    );
}


template <typename PixelT>
typename image::ImageBase<PixelT>::ConstArray image::ImageBase<PixelT>::getArray() const {
    int rowStride = reinterpret_cast<PixelT*>(row_begin(1)) - reinterpret_cast<PixelT*>(row_begin(0));
    typedef lsst::ndarray::detail::ArrayAccess<Array> ArrayAccess;
    typedef typename ArrayAccess::Core ArrayCore;
    return ArrayAccess::construct(
        reinterpret_cast<PixelT*>(row_begin(0)),
        ArrayCore::create(
            lsst::ndarray::makeVector(getHeight(), getWidth()),
            lsst::ndarray::makeVector(rowStride, 1),
            this->_manager
        )
    );
}
//
// Iterators
//
/// Return an STL compliant iterator to the start of the %image
///
/// Note that this isn't especially efficient; see \link imageIterators\endlink for
/// a discussion
template<typename PixelT>
typename image::ImageBase<PixelT>::iterator image::ImageBase<PixelT>::begin() const {
    return _gilView.begin();
}

/// Return an STL compliant iterator to the end of the %image
template<typename PixelT>
typename image::ImageBase<PixelT>::iterator image::ImageBase<PixelT>::end() const {
    return _gilView.end();
}

/// Return an STL compliant reverse iterator to the start of the %image
template<typename PixelT>
typename image::ImageBase<PixelT>::reverse_iterator image::ImageBase<PixelT>::rbegin() const {
    return _gilView.rbegin();
}

/// Return an STL compliant reverse iterator to the end of the %image
template<typename PixelT>
typename image::ImageBase<PixelT>::reverse_iterator image::ImageBase<PixelT>::rend() const {
    return _gilView.rend();
}

/// Return an STL compliant iterator at the point <tt>(x, y)</tt>
template<typename PixelT>
typename image::ImageBase<PixelT>::iterator image::ImageBase<PixelT>::at(int x, int y) const {
    return _gilView.at(x, y);
}

/// Return a fast STL compliant iterator to the start of the %image which must be contiguous
///
/// \exception lsst::pex::exceptions::Runtime
/// Argument \a contiguous is false, or the pixels are not in fact contiguous
template<typename PixelT>
typename image::ImageBase<PixelT>::fast_iterator image::ImageBase<PixelT>::begin(
    bool contiguous         ///< Pixels are contiguous (must be true)
) const {
    if (!contiguous) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                          "Only contiguous == true makes sense");
    }
    if (!this->isContiguous()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                          "Image's pixels are not contiguous");
    }

    return row_begin(0);
}

/// Return a fast STL compliant iterator to the end of the %image which must be contiguous
///
/// \exception lsst::pex::exceptions::Runtime
/// Argument \a contiguous is false, or the pixels are not in fact contiguous
template<typename PixelT>
typename image::ImageBase<PixelT>::fast_iterator image::ImageBase<PixelT>::end(
    bool contiguous         ///< Pixels are contiguous (must be true)
) const {
    if (!contiguous) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                          "Only contiguous == true makes sense"); 
    }
    if (!this->isContiguous()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, 
                          "Image's pixels are not contiguous");
    }

    return row_end(getHeight()-1);
}

/************************************************************************************************************/
/// Set the %image's pixels to rhs
template<typename PixelT>
image::ImageBase<PixelT>& image::ImageBase<PixelT>::operator=(PixelT const rhs) {
    fill_pixels(_gilView, rhs);

    return *this;
}

/************************************************************************************************************/
//
// On to Image itself.  ctors, cctors, and operator=
//
/**
 * Create an initialised Image of the specified size 
 *
 * \note Many lsst::afw::image and lsst::afw::math objects define a \c dimensions member
 * which may be conveniently used to make objects of an appropriate size
 */
template<typename PixelT>
image::Image<PixelT>::Image(geom::Extent2I const & dimensions, ///< Number of columns, rows
                            PixelT initialValue ///< Initial value
                           ) :
    image::ImageBase<PixelT>(dimensions) 
{
    *this = initialValue;
}

/**
 * Create an initialized Image of the specified size
 */
template<typename PixelT>
image::Image<PixelT>::Image(geom::Box2I const & bbox, ///< dimensions and origin of desired Image
                            PixelT initialValue ///< Initial value
                           ) :
    image::ImageBase<PixelT>(bbox) {
    *this = initialValue;
}

/**
 * Copy constructor.
 *
 * \note Unless \c deep is \c true, the new %image will share the old %image's pixels;
 * this may not be what you want.  See also operator<<=() to copy pixels between Image%s
 */
template<typename PixelT>
image::Image<PixelT>::Image(Image const& rhs, ///< Right-hand-side Image
                            bool const deep       ///< If false, new Image shares storage with rhs; if true
                                                  ///< make a new, standalone, ImageBase
                           ) :
    image::ImageBase<PixelT>(rhs, deep) {}

/**
 * Copy constructor to make a copy of part of an Image.
 *
 * The bbox ignores X0/Y0 if origin == LOCAL, and uses it if origin == PARENT.
 *
 * \note Unless \c deep is \c true, the new %image will share the old %image's pixels;
 * this is probably what you want 
 */
template<typename PixelT>
image::Image<PixelT>::Image(Image const& rhs,  ///< Right-hand-side Image
                            geom::Box2I const& bbox,  ///< Specify desired region
                            ImageOrigin const origin, ///< Coordinate system of the bbox
                            bool const deep    ///< If false, new ImageBase shares storage with rhs; if true
                                                   ///< make a new, standalone, ImageBase
                           ) :
    image::ImageBase<PixelT>(rhs, bbox, origin, deep) {}

/// Set the %image's pixels to rhs
template<typename PixelT>
image::Image<PixelT>& image::Image<PixelT>::operator=(PixelT const rhs) {
    this->ImageBase<PixelT>::operator=(rhs);

    return *this;
}

/// Assignment operator.
///
/// \note that this has the effect of making the lhs share pixels with the rhs which may
/// not be what you intended;  to copy the pixels, use operator<<=()
///
/// \note this behaviour is required to make the swig interface work, otherwise I'd
/// declare this function private
template<typename PixelT>
image::Image<PixelT>& image::Image<PixelT>::operator=(Image const& rhs) {
    this->ImageBase<PixelT>::operator=(rhs);
    
    return *this;
}

/************************************************************************************************************/
/**
 * Construct an Image from a FITS file
 *
 * @note We use FITS numbering, so the first HDU is HDU 1, not 0 (although we're nice and interpret 0 meaning
 * the first HDU, i.e. HDU 1).  I.e. if you have a PDU, the numbering is thus [PDU, HDU2, HDU3, ...]
 */
template<typename PixelT>
image::Image<PixelT>::Image(std::string const& fileName, ///< File to read
                            int const hdu,               ///< Desired HDU
                            lsst::daf::base::PropertySet::Ptr metadata, ///< file metadata (may point to NULL)
                            geom::Box2I const& bbox,                           ///< Only read these pixels
                            ImageOrigin const origin    ///< specify the coordinate system of the bbox
                           ) :
    image::ImageBase<PixelT>() {

    typedef boost::mpl::vector<
        unsigned char, 
        unsigned short, 
        short, 
        int,
        unsigned int,
        float,
        double
    > fits_image_types;

    if (!boost::filesystem::exists(fileName)) {
        throw LSST_EXCEPT(lsst::pex::exceptions::NotFoundException,
                          (boost::format("File %s doesn't exist") % fileName).str());
    }
    if (!metadata) {
        metadata = lsst::daf::base::PropertySet::Ptr(new lsst::daf::base::PropertyList);
    }
    if (!fits_read_image<fits_image_types>(fileName, *this, metadata, hdu, bbox, origin)) {
        throw LSST_EXCEPT(image::FitsException,
                          (boost::format("Failed to read %s HDU %d") % fileName % hdu).str());
    }
}

/**
 * Write an Image to the specified file
 */
template<typename PixelT>
void image::Image<PixelT>::writeFits(
    std::string const& fileName,                ///< File to write
    boost::shared_ptr<const lsst::daf::base::PropertySet> metadata_i, //!< metadata to write to header or NULL
    std::string const& mode                     //!< "w" to write a new file; "a" to append
) const {
    using lsst::daf::base::PropertySet;

    if (mode == "pdu") {
        image::fits_write_image(fileName, *this, metadata_i, mode);
        return;
    }

    lsst::daf::base::PropertySet::Ptr metadata;
    PropertySet::Ptr wcsAMetadata =
        image::detail::createTrivialWcsAsPropertySet(image::detail::wcsNameForXY0,
                                                     this->getX0(), this->getY0());
    
    if (metadata_i) {
        metadata = metadata_i->deepCopy();
        metadata->combine(wcsAMetadata);
    } else {
        metadata = wcsAMetadata;
    }

    image::fits_write_image(fileName, *this, metadata, mode);
}

/************************************************************************************************************/

template<typename PixelT>
void image::Image<PixelT>::swap(Image &rhs) {
    using std::swap;                    // See Meyers, Effective C++, Item 25
    ImageBase<PixelT>::swap(rhs);
    ;                                   // no private variables to swap
}

template<typename PixelT>
void image::swap(Image<PixelT>& a, Image<PixelT>& b) {
    a.swap(b);
}

/************************************************************************************************************/
//
// N.b. We could use the STL, but I find boost::lambda clearer, and more easily extended
// to e.g. setting random numbers
//    transform_pixels(_gilView, _gilView, lambda::ret<PixelT>(lambda::_1 + rhs));
// is equivalent to
//    transform_pixels(_gilView, _gilView, std::bind2nd(std::plus<PixelT>(), rhs));
//
namespace bl = boost::lambda;

/// Add scalar rhs to lhs
template<typename PixelT>
void image::Image<PixelT>::operator+=(PixelT const rhs) {
    transform_pixels(_getRawView(), _getRawView(), bl::ret<PixelT>(bl::_1 + rhs));
}

/// Add Image rhs to lhs
template<typename PixelT>
void image::Image<PixelT>::operator+=(Image<PixelT> const& rhs) {
    if (this->getDimensions() != rhs.getDimensions()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Images are of different size, %dx%d v %dx%d") %
                           this->getWidth() % this->getHeight() % rhs.getWidth() % rhs.getHeight()).str());
    }
    transform_pixels(_getRawView(), rhs._getRawView(), _getRawView(), bl::ret<PixelT>(bl::_1 + bl::_2));
}

/**
 * @brief Add a Function2(x, y) to an Image
 */
template<typename PixelT>
void image::Image<PixelT>::operator+=(
        lsst::afw::math::Function2<double> const& function ///< function to add
                                     ) {
    for (int y = 0; y != this->getHeight(); ++y) {
        double const yPos = this->indexToPosition(y, image::Y);
        double xPos = this->indexToPosition(0, image::X);
        for (typename Image<PixelT>::x_iterator ptr = this->row_begin(y), end = this->row_end(y);
             ptr != end; ++ptr, ++xPos) {            
            *ptr += function(xPos, yPos);
        }
    }
}

/// Add Image c*rhs to lhs
template<typename PixelT>
void image::Image<PixelT>::scaledPlus(double const c, Image<PixelT> const& rhs) {
    if (this->getDimensions() != rhs.getDimensions()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Images are of different size, %dx%d v %dx%d") %
                           this->getWidth() % this->getHeight() % rhs.getWidth() % rhs.getHeight()).str());
    }
    transform_pixels(_getRawView(), rhs._getRawView(), _getRawView(),
                     bl::ret<PixelT>(bl::_1 + bl::ret<PixelT>(c*bl::_2)));
}

/// Subtract scalar rhs from lhs
template<typename PixelT>
void image::Image<PixelT>::operator-=(PixelT const rhs) {
    transform_pixels(_getRawView(), _getRawView(), bl::ret<PixelT>(bl::_1 - rhs));
}

/// Subtract Image rhs from lhs
template<typename PixelT>
void image::Image<PixelT>::operator-=(Image<PixelT> const& rhs) {
    if (this->getDimensions() != rhs.getDimensions()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Images are of different size, %dx%d v %dx%d") %
                           this->getWidth() % this->getHeight() % rhs.getWidth() % rhs.getHeight()).str());
    }
    transform_pixels(_getRawView(), rhs._getRawView(), _getRawView(), bl::ret<PixelT>(bl::_1 - bl::_2));
}

/// Subtract Image c*rhs from lhs
template<typename PixelT>
void image::Image<PixelT>::scaledMinus(double const c, Image<PixelT> const& rhs) {
    if (this->getDimensions() != rhs.getDimensions()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Images are of different size, %dx%d v %dx%d") %
                           this->getWidth() % this->getHeight() % rhs.getWidth() % rhs.getHeight()).str());
    }
    transform_pixels(_getRawView(), rhs._getRawView(), _getRawView(),
                     bl::ret<PixelT>(bl::_1 - bl::ret<PixelT>(c*bl::_2)));
}

/**
 * @brief Subtract a Function2(x, y) from an Image
 */
template<typename PixelT>
void image::Image<PixelT>::operator-=(
        lsst::afw::math::Function2<double> const& function ///< function to add
                                     ) {
    for (int y = 0; y != this->getHeight(); ++y) {
        double const yPos = this->indexToPosition(y, image::Y);
        double xPos = this->indexToPosition(0, image::X);
        for (typename Image<PixelT>::x_iterator ptr = this->row_begin(y), end = this->row_end(y);
             ptr != end; ++ptr, ++xPos) {            
            *ptr -= function(xPos, yPos);
        }
    }
}

/// Multiply lhs by scalar rhs
template<typename PixelT>
void image::Image<PixelT>::operator*=(PixelT const rhs) {
    transform_pixels(_getRawView(), _getRawView(), bl::ret<PixelT>(bl::_1 * rhs));
}

/// Multiply lhs by Image rhs (i.e. %pixel-by-%pixel multiplication)
template<typename PixelT>
void image::Image<PixelT>::operator*=(Image<PixelT> const& rhs) {
    if (this->getDimensions() != rhs.getDimensions()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Images are of different size, %dx%d v %dx%d") %
                           this->getWidth() % this->getHeight() % rhs.getWidth() % rhs.getHeight()).str());
    }
    transform_pixels(_getRawView(), rhs._getRawView(), _getRawView(), bl::ret<PixelT>(bl::_1 * bl::_2));
}

/// Multiply lhs by Image c*rhs (i.e. %pixel-by-%pixel multiplication)
template<typename PixelT>
void image::Image<PixelT>::scaledMultiplies(double const c, Image<PixelT> const& rhs) {
    if (this->getDimensions() != rhs.getDimensions()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Images are of different size, %dx%d v %dx%d") %
                           this->getWidth() % this->getHeight() % rhs.getWidth() % rhs.getHeight()).str());
    }
    transform_pixels(_getRawView(), rhs._getRawView(), _getRawView(),
                     bl::ret<PixelT>(bl::_1 * bl::ret<PixelT>(c*bl::_2)));
}

/// Divide lhs by scalar rhs
///
/// \note Floating point types implement this by multiplying by the 1/rhs
template<typename PixelT>
void image::Image<PixelT>::operator/=(PixelT const rhs) {
    transform_pixels(_getRawView(), _getRawView(), bl::ret<PixelT>(bl::_1 / rhs));
}
//
// Specialize float and double for efficiency
//
namespace lsst { namespace afw { namespace image {
template<>
void Image<double>::operator/=(double const rhs) {
    double const irhs = 1/rhs;
    *this *= irhs;
}

template<>
void Image<float>::operator/=(float const rhs) {
    float const irhs = 1/rhs;
    *this *= irhs;
}
}}}

/// Divide lhs by Image rhs (i.e. %pixel-by-%pixel division)
template<typename PixelT>
void image::Image<PixelT>::operator/=(Image<PixelT> const& rhs) {
    if (this->getDimensions() != rhs.getDimensions()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Images are of different size, %dx%d v %dx%d") %
                           this->getWidth() % this->getHeight() % rhs.getWidth() % rhs.getHeight()).str());
    }
    transform_pixels(_getRawView(), rhs._getRawView(), _getRawView(), bl::ret<PixelT>(bl::_1 / bl::_2));
}

/// Divide lhs by Image c*rhs (i.e. %pixel-by-%pixel division)
template<typename PixelT>
void image::Image<PixelT>::scaledDivides(double const c, Image<PixelT> const& rhs) {
    if (this->getDimensions() != rhs.getDimensions()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::LengthErrorException,
                          (boost::format("Images are of different size, %dx%d v %dx%d") %
                           this->getWidth() % this->getHeight() % rhs.getWidth() % rhs.getHeight()).str());
    }
    transform_pixels(_getRawView(), rhs._getRawView(), _getRawView(),
                     bl::ret<PixelT>(bl::_1 / bl::ret<PixelT>(c*bl::_2)));
}

/************************************************************************************************************/

namespace {
/*
 * Worker routine for manipulating images;
 */
template<typename LhsPixelT, typename RhsPixelT>
struct plusEq : public lsst::afw::image::pixelOp2<LhsPixelT, RhsPixelT> {
    LhsPixelT operator()(LhsPixelT lhs, RhsPixelT rhs) const {
        return static_cast<LhsPixelT>(lhs + rhs);
    }
};

template<typename LhsPixelT, typename RhsPixelT>
struct minusEq : public lsst::afw::image::pixelOp2<LhsPixelT, RhsPixelT> {
    LhsPixelT operator()(LhsPixelT lhs, RhsPixelT rhs) const {
        return static_cast<LhsPixelT>(lhs - rhs);
    }
};

template<typename LhsPixelT, typename RhsPixelT>
struct timesEq : public lsst::afw::image::pixelOp2<LhsPixelT, RhsPixelT> {
    LhsPixelT operator()(LhsPixelT lhs, RhsPixelT rhs) const {
        return static_cast<LhsPixelT>(lhs*rhs);
    }
};

template<typename LhsPixelT, typename RhsPixelT>
struct divideEq : public lsst::afw::image::pixelOp2<LhsPixelT, RhsPixelT> {
    LhsPixelT operator()(LhsPixelT lhs, RhsPixelT rhs) const {
        return static_cast<LhsPixelT>(lhs/rhs);
    }
};
}

/// Add lhs to Image rhs (i.e. %pixel-by-%pixel addition) where types are different
///
template<typename LhsPixelT, typename RhsPixelT>
void image::operator+=(image::Image<LhsPixelT> &lhs, image::Image<RhsPixelT> const& rhs) {
    image::for_each_pixel(lhs, rhs, plusEq<LhsPixelT, RhsPixelT>());
}

/// Subtract lhs from Image rhs (i.e. %pixel-by-%pixel subtraction) where types are different
///
template<typename LhsPixelT, typename RhsPixelT>
void image::operator-=(image::Image<LhsPixelT> &lhs, image::Image<RhsPixelT> const& rhs) {
    image::for_each_pixel(lhs, rhs, minusEq<LhsPixelT, RhsPixelT>());
}

/// Multiply lhs by Image rhs (i.e. %pixel-by-%pixel multiplication) where types are different
///
template<typename LhsPixelT, typename RhsPixelT>
void image::operator*=(image::Image<LhsPixelT> &lhs, image::Image<RhsPixelT> const& rhs) {
    image::for_each_pixel(lhs, rhs, timesEq<LhsPixelT, RhsPixelT>());
}

/// Divide lhs by Image rhs (i.e. %pixel-by-%pixel division) where types are different
///
template<typename LhsPixelT, typename RhsPixelT>
void image::operator/=(image::Image<LhsPixelT> &lhs, image::Image<RhsPixelT> const& rhs) {
    image::for_each_pixel(lhs, rhs, divideEq<LhsPixelT, RhsPixelT>());
}

/************************************************************************************************************/
//
// Explicit instantiations
//
/// \cond
#define INSTANTIATE_OPERATOR(OP_EQ, T) \
   template void image::operator OP_EQ(image::Image<T>& lhs, image::Image<boost::uint16_t> const& rhs); \
   template void image::operator OP_EQ(image::Image<T>& lhs, image::Image<int> const& rhs); \
   template void image::operator OP_EQ(image::Image<T>& lhs, image::Image<float> const& rhs); \
   template void image::operator OP_EQ(image::Image<T>& lhs, image::Image<double> const& rhs)

#define INSTANTIATE(T) \
   template class image::ImageBase<T>; \
   template class image::Image<T>; \
   INSTANTIATE_OPERATOR(+=, T); \
   INSTANTIATE_OPERATOR(-=, T); \
   INSTANTIATE_OPERATOR(*=, T); \
   INSTANTIATE_OPERATOR(/=, T)

INSTANTIATE(boost::uint16_t);
INSTANTIATE(int);
INSTANTIATE(float);
INSTANTIATE(double);
/// \endcond
