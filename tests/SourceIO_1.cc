// -*- lsst-c++ -*-
//
//##====----------------                                ----------------====##/
//
//! \file
//! \brief  Testing of IO via the persistence framework for Source and SourceVector.
//
//##====----------------                                ----------------====##/

#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <stdexcept>

#include "boost/cstdint.hpp"

#include "lsst/daf/base.h"
#include "lsst/daf/persistence.h"
#include "lsst/pex/exceptions.h"
#include "lsst/pex/policy/Policy.h"

#include "lsst/afw/detection/Source.h"
#include "lsst/afw/formatters/Utils.h"

using boost::int64_t;

using lsst::daf::base::PropertySet;
using lsst::daf::base::Persistable;
using lsst::daf::persistence::LogicalLocation;
using lsst::daf::persistence::Persistence;
using lsst::daf::persistence::Storage;
using lsst::pex::policy::Policy;

namespace afwFormatters = lsst::afw::formatters;
using namespace lsst::afw::detection;


#define Assert(pred, msg) do { if (!(pred)) { throw LSST_EXCEPT(lsst::pex::exceptions::RuntimeErrorException, (msg)); } } while (false)


static std::string const makeTempFile() {
    char name[32];
    std::strncpy(name, "Source_XXXXXX", 31);
    name[31] = 0;
    int const fd = ::mkstemp(name);
    Assert(fd != -1, "Failed to create temporary file");
    ::close(fd);
    return std::string(name);
}


static void initTestData(SourceVector & v, int sliceId = 0) {
    v.reserve(source_detail::NUM_NULLABLE_FIELDS + 2);
    for (int i = 0; i < source_detail::NUM_NULLABLE_FIELDS + 2; ++i) {
        Source data;
        // make sure each field has a different value, and that IO for each nullable field is tested
        // Note: Source ids are generated in ascending order
        int j = i*source_detail::NUM_NULLABLE_FIELDS;
        data.setId              (j + sliceId*(source_detail::NUM_NULLABLE_FIELDS + 2)*64 + 1);
        data.setAmpExposureId   (j +  1);
        data.setObjectId        (j +  2);
        data.setMovingObjectId  (j +  3);
        data.setRa              (static_cast<double>(j +  8));
        data.setDec             (static_cast<double>(j +  9));
        data.setRaErr4detection (static_cast<double>(j + 10));
        data.setDecErr4detection(static_cast<double>(j + 11));
        data.setRaErr4wcs       (static_cast<double>(j + 12));
        data.setDecErr4wcs      (static_cast<double>(j + 13));
        data.setTaiMidPoint     (static_cast<double>(j + 17));
        data.setTaiRange        (static_cast<double>(j + 18));
        data.setPsfMag          (static_cast<double>(j + 21));
        data.setPsfMagErr       (static_cast<double>(j + 22));
        data.setApMag           (static_cast<double>(j + 23));
        data.setApMagErr        (static_cast<double>(j + 24));
        data.setModelMag        (static_cast<double>(j + 25));
        data.setModelMagErr     (static_cast<double>(j + 26));
        data.setFwhmA           (static_cast<float> (j + 29));
        data.setFwhmB           (static_cast<float> (j + 30));
        data.setFwhmTheta       (static_cast<float> (j + 31));
        data.setApDia           (static_cast<float> (j + 32));
        data.setSnr             (static_cast<float> (j + 39));
        data.setChi2            (static_cast<float> (j + 40));
        data.setFlag4association(j + 42);
        data.setFlag4detection  (j + 43);
        data.setFlag4wcs        (j + 44);
        data.setFilterId        (-1);
        if (i < source_detail::NUM_NULLABLE_FIELDS) {
            data.setNotNull();
            data.setNull(i);
        } else if ((i & 1) == 0) {
            data.setNotNull();
        } else {
            data.setNull();
        }
        v.push_back(data);
    }
}


static void testBoost(void) {
    // Create a blank Policy and PropertySet.
    Policy::Ptr      policy(new Policy);
    PropertySet::Ptr props(new PropertySet);

    // Setup test location
    LogicalLocation loc(makeTempFile());

    // Intialize test data
    Source       ds;
    ds.setId(1);
    SourceVector dsv;
    initTestData(dsv);
    dsv.push_back(ds);
	PersistableSourceVector::Ptr persistPtr(new PersistableSourceVector());
	persistPtr->setSources(dsv);

    Persistence::Ptr pers = Persistence::getPersistence(policy);

    // write out data
    {
        Storage::List storageList;
        storageList.push_back(pers->getPersistStorage("BoostStorage", loc));
        pers->persist(*persistPtr, storageList, props);
    }

    // read in data
    {
        Storage::List storageList;
        storageList.push_back(pers->getRetrieveStorage("BoostStorage", loc));
        Persistable::Ptr p = pers->retrieve("PersistableSourceVector", storageList, props);
        Assert(p.get() != 0, "Failed to retrieve Persistable");
        PersistableSourceVector::Ptr persistVec =
            boost::dynamic_pointer_cast<PersistableSourceVector, Persistable>(p);
        Assert(persistVec.get() == 0, "Couldn't cast to PersistableSourceVector");
        Assert(persistVec->getSources() == dsv, 
        	"persist()/retrieve() resulted in PersistableSourceVector corruption");
    }
    ::unlink(loc.locString().c_str());
}


// Make at least a token attempt at generating a unique visit id
// (in-db table name collisions could cause spurious testcase failures)
static int createVisitId() {
    struct timeval tv;
    ::gettimeofday(&tv, 0);
    return static_cast<int>(tv.tv_sec);
}


static PropertySet::Ptr createDbTestProps(
    int         const   sliceId,
    int         const   numSlices,
    std::string const & itemName
) {
    Assert(sliceId < numSlices && numSlices > 0, "invalid slice parameters");

    PropertySet::Ptr props(new PropertySet);

    if (numSlices > 1) {
        props->add("Source.isPerSliceTable", true);
        props->add("Source.numSlices", numSlices);
    }
    props->add("visitId",  createVisitId());
    props->add("ccdId",    "7");
    props->add("sliceId",  sliceId);
    props->add("itemName", itemName);
    return props;
}


// comparison operator used to sort Source in id order
struct SourceLessThan {
    bool operator()(Source const & d1, Source const & d2) {
        return d1.getId() < d2.getId();
    }
};


static void testDb(std::string const & storageType) {
    // Create the required Policy and PropertySet
    Policy::Ptr policy(new Policy);
    PropertySet::Ptr props = createDbTestProps(0, 1, "Source");

    Persistence::Ptr pers = Persistence::getPersistence(policy);
    LogicalLocation loc("mysql://lsst10.ncsa.uiuc.edu:3306/test");

    // 1. Test on a single Source
    Source ds;
    ds.setId(2);
    SourceVector dsv;
    dsv.push_back(ds);
    PersistableSourceVector::Ptr persistPtr(new PersistableSourceVector);
    persistPtr->setSources(dsv);
    // write out data
    {
        Storage::List storageList;
        storageList.push_back(pers->getPersistStorage(storageType, loc));
        pers->persist(*persistPtr, storageList, props);
    }
    // and read it back in (in a SourceVector)
    {
        Storage::List storageList;
        storageList.push_back(pers->getRetrieveStorage(storageType, loc));
        Persistable::Ptr p = pers->retrieve("PersistableSourceVector", storageList, props);
        Assert(p != 0, "Failed to retrieve Persistable");
        PersistableSourceVector::Ptr v = boost::dynamic_pointer_cast<PersistableSourceVector, Persistable>(p);
        Assert(v.get() != 0, "Couldn't cast to PersistableSourceVector");
        SourceVector vec = v->getSources();
        Assert(vec.at(0) == ds, "persist()/retrieve() resulted in PersistableSourceVector corruption");
    }
    afwFormatters::dropAllVisitSliceTables(loc, policy, props);

    // 2. Test on a SourceVector
    dsv.clear();
    initTestData(dsv);
    persistPtr->setSources(dsv);
    // write out data
    {
        Storage::List storageList;
        storageList.push_back(pers->getPersistStorage(storageType, loc));
        pers->persist(*persistPtr, storageList, props);
    }
    // and read it back in
    {
        Storage::List storageList;
        storageList.push_back(pers->getRetrieveStorage(storageType, loc));
        Persistable::Ptr p = pers->retrieve("PersistableSourceVector", storageList, props);
        Assert(p != 0, "Failed to retrieve Persistable");
        PersistableSourceVector::Ptr persistVec = 
        		boost::dynamic_pointer_cast<PersistableSourceVector, Persistable>(p);
        Assert(persistVec.get() != 0, "Couldn't cast to PersistableSourceVector");
        SourceVector & vec(persistVec->getSources());
        // sort in ascending id order (database does not give any ordering guarantees
        // in the absence of an ORDER BY clause)
        std::sort(vec.begin(), vec.end(), SourceLessThan());
        Assert(vec == dsv, "persist()/retrieve() resulted in SourceVector corruption");
    }
    afwFormatters::dropAllVisitSliceTables(loc, policy, props);
}


static void testDb2(std::string const & storageType) {
    // Create the required Policy and PropertySet
    Policy::Ptr policy(new Policy);
    std::string policyRoot("Formatter.SourceVector");
    // use custom table name patterns for this test
    policy->set(policyRoot + ".Source.perVisitTableNamePattern", "Source_%1%");
    policy->set(policyRoot + ".Source.perSliceAndVisitTableNamePattern", "Source_%1%_%2%");

    Policy::Ptr nested(policy->getPolicy(policyRoot));

    Persistence::Ptr pers = Persistence::getPersistence(policy);
    LogicalLocation loc("mysql://lsst10.ncsa.uiuc.edu:3306/test");

    SourceVector all;
    int const numSlices = 3;
    PropertySet::Ptr props = createDbTestProps(0, numSlices, "Source");

    // 1. Write out each slice table seperately
    for (int sliceId = 0; sliceId < numSlices; ++sliceId) {
        props->set("sliceId", sliceId);
        SourceVector dsv;
        initTestData(dsv, sliceId);

        all.insert(all.end(), dsv.begin(), dsv.end());
        PersistableSourceVector persistVec;
        persistVec.setSources(dsv);
        Storage::List storageList;
        storageList.push_back(pers->getPersistStorage(storageType, loc));
        pers->persist(persistVec, storageList, props);
    }

    // 2. Read in all slice tables - simulates association pipeline
    //    gathering the results of numSlices image processing pipeline slices
    Storage::List storageList;
    storageList.push_back(pers->getRetrieveStorage(storageType, loc));
    Persistable::Ptr p = pers->retrieve("PersistableSourceVector", storageList, props);
    Assert(p != 0, "Failed to retrieve Persistable");
    PersistableSourceVector::Ptr persistPtr = 
    	boost::dynamic_pointer_cast<PersistableSourceVector, Persistable>(p);
    Assert(persistPtr.get() != 0, "Couldn't cast to PersistableSourceVector");
    // sort in ascending id order (database does not give any ordering guarantees
    // in the absence of an ORDER BY clause)
    SourceVector vec = persistPtr->getSources();
    std::sort(vec.begin(), vec.end(), SourceLessThan());
    Assert(vec == all, "persist()/retrieve() resulted in SourceVector corruption");
    afwFormatters::dropAllVisitSliceTables(loc, nested, props);
}


int main(int const argc, char const * const * const argv) {
    try {
        testBoost();
        if (lsst::daf::persistence::DbAuth::available()) {
            testDb("DbStorage");
            testDb("DbTsvStorage");
            testDb2("DbStorage");
            testDb2("DbTsvStorage");
        }
        if (lsst::daf::base::Citizen::census(0) == 0) {
            std::clog << "No leaks detected" << std::endl;
        } else {
            Assert(false, "Detected memory leaks");
        }
        return EXIT_SUCCESS;
    } catch (std::exception const & ex) {
        std::clog << ex.what() << std::endl;
    }

    if (lsst::daf::base::Citizen::census(0) != 0) {
        std::clog << "Leaked memory blocks:" << std::endl;
        lsst::daf::base::Citizen::census(std::clog);
    }

    return EXIT_FAILURE;
}
