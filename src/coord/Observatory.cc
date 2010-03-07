// -*- lsst-c++ -*-
/**
 * @file Date.cc
 * @brief Provide functions to handle dates
 * @ingroup afw
 * @author Steve Bickerton
 *
 */
#include <sstream>
#include <cmath>

#include "lsst/pex/exceptions.h"
#include "boost/algorithm/string.hpp"
#include "boost/tuple/tuple.hpp"

#include "lsst/afw/coord/Coord.h"
#include "lsst/afw/coord/Observatory.h"

namespace coord        = lsst::afw::coord;
namespace ex           = lsst::pex::exceptions;

/*
 * @brief 
 *
 */
coord::Observatory::Observatory(std::string const latitude,
                                std::string const longitude,
                                double const elevation) : 
    _latitude(coord::dmsStringToDegrees(latitude)),
    _longitude(coord::dmsStringToDegrees(longitude)),
    _elevation(elevation) {
}

std::string coord::Observatory::getLatitudeStr()  {
    return coord::degreesToDmsString(_latitude);
}
std::string coord::Observatory::getLongitudeStr() {
    return coord::degreesToDmsString(_longitude);
}