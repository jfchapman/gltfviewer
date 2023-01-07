#pragma once

#include <GLTFSDK/ExtensionHandlers.h>

#include <optional>

#include "light.h"

namespace gltfviewer {

struct KHR_lights_punctual : Microsoft::glTF::Extension, Microsoft::glTF::glTFProperty
{
    static constexpr const char* Name = "KHR_lights_punctual";

    // Lights array (top level).
    std::vector<Light> m_lights;

    // Light index (node instance level).
    std::optional<uint32_t> m_lightIndex;

    std::unique_ptr<Extension> Clone() const override;
    bool IsEqual( const Extension& rhs ) const override;
};

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_lights_punctual( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer );

} // namespace gltfviewer
