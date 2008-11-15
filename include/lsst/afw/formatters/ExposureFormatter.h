// -*- lsst-c++ -*-
#ifndef LSST_AFW_FORMATTERS_EXPOSUREFORMATTER_H
#define LSST_AFW_FORMATTERS_EXPOSUREFORMATTER_H

/** @file
 * @brief Interface for ExposureFormatter class
 *
 * @author $Author: ktlim $
 * @version $Revision: 2377 $
 * @date $Date$
 *
 * Contact: Kian-Tat Lim (ktl@slac.stanford.edu)
 * @ingroup afw
 */

/** @class lsst::afw::formatters::ExposureFormatter
 * @brief Class implementing persistence and retrieval for Exposures.
 *
 * @ingroup afw
 */

#include "lsst/daf/base.h"
#include "lsst/daf/persistence.h"
#include "lsst/pex/policy/Policy.h"

namespace lsst {
namespace afw {
namespace formatters {

template<typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
class ExposureFormatter : public lsst::daf::persistence::Formatter {
public:       
    virtual ~ExposureFormatter(void);

    virtual void write(
        lsst::daf::base::Persistable const* persistable,
        lsst::daf::persistence::Storage::Ptr storage,
        lsst::daf::base::DataProperty::PtrType additionalData
    );
    virtual lsst::daf::base::Persistable* read(
        lsst::daf::persistence::Storage::Ptr storage,
        lsst::daf::base::DataProperty::PtrType additionalData
    );
    virtual void update(
        lsst::daf::base::Persistable* persistable,
        lsst::daf::persistence::Storage::Ptr storage,
        lsst::daf::base::DataProperty::PtrType additionalData
    );

    static lsst::daf::persistence::Formatter::Ptr createInstance(
        lsst::pex::policy::Policy::Ptr policy
    );

    template <class Archive>
    static void delegateSerialize(
        Archive& ar, unsigned int const version,
        lsst::daf::base::Persistable* persistable
    );
private:
    explicit ExposureFormatter(lsst::pex::policy::Policy::Ptr policy);

    lsst::pex::policy::Policy::Ptr _policy;

    static lsst::daf::persistence::FormatterRegistration registration;
};

}}} // namespace lsst::afw::formatters

#endif
