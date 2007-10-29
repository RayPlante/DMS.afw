// __BEGIN_LICENSE__
// 
// Copyright (C) 2006 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration
// (NASA).  All Rights Reserved.
// 
// Copyright 2006 Carnegie Mellon University. All rights reserved.
// 
// This software is distributed under the NASA Open Source Agreement
// (NOSA), version 1.3.  The NOSA has been approved by the Open Source
// Initiative.  See the file COPYING at the top of the distribution
// directory tree for the complete NOSA document.
// 
// THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF ANY
// KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT
// LIMITED TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO
// SPECIFICATIONS, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR
// A PARTICULAR PURPOSE, OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT
// THE SUBJECT SOFTWARE WILL BE ERROR FREE, OR ANY WARRANTY THAT
// DOCUMENTATION, IF PROVIDED, WILL CONFORM TO THE SUBJECT SOFTWARE.
// 
// __END_LICENSE__

//
// This file is a copy of src/vw/Image/ImageResource.cc, modified
// by adding a bool argument to convert, and disabling the automatic
// scaling of float images to [0, 1]

/// \file
/// 
/// Defines a run-type-typed image buffer.
///
#ifdef _MSC_VER
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4996)
#include <vector>
#endif

#include <map>

#include <boost/integer_traits.hpp>

#include <vw/Core/Debugging.h>
#include <vw/Image/PixelTypes.h>
#include <vw/Image/ImageResource.h>
using namespace vw;

typedef void (*channel_convert_func)(void* src, void* dest);

template <class SrcT, class DestT>
void channel_convert_cast( SrcT* src, DestT* dest ) {
  *dest = DestT(*src);
}

namespace {                             // don't put these into the global namespace
    void channel_convert_uint16_to_uint8( uint16* src, uint8* dest ) {
        *dest = uint8( *src / (65535/255) );
    }
    
    void channel_convert_uint8_to_uint16( uint8* src, uint16* dest ) {
        *dest = uint16( *src ) * (65535/255);
    }
}

template <class SrcT, class DestT>
void channel_convert_int_to_float( SrcT* src, DestT* dest ) {
    *dest =
#if 0                                   // original vw code
        DestT(*src) * (DestT(1.0)/boost::integer_traits<SrcT>::const_max)
#else
        DestT(*src)
#endif
        ;
}

template <class SrcT, class DestT>
void channel_convert_float_to_int( SrcT* src, DestT* dest ) {
  if( *src > SrcT(1.0) ) *dest = boost::integer_traits<DestT>::const_max;
  else if( *src < SrcT(0.0) ) *dest = DestT(0);
  else *dest = DestT( *src * boost::integer_traits<DestT>::const_max );
}

std::map<std::pair<ChannelTypeEnum,ChannelTypeEnum>,channel_convert_func> *channel_convert_map = 0;

class ChannelConvertMapEntry {
public:
  template <class SrcT, class DstT>
  ChannelConvertMapEntry( void (*func)(SrcT*,DstT*) ) {
    if( !channel_convert_map )
      channel_convert_map = new std::map<std::pair<ChannelTypeEnum,ChannelTypeEnum>,channel_convert_func>();
    ChannelTypeEnum src = ChannelTypeID<SrcT>::value;
    ChannelTypeEnum dst = ChannelTypeID<DstT>::value;
    channel_convert_map->operator[]( std::make_pair(src,dst) ) = (channel_convert_func)func;
  }
};

ChannelConvertMapEntry _conv_i8i8  ( &channel_convert_cast<int8,int8>   );
ChannelConvertMapEntry _conv_i8u8  ( &channel_convert_cast<int8,uint8>  );
ChannelConvertMapEntry _conv_i8i16 ( &channel_convert_cast<int8,int16>  );
ChannelConvertMapEntry _conv_i8u16 ( &channel_convert_cast<int8,uint16> );
ChannelConvertMapEntry _conv_i8i32 ( &channel_convert_cast<int8,int32>  );
ChannelConvertMapEntry _conv_i8u32 ( &channel_convert_cast<int8,uint32> );
ChannelConvertMapEntry _conv_i8i64 ( &channel_convert_cast<int8,int64>  );
ChannelConvertMapEntry _conv_i8u64 ( &channel_convert_cast<int8,uint64> );
ChannelConvertMapEntry _conv_i8f32 ( &channel_convert_int_to_float<int8,float>  );
ChannelConvertMapEntry _conv_i8f64 ( &channel_convert_int_to_float<int8,double> );
ChannelConvertMapEntry _conv_u8i8  ( &channel_convert_cast<uint8,int8>   );
ChannelConvertMapEntry _conv_u8u8  ( &channel_convert_cast<uint8,uint8>  );
ChannelConvertMapEntry _conv_u8i16 ( &channel_convert_cast<uint8,int16>  );
ChannelConvertMapEntry _conv_u8u16 ( &channel_convert_uint8_to_uint16 );
ChannelConvertMapEntry _conv_u8i32 ( &channel_convert_cast<uint8,int32>  );
ChannelConvertMapEntry _conv_u8u32 ( &channel_convert_cast<uint8,uint32> );
ChannelConvertMapEntry _conv_u8i64 ( &channel_convert_cast<uint8,int64>  );
ChannelConvertMapEntry _conv_u8u64 ( &channel_convert_cast<uint8,uint64> );
ChannelConvertMapEntry _conv_u8f32 ( &channel_convert_int_to_float<uint8,float>  );
ChannelConvertMapEntry _conv_u8f64 ( &channel_convert_int_to_float<uint8,double> );
ChannelConvertMapEntry _conv_i16i8 ( &channel_convert_cast<int16,int8>   );
ChannelConvertMapEntry _conv_i16u8 ( &channel_convert_cast<int16,uint8>  );
ChannelConvertMapEntry _conv_i16i16( &channel_convert_cast<int16,int16>  );
ChannelConvertMapEntry _conv_i16u16( &channel_convert_cast<int16,uint16> );
ChannelConvertMapEntry _conv_i16i32( &channel_convert_cast<int16,int32>  );
ChannelConvertMapEntry _conv_i16u32( &channel_convert_cast<int16,uint32> );
ChannelConvertMapEntry _conv_i16i64( &channel_convert_cast<int16,int64>  );
ChannelConvertMapEntry _conv_i16u64( &channel_convert_cast<int16,uint64> );
ChannelConvertMapEntry _conv_i16f32( &channel_convert_int_to_float<int16,float>  );
ChannelConvertMapEntry _conv_i16f64( &channel_convert_int_to_float<int16,double> );
ChannelConvertMapEntry _conv_u16i8 ( &channel_convert_cast<uint16,int8>   );
ChannelConvertMapEntry _conv_u16u8 ( &channel_convert_uint16_to_uint8 );
ChannelConvertMapEntry _conv_u16i16( &channel_convert_cast<uint16,int16>  );
ChannelConvertMapEntry _conv_u16u16( &channel_convert_cast<uint16,uint16> );
ChannelConvertMapEntry _conv_u16i32( &channel_convert_cast<uint16,int32>  );
ChannelConvertMapEntry _conv_u16u32( &channel_convert_cast<uint16,uint32> );
ChannelConvertMapEntry _conv_u16i64( &channel_convert_cast<uint16,int64>  );
ChannelConvertMapEntry _conv_u16u64( &channel_convert_cast<uint16,uint64> );
ChannelConvertMapEntry _conv_u16f32( &channel_convert_int_to_float<uint16,float>  );
ChannelConvertMapEntry _conv_u16f64( &channel_convert_int_to_float<uint16,double> );
ChannelConvertMapEntry _conv_i32i8 ( &channel_convert_cast<int32,int8>   );
ChannelConvertMapEntry _conv_i32u8 ( &channel_convert_cast<int32,uint8>  );
ChannelConvertMapEntry _conv_i32i16( &channel_convert_cast<int32,int16>  );
ChannelConvertMapEntry _conv_i32u16( &channel_convert_cast<int32,uint16> );
ChannelConvertMapEntry _conv_i32i32( &channel_convert_cast<int32,int32>  );
ChannelConvertMapEntry _conv_i32u32( &channel_convert_cast<int32,uint32> );
ChannelConvertMapEntry _conv_i32i64( &channel_convert_cast<int32,int64>  );
ChannelConvertMapEntry _conv_i32u64( &channel_convert_cast<int32,uint64> );
ChannelConvertMapEntry _conv_i32f32( &channel_convert_int_to_float<int32,float>  );
ChannelConvertMapEntry _conv_i32f64( &channel_convert_int_to_float<int32,double> );
ChannelConvertMapEntry _conv_u32i8 ( &channel_convert_cast<uint32,int8>   );
ChannelConvertMapEntry _conv_u32u8 ( &channel_convert_cast<uint32,uint8>  );
ChannelConvertMapEntry _conv_u32i16( &channel_convert_cast<uint32,int16>  );
ChannelConvertMapEntry _conv_u32u16( &channel_convert_cast<uint32,uint16> );
ChannelConvertMapEntry _conv_u32i32( &channel_convert_cast<uint32,int32>  );
ChannelConvertMapEntry _conv_u32u32( &channel_convert_cast<uint32,uint32> );
ChannelConvertMapEntry _conv_u32i64( &channel_convert_cast<uint32,int64>  );
ChannelConvertMapEntry _conv_u32u64( &channel_convert_cast<uint32,uint64> );
ChannelConvertMapEntry _conv_u32f32( &channel_convert_int_to_float<uint32,float>  );
ChannelConvertMapEntry _conv_u32f64( &channel_convert_int_to_float<uint32,double> );
ChannelConvertMapEntry _conv_i64i8 ( &channel_convert_cast<int64,int8>   );
ChannelConvertMapEntry _conv_i64u8 ( &channel_convert_cast<int64,uint8>  );
ChannelConvertMapEntry _conv_i64i16( &channel_convert_cast<int64,int16>  );
ChannelConvertMapEntry _conv_i64u16( &channel_convert_cast<int64,uint16> );
ChannelConvertMapEntry _conv_i64i32( &channel_convert_cast<int64,int32>  );
ChannelConvertMapEntry _conv_i64u32( &channel_convert_cast<int64,uint32> );
ChannelConvertMapEntry _conv_i64i64( &channel_convert_cast<int64,int64>  );
ChannelConvertMapEntry _conv_i64u64( &channel_convert_cast<int64,uint64> );
ChannelConvertMapEntry _conv_i64f32( &channel_convert_int_to_float<int64,float>  );
ChannelConvertMapEntry _conv_i64f64( &channel_convert_int_to_float<int64,double> );
ChannelConvertMapEntry _conv_u64i8 ( &channel_convert_cast<uint64,int8>   );
ChannelConvertMapEntry _conv_u64u8 ( &channel_convert_cast<uint64,uint8>  );
ChannelConvertMapEntry _conv_u64i16( &channel_convert_cast<uint64,int16>  );
ChannelConvertMapEntry _conv_u64u16( &channel_convert_cast<uint64,uint16> );
ChannelConvertMapEntry _conv_u64i32( &channel_convert_cast<uint64,int32>  );
ChannelConvertMapEntry _conv_u64u32( &channel_convert_cast<uint64,uint32> );
ChannelConvertMapEntry _conv_u64i64( &channel_convert_cast<uint64,int64>  );
ChannelConvertMapEntry _conv_u64u64( &channel_convert_cast<uint64,uint64> );
ChannelConvertMapEntry _conv_u64f32( &channel_convert_int_to_float<uint64,float>  );
ChannelConvertMapEntry _conv_u64f64( &channel_convert_int_to_float<uint64,double> );
ChannelConvertMapEntry _conv_f32i8 ( &channel_convert_float_to_int<float,int8>   );
ChannelConvertMapEntry _conv_f32u8 ( &channel_convert_float_to_int<float,uint8>  );
ChannelConvertMapEntry _conv_f32i16( &channel_convert_float_to_int<float,int16>  );
ChannelConvertMapEntry _conv_f32u16( &channel_convert_float_to_int<float,uint16> );
ChannelConvertMapEntry _conv_f32i32( &channel_convert_float_to_int<float,int32>  );
ChannelConvertMapEntry _conv_f32u32( &channel_convert_float_to_int<float,uint32> );
ChannelConvertMapEntry _conv_f32i64( &channel_convert_float_to_int<float,int64>  );
ChannelConvertMapEntry _conv_f32u64( &channel_convert_float_to_int<float,uint64> );
ChannelConvertMapEntry _conv_f32f32( &channel_convert_cast<float,float>  );
ChannelConvertMapEntry _conv_f32f64( &channel_convert_cast<float,double> );
ChannelConvertMapEntry _conv_f64i8 ( &channel_convert_float_to_int<double,int8>   );
ChannelConvertMapEntry _conv_f64u8 ( &channel_convert_float_to_int<double,uint8>  );
ChannelConvertMapEntry _conv_f64i16( &channel_convert_float_to_int<double,int16>  );
ChannelConvertMapEntry _conv_f64u16( &channel_convert_float_to_int<double,uint16> );
ChannelConvertMapEntry _conv_f64i32( &channel_convert_float_to_int<double,int32>  );
ChannelConvertMapEntry _conv_f64u32( &channel_convert_float_to_int<double,uint32> );
ChannelConvertMapEntry _conv_f64i64( &channel_convert_float_to_int<double,int64>  );
ChannelConvertMapEntry _conv_f64u64( &channel_convert_float_to_int<double,uint64> );
ChannelConvertMapEntry _conv_f64f32( &channel_convert_cast<double,float>  );
ChannelConvertMapEntry _conv_f64f64( &channel_convert_cast<double,double> );

typedef void (*channel_set_max_func)(void* dest);

template <class DestT>
void channel_set_max_int( DestT* dest ) {
  *dest = boost::integer_traits<DestT>::const_max;
}

template <class DestT>
void channel_set_max_float( DestT* dest ) {
  *dest = DestT(1.0);
}

std::map<ChannelTypeEnum,channel_set_max_func> *channel_set_max_map = 0;

class ChannelSetMaxMapEntry {
public:
  template <class DstT>
  ChannelSetMaxMapEntry( void (*func)(DstT*) ) {
    if( !channel_set_max_map )
      channel_set_max_map = new std::map<ChannelTypeEnum,channel_set_max_func>();
    ChannelTypeEnum dst = ChannelTypeID<DstT>::value;
    channel_set_max_map->operator[]( dst ) = (channel_set_max_func)func;
  }
};

ChannelSetMaxMapEntry _setmax_i8 ( &channel_set_max_int<int8> );
ChannelSetMaxMapEntry _setmax_u8 ( &channel_set_max_int<uint8> );
ChannelSetMaxMapEntry _setmax_i16( &channel_set_max_int<int16> );
ChannelSetMaxMapEntry _setmax_u16( &channel_set_max_int<uint16> );
ChannelSetMaxMapEntry _setmax_i32( &channel_set_max_int<int32> );
ChannelSetMaxMapEntry _setmax_u32( &channel_set_max_int<uint32> );
ChannelSetMaxMapEntry _setmax_i64( &channel_set_max_int<int64> );
ChannelSetMaxMapEntry _setmax_u64( &channel_set_max_int<uint64> );
ChannelSetMaxMapEntry _setmax_f32( &channel_set_max_float<float> );
ChannelSetMaxMapEntry _setmax_f64( &channel_set_max_float<double> );

typedef void (*channel_average_func)(void* src, void* dest, unsigned len);

template <class T>
void channel_average( T* src, T* dest, unsigned len ) {
  typename AccumulatorType<T>::type accum = typename AccumulatorType<T>::type();
  for( unsigned i=0; i<len; ++i ) accum += src[i];
  *dest = accum / len;
}

std::map<ChannelTypeEnum,channel_average_func> *channel_average_map = 0;

class ChannelAverageMapEntry {
public:
  template <class T>
  ChannelAverageMapEntry( void (*func)(T*,T*,unsigned) ) {
    if( !channel_average_map )
      channel_average_map = new std::map<ChannelTypeEnum,channel_average_func>();
    ChannelTypeEnum ctid = ChannelTypeID<T>::value;
    channel_average_map->operator[]( ctid ) = (channel_average_func)func;
  }
};

ChannelAverageMapEntry _average_i8 ( &channel_average<int8> );
ChannelAverageMapEntry _average_u8 ( &channel_average<uint8> );
ChannelAverageMapEntry _average_i16( &channel_average<int16> );
ChannelAverageMapEntry _average_u16( &channel_average<uint16> );
ChannelAverageMapEntry _average_i32( &channel_average<int32> );
ChannelAverageMapEntry _average_u32( &channel_average<uint32> );
ChannelAverageMapEntry _average_i64( &channel_average<int64> );
ChannelAverageMapEntry _average_u64( &channel_average<uint64> );
ChannelAverageMapEntry _average_f32( &channel_average<float> );
ChannelAverageMapEntry _average_f64( &channel_average<double> );

typedef void (*channel_premultiply_func)(void* src, void* dst, unsigned len);

template <class T>
void channel_premultiply_int( T* src, T* dst, unsigned len ) {
  double scale = src[len-1] / (double)(boost::integer_traits<T>::const_max);
  for( unsigned i=0; i<len-1; ++i ) dst[i] = T( src[i] * scale );
  dst[len-1] = src[len-1];
}

template <class T>
void channel_premultiply_float( T* src, T* dst, unsigned len ) {
  double scale = (double)(src[len-1]);
  for( unsigned i=0; i<len-1; ++i ) dst[i] = T( src[i] * scale );
  dst[len-1] = src[len-1];
}

std::map<ChannelTypeEnum,channel_premultiply_func> *channel_premultiply_map = 0;

class ChannelPremultiplyMapEntry {
public:
  template <class T>
  ChannelPremultiplyMapEntry( void (*func)(T*,T*,unsigned) ) {
    if( !channel_premultiply_map )
      channel_premultiply_map = new std::map<ChannelTypeEnum,channel_premultiply_func>();
    ChannelTypeEnum ctid = ChannelTypeID<T>::value;
    channel_premultiply_map->operator[]( ctid ) = (channel_premultiply_func)func;
  }
};

ChannelPremultiplyMapEntry _premultiply_i8 ( &channel_premultiply_int<int8> );
ChannelPremultiplyMapEntry _premultiply_u8 ( &channel_premultiply_int<uint8> );
ChannelPremultiplyMapEntry _premultiply_i16( &channel_premultiply_int<int16> );
ChannelPremultiplyMapEntry _premultiply_u16( &channel_premultiply_int<uint16> );
ChannelPremultiplyMapEntry _premultiply_i32( &channel_premultiply_int<int32> );
ChannelPremultiplyMapEntry _premultiply_u32( &channel_premultiply_int<uint32> );
ChannelPremultiplyMapEntry _premultiply_i64( &channel_premultiply_int<int64> );
ChannelPremultiplyMapEntry _premultiply_u64( &channel_premultiply_int<uint64> );
ChannelPremultiplyMapEntry _premultiply_f32( &channel_premultiply_float<float> );
ChannelPremultiplyMapEntry _premultiply_f64( &channel_premultiply_float<double> );

typedef void (*channel_unpremultiply_func)(void* src, void* dst, unsigned len);

template <class T>
void channel_unpremultiply_int( T* src, T* dst, unsigned len ) {
  double scale = src[len-1] / (double)(boost::integer_traits<T>::const_max);
  for( unsigned i=0; i<len-1; ++i ) dst[i] = T( src[i] / scale );
  dst[len-1] = src[len-1];
}

template <class T>
void channel_unpremultiply_float( T* src, T* dst, unsigned len ) {
  double scale = (double)(src[len-1]);
  for( unsigned i=0; i<len-1; ++i ) dst[i] = T( src[i] / scale );
  dst[len-1] = src[len-1];
}

std::map<ChannelTypeEnum,channel_unpremultiply_func> *channel_unpremultiply_map = 0;

class ChannelUnpremultiplyMapEntry {
public:
  template <class T>
  ChannelUnpremultiplyMapEntry( void (*func)(T*,T*,unsigned) ) {
    if( !channel_unpremultiply_map )
      channel_unpremultiply_map = new std::map<ChannelTypeEnum,channel_unpremultiply_func>();
    ChannelTypeEnum ctid = ChannelTypeID<T>::value;
    channel_unpremultiply_map->operator[]( ctid ) = (channel_unpremultiply_func)func;
  }
};

ChannelUnpremultiplyMapEntry _unpremultiply_i8 ( &channel_unpremultiply_int<int8> );
ChannelUnpremultiplyMapEntry _unpremultiply_u8 ( &channel_unpremultiply_int<uint8> );
ChannelUnpremultiplyMapEntry _unpremultiply_i16( &channel_unpremultiply_int<int16> );
ChannelUnpremultiplyMapEntry _unpremultiply_u16( &channel_unpremultiply_int<uint16> );
ChannelUnpremultiplyMapEntry _unpremultiply_i32( &channel_unpremultiply_int<int32> );
ChannelUnpremultiplyMapEntry _unpremultiply_u32( &channel_unpremultiply_int<uint32> );
ChannelUnpremultiplyMapEntry _unpremultiply_i64( &channel_unpremultiply_int<int64> );
ChannelUnpremultiplyMapEntry _unpremultiply_u64( &channel_unpremultiply_int<uint64> );
ChannelUnpremultiplyMapEntry _unpremultiply_f32( &channel_unpremultiply_float<float> );
ChannelUnpremultiplyMapEntry _unpremultiply_f64( &channel_unpremultiply_float<double> );

namespace vw {
void convert( ImageBuffer const& dst, ImageBuffer const& src,
              bool                      //!< Modify signature to disable rescaling src
            ) {
  VW_ASSERT( dst.format.cols==src.format.cols && dst.format.rows==src.format.rows,
             ArgumentErr() << "Destination buffer has wrong size." );

  // We only support a few special conversions, and the general case where 
  // the source and destination formats are the same.  Below we assume that 
  // we're doing a supported conversion, so we check first.
  if( dst.format.pixel_format != src.format.pixel_format ) {
    // We freely convert between multi-channel and multi-plane images,
    // by aliasing the multi-channel buffer as a multi-plane buffer.
    if( src.format.pixel_format==VW_PIXEL_SCALAR && dst.format.planes==1
        && src.format.planes==num_channels( dst.format.pixel_format ) ) {
      ImageBuffer new_dst = dst;
      new_dst.format.pixel_format = VW_PIXEL_SCALAR;
      new_dst.format.planes = src.format.planes;
      new_dst.pstride = channel_size( dst.format.channel_type );
      return convert( new_dst, src );
    }
    else if( dst.format.pixel_format==VW_PIXEL_SCALAR && src.format.planes==1
             && dst.format.planes==num_channels( src.format.pixel_format ) ) {
      ImageBuffer new_src = src;
      new_src.format.pixel_format = VW_PIXEL_SCALAR;
      new_src.format.planes = dst.format.planes;
      new_src.pstride = channel_size( src.format.channel_type );
      return convert( dst, new_src );
    }
    // Other than that, we only support conversion between the core pixel formats
    if( ( src.format.pixel_format!=VW_PIXEL_GRAY && src.format.pixel_format!=VW_PIXEL_GRAYA && 
          src.format.pixel_format!=VW_PIXEL_RGB && src.format.pixel_format!=VW_PIXEL_RGBA ) ||
        ( dst.format.pixel_format!=VW_PIXEL_GRAY && dst.format.pixel_format!=VW_PIXEL_GRAYA && 
          dst.format.pixel_format!=VW_PIXEL_RGB && dst.format.pixel_format!=VW_PIXEL_RGBA ) ) {
      vw_throw( ArgumentErr() << "Source and destination buffers have incompatibile pixel formats ("
                << src.format.pixel_format << " vs. " << dst.format.pixel_format << ")." );
    }
  }

  unsigned src_channels = num_channels( src.format.pixel_format );
  unsigned dst_channels = num_channels( dst.format.pixel_format );
  ptrdiff_t src_chstride = channel_size( src.format.channel_type );
  ptrdiff_t dst_chstride = channel_size( dst.format.channel_type );

  unsigned copy_length = (src_channels==dst_channels) ? src_channels : (src_channels<3) ? 1 : (dst_channels>=3) ? 3 : 0;

  bool unpremultiply_src = (src.format.pixel_format==VW_PIXEL_GRAYA || src.format.pixel_format==VW_PIXEL_RGBA)
    && !src.unpremultiplied && dst.unpremultiplied;
  bool premultiply_src = (src.format.pixel_format==VW_PIXEL_GRAYA || src.format.pixel_format==VW_PIXEL_RGBA)
    && (dst.format.pixel_format==VW_PIXEL_GRAY || dst.format.pixel_format==VW_PIXEL_RGB) && src.unpremultiplied;
  bool premultiply_dst = (dst.format.pixel_format==VW_PIXEL_GRAYA || dst.format.pixel_format==VW_PIXEL_RGBA)
    && (src.format.pixel_format==VW_PIXEL_GRAYA || src.format.pixel_format==VW_PIXEL_RGBA)
    && src.unpremultiplied && !dst.unpremultiplied;

  bool triplicate = src_channels<3 && dst_channels>=3;
  bool average = src_channels >=3 && dst_channels<3;
  bool add_alpha = src_channels%2==1 && dst_channels%2==0;
  bool copy_alpha = src_channels!=dst_channels && src_channels%2==0 && dst_channels%2==0;

  channel_convert_func conv_func = channel_convert_map->operator[](std::make_pair(src.format.channel_type,dst.format.channel_type));
  channel_set_max_func max_func = channel_set_max_map->operator[](dst.format.channel_type);
  channel_average_func avg_func = channel_average_map->operator[](dst.format.channel_type);
  channel_unpremultiply_func unpremultiply_src_func = channel_unpremultiply_map->operator[](src.format.channel_type);
  channel_premultiply_func premultiply_src_func = channel_premultiply_map->operator[](src.format.channel_type);
  channel_premultiply_func premultiply_dst_func = channel_premultiply_map->operator[](dst.format.channel_type);
  if( !conv_func || !max_func || !avg_func || !unpremultiply_src_func || !premultiply_dst_func || !premultiply_src_func ) 
    vw_throw( NoImplErr() << "Unsupported channel type combination in convert (" << src.format.channel_type << ", " << dst.format.channel_type << ")!" );

  unsigned max_channels = std::max( src_channels, dst_channels );

#ifdef _MSC_VER
  std::vector<uint8> src_buf_vec(max_channels*src_chstride);
  uint8 *src_buf = &src_buf_vec[0];
  std::vector<uint8> dst_buf_vec(max_channels*dst_chstride);
  uint8 *dst_buf = &dst_buf_vec[0];
#else
  uint8 src_buf[max_channels*src_chstride];
  uint8 dst_buf[max_channels*dst_chstride];
#endif

  uint8 *src_ptr_p = (uint8*)src.data;
  uint8 *dst_ptr_p = (uint8*)dst.data;
  for( unsigned p=0; p<src.format.planes; ++p ) {
    uint8 *src_ptr_r = src_ptr_p;
    uint8 *dst_ptr_r = dst_ptr_p;
    for( unsigned r=0; r<src.format.rows; ++r ) {
      uint8 *src_ptr_c = src_ptr_r;
      uint8 *dst_ptr_c = dst_ptr_r;
      for( unsigned c=0; c<src.format.cols; ++c ) {

        // Setup the buffers, adjusting premultiplication if needed
        uint8 *src_ptr = src_ptr_c;
        uint8 *dst_ptr = dst_ptr_c;
        if( unpremultiply_src ) {
          unpremultiply_src_func( src_ptr, src_buf, src_channels );
          src_ptr = src_buf;
        }
        else if( premultiply_src ) {
          premultiply_src_func( src_ptr, src_buf, src_channels );
          src_ptr = src_buf;
        }
 
        // Copy/convert, unrolling the common multi-channel cases
        if( copy_length==4 ) {
          conv_func( src_ptr, dst_ptr );
          conv_func( src_ptr+src_chstride, dst_ptr+dst_chstride );
          conv_func( src_ptr+2*src_chstride, dst_ptr+2*dst_chstride );
          conv_func( src_ptr+3*src_chstride, dst_ptr+3*dst_chstride );
        }
        else if( copy_length==3 ) {
          conv_func( src_ptr, dst_ptr );
          conv_func( src_ptr+src_chstride, dst_ptr+dst_chstride );
          conv_func( src_ptr+2*src_chstride, dst_ptr+2*dst_chstride );
        }
        else if( copy_length==2) {
          conv_func( src_ptr, dst_ptr );
          conv_func( src_ptr+src_chstride, dst_ptr+dst_chstride );
        }
        else if( copy_length==1 ) {
          conv_func( src_ptr, dst_ptr );
        }
        else {
          for( unsigned ch=0; ch<copy_length; ++ch ) {
            conv_func( src_ptr+ch*src_chstride, dst_ptr+ch*dst_chstride );
          }
        }

        // Handle the special pixel format conversions
        if( triplicate ) {
          conv_func( src_ptr, dst_ptr+dst_chstride );
          conv_func( src_ptr, dst_ptr+2*dst_chstride );
        }
        else if( average ) {
          for( unsigned ch=0; ch<3; ++ch ) {
            conv_func( src_ptr+ch*src_chstride, dst_buf+ch*dst_chstride );
          }
          avg_func( dst_buf, dst_ptr, 3 );
        }
        if( copy_alpha ) {
          conv_func( src_ptr+(src_channels-1)*src_chstride, dst_ptr+(dst_channels-1)*dst_chstride );
        }
        else if( add_alpha ) {
          max_func( dst_ptr+(dst_channels-1)*dst_chstride );
        }

        // Finally, adjust destination premultiplication if needed
        if( premultiply_dst ) {
          premultiply_dst_func( dst_ptr, dst_ptr, dst_channels );
        }
        src_ptr_c += src.cstride;
        dst_ptr_c += dst.cstride;
      }
      src_ptr_r += src.rstride;
      dst_ptr_r += dst.rstride;
    }
    src_ptr_p += src.pstride;
    dst_ptr_p += dst.pstride;
  }
}
}
