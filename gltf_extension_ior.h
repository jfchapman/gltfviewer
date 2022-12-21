#pragma once

#include <GLTFSDK/ExtensionHandlers.h>

#include <optional>

namespace gltfviewer {

struct KHR_materials_ior : Microsoft::glTF::Extension, Microsoft::glTF::glTFProperty
{
    static constexpr const char* Name = "KHR_materials_ior";

    float m_ior = 1.5f;

    std::unique_ptr<Extension> Clone() const override;
    bool IsEqual( const Extension& rhs ) const override;
};

std::unique_ptr<Microsoft::glTF::Extension> Deserialize_KHR_materials_ior( const std::string& json, const Microsoft::glTF::ExtensionDeserializer& extensionDeserializer );

} // namespace gltfviewer
