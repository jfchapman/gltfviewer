#include "color_processor.h"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

namespace gltfviewer {

ColorProcessor::ColorProcessor( const std::filesystem::path& configPath )
{
  try {
    m_config = OCIO::Config::CreateFromFile( reinterpret_cast<const char*>( configPath.u8string().c_str() ) );
    OCIO::SetCurrentConfig( m_config );
  } catch ( const OCIO::Exception& ) {
    m_config = OCIO::GetCurrentConfig();
  }

  const int lookCount = m_config->getNumLooks();
  for ( int index = 0; index < lookCount; index++ ) {
    m_looks.push_back( m_config->getLookNameByIndex( index ) );
  }
}

static OCIO::ChannelOrdering GetChannelOrdering( const gltfviewer_image& image )
{
  switch ( image.pixel_format ) {
    case gltfviewer_image_pixelformat_floatRGBA:
      return OCIO::CHANNEL_ORDERING_RGBA;
    case gltfviewer_image_pixelformat_ucharBGRA:
      return OCIO::CHANNEL_ORDERING_BGRA;
    default:
      return OCIO::CHANNEL_ORDERING_RGBA;
  }
}

static OCIO::BitDepth GetBitDepth( const gltfviewer_image& image )
{
  switch ( image.pixel_format ) {
    case gltfviewer_image_pixelformat_floatRGBA:
      return OCIO::BIT_DEPTH_F32;
    case gltfviewer_image_pixelformat_ucharBGRA:
      return OCIO::BIT_DEPTH_UINT8;
    default:
      return OCIO::BIT_DEPTH_UNKNOWN;
  }
}

static int GetChanStrideBytes( const gltfviewer_image& image )
{
  switch ( image.pixel_format ) {
    case gltfviewer_image_pixelformat_floatRGBA:
      return 4;
    case gltfviewer_image_pixelformat_ucharBGRA:
      return 1;
    default:
      return 0;
  }
}

static int GetXStrideBytes( const gltfviewer_image& image )
{
  switch ( image.pixel_format ) {
    case gltfviewer_image_pixelformat_floatRGBA:
      return 16;
    case gltfviewer_image_pixelformat_ucharBGRA:
      return 4;
    default:
      return 0;
  }
}

static int GetChannels( const gltfviewer_image& image )
{
  switch ( image.pixel_format ) {
    case gltfviewer_image_pixelformat_floatRGBA:
    case gltfviewer_image_pixelformat_ucharBGRA:
      return 4;
    default:
      return 0;
  }
}

static OpenImageIO_v2_3::TypeDesc GetImageDataType( const gltfviewer_image& image )
{
  switch ( image.pixel_format ) {
    case gltfviewer_image_pixelformat_floatRGBA:
      return OpenImageIO_v2_3::TypeDesc::FLOAT;
    case gltfviewer_image_pixelformat_ucharBGRA:
      return OpenImageIO_v2_3::TypeDesc::UINT8;
    default:
      return OpenImageIO_v2_3::TypeDesc::UNKNOWN;
  }
}

static std::optional<std::vector<int>> GetSwappedChannelOrder( const gltfviewer_image& sourceImage, gltfviewer_image& destImage )
{
  const auto sourceChannelOrder = GetChannelOrdering( sourceImage );
  const auto destChannelOrder = GetChannelOrdering( destImage );

  switch ( sourceChannelOrder ) {
    case OCIO::CHANNEL_ORDERING_RGBA: {
      switch ( destChannelOrder ) {
        case OCIO::CHANNEL_ORDERING_BGRA:
          return std::make_optional<std::vector<int>>( { 2, 1, 0, 3 } );
      }
      break;
    }
    case OCIO::CHANNEL_ORDERING_BGRA: {
      switch ( destChannelOrder ) {
        case OCIO::CHANNEL_ORDERING_RGBA:
          return std::make_optional<std::vector<int>>( { 2, 1, 0, 3 } );
      }
      break;
    }
  }

  return std::nullopt;
}

bool ColorProcessor::Convert( const float exposure, const int32_t look_index, const gltfviewer_image& sourceImage, gltfviewer_image& destImage )
{
  const auto sourceChannelOrder = GetChannelOrdering( sourceImage );
  const auto sourceBitDepth = GetBitDepth( sourceImage );
  const int sourceChanStrideBytes = GetChanStrideBytes( sourceImage );
  const int sourceXStrideBytes = GetXStrideBytes( sourceImage );
  const int sourceYStrideBytes = ( 0 == sourceImage.stride_bytes ) ? ( sourceXStrideBytes * sourceImage.width ) : sourceImage.stride_bytes;
  const int sourceChannels = GetChannels( sourceImage );
  const auto sourceDataType = GetImageDataType( sourceImage );

  if ( ( OCIO::BIT_DEPTH_UNKNOWN == sourceBitDepth ) || ( 0 == sourceChannels ) || ( OpenImageIO_v2_3::TypeDesc::UNKNOWN == sourceDataType ) || ( 0 == sourceImage.width ) || ( 0 == sourceImage.height ) || ( nullptr == sourceImage.pixels ) )
    return false;

  const auto destChannelOrder = GetChannelOrdering( destImage );
  const auto destBitDepth = GetBitDepth( destImage );
  const int destChanStrideBytes = GetChanStrideBytes( destImage );
  const int destXStrideBytes = GetXStrideBytes( destImage );
  const int destYStrideBytes = ( 0 == destImage.stride_bytes ) ? ( destXStrideBytes * destImage.width ) : destImage.stride_bytes;
  const int destChannels = GetChannels( destImage );
  const auto destDataType = GetImageDataType( destImage );

  if ( ( OCIO::BIT_DEPTH_UNKNOWN == destBitDepth ) || ( 0 == destChannels ) || ( OpenImageIO_v2_3::TypeDesc::UNKNOWN == destDataType ) || ( 0 == destImage.width ) || ( 0 == destImage.height ) || ( nullptr == destImage.pixels ) )
    return false;

  std::vector<uint8_t> scratchImagePixels;
  const int sourcePixelDataSize = sourceYStrideBytes * sourceImage.height;
  scratchImagePixels.resize( sourcePixelDataSize );
  gltfviewer_image scratchImage = sourceImage;
  scratchImage.pixels = scratchImagePixels.data();

  if ( ( 0 != exposure ) && ( ( OCIO::CHANNEL_ORDERING_RGBA == sourceChannelOrder ) || ( OCIO::CHANNEL_ORDERING_BGRA == sourceChannelOrder ) ) ) {
    const float value = powf( 2.0f, exposure );

    const OpenImageIO_v2_3::ImageSpec sourceImageSpec( sourceImage.width, sourceImage.height, sourceChannels, sourceDataType );
    const OpenImageIO_v2_3::ImageBuf sourceImageBuf( sourceImageSpec, sourceImage.pixels );

    OpenImageIO_v2_3::ImageSpec scratchImageSpec( scratchImage.width, scratchImage.height, sourceChannels, sourceDataType );
    OpenImageIO_v2_3::ImageBuf scratchImageBuf( scratchImageSpec, scratchImage.pixels );

    OpenImageIO_v2_3::ImageBufAlgo::mul( scratchImageBuf, sourceImageBuf, { value, value, value, 1.0f } );
  } else {
    const uint8_t* sourcePixels = static_cast<const uint8_t*>( sourceImage.pixels );
    std::copy( sourcePixels, sourcePixels + sourcePixelDataSize, scratchImagePixels.data() );
  }

  if ( !ConvertColorSpace( look_index, scratchImage ) && ( gltfviewer_image_convert_linear_to_srgb == look_index ) && ( ( OCIO::CHANNEL_ORDERING_RGBA == sourceChannelOrder ) || ( OCIO::CHANNEL_ORDERING_BGRA == sourceChannelOrder ) ) ) {
    // Apply a simple gamma adjustment if OCIO color conversion failed.
    constexpr float gamma = 1 / 2.2f;
    OpenImageIO_v2_3::ImageSpec scratchImageSpec( scratchImage.width, scratchImage.height, sourceChannels, sourceDataType );
    OpenImageIO_v2_3::ImageBuf scratchImageBuf( scratchImageSpec, scratchImage.pixels );
    OpenImageIO_v2_3::ImageBufAlgo::pow( scratchImageBuf, scratchImageBuf, { gamma, gamma, gamma, 1.0f } );
  }

  const OpenImageIO_v2_3::ImageSpec scratchImageSpec( scratchImage.width, scratchImage.height, sourceChannels, sourceDataType );
  const OpenImageIO_v2_3::ImageBuf scratchImageBuf( scratchImageSpec, scratchImage.pixels );

  if ( const auto swappedChannelOrder = GetSwappedChannelOrder( sourceImage, destImage ); swappedChannelOrder ) {
    const OpenImageIO_v2_3::ImageBuf swappedImageBuf = OpenImageIO_v2_3::ImageBufAlgo::channels( scratchImageBuf, sourceChannels, *swappedChannelOrder );
    swappedImageBuf.get_pixels( {}, sourceDataType, scratchImage.pixels );
  }

  if ( ( sourceImage.width == destImage.width ) && ( sourceImage.height == destImage.height ) ) {
    OpenImageIO_v2_3::ImageSpec targetImageSpec( destImage.width, destImage.height, destChannels, destDataType );
    OpenImageIO_v2_3::ImageBuf targetImageBuf( targetImageSpec, destImage.pixels );
    return targetImageBuf.copy_pixels( scratchImageBuf );
  } else {
    OpenImageIO_v2_3::ImageSpec targetImageSpec( destImage.width, destImage.height, destChannels, destDataType );
    OpenImageIO_v2_3::ImageBuf targetImageBuf( targetImageSpec, destImage.pixels );
    const OpenImageIO_v2_3::ROI roi( 0, destImage.width, 0, destImage.height );
    return OpenImageIO_v2_3::ImageBufAlgo::resize( targetImageBuf, scratchImageBuf, nullptr, roi );
  }

  return false;
}

bool ColorProcessor::ConvertColorSpace( const int32_t look_index, gltfviewer_image& image )
{
  try {
    OCIO::ConstProcessorRcPtr processor;
    if ( gltfviewer_image_convert_linear_to_srgb == look_index ) {
      processor = m_config->getProcessor( "scene_linear", "default_sequencer" );
    } else if ( ( look_index >= 0 ) && ( look_index < m_looks.size() ) ) {
      const auto look_name = m_looks[ look_index ];
      OCIO::ConstLookRcPtr look = m_config->getLook( look_name.c_str() );
      if ( !look )
        return false;

      const std::string colorSpace = look->getProcessSpace();
      if ( colorSpace.empty() )
        return false;

      OCIO::LookTransformRcPtr transform = OCIO::LookTransform::Create();
      transform->setLooks( look_name.c_str() );
      transform->setSrc( "scene_linear" );
      transform->setDst( colorSpace.c_str() );
      processor = m_config->getProcessor( transform );
    }

    if ( !processor )
      return false;

    OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
    if ( !cpuProcessor )
      return false;

    const auto channelOrder = GetChannelOrdering( image );
    const auto bitDepth = GetBitDepth( image );
    const int chanStrideBytes = GetChanStrideBytes( image );
    const int xStrideBytes = GetXStrideBytes( image );
    const int yStrideBytes = ( 0 == image.stride_bytes ) ? ( xStrideBytes * image.width ) : image.stride_bytes;

    if ( ( OCIO::BIT_DEPTH_UNKNOWN != bitDepth ) && ( image.width > 0 ) && ( image.height > 0 ) ) {
      OCIO::PackedImageDesc imageDesc( image.pixels, image.width, image.height, channelOrder, bitDepth, chanStrideBytes, xStrideBytes, yStrideBytes );
      cpuProcessor->apply( imageDesc );
      return true;
    }
  } catch ( const OCIO::Exception& ) {
  }
  return false;
}

} // namespace gltfviewer
