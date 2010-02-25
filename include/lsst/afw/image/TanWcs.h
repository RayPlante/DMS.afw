#ifndef LSST_AFW_IMAGE_TANWCS_H
#define LSST_AFW_IMAGE_TANWCS_H

#include "Eigen/Core.h"
#include "lsst/daf/base.h"
#include "lsst/daf/data/LsstBase.h"
#include "lsst/afw/image/Image.h"
#include "lsst/afw/geom/AffineTransform.h"
#include "lsst/afw/image/Wcs.h" 


struct wcsprm;                          // defined in wcs.h

namespace lsst {
namespace afw {
    namespace formatters {
        class TanWcsFormatter;
    }
namespace image {

/// 
/// @brief Implementation of the WCS standard for the special case of the Gnomic (tangent plane) projection.
/// 
/// This class treats the special case of tangent plane projection. It extends the Wcs standard by 
/// optionally accounting for distortion in the image plane using the Simple Imaging Polynomial (SIP) convention.
/// This convention is described in Shupe et al. (2005) (Astronomical Data Analysis Software and Systems
/// XIV, Asp Conf. Series Vol XXX, Ed: Shopbell et al.), and descibed in some more detail in
/// http://web.ipac.caltech.edu/staff/fmasci/home/wise/codeVdist.html
/// 
/// To convert from pixel coordintates to radec ("intermediate world coordinates"), first use the matrices
/// _sipA and _sipB to calculate undistorted coorinates (i.e where on the chip the image would lie if
/// the optics gave undistorted images), then pass these undistorted coorinates wcsp2s() to calculate radec.
/// 
/// For the reverse, radec -> pixels, convert the radec to undistorted coords, and then use the _sipAp and
/// _sipBp matrices to add in the distortion terms.
/// 
    class TanWcs : public lsst::afw::image::Wcs {
    public:
        typedef boost::shared_ptr<lsst::afw::image::TanWcs> Ptr;    
        typedef boost::shared_ptr<lsst::afw::image::TanWcs const> ConstPtr;    

        //Constructors
        TanWcs();
        friend lsst::afw::image::Wcs::Ptr lsst::afw::image::makeWcs(lsst::daf::base::PropertySet::Ptr \
            fitsMetadata);
        TanWcs(const lsst::afw::image::PointD crval, const lsst::afw::image::PointD crpix, const Eigen::Matrix2d &CD, 
                double equinox=2000, std::string raDecSys="FK5",
                const std::string cunits1="deg", const std::string cunits2="deg"
               );

        TanWcs(const lsst::afw::image::PointD crval, const lsst::afw::image::PointD crpix, const Eigen::Matrix2d &CD, 
                Eigen::MatrixXd const & sipA, 
                Eigen::MatrixXd const & sipB, 
                Eigen::MatrixXd const & sipAp,
                Eigen::MatrixXd const & sipBp,  
                double equinox=2000, std::string raDecSys="FK5",
                const std::string cunits1="deg", const std::string cunits2="deg"
              );

        TanWcs(lsst::afw::image::TanWcs const & rhs);
        TanWcs & operator = (const TanWcs &);        
        inline ~TanWcs() {};
        
        //Accessors
        PointD skyToPixel(const lsst::afw::image::PointD sky) const;
        PointD pixelToSky(const lsst::afw::image::PointD pixel) const;

        PointD skyToPixel(double sky1, double sky2) const;
        PointD pixelToSky(double pixel1, double pixel2) const;
        
        lsst::daf::base::PropertySet::Ptr getFitsMetadata() const;        
        
        lsst::afw::geom::AffineTransform getAffineTransform() const;
        lsst::afw::geom::AffineTransform linearizeAt(lsst::afw::geom::PointD const & pix) const;
        

        //Mutators
       //Because the base class provides the option of creating a Wcs without distortion coefficients
       //we supply a way of setting them here. This also help make code neater by breaking an
       //enormous constructor (above) into two small pieces 
       void setDistortionMatrices(Eigen::MatrixXd const & sipA, 
                                  Eigen::MatrixXd const & sipB,
                                  Eigen::MatrixXd const & sipAp,
                                  Eigen::MatrixXd const & sipBp
                                 );

    private:
        //If you want to create a TanWcs object from a fits header, use makeWcs()
        TanWcs(lsst::daf::base::PropertySet::Ptr const fitsMetadata);
        
        //Allow the formatter to access private goo
        LSST_PERSIST_FORMATTER(lsst::afw::formatters::TanWcsFormatter);

        bool _hasDistortion;
        Eigen::MatrixXd _sipA, _sipB, _sipAp, _sipBp;
    
    };

}}}

#endif
