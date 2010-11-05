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
 

/** @file
 * @brief Implementation of WcsFormatter class
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

// not used? #include <stdlib.h>

//#include "boost/serialization/shared_ptr.hpp"
#include "wcslib/wcs.h"

#include "lsst/daf/base.h"
#include "lsst/daf/persistence.h"
#include "lsst/daf/persistence/PropertySetFormatter.h"
#include "lsst/pex/exceptions.h"
#include "lsst/pex/logging/Trace.h"
#include "lsst/afw/formatters/ImageFormatter.h"
#include "lsst/afw/formatters/MaskedImageFormatter.h"
#include "lsst/afw/formatters/WcsFormatter.h"
#include "lsst/afw/image/Wcs.h"

#define EXEC_TRACE  20
static void execTrace(std::string s, int level = EXEC_TRACE) {
    lsst::pex::logging::Trace("afw.WcsFormatter", level, s);
}


namespace afwForm = lsst::afw::formatters;
namespace afwImg = lsst::afw::image;
namespace dafBase = lsst::daf::base;
namespace dafPersist = lsst::daf::persistence;
namespace pexPolicy = lsst::pex::policy;
namespace pexExcept = lsst::pex::exceptions;


dafPersist::FormatterRegistration afwForm::WcsFormatter::registration(
    "Wcs", typeid(afwImg::Wcs), createInstance);

afwForm::WcsFormatter::WcsFormatter(
    pexPolicy::Policy::Ptr) :
    dafPersist::Formatter(typeid(this)) {
}

afwForm::WcsFormatter::~WcsFormatter(void) {
}

void afwForm::WcsFormatter::write(
    dafBase::Persistable const* persistable,
    dafPersist::Storage::Ptr storage,
    dafBase::PropertySet::Ptr) {
    execTrace("WcsFormatter write start");
    afwImg::Wcs const* ip =
        dynamic_cast<afwImg::Wcs const*>(persistable);
    if (ip == 0) {
        throw LSST_EXCEPT(pexExcept::RuntimeErrorException, "Persisting non-Wcs");
    }
    if (typeid(*storage) == typeid(dafPersist::BoostStorage)) {
        execTrace("WcsFormatter write BoostStorage");
        dafPersist::BoostStorage* boost = dynamic_cast<dafPersist::BoostStorage*>(storage.get());
        boost->getOArchive() & *ip;
        execTrace("WcsFormatter write end");
        return;
    }
    throw LSST_EXCEPT(pexExcept::RuntimeErrorException, "Unrecognized Storage for Wcs");
}

dafBase::Persistable* afwForm::WcsFormatter::read(
    dafPersist::Storage::Ptr storage,
    dafBase::PropertySet::Ptr additionalData) {
    execTrace("WcsFormatter read start");
    if (typeid(*storage) == typeid(dafPersist::BoostStorage)) {
        afwImg::Wcs* ip = new afwImg::Wcs;
        execTrace("WcsFormatter read BoostStorage");
        dafPersist::BoostStorage* boost = dynamic_cast<dafPersist::BoostStorage*>(storage.get());
        boost->getIArchive() & *ip;
        execTrace("WcsFormatter read end");
        return ip;
    }
    else if (typeid(*storage) == typeid(dafPersist::FitsStorage)) {
        execTrace("WcsFormatter read FitsStorage");
        dafPersist::FitsStorage* fits = dynamic_cast<dafPersist::FitsStorage*>(storage.get());
        int hdu = additionalData->get<int>("hdu", 0);
        dafBase::PropertySet::Ptr md =
            afwImg::readMetadata(fits->getPath(), hdu);
        afwImg::Wcs* ip = new afwImg::Wcs(md);
        execTrace("WcsFormatter read end");
        return ip;
    }
    throw LSST_EXCEPT(pexExcept::RuntimeErrorException, "Unrecognized Storage for Wcs");
}

void afwForm::WcsFormatter::update(
    dafBase::Persistable*,
    dafPersist::Storage::Ptr,
    dafBase::PropertySet::Ptr) {
    throw LSST_EXCEPT(pexExcept::RuntimeErrorException, "Unexpected call to update for Wcs");
}


dafBase::PropertySet::Ptr
afwForm::WcsFormatter::generatePropertySet(afwImg::Wcs const& wcs) {
    // Only generates properties for the first wcsInfo.
    dafBase::PropertySet::Ptr wcsProps(new dafBase::PropertySet());

    if (!wcs) {                  // if wcs hasn't been initialised
        return wcsProps;
    }

    wcsProps->add("NAXIS", wcs._wcsInfo[0].naxis);
    wcsProps->add("EQUINOX", wcs._wcsInfo[0].equinox);
    wcsProps->add("RADESYS", std::string(wcs._wcsInfo[0].radesys));
    wcsProps->add("CRPIX1", wcs._wcsInfo[0].crpix[0]);
    wcsProps->add("CRPIX2", wcs._wcsInfo[0].crpix[1]);
    wcsProps->add("CD1_1", wcs._wcsInfo[0].cd[0]);
    wcsProps->add("CD1_2", wcs._wcsInfo[0].cd[1]);
    wcsProps->add("CD2_1", wcs._wcsInfo[0].cd[2]);
    wcsProps->add("CD2_2", wcs._wcsInfo[0].cd[3]);
    wcsProps->add("CRVAL1", wcs._wcsInfo[0].crval[0]);
    wcsProps->add("CRVAL2", wcs._wcsInfo[0].crval[1]);
    wcsProps->add("CUNIT1", std::string(wcs._wcsInfo[0].cunit[0]));
    wcsProps->add("CUNIT2", std::string(wcs._wcsInfo[0].cunit[1]));

    std::string ctype1(wcs._wcsInfo[0].ctype[0]);
    std::string ctype2(wcs._wcsInfo[0].ctype[1]);
    wcsProps->add("CTYPE1", ctype1);
    wcsProps->add("CTYPE2", ctype2);

    return wcsProps;
}

template <class Archive>
void afwForm::WcsFormatter::delegateSerialize(
    Archive& ar, int const, dafBase::Persistable* persistable) {
    execTrace("WcsFormatter delegateSerialize start");
    afwImg::Wcs* ip = dynamic_cast<afwImg::Wcs*>(persistable);
    if (ip == 0) {
        throw LSST_EXCEPT(pexExcept::RuntimeErrorException, "Serializing non-Wcs");
    }

    // Serialize most fields normally
    ar & ip->_nWcsInfo & ip->_relax;
    ar & ip->_wcsfixCtrl & ip->_wcshdrCtrl & ip->_nReject;
    
    
    // If we are loading, create the array of Wcs parameter structs
    if (Archive::is_loading::value) {
        ip->_wcsInfo =
            reinterpret_cast<wcsprm*>(malloc(sizeof(wcsprm)));
    }


    // If we are loading, initialize the struct first
    if (Archive::is_loading::value) {
        ip->_wcsInfo[0].flag = -1;
        wcsini(1, 2, &(ip->_wcsInfo[0]));
    }

    // Serialize only critical Wcs parameters
    //wcslib provides support for arrays of wcs', but we only
    //implement support for one.
    ar & ip->_wcsInfo[0].naxis;
    ar & ip->_wcsInfo[0].equinox;
    ar & ip->_wcsInfo[0].radesys;
    ar & ip->_wcsInfo[0].crpix[0];
    ar & ip->_wcsInfo[0].crpix[1];
    ar & ip->_wcsInfo[0].cd[0];
    ar & ip->_wcsInfo[0].cd[1];
    ar & ip->_wcsInfo[0].cd[2];
    ar & ip->_wcsInfo[0].cd[3];
    ar & ip->_wcsInfo[0].crval[0];
    ar & ip->_wcsInfo[0].crval[1];
    ar & ip->_wcsInfo[0].cunit[0];
    ar & ip->_wcsInfo[0].cunit[1];
    ar & ip->_wcsInfo[0].ctype[0];
    ar & ip->_wcsInfo[0].ctype[1];

    // If we are loading, compute intermediate values given those above
    if (Archive::is_loading::value) {
        ip->_wcsInfo[0].flag = 0;
        wcsset(&(ip->_wcsInfo[0]));
    }
    execTrace("WcsFormatter delegateSerialize end");
}

dafPersist::Formatter::Ptr afwForm::WcsFormatter::createInstance(
    pexPolicy::Policy::Ptr policy) {
    return dafPersist::Formatter::Ptr(new afwForm::WcsFormatter(policy));
}

