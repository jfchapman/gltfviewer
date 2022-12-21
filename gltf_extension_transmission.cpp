#include "gltf_extension_transmission.h"

#include <nlohmann/json.hpp>

namespace gltfviewer {

std::unique_ptr<Microsoft::glTF::Extension> KHR_materials_transmission::Clone() const
{
    return std::make_unique<KHR_materials_transmission>( *this );
}

bool KHR_materials_transmission::IsEqual( const Microsoft::glTF::Extension& rhs ) const
{
  const KHR_materials_transmission* extension = dynamic_cast<const KHR_materials_transmission*>( &rhs );
  return ( nullptr != extension ) && ( m_transmissionFactor == extension->m_transmissionFactor ) && ( m_transmissionTexture == extension->m_transmissionTexture );
}

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_transmission( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer )
{
  try {
    auto extension = std::make_unique<KHR_materials_transmission>();

    const auto document = nlohmann::json::parse( json );
    if ( auto it = document.find( "transmissionFactor" ); ( document.end() != it ) && it->is_number() ) {
      extension->m_transmissionFactor = *it;
    }
    if ( auto it = document.find( "transmissionTexture" ); ( document.end() != it ) && it->is_object() ) {
      extension->m_transmissionTexture = std::make_optional<Microsoft::glTF::TextureInfo>();
      if ( auto index = it->find( "index" ); ( it->end() != index ) && index->is_number_unsigned() ) {
        const uint32_t texIndex = *index;
        extension->m_transmissionTexture->textureId = std::to_string( texIndex );
      }
      if ( auto texCoord = it->find( "texCoord" ); ( it->end() != texCoord ) && texCoord->is_number_unsigned() ) {
        extension->m_transmissionTexture->texCoord = *texCoord;
      }
    }

    return extension;
  } catch ( const nlohmann::json::exception& ) {
    return nullptr;
  }
}

} // namespace gltfviewer
