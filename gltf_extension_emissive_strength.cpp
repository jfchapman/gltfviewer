#include "gltf_extension_emissive_strength.h"

#include <nlohmann/json.hpp>

namespace gltfviewer {

std::unique_ptr<Microsoft::glTF::Extension> KHR_materials_emissive_strength::Clone() const
{
    return std::make_unique<KHR_materials_emissive_strength>( *this );
}

bool KHR_materials_emissive_strength::IsEqual( const Microsoft::glTF::Extension& rhs ) const
{
  const KHR_materials_emissive_strength* extension = dynamic_cast<const KHR_materials_emissive_strength*>( &rhs );
  return ( nullptr != extension ) && ( m_emissive_strength == extension->m_emissive_strength );
}

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_emissive_strength( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer )
{
  try {
    auto extension = std::make_unique<KHR_materials_emissive_strength>();
    const auto document = nlohmann::json::parse( json );
    if ( auto it = document.find( "emissiveStrength" ); ( document.end() != it ) && it->is_number() ) {
      extension->m_emissive_strength = *it;
    }
    return extension;
  } catch ( const nlohmann::json::exception& ) {
    return nullptr;
  }
}

} // namespace gltfviewer
