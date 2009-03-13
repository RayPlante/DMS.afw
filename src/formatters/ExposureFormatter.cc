// -*- lsst-c++ -*-

/** @file
 * @brief Implementation of ExposureFormatter class
 *
 * @author $Author: ktlim $
 * @version $Revision: 2151 $
 * @date $Date$
 *
 * Contact: Kian-Tat Lim (ktl@slac.stanford.edu)
 *
 * @ingroup afw
 */

#ifndef __GNUC__
#  define __attribute__(x) /*NOTHING*/
#endif
static char const* SVNid __attribute__((unused)) = "$Id$";

#include "boost/serialization/shared_ptr.hpp"

#include "lsst/daf/base.h"
#include "lsst/pex/exceptions.h"
#include "lsst/daf/persistence.h"
#include "lsst/pex/logging/Trace.h"
#include "lsst/daf/persistence/PropertySetFormatter.h"
#include "lsst/afw/formatters/ExposureFormatter.h"
#include "lsst/afw/formatters/Utils.h"
#include "lsst/afw/formatters/WcsFormatter.h"
#include "lsst/afw/image/Exposure.h"


// #include "lsst/afw/image/LSSTFitsResource.h"

#define EXEC_TRACE  20
static void execTrace(std::string s, int level = EXEC_TRACE) {
    lsst::pex::logging::Trace("afw.ExposureFormatter", level, s);
}

namespace afwForm = lsst::afw::formatters;
namespace afwImg = lsst::afw::image;
namespace dafBase = lsst::daf::base;
namespace dafPersist = lsst::daf::persistence;

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
class ExposureFormatterTraits {
public:
    static std::string name;
};

template<> std::string ExposureFormatterTraits<boost::uint16_t, afwImg::MaskPixel, afwImg::VariancePixel>::name("ExposureU");
template<> std::string ExposureFormatterTraits<int, afwImg::MaskPixel, afwImg::VariancePixel>::name("ExposureI");
template<> std::string ExposureFormatterTraits<float, afwImg::MaskPixel, afwImg::VariancePixel>::name("ExposureF");
template<> std::string ExposureFormatterTraits<double, afwImg::MaskPixel, afwImg::VariancePixel>::name("ExposureD");


template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
lsst::daf::persistence::FormatterRegistration afwForm::ExposureFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::registration(
    ExposureFormatterTraits<ImagePixelT, MaskPixelT, VariancePixelT>::name,
    typeid(afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>),
    createInstance);

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
afwForm::ExposureFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::ExposureFormatter(
    lsst::pex::policy::Policy::Ptr policy) :
    lsst::daf::persistence::Formatter(typeid(*this)), _policy(policy) {
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
afwForm::ExposureFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::~ExposureFormatter(void) {
}

/** Lookup a filter number in the database to find a filter name.
 */
static std::string lookupFilterName(
    dafPersist::DbStorage* db,  //!< Database to look in
    int filterId    //!< Number of filter to lookup
    ) {
    db->setTableForQuery("Filter");
    db->outColumn("filterName");
    db->condParam<int>("id", filterId);
    db->setQueryWhere("filterId = :id");
    db->query();
    if (!db->next() || db->columnIsNull(0)) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                          "Unable to get name for filter id: " + filterId);
    }
    std::string filterName = db->getColumnByPos<std::string>(0);
    if (db->next()) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                          "Multiple names for filter id: " + filterId);

    }
    db->finishQuery();
    return filterName;
}


/** Set an output column's value from a PropertySet, setting it to NULL if
 * the desired property does not exist.
 */
template <typename T>
static void setColumn(
    dafPersist::DbStorage* db,                            //!< Destination database
    std::string const& colName,               //!< Output column name
    lsst::daf::base::PropertySet::Ptr source, //!< Source PropertySet
    std::string const& propName               //!< Property name
    ) {
    if (!source->exists(propName)) {
        db->setColumnToNull(colName);
    } else {
        db->setColumn<T>(colName, source->get<T>(propName));
    }
}

/** Set an output column's value from a PropertySet, setting it to NULL if
 * the desired property does not exist.  Casts from PropertySet type to
 * database field type.
 */
template <typename T1, typename T2>
static void setColumn(
    dafPersist::DbStorage* db,                            //!< Destination database
    std::string const& colName,               //!< Output column name
    lsst::daf::base::PropertySet::Ptr source, //!< Source PropertySet
    std::string const& propName               //!< Property name
    ) {
    if (!source->exists(propName)) {
        db->setColumnToNull(colName);
    } else {
        db->setColumn<T1>(colName, static_cast<T1>(source->get<T2>(propName)));
    }
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void afwForm::ExposureFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::write(
    dafBase::Persistable const* persistable,
    dafPersist::Storage::Ptr storage,
    lsst::daf::base::PropertySet::Ptr additionalData) {
    execTrace("ExposureFormatter write start");
    afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT> const* ip =
        dynamic_cast<afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT> const*>(persistable);
    if (ip == 0) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Persisting non-Exposure");
    }
    if (typeid(*storage) == typeid(dafPersist::BoostStorage)) {
        execTrace("ExposureFormatter write BoostStorage");
        dafPersist::BoostStorage* boost = dynamic_cast<dafPersist::BoostStorage*>(storage.get());
        boost->getOArchive() & *ip;
        execTrace("ExposureFormatter write end");
        return;
    }
    else if (typeid(*storage) == typeid(dafPersist::FitsStorage)) {
        execTrace("ExposureFormatter write FitsStorage");
        dafPersist::FitsStorage* fits = dynamic_cast<dafPersist::FitsStorage*>(storage.get());

        lsst::daf::base::PropertySet::Ptr wcsProps =
            lsst::afw::formatters::WcsFormatter::generatePropertySet(*(ip->_wcs));

        afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>* vip =
            const_cast<afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>*>(ip);
        vip->getMaskedImage().getMetadata()->add("Wcs", wcsProps);
        ip->_maskedImage.writeFits(fits->getPath());
        execTrace("ExposureFormatter write end");
        return;
    } else if (typeid(*storage) == typeid(dafPersist::DbStorage)) {
        execTrace("ExposureFormatter write DbStorage");
        dafPersist::DbStorage* db = dynamic_cast<dafPersist::DbStorage*>(storage.get());

        // Get the Wcs headers.
        lsst::daf::base::PropertySet::Ptr wcsProps =
            lsst::afw::formatters::WcsFormatter::generatePropertySet(*(ip->_wcs));

        // Get the image headers.
        lsst::daf::base::PropertySet::Ptr dp = ip->getMaskedImage().getMetadata();
        if (!dp) {
            throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                              "Unable to retrieve metadata from MaskedImage's Image");
        }

        // Select a table to insert into based on the itemName.
        std::string itemName = additionalData->get<std::string>("itemName");
        std::string tableName = itemName;
        if (_policy->exists(itemName)) {
            lsst::pex::policy::Policy::Ptr itemPolicy = _policy->getPolicy(itemName);
            if (itemPolicy->exists("TableName")) {
                tableName = itemPolicy->getString("TableName");
            }
        }
        if (tableName != "Raw_Amp_Exposure" &&
            tableName != "Science_Amp_Exposure") {
            throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                              "Unknown table name for persisting Exposure to DbStorage: " +
                              tableName + "for item " + itemName);
        }
        db->setTableForInsert(tableName);

        // Set the identifier columns.

        int ampId = extractAmpId(additionalData);
        // int ccdId = extractCcdId(additionalData);
        // int64_t exposureId = extractExposureId(additionalData);
        int64_t ccdExposureId = extractCcdExposureId(additionalData);
        int64_t ampExposureId = extractAmpExposureId(additionalData);

        if (tableName == "Raw_Amp_Exposure") {
            db->setColumn<long long>("rawAmpExposureId", ampExposureId);
            db->setColumn<long long>("rawCCDExposureId", ccdExposureId);
        }
        else { // Science_Amp_Exposure
            db->setColumn<long long>("scienceAmpExposureId", ampExposureId);
            db->setColumn<long long>("scienceCCDExposureId", ccdExposureId);
            db->setColumn<long long>("rawAmpExposureId", ampExposureId);
            /// \todo Check that rawCCDExposureId == scienceCCDExposureId --
            /// KTL -- 2008-01-25
        }

        db->setColumn<int>("ampId", ampId);

        // Set the URL column with the location of the FITS file.
        setColumn<std::string>(db, "url",
                              additionalData, "StorageLocation.FitsStorage");


        // Set the Wcs information columns.
        setColumn<std::string>(db, "ctype1", wcsProps, "CTYPE1");
        setColumn<std::string>(db, "ctype2", wcsProps, "CTYPE2");
        setColumn<float, double>(db, "crpix1", wcsProps, "CRPIX1");
        setColumn<float, double>(db, "crpix2", wcsProps, "CRPIX2");
        setColumn<double>(db, "crval1", wcsProps, "CRVAL1");
        setColumn<double>(db, "crval2", wcsProps, "CRVAL2");
        setColumn<double>(db, "cd11", wcsProps, "CD1_1");
        setColumn<double>(db, "cd21", wcsProps, "CD2_1");
        setColumn<double>(db, "cd12", wcsProps, "CD1_2");
        setColumn<double>(db, "cd22", wcsProps, "CD2_2");

        if (tableName == "Science_Amp_Exposure") {
            // Set calibration data columns.
            setColumn<float, double>(db, "photoFlam", dp, "PHOTFLAM");
            setColumn<float, double>(db, "photoZP", dp, "PHOTZP");
        }

        // Phew!  Insert the row now.
        db->insertRow();

        execTrace("ExposureFormatter write end");
        return;
    }
    throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Unrecognized Storage for Exposure");
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
dafBase::Persistable* afwForm::ExposureFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::read(
    dafPersist::Storage::Ptr storage,
    lsst::daf::base::PropertySet::Ptr additionalData) {
    execTrace("ExposureFormatter read start");
    if (typeid(*storage) == typeid(dafPersist::BoostStorage)) {
        execTrace("ExposureFormatter read BoostStorage");
        dafPersist::BoostStorage* boost = dynamic_cast<dafPersist::BoostStorage*>(storage.get());
        afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>* ip =
            new afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>;
        boost->getIArchive() & *ip;
        execTrace("ExposureFormatter read end");
        return ip;
    } else if (typeid(*storage) == typeid(dafPersist::FitsStorage)) {
        execTrace("ExposureFormatter read FitsStorage");
        dafPersist::FitsStorage* fits = dynamic_cast<dafPersist::FitsStorage*>(storage.get());
        int hdu = additionalData->get<int>("hdu", 0);
        afwImg::BBox box;
        if (additionalData->exists("llcX")) {
            int llcX = additionalData->get<int>("llcX");
            int llcY = additionalData->get<int>("llcY");
            int width = additionalData->get<int>("width");
            int height = additionalData->get<int>("height");
            box = afwImg::BBox(afwImg::PointI(llcX, llcY), width, height);
        }
        afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>* ip =
            new afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>(
                fits->getPath(), hdu, box);
        execTrace("ExposureFormatter read end");
        return ip;
    } else if (typeid(*storage) == typeid(dafPersist::DbStorage)) {
        execTrace("ExposureFormatter read DbStorage");
        dafPersist::DbStorage* db = dynamic_cast<dafPersist::DbStorage*>(storage.get());

        // Select a table to retrieve from based on the itemName.
        std::string itemName = additionalData->get<std::string>("itemName");
        std::string tableName = itemName;
        if (_policy->exists(itemName)) {
            lsst::pex::policy::Policy::Ptr itemPolicy =
                _policy->getPolicy(itemName);
            if (itemPolicy->exists("TableName")) {
                tableName = itemPolicy->getString("TableName");
            }
        }
        if (tableName != "Raw_Amp_Exposure" &&
            tableName != "Science_Amp_Exposure") {
            throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException,
                              "Unknown table name for retrieving Exposure from DbStorage: " +
                              tableName + " for item " + itemName);
        }
        db->setTableForQuery(tableName);

        // Set the identifier column tests.
        db->condParam<int64_t>("id", additionalData->getAsInt64("ampExposureId"));
        if (tableName == "Raw_Amp_Exposure") {
            db->setQueryWhere("rawAmpExposureId = :id");
        }
        else { // Science_Amp_Exposure
            db->setQueryWhere("scienceAmpExposureId = :id");
        }

        db->outColumn("url");

        if (tableName == "Science_Amp_Exposure") {
            // Set the Wcs information columns.
            db->outColumn("ctype1");
            db->outColumn("ctype2");
            db->outColumn("crpix1");
            db->outColumn("crpix2");
            db->outColumn("crval1");
            db->outColumn("crval2");
            db->outColumn("cd11");
            db->outColumn("cd21");
            db->outColumn("cd12");
            db->outColumn("cd22");

            // Set calibration data columns.
            db->outColumn("photoFlam");
            db->outColumn("photoZP");
        }

        // Phew!  Run the query.
        db->query();
        if (!db->next()) {
            throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Unable to retrieve row");
        }
        // ...
        if (db->next()) {
            throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Non-unique Exposure retrieved");
        }
        db->finishQuery();

        //! \todo Should really have FITS be a separate Storage.
        // - KTL - 2007-11-29

        // Restore image from FITS...
        afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>* ip =
            new afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>(db->getColumnByPos<std::string>(0));
        lsst::daf::base::PropertySet::Ptr dp = ip->getMaskedImage().getMetadata();

        // Look up the filter name given the ID.
        int filterId = db->getColumnByPos<int>(1);
        std::string filterName = lookupFilterName(db, filterId);
        dp->set("FILTER", filterName);

        // Set the image headers.
        // Set the Wcs headers in ip->_wcs.

        //! \todo Need to implement overwriting of FITS metadata PropertySet
        // with values from database. - KTL - 2007-12-18

        execTrace("ExposureFormatter read end");
        return ip;
    }
    throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Unrecognized Storage for Exposure");
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
void afwForm::ExposureFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::update(
    dafBase::Persistable* persistable,
    dafPersist::Storage::Ptr storage,
    lsst::daf::base::PropertySet::Ptr additionalData) {
    //! \todo Implement update from FitsStorage, keeping DB-provided headers.
    // - KTL - 2007-11-29
    throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Unexpected call to update for Exposure");
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT> template <class Archive>
void afwForm::ExposureFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::delegateSerialize(
    Archive& ar, unsigned int const version, dafBase::Persistable* persistable) {
    execTrace("ExposureFormatter delegateSerialize start");
    afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>* ip =
        dynamic_cast<afwImg::Exposure<ImagePixelT, MaskPixelT, VariancePixelT>*>(persistable);
    if (ip == 0) {
        throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, "Serializing non-Exposure");
    }
    ar & *ip->getMaskedImage().getMetadata() & ip->_maskedImage & ip->_wcs;
    execTrace("ExposureFormatter delegateSerialize end");
}

template <typename ImagePixelT, typename MaskPixelT, typename VariancePixelT>
lsst::daf::persistence::Formatter::Ptr afwForm::ExposureFormatter<ImagePixelT, MaskPixelT, VariancePixelT>::createInstance(
    lsst::pex::policy::Policy::Ptr policy) {
    return lsst::daf::persistence::Formatter::Ptr(new afwForm::ExposureFormatter<ImagePixelT, MaskPixelT, VariancePixelT>(policy));
}

#define INSTANTIATE(I, M, V) \
    template class afwForm::ExposureFormatter<I, M, V>; \
    template void afwForm::ExposureFormatter<I, M, V>::delegateSerialize<boost::archive::text_oarchive>( \
        boost::archive::text_oarchive &, unsigned int const, dafBase::Persistable *); \
    template void afwForm::ExposureFormatter<I, M, V>::delegateSerialize<boost::archive::text_iarchive>( \
        boost::archive::text_iarchive &, unsigned int const, dafBase::Persistable *);

INSTANTIATE(uint16_t, afwImg::MaskPixel, afwImg::VariancePixel)
INSTANTIATE(int, afwImg::MaskPixel, afwImg::VariancePixel)
INSTANTIATE(float, afwImg::MaskPixel, afwImg::VariancePixel)
INSTANTIATE(double, afwImg::MaskPixel, afwImg::VariancePixel)
