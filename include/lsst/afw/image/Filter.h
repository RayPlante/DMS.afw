// -*- lsst-c++ -*-
//
//##====----------------                                ----------------====##/
//
//! \file   Filter.h
//! \brief  Class encapsulating an identifier for an LSST filter.
//
//##====----------------                                ----------------====##/

#ifndef LSST_FW_IMAGE_FILTER_H
#define LSST_FW_IMAGE_FILTER_H

#include <cassert>
#include <string>

#include <lsst/pex/persistence/LogicalLocation.h>
#include <lsst/pex/persistence/DbStorage.h>


namespace lsst {
namespace fw {


/*!
    \brief  Holds an integer identifier for an LSST filter.

    Currently uses a table named \b Filter to map between names and integer identifiers for a filter.
    The \b Filter table is part of the LSST Database Schema.
 */
class Filter {

public :

    enum { U = 0, G, R, I, Z, Y, NUM_FILTERS };

    Filter() : _id(U) {}

    /*! Creates a Filter with the given identifier. Implicit conversions from \c int are allowed. */
    Filter(int id) : _id(id) { assert(id >= U && id < NUM_FILTERS); }

    /*!
        Creates a Filter with the given name, using the \b Filter table in the database currently
        set on the given DbStorage to map the filter name to an integer identifier.
     */
    Filter(lsst::pex::persistence::DbStorage & db, std::string const & name) : _id(nameToId(db, name)) {}

    Filter(lsst::pex::persistence::LogicalLocation const & location, std::string const & name);

    operator int() const { return _id; }
    int getId()    const { return _id; }

    std::string const toString(lsst::pex::persistence::DbStorage & db);
    std::string const toString(lsst::pex::persistence::LogicalLocation const & location);

private :

    int _id;

    static int nameToId(lsst::pex::persistence::DbStorage & db, std::string const & name);
};


}}  // end of namespace lsst::afw

#endif // LSST_FW_IMAGE_FILTER_H
