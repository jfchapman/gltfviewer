#include "gltf_extension_lights_punctual.h"

#include <nlohmann/json.hpp>

namespace gltfviewer {

static const std::map<std::string, Light::Type> kLightTypes = { { "point", Light::Type::Point }, { "spot", Light::Type::Spot }, { "directional", Light::Type::Directional } };

std::unique_ptr<Microsoft::glTF::Extension> KHR_lights_punctual::Clone() const
{
    return std::make_unique<KHR_lights_punctual>( *this );
}

bool KHR_lights_punctual::IsEqual( const Microsoft::glTF::Extension& rhs ) const
{
  const KHR_lights_punctual* extension = dynamic_cast<const KHR_lights_punctual*>( &rhs );
  return ( nullptr != extension ) && ( m_lights == extension->m_lights ) && ( m_lightIndex == m_lightIndex );
}

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_lights_punctual( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer )
{
  try {
    auto extension = std::make_unique<KHR_lights_punctual>();
    const auto document = nlohmann::json::parse( json );
    if ( auto lightArray = document.find( "lights" ); ( document.end() != lightArray ) && lightArray->is_array() ) {
      for ( const auto& lightObject : *lightArray ) {
        if ( lightObject.is_object() ) {
          if ( const auto lightTypeStr = lightObject.find( "type" ); ( lightObject.end() != lightTypeStr ) && lightTypeStr->is_string() ) {
            if ( const auto lightType = kLightTypes.find( *lightTypeStr ); kLightTypes.end() != lightType ) {
              Light light;
              light.m_type = lightType->second;
              if ( const auto it = lightObject.find( "name" ); ( lightObject.end() != it ) && it->is_string() ) {
                light.m_name = *it;
              }
              if ( const auto it = lightObject.find( "color" ); ( lightObject.end() != it ) && it->is_array() && ( 3 == it->size() ) ) {
                if ( it->at( 0 ).is_number() && it->at( 1 ).is_number() && it->at( 2 ).is_number() ) {
                  const float r = it->at( 0 );
                  const float g = it->at( 1 );
                  const float b = it->at( 2 );
                  light.m_color = { r, g, b };
                }
              }
              if ( const auto it = lightObject.find( "intensity" ); ( lightObject.end() != it ) && it->is_number() ) {
                light.m_intensity = *it;
              }
              if ( Light::Type::Spot == light.m_type ) {
                if ( const auto spot = lightObject.find( "spot" ); ( lightObject.end() != spot ) && spot->is_object() ) {
                  if ( const auto it = spot->find( "innerConeAngle" ); ( spot->end() != it ) && it->is_number() ) {
                    light.m_innerConeAngle = std::clamp( static_cast<float>( *it ), 0.0f, static_cast<float>( M_PI_2 ) );
                  }
                  if ( const auto it = spot->find( "outerConeAngle" ); ( spot->end() != it ) && it->is_number() ) {
                    light.m_outerConeAngle = std::clamp( static_cast<float>( *it ), 0.0f, static_cast<float>( M_PI_2 ) );
                  }
                  if ( light.m_innerConeAngle >= light.m_outerConeAngle ) {
                    light.m_outerConeAngle = std::max( light.m_innerConeAngle, light.m_outerConeAngle );
                    light.m_innerConeAngle = 0;
                  }
                }
              }
              extension->m_lights.emplace_back( light );
            }
          }
        }
      }
    } else if ( auto it = document.find( "light" ); ( document.end() != it ) && it->is_number() ) {
      extension->m_lightIndex = *it;
    }
    return extension;
  } catch ( const nlohmann::json::exception& ) {
    return nullptr;
  }
}

} // namespace gltfviewer
