// -*- LSST-C++ -*- // fixed format comment for emacs
/**
  * \file Exposure.h
  *
  * \class lsst::fw::Exposure 
  *
  * \ingroup fw
  *
  * \brief Declaration of the templated Exposure Class for LSST.  Create an
  * Exposure from a lsst::fw::MaskedImage.
  *
  * \author Nicole M. Silvestri, University of Washington
  *
  * Contact: nms@astro.washington.edu
  *
  * Created on: Mon Apr 23 1:01:14 2007
  *
  * \version 
  *
  * LSST Legalese here...
  */

#if !defined(EA_D499575A_A50B_4725_A632_F121B26310F0__INCLUDED_)
#define EA_D499575A_A50B_4725_A632_F121B26310F0__INCLUDED_

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

#include <vw/Math/BBox.h>

#include <lsst/mwi/data/LsstBase.h>
#include <lsst/fw/MaskedImage.h>
#include <lsst/fw/WCS.h>

namespace lsst {
namespace fw {
    
    template<class ImageT, class MaskT> class Exposure;
        
    template<typename ImageT, typename MaskT> 
    class Exposure : public lsst::mwi::data::LsstBase {
                
    public:    

        typedef boost::shared_ptr<lsst::fw::WCS> wscPtrType;

        // Class Constructors and Destructor
        explicit Exposure();
        explicit Exposure(MaskedImage<ImageT, MaskT> const &maskedImage);
        explicit Exposure(MaskedImage<ImageT, MaskT> const &maskedImage, WCS const &wcs);
        explicit Exposure(unsigned cols, unsigned rows, WCS const &wcs);
        explicit Exposure(unsigned cols, unsigned rows);
        virtual ~Exposure(); 
        
        // Get Members (getMaskedImage is inline) 
        MaskedImage<ImageT, MaskT> getMaskedImage() const { return _maskedImage; };
        WCS getWcs() const;
        Exposure<ImageT, MaskT> getSubExposure(const vw::BBox2i&) const;
        
        // Set Members
        void setMaskedImage(MaskedImage<ImageT, MaskT> &maskedImage);
        void setWcs(WCS const &wcs);
        
        // Has Member (inline)
        bool hasWcs() const { return static_cast<bool>(_wcsPtr); };
        
        // Read Fits and Write Fits Members
        void readFits(std::string const &expInFile);
        void writeFits(std::string const &expOutFile) const;
        
    private:
        MaskedImage<ImageT, MaskT> _maskedImage;             
        boost::shared_ptr<WCS> _wcsPtr;    
    };     
}} // fw::lsst

#endif // !defined(EA_D499575A_A50B_4725_A632_F121B26310F0__INCLUDED_)
