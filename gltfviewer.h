// gltfviewer interface
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdbool.h>

#ifdef GLTFVIEWER_EXPORTS
#define gltfviewer_export extern "C" __declspec( dllexport )
#else
#define gltfviewer_export
#endif

#define gltfviewer_success                  1
#define gltfviewer_error                    0

#define gltfviewer_default_scene_index     -1
#define gltfviewer_default_scene_materials -1

typedef int32_t gltfviewer_handle;

// Render settings.
typedef struct {
  uint32_t  width;
  uint32_t  height;

  // Cycles specific settings.
  uint32_t  samples;    // number of samples, or zero to use a default.
  uint32_t  tile_size;  // tile size, preferably a power of 2, or zero to use a default.
} gltfviewer_render_settings;

// Camera projection.
typedef enum {
  gltfviewer_camera_projection_perspective,
  gltfviewer_camera_projection_orthographic
} gltfviewer_camera_projection;

// Preset camera view position.
typedef enum {
  gltfviewer_camera_preset_none,

  gltfviewer_camera_preset_front,
  gltfviewer_camera_preset_back,
  gltfviewer_camera_preset_left,
  gltfviewer_camera_preset_right,
  gltfviewer_camera_preset_top,
  gltfviewer_camera_preset_bottom,

  gltfviewer_camera_preset_default = gltfviewer_camera_preset_front
} gltfviewer_camera_preset;

// Camera settings.
typedef struct {
  gltfviewer_camera_projection projection;  // Camera projection.
  gltfviewer_camera_preset preset;          // Preset camera view position. If this is other than 'gltfviewer_camera_preset_none', the remaining properties are ignored.

  float matrix[4][4];                       // View matrix. This uses the glTF coordinate system (glTF defines +Y as up, +Z as forward, and -X as right).

  float farclip;                            // Far clipping distance, in metres.
  float nearclip;                           // Near clipping distance, in metres.

  float perspective_fov;                    // For perspective projections, the vertical field of view, in degrees.

  float orthographic_width;                 // For orthographic projections, the view width, in metres.
  float orthographic_height;                // For orthographic projections, the view height, in metres.
} gltfviewer_camera;

// Environment settings.
// TODO support for HDR backgrounds, and maybe expose more sky parameters).
typedef struct {
  float sky_intensity;                      // Skylight intensity in arbitrary units, with 0.0 being no skylight and 1.0 being 'standard' skylight intensity.
  float sun_intensity;                      // Sunlight intensity in arbitrary units, with 0.0 being no sunlight and 1.0 being 'standard' sunlight intensity.
  float sun_elevation;                      // Elevation of the sun from the horizon, in degrees.
  float sun_rotation;                       // Rotation of the sun around the zenith, in degrees.
  bool transparent_background;              // Renders to a transparent canvas (i.e. the background is not visible).
} gltfviewer_environment_settings;

// Image structure used by the render callback.
// TODO change callback to pass linear floating point data instead of sRGB (add library function/settings to do full colorspace conversion, filmic look, etc.)
typedef struct {
  uint32_t  width;
  uint32_t  height;
  uint8_t   bits_per_pixel;
  uint32_t  stride;
  void*     pixels;
} gltfviewer_image;

// Callback used to return results when rendering.
// 'image' - current render result (the image is temporary, so the client should make its own copy of the data).
// 'context' - client context.
typedef void ( *gltfviewer_render_callback ) ( gltfviewer_image* image, void* context );

// Initialises the gltfviewer library - call once on startup.
// Returns gltfviewer_success, or (TODO) an error code.
gltfviewer_export int32_t gltfviewer_init();

// Frees the gltfviewer library and all model handles - call once on shutdown.
gltfviewer_export void gltfviewer_free();

// Loads a glTF/glb model.
// 'filename' - glTF/glb filename (UTF-8 encoded).
// Returns a gltfviewer model handle, or gltfviewer_error if the file was not loaded.
gltfviewer_export gltfviewer_handle gltfviewer_load_model( const char* filename );

// Frees a glTF/glb model handle.
gltfviewer_export void gltfviewer_free_model( gltfviewer_handle model_handle );

// Returns the number of scenes in a glTF/glb model.
gltfviewer_export uint32_t gltfviewer_get_scene_count( gltfviewer_handle model_handle );

// Gets the name (UTF-8 encoded) of a scene in a glTF/glb model.
// 'model_handle' - the model handle returned from gltfviewer_load_model.
// 'scene_index' - for glTF models with n scenes, this specifies the index of the scene to query (from 0 to n-1).
// 'name_buffer' - name buffer supplied by the client.
// 'name_buffer_size' - the size of the name buffer.
// Returns the required size of the name buffer to hold the scene name (including terminating null), or zero if the scene was not found.
// The name buffer is filled with the scene name (including terminating null) if the name buffer size is large enough.
gltfviewer_export uint32_t gltfviewer_get_scene_name( gltfviewer_handle model_handle, uint32_t scene_index, char* name_buffer, uint32_t name_buffer_size );

// Gets the number of cameras in a glTF/glb model.
// 'model_handle' - the model handle returned from gltfviewer_load_model.
// 'scene_index' - for glTF models with n scenes, this selects the index of the scene to query (from 0 to n-1). Alternatively, use a value of 'gltfviewer_default_scene_index' (-1) to query the default scene.
// Returns the number of cameras in the scene (this can be, and often is, zero).
gltfviewer_export uint32_t gltfviewer_get_camera_count( gltfviewer_handle model_handle, int32_t scene_index );

// Returns the number of cameras in a glTF/glb model.
// 'model_handle' - the model handle returned from gltfviewer_load_model.
// 'scene_index' - for glTF models with n scenes, this selects the index of the scene to query (from 0 to n-1). Alternatively, use a value of 'gltfviewer_default_scene_index' (-1) to query the default scene.
// 'cameras' - pointer to an array of gltfviewer_camera objects supplied by the client, which will be filled by this function.
// 'camera_count' - size of the gltfviewer_camera object array supplied by the client.
// Returns true if the size of the gltfviewer_camera object array was sufficient to store the results, false otherwise.
gltfviewer_export bool gltfviewer_get_cameras( gltfviewer_handle model_handle, int32_t scene_index, gltfviewer_camera* cameras, uint32_t camera_count );

// Returns the number of material variants in a glTF/glb model.
// 'model_handle' - the model handle returned from gltfviewer_load_model.
gltfviewer_export uint32_t gltfviewer_get_material_variants_count( gltfviewer_handle model_handle );

// Gets the name (UTF-8 encoded) of a material variant in a glTF/glb model.
// 'model_handle' - the model handle returned from gltfviewer_load_model.
// 'material_variant_index' - for glTF models with n material variants, this specifies the index of the variant to query (from 0 to n-1).
// 'name_buffer' - name buffer supplied by the client.
// 'name_buffer_size' - the size of the name buffer.
// Returns the required size of the name buffer to hold the material variant name (including terminating null), or zero if the material variant was not found.
// The name buffer is filled with the material variant name (including terminating null) if the name buffer size is large enough.
gltfviewer_export uint32_t gltfviewer_get_material_variant_name( gltfviewer_handle model_handle, uint32_t material_variant_index, char* name_buffer, uint32_t name_buffer_size );

// Starts a render for a model.
// 'model_handle' - the model handle returned from gltfviewer_load_model.
// 'scene_index' - for glTF models with n scenes, this selects the index of the scene to render (from 0 to n-1). Use a value of 'gltfviewer_default_scene_index' (-1) to select the default scene.
// 'material_variant_index' - for glTF models with n material variants, this selects the index of the variant to use (from 0 to n-1). Use a value of 'gltfviewer_default_scene_materials' (-1) to use the default materials.
// 'camera' - camera settings.
// 'render_settings' - render settings.
// 'environment_settings' - environment settings (background, skylight, etc.)
// 'render_callback' - callback which supplies the render results.
// 'render_callback_context' - client context supplied to the render callback.
// Returns true if the render was started successfully, false otherwise.
gltfviewer_export bool gltfviewer_start_render( 
  gltfviewer_handle                 model_handle, 
  int32_t                           scene_index,
  int32_t                           material_variant_index,
  gltfviewer_camera                 camera, 
  gltfviewer_render_settings        render_settings, 
  gltfviewer_environment_settings   environment_settings, 
  gltfviewer_render_callback        render_callback, 
  void*                             render_callback_context );

// Stops a render for a model.
gltfviewer_export void gltfviewer_stop_render( gltfviewer_handle model_handle );

#ifdef __cplusplus
}
#endif
