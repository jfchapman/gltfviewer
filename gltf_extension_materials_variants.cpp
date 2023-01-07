#include "gltf_extension_materials_variants.h"

#include <nlohmann/json.hpp>

namespace gltfviewer {

std::unique_ptr<Microsoft::glTF::Extension> KHR_materials_variants::Clone() const
{
    return std::make_unique<KHR_materials_variants>( *this );
}

bool KHR_materials_variants::IsEqual( const Microsoft::glTF::Extension& rhs ) const
{
  const KHR_materials_variants* extension = dynamic_cast<const KHR_materials_variants*>( &rhs );
  return ( nullptr != extension ) && ( m_variants == extension->m_variants ) && ( m_mappings == m_mappings );
}

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_variants( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer )
{
  try {
    auto extension = std::make_unique<KHR_materials_variants>();
    const auto document = nlohmann::json::parse( json );
    if ( auto variantArray = document.find( "variants" ); ( document.end() != variantArray ) && variantArray->is_array() ) {
      for ( const auto& variantObject : *variantArray ) {
        if ( variantObject.is_object() ) {
          if ( const auto variantName = variantObject.find( "name" ); ( variantObject.end() != variantName ) && variantName->is_string() ) {
            extension->m_variants.push_back( *variantName );
          } else {
            extension->m_variants.push_back( "Variant " + std::to_string( extension->m_variants.size() ) );
          }
        }
      }
    } else if ( auto mappingArray = document.find( "mappings" ); ( document.end() != mappingArray ) && mappingArray->is_array() ) {
      for ( const auto& mappingObject : *mappingArray ) {
        if ( mappingObject.is_object() ) {
          if ( const auto materialIndex = mappingObject.find( "material" ); ( mappingObject.end() != materialIndex ) && materialIndex->is_number_unsigned() ) {
            if ( const auto variantArray = mappingObject.find( "variants" ); ( mappingObject.end() != variantArray ) && variantArray->is_array() ) {
              for ( const auto& variantIndex : *variantArray ) {
                if ( variantIndex.is_number_unsigned() ) {
                  extension->m_mappings.insert( { variantIndex, std::to_string( static_cast<uint32_t>( *materialIndex ) ) } );
                }
              }
            }
          }
        }
      }
    }
    return extension;
  } catch ( const nlohmann::json::exception& ) {
    return nullptr;
  }
}

} // namespace gltfviewer
