/**
 * \file
 * \brief  Internal support for reading and writing FITS files
 *
 * Tell doxygen to (usually) ignore this file \cond GIL_IMAGE_INTERNALS
 * \author Robert Lupton (rhl@astro.princeton.edu)
 *         Princeton University
 * \date   September 2008
 */
#if !defined(LSST_FITS_IO_MPL_H)
#define LSST_FITS_IO_MPL_H 1

#include <exception>
#include "boost/mpl/for_each.hpp"
#include "boost/mpl/vector.hpp"

#include "boost/gil/gil_all.hpp"

#include "lsst/afw/image/lsstGil.h"
#include "fits_io.h"

namespace {
struct found_type : std::exception { }; // type to throw when we've read our data

template<typename ImageT, typename ExceptionT>
class try_fits_read_image {
public:
    try_fits_read_image(const std::string& file, ImageT& img,
                        lsst::daf::base::PropertySet::Ptr metadata,
                        int hdu,
                        lsst::afw::image::BBox const& bbox
                       ) : _file(file), _img(img), _metadata(metadata), _hdu(hdu), _bbox(bbox) { }

    void operator()(ImageT x) {         // read directly into the desired type if the file's the same type
        try {
            lsst::afw::image::fits_read_image(_file, _img, _metadata, _hdu, _bbox);
            throw Exception(T);         // signal that we've succeeded
        } catch(lsst::afw::image::FitsWrongTypeException const&) {
            // ah well.  We'll try another image type
        }
    }

    template<typename U> void operator()(U x) { // read and convert into the desired type
        try {
            U img;
            lsst::afw::image::fits_read_image(_file, img, _metadata, _hdu, _bbox);

            _img.recreate(img.dimensions());
            boost::gil::copy_and_convert_pixels(const_view(img), view(_img));

            throw Exception(U);         // signal that we've succeeded
        } catch(lsst::afw::image::FitsWrongTypeException const&) {
            // pass
        }
    }
private:
    std::string _file;
    ImageT& _img;
    lsst::daf::base::PropertySet::Ptr _metadata;
    int _hdu;
    lsst::afw::image::BBox const& _bbox;
};

}

namespace lsst { namespace afw { namespace image {
template<typename fits_img_types, typename ImageT>
bool fits_read_image(std::string const& file, ImageT& img,
                     lsst::daf::base::PropertySet::Ptr metadata = lsst::daf::base::PropertySet::Ptr(),
                     int hdu=0,
                     BBox const& bbox=BBox()
                    ) {
    try {
        boost::mpl::for_each<fits_img_types>(try_fits_read_image<ImageT, found_type>(file, img, metadata, hdu, bbox));
    } catch (found_type &e) {
        return true;                    // success
    }

    return false;
}
}}}                                     // lsst::afw::image
/// \endcond
#endif
