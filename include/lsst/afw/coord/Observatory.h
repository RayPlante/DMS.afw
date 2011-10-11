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
 
#if !defined(LSST_AFW_COORD_OBSERVATORY_H)
#define LSST_AFW_COORD_OBSERVATORY_H
/**
 * @file
 * @brief Class to hold observatory information
 * @ingroup afw
 * @author Steve Bickerton
 *
 *
 */ 

#include <iostream>
#include "lsst/afw/coord/Utils.h"
#include "lsst/afw/geom/Angle.h"

namespace lsst {
namespace afw {    
namespace coord {


/**
 * @class Observatory
 * @brief Store information about an observatory ... lat/long, elevation
 */
class Observatory {
public:
    
    Observatory(lsst::afw::geom::Angle const longitude, lsst::afw::geom::Angle const latitude, double const elevation);
    Observatory(std::string const longitude, std::string const latitude, double const elevation);
    
    void setLatitude(lsst::afw::geom::Angle const latitude);
    void setLongitude(lsst::afw::geom::Angle const longitude);
    void setElevation(double const elevation);
    
    lsst::afw::geom::Angle getLatitude() const;
    lsst::afw::geom::Angle getLongitude() const;
    double getElevation() const { return _elevation; }
    
    std::string getLatitudeStr() const;
    std::string getLongitudeStr() const;

    bool operator==(Observatory const& rhs) const {
        return
            ((_latitude - rhs._latitude) == 0.0) &&
            ((_longitude - rhs._longitude) == 0.0) &&
            ((_elevation - rhs._elevation) == 0.0);
    }
    bool operator!=(Observatory const& rhs) const {
        return !(*this == rhs);
    }

private:
    lsst::afw::geom::Angle _latitude;
    lsst::afw::geom::Angle _longitude;
    double _elevation;
};

std::ostream & operator<<(std::ostream &os, Observatory const& obs);

}}}

#endif
