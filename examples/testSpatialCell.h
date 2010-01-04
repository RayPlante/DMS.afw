#if !defined(TESTSPATIALCELL_H)
#define TESTSPATIALCELL_H
#include "boost/shared_ptr.hpp"
#include "lsst/pex/policy.h"
#include "lsst/afw/math.h"

/************************************************************************************************************/
/*
 * Test class for SpatialCellImageCandidate
 */
class ExampleCandidate : public lsst::afw::math::SpatialCellImageCandidate<lsst::afw::image::Image<float> > {
public:
    typedef boost::shared_ptr<ExampleCandidate> Ptr;
    typedef lsst::afw::image::Image<float> ImageT;

    ExampleCandidate(float const xCenter, float const yCenter,
                       ImageT::ConstPtr parent, lsst::afw::image::BBox bbox);

    lsst::afw::image::BBox getBBox() const { return _bbox; }

    double getCandidateRating() const;

    ImageT::ConstPtr getImage() const;
private:
    ExampleCandidate::ImageT::ConstPtr _parent;
    lsst::afw::image::BBox _bbox;
};

/************************************************************************************************************/
/*
 * Class to pass around to all our ExampleCandidates.  All this one does is count acceptable candidates
 */
class ExampleCandidateVisitor : public lsst::afw::math::CandidateVisitor {
public:
    ExampleCandidateVisitor() : lsst::afw::math::CandidateVisitor(), _n(0), _npix(0) {}

    // Called by SpatialCellSet::visitCandidates before visiting any Candidates
    void reset() { _n = _npix = 0; }

    // Called by SpatialCellSet::visitCandidates for each Candidate
    void processCandidate(lsst::afw::math::SpatialCellCandidate *candidate) {
        ++_n;

        int w, h;
        boost::tie(w, h) = dynamic_cast<ExampleCandidate *>(candidate)->getBBox().getDimensions();
        _npix += w*h;
    }

    int getN() const { return _n; }
    int getNPix() const { return _npix; }
private:
    int _n;                         // number of ExampleCandidates
    int _npix;                      // number of pixels in ExampleCandidates's bounding boxes
};

#endif