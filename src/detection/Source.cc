// -*- lsst-c++ -*-
//
//##====----------------                                ----------------====##/
//!
//! \file
//! \brief Support for Sources
//!
//##====----------------                                ----------------====##/

#include "lsst/daf/base.h"
#include "lsst/afw/detection/Source.h"
#include "lsst/pex/exceptions/Runtime.h"

namespace det = lsst::afw::detection;

/**
 * Default Contructor
 */
det::Source::Source()
    : _petroFlux(0.0),
      _petroFluxErr(0.0),
      _sky(0.0),
      _skyErr(0.0)
{}

/**
 * Copy Constructor
 */
det::Source::Source(Source const & other)
    : BaseSourceAttributes<NUM_SOURCE_NULLABLE_FIELDS>(other),
      _petroFlux(other._petroFlux),
      _petroFluxErr(other._petroFluxErr),
      _sky(other._sky),
      _skyErr(other._skyErr)      
{
    for (int i =0; i != NUM_SOURCE_NULLABLE_FIELDS; ++i) {
        setNull(i, other.isNull(i));
    }
}

/**
 * Test for equality between DiaSource
 * \return true if all of the fields are equal or null in both DiaSource
 */
bool det::Source::operator==(Source const & d) const {
    if (areEqual(_id, d._id) &&
        areEqual(_ampExposureId, d._ampExposureId,  AMP_EXPOSURE_ID) &&
        areEqual(_filterId, d._filterId) &&
        areEqual(_objectId, d._objectId,  OBJECT_ID) &&
        areEqual(_movingObjectId, d._movingObjectId,  MOVING_OBJECT_ID) &&
        areEqual(_procHistoryId, d._procHistoryId) &&
        areEqual(_ra, d._ra) &&
        areEqual(_dec, d._dec) &&
        areEqual(_raErrForWcs, d._raErrForWcs) &&
        areEqual(_decErrForWcs, d._decErrForWcs) &&
        areEqual(_raErrForDetection, d._raErrForDetection,  
                RA_ERR_FOR_DETECTION) &&
        areEqual(_decErrForDetection, d._decErrForDetection,  
                DEC_ERR_FOR_DETECTION) &&
        areEqual(_xFlux, d._xFlux,  X_FLUX) &&
        areEqual(_xFluxErr, d._xFluxErr,  X_FLUX_ERR) &&
        areEqual(_yFlux, d._yFlux,  Y_FLUX) &&
        areEqual(_yFluxErr, d._yFluxErr,  Y_FLUX_ERR) &&
        areEqual(_xPeak, d._xPeak,  X_PEAK) &&
        areEqual(_yPeak, d._yPeak,  Y_PEAK) &&
        areEqual(_raPeak, d._raPeak,  RA_PEAK) && 
        areEqual(_decPeak, d._decPeak,  DEC_PEAK) &&
        areEqual(_xAstrom, d._xAstrom,  X_ASTROM) &&
        areEqual(_xAstromErr, d._xAstromErr,  X_ASTROM_ERR) &&
        areEqual(_yAstrom, d._yAstrom,  Y_ASTROM) &&
        areEqual(_yAstromErr, d._yAstromErr,  Y_ASTROM_ERR) &&
        areEqual(_raAstrom, d._raAstrom,  RA_ASTROM) &&
        areEqual(_raAstromErr, d._raAstromErr,  RA_ASTROM_ERR) &&
        areEqual(_decAstrom, d._decAstrom,  DEC_ASTROM) &&
        areEqual(_decAstromErr, d._decAstromErr,  DEC_ASTROM_ERR) &&
        areEqual(_taiMidPoint, d._taiMidPoint) &&
        areEqual(_taiRange, d._taiRange,  TAI_RANGE) &&
        areEqual(_psfFlux, d._psfFlux) &&
        areEqual(_psfFluxErr, d._psfFluxErr) &&
        areEqual(_apFlux, d._apFlux) &&
        areEqual(_apFluxErr, d._apFluxErr) &&
        areEqual(_modelFlux, d._modelFlux) &&
        areEqual(_modelFluxErr, d._modelFluxErr) &&
        areEqual(_petroFlux, d._petroFlux,  PETRO_FLUX) &&
        areEqual(_petroFluxErr, d._petroFluxErr,  PETRO_FLUX_ERR) &&            
        areEqual(_instFlux, d._instFlux) &&
        areEqual(_instFluxErr, d._instFluxErr) &&
        areEqual(_nonGrayCorrFlux, d._nonGrayCorrFlux,  NON_GRAY_CORR_FLUX) &&
        areEqual(_nonGrayCorrFluxErr, d._nonGrayCorrFluxErr,  
                NON_GRAY_CORR_FLUX_ERR) &&
        areEqual(_atmCorrFlux, d._atmCorrFlux,  ATM_CORR_FLUX) &&
        areEqual(_atmCorrFluxErr, d._atmCorrFluxErr,  ATM_CORR_FLUX_ERR) &&
        areEqual(_apDia, d._apDia,  AP_DIA) &&
        areEqual(_snr, d._snr) &&
        areEqual(_chi2, d._chi2) &&
        areEqual(_sky, d._sky,  SKY) &&
        areEqual(_skyErr, d._skyErr,  SKY_ERR) &&
        areEqual(_flagForAssociation, d._flagForAssociation,  
                FLAG_FOR_ASSOCIATION) &&
        areEqual(_flagForDetection, d._flagForDetection,  
                FLAG_FOR_DETECTION) &&
        areEqual(_flagForWcs, d._flagForWcs,  FLAG_FOR_WCS)) 
    {
    	//check NULLABLE field state equality
        for (int i = 0; i < NUM_SOURCE_NULLABLE_FIELDS; ++i) {
            if (isNull(i) != d.isNull(i)) {
                return false;
            }
        }
        return true;
    }
    
    return false;
}
