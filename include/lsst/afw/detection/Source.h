// -*- lsst-c++ -*-
//
//##====----------------                                ----------------====##/
//
//! \file
//! \brief  The C++ representation of a Deep Detection Source.
//
//##====----------------                                ----------------====##/

#ifndef LSST_AFW_DETECTION_SOURCE_H
#define LSST_AFW_DETECTION_SOURCE_H

#include <bitset>
#include <string>
#include <vector>

#include "boost/shared_ptr.hpp"

#include "lsst/daf/base/Citizen.h"
#include "lsst/daf/base/Persistable.h"
#include "lsst/afw/detection/BaseSourceAttributes.h"

namespace boost {
namespace serialization {
    class access;
}}

namespace lsst {
namespace afw {
    namespace formatters {
        class SourceVectorFormatter;
    }
    
namespace detection {

/*! An integer id for each nullable field in Source. */
enum SourceNullableField {
    AMP_EXPOSURE_ID = NUM_SHARED_NULLABLE_FIELDS,
    TAI_RANGE,
    X_ASTROM,
    Y_ASTROM,
    PETRO_FLUX,
    PETRO_FLUX_ERR,
    SKY,
    SKY_ERR,
    RA_OBJECT,
    DEC_OBJECT,
    NUM_SOURCE_NULLABLE_FIELDS
};


/**
 * In-code representation of an entry in the Source catalog for
 *   persisting/retrieving Sources
 */
class Source 
    : public BaseSourceAttributes< NUM_SOURCE_NULLABLE_FIELDS> {
public :
    typedef boost::shared_ptr<Source> Ptr;

    Source();
    Source(Source const & other);  
    virtual ~Source(){};

    // getters
    boost::int64_t getSourceId() const { return _id; }
    double getPetroFlux() const { return _petroFlux; }
    float  getPetroFluxErr() const { return _petroFluxErr; }    
    float  getSky() const { return _sky; }
    float  getSkyErr() const { return _skyErr; }
    double getRaObject() const { return _raObject; }
    double getDecObject() const { return _decObject; }

    // setters
    void setSourceId( boost::int64_t const sourceId) {setId(sourceId);}

    void setPetroFlux(double const petroFlux) { 
        set(_petroFlux, petroFlux, PETRO_FLUX);         
    }
    void setPetroFluxErr(float const petroFluxErr) { 
        set(_petroFluxErr, petroFluxErr, PETRO_FLUX_ERR);    
    }
    void setSky(float const sky) { 
        set(_sky, sky, SKY);       
    }
    void setSkyErr (float const skyErr) {
        set(_skyErr, skyErr, SKY_ERR);
    }
    void setRaObject(double const raObject) {
        set(_raObject, raObject, RA_OBJECT);
    }
    void setDecObject(double const decObject) {
        set(_decObject, decObject, DEC_OBJECT);
    }
    
    //overloaded setters
    //Because these fields are not NULLABLE in all sources, 
    //  special behavior must be defined in the derived class
    void setAmpExposureId (boost::int64_t const ampExposureId) { 
        set(_ampExposureId, ampExposureId, AMP_EXPOSURE_ID);
    }
    void setTaiRange (double const taiRange) { 
        set(_taiRange, taiRange, TAI_RANGE);         
    }
    void setXAstrom(double const xAstrom) { 
        set(_xAstrom, xAstrom, X_ASTROM);            
    }
    void setYAstrom(double const yAstrom) { 
        set(_yAstrom, yAstrom, Y_ASTROM);            
    }
    
    bool operator==(Source const & d) const;

private :
    double _raObject;
    double _decObject;
    double _petroFlux;  
    float  _petroFluxErr;
    float  _sky;
    float  _skyErr;

    template <typename Archive> 
    void serialize(Archive & ar, unsigned int const version) {
        ar & _raObject;
        ar & _decObject;
        ar & _petroFlux;
        ar & _petroFluxErr;
        ar & _sky;
        ar & _skyErr;

        BaseSourceAttributes<NUM_SOURCE_NULLABLE_FIELDS>::serialize(ar, version);
    }

    friend class boost::serialization::access;
    friend class formatters::SourceVectorFormatter;   
};

inline bool operator!=(Source const & lhs, Source const & rhs) {
    return !(lhs==rhs);
}


typedef std::vector<Source::Ptr> SourceSet;
 
class PersistableSourceVector : public lsst::daf::base::Persistable {
public:
    typedef boost::shared_ptr<PersistableSourceVector> Ptr;
    PersistableSourceVector() {}
    PersistableSourceVector(SourceSet const & sources)
        : _sources(sources) {}
    ~PersistableSourceVector(){_sources.clear();}
        
    SourceSet getSources() const {return _sources; }
    void setSources(SourceSet const & sources) {_sources = sources; }
    
    bool operator==(SourceSet const & other) const {
        if (_sources.size() != other.size())
            return false;
                    
        SourceSet::size_type i;
        for (i = 0; i < _sources.size(); ++i) {
            if (*_sources[i] != *other[i])
                return false;            
        }
        
        return true;
    }
    
    bool operator==(PersistableSourceVector const & other) const {
        return other==_sources;
    }
private:

    LSST_PERSIST_FORMATTER(lsst::afw::formatters::SourceVectorFormatter)
    SourceSet _sources;
}; 


}}}  // namespace lsst::afw::detection

#endif // LSST_AFW_DETECTION_SOURCE_H

