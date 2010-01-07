// -*- lsst-c++ -*-

/** @file
 * @brief Implementation of MaskedImageFormatter class
 *
 * @author $Author: ktlim $
 * @version $Revision: 2151 $
 * @date $Date: 2009-01-06 15:45:35 -0700 (Tue, 06 Jan 2009) $
 *
 * Contact: Kian-Tat Lim (ktl@slac.stanford.edu)
 *
 * @ingroup afw
 */

#ifndef __GNUC__
#  define __attribute__(x) /*NOTHING*/
#endif
static char const* SVNid __attribute__((unused)) = "$Id: MaskedImageFormatter.cc 6892 2009-01-06 22:45:35Z smm $";

#include "boost/serialization/shared_ptr.hpp"

#include "lsst/daf/base.h"
#include "lsst/daf/persistence.h"
#include "lsst/pex/logging/Trace.h"
#include "lsst/afw/formatters/MaskedImageFormatter.h"
#include "lsst/afw/formatters/ImageFormatter.h"
#include "lsst/afw/formatters/MaskFormatter.h"
#include "lsst/afw/image/MaskedImage.h"

#define EXEC_TRACE  20
static void execTrace(std::string s, int level = EXEC_TRACE) {
    lsst::pex::logging::Trace("afw.MaskedImageFormatter", level, s);
}

using lsst::daf::base::Persistable;
using lsst::daf::persistence::BoostStorage;
using lsst::daf::persistence::FitsStorage;
using lsst::daf::persistence::Storage;
using lsst::afw::image::MaskedImage;
using lsst::afw::image::MaskPixel;
using lsst::afw::image::VariancePixel;

namespace lsst {
namespace afw {
namespace formatters {

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
class MaskedImageFormatterTraits {
public:
    static std::string name;
};

template<> std::string MaskedImageFormatterTraits<boost::uint16_t, MaskPixel, VariancePixel>::name("MaskedImageU");
template<> std::string MaskedImageFormatterTraits<int, MaskPixel, VariancePixel>::name("MaskedImageI");
template<> std::string MaskedImageFormatterTraits<float, MaskPixel, VariancePixel>::name("MaskedImageF");
template<> std::string MaskedImageFormatterTraits<double, MaskPixel, VariancePixel>::name("MaskedImageD");

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
lsst::daf::persistence::FormatterRegistration MaskedImageFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::registration(
    MaskedImageFormatterTraits<ImagePixelT, MaskPixelT, VariancePixelT>::name,
    typeid(MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>),
    createInstance);

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
MaskedImageFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::MaskedImageFormatter(
    lsst::pex::policy::Policy::Ptr policy) :
    lsst::daf::persistence::Formatter(typeid(*this)) {
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
MaskedImageFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::~MaskedImageFormatter(void) {
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void MaskedImageFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::write(
    Persistable const* persistable,
    Storage::Ptr storage,
    lsst::daf::base::PropertySet::Ptr additionalData) {
    execTrace("MaskedImageFormatter write start");
    MaskedImage<ImagePixelT, MaskPixelT> const* ip =
        dynamic_cast<MaskedImage<ImagePixelT, MaskPixelT> const*>(persistable);
    if (ip == 0) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Persisting non-MaskedImage");
    }
    if (typeid(*storage) == typeid(BoostStorage)) {
        execTrace("MaskedImageFormatter write BoostStorage");
        BoostStorage* boost = dynamic_cast<BoostStorage*>(storage.get());
        boost->getOArchive() & *ip;
        execTrace("MaskedImageFormatter write end");
        return;
    }
    else if (typeid(*storage) == typeid(FitsStorage)) {
        execTrace("MaskedImageFormatter write FitsStorage");
        FitsStorage* fits = dynamic_cast<FitsStorage*>(storage.get());
        ip->writeFits(fits->getPath());
        execTrace("MaskedImageFormatter write end");
        return;
    }
    throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Unrecognized Storage for MaskedImage");
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
Persistable* MaskedImageFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::read(
    Storage::Ptr storage,
    lsst::daf::base::PropertySet::Ptr additionalData) {
    execTrace("MaskedImageFormatter read start");
    if (typeid(*storage) == typeid(BoostStorage)) {
        execTrace("MaskedImageFormatter read BoostStorage");
        BoostStorage* boost = dynamic_cast<BoostStorage*>(storage.get());
        MaskedImage<ImagePixelT, MaskPixelT>* ip = new MaskedImage<ImagePixelT, MaskPixelT>;
        boost->getIArchive() & *ip;
        execTrace("MaskedImageFormatter read end");
        return ip;
    }
    else if (typeid(*storage) == typeid(FitsStorage)) {
        execTrace("MaskedImageFormatter read FitsStorage");
        FitsStorage* fits = dynamic_cast<FitsStorage*>(storage.get());
        MaskedImage<ImagePixelT, MaskPixelT>* ip = new MaskedImage<ImagePixelT, MaskPixelT>(fits->getPath());
        execTrace("MaskedImageFormatter read end");
        return ip;
    }
    throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Unrecognized Storage for MaskedImage");
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void MaskedImageFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::update(
    Persistable* persistable,
    Storage::Ptr storage,
    lsst::daf::base::PropertySet::Ptr additionalData) {
    throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Unexpected call to update for MaskedImage");
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT> template <class Archive>
void MaskedImageFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::delegateSerialize(
    Archive& ar, unsigned int const version, Persistable* persistable) {
    execTrace("MaskedImageFormatter delegateSerialize start");
    MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>* ip =
        dynamic_cast<MaskedImage<ImagePixelT, MaskPixelT, VariancePixelT>*>(persistable);
    if (ip == 0) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Serializing non-MaskedImage");
    }
    ar & ip->_image & ip->_variance & ip->_mask;
    execTrace("MaskedImageFormatter delegateSerialize end");
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
lsst::daf::persistence::Formatter::Ptr MaskedImageFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::createInstance(
    lsst::pex::policy::Policy::Ptr policy) {
    return lsst::daf::persistence::Formatter::Ptr(
        new MaskedImageFormatter<ImagePixelT, MaskPixelT, VariancePixelT>(policy));
}

#define INSTANTIATE(I, M, V) \
    template class MaskedImageFormatter<I, M, V>; \
    template void MaskedImageFormatter<I, M, V>::delegateSerialize<boost::archive::text_oarchive>( \
        boost::archive::text_oarchive &, unsigned int const, Persistable *); \
    template void MaskedImageFormatter<I, M, V>::delegateSerialize<boost::archive::text_iarchive>( \
        boost::archive::text_iarchive &, unsigned int const, Persistable *);

INSTANTIATE(uint16_t, MaskPixel, VariancePixel)
INSTANTIATE(int, MaskPixel, VariancePixel)
INSTANTIATE(float, MaskPixel, VariancePixel)
INSTANTIATE(double, MaskPixel, VariancePixel)

}}} // namespace lsst::afw::formatters
