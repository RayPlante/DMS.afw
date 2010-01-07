// -*- lsst-c++ -*-
//
//##====----------------                                ----------------====##/
//
//! \file
//! \brief Support for formatters
//
//##====----------------                                ----------------====##/

#include "boost/cstdint.hpp"
#include "boost/format.hpp"

#include "lsst/pex/exceptions.h"
#include "lsst/daf/persistence/LogicalLocation.h"
#include "lsst/daf/persistence/DbTsvStorage.h"
#include "lsst/afw/formatters/Utils.h"

using boost::int64_t;
namespace ex = lsst::pex::exceptions;
using lsst::daf::base::PropertySet;
using lsst::pex::policy::Policy;
using lsst::daf::persistence::LogicalLocation;

namespace lsst {
namespace afw {
namespace formatters {

int extractSliceId(PropertySet::Ptr const & properties) {
    if (properties->isArray("sliceId")) {
        throw LSST_EXCEPT(ex::RuntimeErrorException, "\"sliceId\" property has multiple values");
    }
    int sliceId = properties->getAsInt("sliceId");
    if (sliceId < 0) {
        throw LSST_EXCEPT(ex::RangeErrorException, "negative \"sliceId\"");
    }
    if (properties->exists("universeSize") && !properties->isArray("universeSize")) {
        int universeSize = properties->getAsInt("universeSize");
        if (sliceId >= universeSize) {
            throw LSST_EXCEPT(ex::RangeErrorException, "\"sliceId\" must be less than \"universeSize \"");
        }
    }
    return sliceId;
}
                        
int extractVisitId(PropertySet::Ptr const & properties) {
    if (properties->isArray("visitId")) {
        throw LSST_EXCEPT(ex::RuntimeErrorException, "\"visitId\" property has multiple values");
    }
    int visitId = properties->getAsInt("visitId");
    if (visitId < 0) {
        throw LSST_EXCEPT(ex::RangeErrorException, "negative \"visitId\"");
    }
    return visitId;
}

int64_t extractFpaExposureId(PropertySet::Ptr const & properties) {
    if (properties->isArray("fpaExposureId")) {
        throw LSST_EXCEPT(ex::RuntimeErrorException, "\"fpaExposureId\" property has multiple values");
    }
    int64_t fpaExposureId = properties->getAsInt64("fpaExposureId");
    if (fpaExposureId < 0) {
        throw LSST_EXCEPT(ex::RangeErrorException, "negative \"fpaExposureId\"");
    }
    if ((fpaExposureId & 0xfffffffe00000000LL) != 0LL) {
        throw LSST_EXCEPT(ex::RangeErrorException, "\"fpaExposureId\" is too large");
    }
    return fpaExposureId;
}

int extractCcdId(PropertySet::Ptr const & properties) {
    if (properties->isArray("ccdId")) {
        throw LSST_EXCEPT(ex::RuntimeErrorException, "\"ccdId\" property has multiple values");
    }
    int ccdId = properties->getAsInt("ccdId");
    if (ccdId < 0) {
        throw LSST_EXCEPT(ex::RangeErrorException, "negative \"ccdId\"");
    }
    if (ccdId > 255) {
        throw LSST_EXCEPT(ex::RangeErrorException, "\"ccdId\" is too large");
    }
    return static_cast<int>(ccdId);
}

int extractAmpId(PropertySet::Ptr const & properties) {
    if (properties->isArray("ampId")) {
        throw LSST_EXCEPT(ex::RuntimeErrorException, "\"ampId\" property has multiple values");
    }
    int ampId = properties->getAsInt("ampId");
    if (ampId < 0) {
        throw LSST_EXCEPT(ex::RangeErrorException, "negative \"ampId\"");
    }
    if (ampId > 63) {
        throw LSST_EXCEPT(ex::RangeErrorException, "\"ampId\" is too large");
    }
    return (extractCcdId(properties) << 6) + ampId;
}

int64_t extractCcdExposureId(PropertySet::Ptr const & properties) {
    if (properties->isArray("ccdExposureId")) {
        throw LSST_EXCEPT(ex::RuntimeErrorException, "\"ccdExposureId\" property has multiple values");
    }
    int64_t ccdExposureId = properties->getAsInt64("ccdExposureId");
    if (ccdExposureId < 0) {
        throw LSST_EXCEPT(ex::RangeErrorException, "negative \"ccdExposureId\"");
    }
    return ccdExposureId;
}

int64_t extractAmpExposureId(PropertySet::Ptr const & properties) {
    if (properties->isArray("ampExposureId")) {
        throw LSST_EXCEPT(ex::RuntimeErrorException, "\"ampExposureId\" property has multiple values");
    }
    int64_t ampExposureId = properties->getAsInt64("ampExposureId");
    if (ampExposureId < 0) {
        throw LSST_EXCEPT(ex::RangeErrorException, "negative \"ampExposureId\"");
    }
    return ampExposureId;
}

/**
 * Extracts and returns the string-valued @c "itemName" property from the given data property object.
 *  
 * @throw lsst::pex::exceptions::InvalidParameterException
 *        If the given pointer is null, or the @c PropertySet pointed
 *        to does not contain a unique property named @c "itemName".
 */
std::string const getItemName(PropertySet::Ptr const & properties) {
    if (!properties) {
        throw LSST_EXCEPT(ex::InvalidParameterException, "Null PropertySet::Ptr");
    }
    if (properties->isArray("itemName")) {
        throw LSST_EXCEPT(ex::InvalidParameterException, "\"itemName\" property has multiple values");
    } 
    return properties->getAsString("itemName");
}


/**
 * Returns @c true if and only if @a properties is non-null and contains a
 * unique property with the given name that has type @c bool and a value of @c true.
 */
bool extractOptionalFlag(
    PropertySet::Ptr const & properties,
    std::string      const & name
) {
    if (properties && properties->exists(name)) {
        return properties->getAsBool(name);
    }
    return false;
}


/**
 * Returns the name of the table that a single slice of a pipeline involved in the processing
 * of a single visit should use for persistence of a particular output. All slices can be
 * configured to use the same (per-visit) table name using policy parameters.
 *
 * @param[in] policy   The @c Policy containing the table name pattern ("${itemName}.tableNamePattern",
 *                     where ${itemName} is looked up in @a properties using the "itemName" key)
 *                     from which the the actual table name is derived. This pattern may contain
 * a set of parameters in @c %(key) format - these are interpolated by looking up @c "key" in
 * the @a properties PropertySet.
 *
 * @param[in] properties   Provides runtime specific properties necessary to construct the
 *                         output table name.
 * @return table name
 */
std::string const getTableName(
    Policy::Ptr      const & policy,
    PropertySet::Ptr const & properties
) {
    std::string itemName(getItemName(properties));
    return LogicalLocation(policy->getString(itemName + ".tableNamePattern"), properties).locString();
}


/**
 * Stores the name of the table that each slice of a pipeline involved in processing a visit
 * used for persistence of its outputs. If slices were configured to all use the same (per-visit)
 * table name, a single name is stored.
 *
 * @param[in] policy   The @c Policy containing the table name pattern ("${itemName}.tableNamePattern",
 *                     where ${itemName} is looked up in @a properties using the "itemName" key)
 *                     from which the the actual table name is derived. This pattern may contain
 * a set of parameters in @c %(key) format - these are interpolated by looking up @c "key" in
 * the @a properties PropertySet.
 *
 * @param[in] properties   The runtime specific properties necessary to construct the table names.
 *
 * string. The @c "visitId" property must also be present, and shall be a non-negative integer of type
 * @c int64_t uniquely identifying the current LSST visit. If the @c "${itemName}.isPerSliceTable"
 * property is present, is of type @c bool and is set to @c true, then it is assumed that
 * @c "${itemName}.numSlices" (a positive integer of type @c int) output tables exist and
 * are to be read in.
 *
 * @return a list of table names
 * @sa getTableName()
 */
std::vector<std::string> getAllSliceTableNames(
    Policy::Ptr        const & policy,
    PropertySet::Ptr   const & properties
) {
    std::string itemName(getItemName(properties));
    std::string pattern(policy->getString(itemName + ".tableNamePattern"));
    int numSlices = 1;
    if (properties->exists(itemName + ".numSlices")) {
        numSlices = properties->getAsInt(itemName + ".numSlices");
    }
    if (numSlices <= 0) {
        throw LSST_EXCEPT(ex::RuntimeErrorException,
                          itemName + " \".numSlices\" property value must be positive");
    }
    std::vector<std::string> names;
    names.reserve(numSlices);
    PropertySet::Ptr props = properties->deepCopy();
    for (int i = 0; i < numSlices; ++i) {
        props->set("sliceId", i);
        names.push_back(LogicalLocation(pattern, props).locString());
    }
    return names;
}


/**
 * Creates the table identified by calling getTableName() with the given @a policy and @a properties.
 * A key named  @c "${itemName}.templateTableName" (where @c ${itemName} refers to the value of a
 * property named @c "itemName" extracted from @a properties) must be available and set to the name
 * of the template table to use for creation.
 *
 * Note that the template table must exist in the database identified by @a location, and that if
 * the desired table already exists, an exception is thrown.
 */
void createTable(
    lsst::daf::persistence::LogicalLocation const & location,
    lsst::pex::policy::Policy::Ptr const & policy,
    PropertySet::Ptr const & properties
) {
    std::string itemName(getItemName(properties));
    std::string name(getTableName(policy, properties));
    std::string model(policy->getString(itemName + ".templateTableName"));

    lsst::daf::persistence::DbTsvStorage db;
    db.setPersistLocation(location);
    db.createTableFromTemplate(name, model);
}


/** Drops the database table(s) identified by getAllSliceTables(). */
void dropAllSliceTables(
    lsst::daf::persistence::LogicalLocation const & location,
    lsst::pex::policy::Policy::Ptr const & policy,
    PropertySet::Ptr const & properties
) {
    std::vector<std::string> names = getAllSliceTableNames(policy, properties);

    lsst::daf::persistence::DbTsvStorage db;
    db.setPersistLocation(location);
    for (std::vector<std::string>::const_iterator i(names.begin()), end(names.end()); i != end; ++i) {
        db.dropTable(*i);
    }
}


std::string const formatFitsProperties(lsst::daf::base::PropertySet::Ptr prop) {
    typedef std::vector<std::string> NameList;
    std::string sout;

    NameList paramNames = prop->paramNames(false);

    for (NameList::const_iterator i = paramNames.begin(), end = paramNames.end(); i != end; ++i) {
       std::size_t lastPeriod = i->rfind(char('.'));
       std::string name = (lastPeriod == std::string::npos) ? *i : i->substr(lastPeriod + 1);
       std::type_info const & type = prop->typeOf(*i);

       std::string out = "";
       if (name.size() > 8) {           // Oh dear; too long for a FITS keyword
           out += "HIERARCH = " + name;
       } else {
           out = (boost::format("%-8s= ") % name).str();
       }
       
       if (type == typeid(int)) {
           out += (boost::format("%20d") % prop->get<int>(*i)).str();
       } else if (type == typeid(double)) {
           out += (boost::format("%20.15g") % prop->get<double>(*i)).str();
       } else if (type == typeid(std::string)) {
           out += (boost::format("'%-67s' ") % prop->get<std::string>(*i)).str();
       }

       int const len = out.size();
       if (len < 80) {
           out += std::string(80 - len, ' ');
       } else {
           out = out.substr(0, 80);
       }

       sout += out;
    }

    return sout.c_str();
}


int countFitsHeaderCards(lsst::daf::base::PropertySet::Ptr prop) {
    return prop->paramNames(false).size();
}


}}} // namespace lsst::afw::formatters
