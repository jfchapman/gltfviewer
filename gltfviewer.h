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

#define gltfviewer_success                            1
#define gltfviewer_error                              0

#define gltfviewer_default_scene_index               -1
#define gltfviewer_default_scene_materials           -1
#define gltfviewer_image_convert_linear_to_srgb      -1
#define gltfviewer_default_exposure                   0

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

// Image pixel formats.
typedef enum {
  gltfviewer_image_pixelformat_floatRGBA,   // 4 floats (fp32) per pixel, 4 channels, RGBA channel order (used by gltfviewer_render_callback).
  gltfviewer_image_pixelformat_ucharBGRA    // 4 bytes per pixel, 4 channels, BGRA channel order (equivalent to Windows PixelFormat32bppARGB).
} gltfviewer_image_pixelformat;

// Image structure.
typedef struct {
  gltfviewer_image_pixelformat  pixel_format;
  uint32_t                      width;
  uint32_t                      height;
  uint32_t                      stride_bytes; // If zero, then the stride will be deduced based on the pixel_format & width.
  void*                         pixels;
} gltfviewer_image;


///////////////////////////////////////////////////////////////////////////////
// Callbacks
///////////////////////////////////////////////////////////////////////////////

// Callback used to return the current image result while rendering.
// 'image' - render image, floatRGBA pixel format, in a linear colorspace (the image is temporary, so the client should make its own copy of the pixel data).
// 'context' - client context.
// NOTE except for the image helper conversion functions, no other gltfviewer library functions should be called from within the callback.
typedef void ( *gltfviewer_render_callback ) ( gltfviewer_image* image, void* context );

// Callback used to return the current progress status while rendering.
// 'progress' - render progress, in the range 0.0 to 1.0.
// 'status' - status information (if not null, this is a temporary string, so the client should make its own copy).
// 'context' - client context.
// NOTE except for the image helper conversion functions, no other gltfviewer library functions should be called from within the callback.
typedef void ( *gltfviewer_progress_callback ) ( float progress, const char* status, void* context );

// Callback used to return the final image results when rendering has completed.
// 'original_image' - original (noisy) render image, floatRGBA pixel format, in a linear colorspace (the image is temporary, so the client should make its own copy of the pixel data).
// 'denoised_image' - denoised render image, floatRGBA pixel format, in a linear colorspace (the image is temporary, so the client should make its own copy of the pixel data).
// 'context' - client context.
// NOTE except for the image helper conversion functions, no other gltfviewer library functions should be called from within the callback.
typedef void ( *gltfviewer_finish_callback ) ( gltfviewer_image* original_image, gltfviewer_image* denoised_image, void* context );


///////////////////////////////////////////////////////////////////////////////
// General functions
///////////////////////////////////////////////////////////////////////////////

// Initialises the gltfviewer library - call once on startup.
// Returns gltfviewer_success, or (TODO) an error code.
gltfviewer_export int32_t gltfviewer_init();

// Frees the gltfviewer library and all model handles - call once on shutdown.
gltfviewer_export void gltfviewer_free();


///////////////////////////////////////////////////////////////////////////////
// glTF/glb functions
///////////////////////////////////////////////////////////////////////////////

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


///////////////////////////////////////////////////////////////////////////////
// Render functions
///////////////////////////////////////////////////////////////////////////////

// Starts a render for a model.
// 'model_handle' - the model handle returned from gltfviewer_load_model.
// 'scene_index' - for glTF models with n scenes, this selects the index of the scene to render (from 0 to n-1). Use a value of 'gltfviewer_default_scene_index' (-1) to select the default scene.
// 'material_variant_index' - for glTF models with n material variants, this selects the index of the variant to use (from 0 to n-1). Use a value of 'gltfviewer_default_scene_materials' (-1) to use the default materials.
// 'camera' - camera settings.
// 'render_settings' - render settings.
// 'environment_settings' - environment settings (background, skylight, etc.)
// 'render_callback' - callback which supplies the current render image.
// 'progress_callback' - callback which supplies the current render status (can be null).
// 'finish_callback' - callback which supplies the final (original & denoised) render image results (can be null).
// 'context' - client context supplied to the callback functions.
// Returns true if the render was started successfully, false otherwise.
gltfviewer_export bool gltfviewer_start_render( 
  gltfviewer_handle                 model_handle, 
  int32_t                           scene_index,
  int32_t                           material_variant_index,
  gltfviewer_camera                 camera, 
  gltfviewer_render_settings        render_settings, 
  gltfviewer_environment_settings   environment_settings, 
  gltfviewer_render_callback        render_callback,
  gltfviewer_progress_callback      progress_callback,
  gltfviewer_finish_callback        finish_callback,
  void*                             context );

// Stops a render for a model.
gltfviewer_export void gltfviewer_stop_render( gltfviewer_handle model_handle );


///////////////////////////////////////////////////////////////////////////////
// Image conversion helper functions
///////////////////////////////////////////////////////////////////////////////

// Returns the number of 'looks' supported by the library (where a 'look' determines how an image in linear colorspace is to be converted).
gltfviewer_export uint32_t gltfviewer_get_look_count();

// Gets the name (UTF-8 encoded) of a look.
// 'look_index' - specifies the index of the look to query (from 0 to n-1, where n is the value returned by gltfviewer_image_get_look_count).
// 'name_buffer' - name buffer supplied by the client.
// 'name_buffer_size' - the size of the name buffer.
// Returns the required size of the name buffer to hold the look name (including terminating null), or zero if the look was not found.
// The name buffer is filled with the look name (including terminating null) if the name buffer size is large enough.
gltfviewer_export uint32_t gltfviewer_get_look_name( uint32_t look_index, char* name_buffer, uint32_t name_buffer_size );

// Converts an image.
// 'source_image' - the source image to convert from.
// 'target_image' - the target image to convert into (note that the caller should supply an image pointing to a pixel buffer of the appropriate size).
// 'look_index' - specifies the index of the look to use (from 0 to n-1, where n is the value returned by gltfviewer_image_get_look_count). 
//                Alternatively, use a value of 'gltfviewer_image_convert_linear_to_srgb' (-1) to convert a linear image to the sRGB color space.
// 'exposure' - exposure value. Use values < 0 to darken the image, values > 0 to lighten the image, or 0 (gltfviewer_default_exposure) to use the default exposure level.
// Returns true if the image was converted successfully.
gltfviewer_export bool gltfviewer_image_convert( gltfviewer_image* source_image, gltfviewer_image* target_image, int32_t look_index, float exposure );

#ifdef __cplusplus
}
#endif
