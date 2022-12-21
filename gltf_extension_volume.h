#pragma once

#include <GLTFSDK/ExtensionHandlers.h>

#include <optional>

namespace gltfviewer {

struct KHR_materials_volume : Microsoft::glTF::Extension, Microsoft::glTF::glTFProperty
{
    static constexpr const char* Name = "KHR_materials_volume";

    float m_attenutationDistance = FLT_MAX;
    Microsoft::glTF::Color3 m_attenuationColor = { 1.0f, 1.0f, 1.0f };

    std::unique_ptr<Extension> Clone() const override;
    bool IsEqual( const Extension& rhs ) const override;
};

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_volume( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer );

} // namespace gltfviewer
