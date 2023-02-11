using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Text;

// TODO move to a class library once the interface is finalized.
namespace gltfviewer
{
  /// <summary>
  /// An image in linear color space, 4 floats per pixel, 4 channels, RGBA channel order (as returned by the gltfviewer library callbacks).
  /// </summary>
  public class RenderImage
  {
    public RenderImage( uint width, uint height, IntPtr pixels, bool hasAlpha )
    {
      Width = width;
      Height = height;
      if ( pixels != IntPtr.Zero ) {
        Pixels = new float[ Width * Height * 4 ];
        Marshal.Copy( pixels, Pixels, 0, (int)( Width * Height * 4 ) );
      }
      HasAlpha = hasAlpha;
    }

    public uint Width { get; }

    public uint Height { get; }

    public float[]? Pixels { get; }

    public bool HasAlpha { get; }
  }

  /// <summary>
  /// Event arguments for the gltfviewer library updated callback.
  /// </summary>
  public class RenderUpdatedEventArgs : EventArgs
  {
    public RenderImage? Image = null;
  }

  /// <summary>
  /// Event arguments for the gltfviewer library finished callback.
  /// </summary>
  public class RenderFinishedEventArgs : EventArgs
  {
    public RenderImage? OriginalImage = null;
    public RenderImage? DenoisedImage = null;
  }

  /// <summary>
  /// Event arguments for the gltfviewer library progress callback.
  /// </summary>
  public class RenderProgressEventArgs : EventArgs
  {
    public float Progress = 0;
    public string? Status = null;
  }

  /// <summary>
  /// Camera projections supported by the gltfviwer library.
  /// </summary>
  public enum CameraProjection : int
  {
    Perspective = 0,
    Orthographic = 1
  }

  /// <summary>
  /// Preset camera positions supported by the gltfviewer library.
  /// </summary>
  public enum CameraPreset : int
  {
    None = 0,
    Front = 1,
    Back = 2,
    Left = 3,
    Right = 4,
    Top = 5,
    Bottom = 6,
    Default = Front
  }

  /// <summary>
  /// Camera settings for the gltfviewer library.
  /// </summary>
	[StructLayout(LayoutKind.Sequential, Pack = 0)]
	public struct Camera
	{
    public CameraProjection Projection;
    public CameraPreset Preset;

    /// <summary>
    /// Column-major order.
    /// </summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
    public float[] Matrix;

		public float FarClip;
		public float NearClip;
		public float PerspectiveDegreesVerticalFieldOfView;

    public float OrthographicViewWidth;
    public float OrthographicViewHeight;
	}

  /// <summary>
  /// Render settings for the gltfviewer library.
  /// </summary>
	[StructLayout(LayoutKind.Sequential, Pack = 0)]
  public struct RenderSettings
  {
    public uint Width;
    public uint Height;

    public uint Samples;
    public uint TileSize;
  }

  /// <summary>
  /// Environment settings for the gltfviewer library.
  /// </summary>
	[StructLayout(LayoutKind.Sequential, Pack = 0)]
  public struct EnvironmentSettings
  {
    public float SkyIntensity;
    public float SunIntensity;
    public float SunElevation;
    public float SunRotation;
    public bool TransparentBackground;
  }

  /// <summary>
  /// Handles interop with the gltfviewer library to provide a full render for a glTF file, using Cycles.
  /// </summary>
  internal class Renderer : IDisposable
  {
    public const int DefaultSceneIndex = -1;
    public const int DefaultMaterials = -1;

    public const int DefaultColorProfile = 0;
    public const float DefaultExposure = 0;

    /// <summary>
    /// While rendering, render image updates are passed back via this event.
    /// </summary>
    public event EventHandler<RenderUpdatedEventArgs>? RenderUpdated;

    /// <summary>
    /// While rendering, progress and status information is passed back via this event.
    /// </summary>
    public event EventHandler<RenderProgressEventArgs>? RenderProgress;

    /// <summary>
    /// When a render has completed, the final render images (original and denoised) are passed back via this event.
    /// </summary>
    public event EventHandler<RenderFinishedEventArgs>? RenderFinished;

    /// <summary>
    /// List of scene names contained in the model, with the list of model cameras (if any) for each scene.
    /// </summary>
    public List<(string, List<Camera>?)> ModelScenes{ get; } = new List<(string, List<Camera>?)>();

    /// <summary>
    /// List of material variant names contained in the model.
    /// </summary>
    public List<string> MaterialVariants { get; } = new List<string>();

    /// <summary>
    /// True if rendering is in progress, false if not.
    /// </summary>
    public bool IsRendering { get; private set; } = false;

    /// <summary>
    /// Color profiles supported by the gltfviewer library, for use when converting render images to bitmaps.
    /// </summary>
    public static List<string>? ColorProfiles { get; private set; } = null;

    /// <summary>
    /// Initializes the gltfviewer library. Should be called once on application startup.
    /// </summary>
    /// <returns>True if the library was initialized successfully, false if not.</returns>
    public static bool InitLibrary()
    {
      try {
        gltfviewer_init();

        ColorProfiles = new List<string>() { "Default sRGB" };
        uint lookCount = gltfviewer_get_look_count();
        for ( uint lookIndex = 0; lookIndex < lookCount; lookIndex++ ) {
          uint nameBufferSize = 256;
          StringBuilder nameBuffer = new StringBuilder( (int)nameBufferSize );
          gltfviewer_get_look_name( lookIndex, nameBuffer, nameBufferSize );
          ColorProfiles.Add( nameBuffer.ToString() );
        }
      } catch ( Exception ) {
        return false;
      }
      return true;
    }

    /// <summary>
    /// Constructs a new renderer instance from a glTF/glb file.
    /// </summary>
    /// <param name="filename">glTF/glb file to load.</param>
    public Renderer( string filename )
    {
      LoadModel( filename );
    }

    public void Dispose()
    {
      Dispose( true );
    }

    /// <summary>
    /// Starts rendering (or restarts, if rendering is in progress).
    /// </summary>
    /// <param name="cameraSettings">Camera settings.</param>
    /// <param name="renderSettings">Render settings.</param>
    /// <param name="environmentSettings">Environment settings.</param>
    /// <param name="sceneIndex">Scene index (from 0 to ModelScenes.Count - 1) or DefaultSceneIndex (-1) to render the default model scene.</param>
    /// <param name="materialVariantIndex">Material variants index (from 0 to MaterialVariants - 1) or DefaultMaterials (-1) to render using the default model materials.</param>
    /// <returns>True if the render was started, false if not.</returns>
    public bool StartRender( Camera cameraSettings, RenderSettings renderSettings, EnvironmentSettings environmentSettings, int sceneIndex, int materialVariantIndex )
    {
      StopRender();
      IsRendering = gltfviewer_start_render( _modelHandle, sceneIndex, materialVariantIndex, cameraSettings, renderSettings, environmentSettings, _renderCallback, _progressCallback, _finishCallback, GCHandle.ToIntPtr( _callbackContext.GetValueOrDefault() ) );
      _renderImageHasAlpha = environmentSettings.TransparentBackground;
      return IsRendering;
    }

    /// <summary>
    /// Stops rendering.
    /// </summary>
    public void StopRender()
    {
      gltfviewer_stop_render( _modelHandle );
      IsRendering = false;
    }

    /// <summary>
    /// Frees the gltfviewer library and all library resources. Should be called once on appllication shutdown.
    /// </summary>
    public static void FreeLibrary()
    {
      gltfviewer_free();
    }

    /// <summary>
    /// Converts a vertical field of view angle to a horizontal field of view angle.
    /// </summary>
    /// <param name="verticalDegreesFieldOfView">Vertical field of view angle, in degrees.</param>
    /// <param name="aspectRatio">View aspect ratio.</param>
    /// <returns>Horizontal field of view angle, in degrees.</returns>
    public static double ToHorizontalDegreesFieldOfView( double verticalDegreesFieldOfView, double aspectRatio )
    {
      if ( verticalDegreesFieldOfView > 180 )
        return 180;

      double verticalRadians = verticalDegreesFieldOfView * Math.PI / 180;
      double horizontalRadians = 2 * Math.Atan( Math.Tan( verticalRadians / 2 ) * aspectRatio );
      double horizontalDegrees = horizontalRadians * 180 / Math.PI;
      return horizontalDegrees;
    }

    /// <summary>
    /// Converts a horizontal field of view angle to a vertical field of view angle.
    /// </summary>
    /// <param name="verticalDegreesFieldOfView">Horizontal field of view angle, in degrees.</param>
    /// <param name="aspectRatio">View aspect ratio.</param>
    /// <returns>Vertical field of view angle, in degrees.</returns>
    public static double ToVerticalDegreesFieldOfView( double horizontalDegreesFieldOfView, double aspectRatio )
    {
      if ( aspectRatio <= 0 )
        return horizontalDegreesFieldOfView;

      if ( horizontalDegreesFieldOfView > 180 )
        return 180;

      double horizontalRadians = horizontalDegreesFieldOfView * Math.PI / 180;
      double verticalRadians = 2 * Math.Atan( Math.Tan( horizontalRadians / 2 ) / aspectRatio );
      double verticalDegrees = verticalRadians * 180 / Math.PI;
      return verticalDegrees;
    }

    /// <summary>
    /// Converts a render image to a bitmap.
    /// </summary>
    /// <param name="image">The render image to convert.</param>
    /// <param name="colorProfileIndex">Color profile index to use for image conversion (from 0 to ColorProfiles.Count - 1).</param>
    /// <param name="exposure">Exposure setting.</param>
    /// <returns>Converted bitmap, or null on failure.</returns>
    public static Bitmap? ConvertRenderImage( RenderImage image, int colorProfileIndex, float exposure )
    {
      Bitmap? bitmap = null;
      if ( ( image.Width > 0 ) && ( image.Height > 0 ) && ( null != image.Pixels ) && ( image.Pixels.Length == ( image.Width * image.Height * 4 ) ) ) {
        bitmap = new Bitmap( (int)image.Width, (int)image.Height, PixelFormat.Format32bppArgb );
        if ( null != bitmap ) {
          Rectangle rect = new Rectangle( 0, 0, bitmap.Width, bitmap.Height );
          BitmapData bitmapData = bitmap.LockBits( rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb );
          
          gltfviewer_image sourceImage;
          sourceImage.width = image.Width;
          sourceImage.height = image.Height;
          sourceImage.pixel_format = gltfviewer_image_pixelformat.gltfviewer_image_pixelformat_floatRGBA;
          sourceImage.stride_bytes = image.Width * 16;
          
          var tempPixels = Marshal.AllocHGlobal( image.Pixels.Length * 4 );
          Marshal.Copy( image.Pixels, 0, tempPixels, image.Pixels.Length );
          sourceImage.pixels = tempPixels;

          gltfviewer_image convertedImage;
          convertedImage.width = image.Width;
          convertedImage.height = image.Height;
          convertedImage.pixel_format = gltfviewer_image_pixelformat.gltfviewer_image_pixelformat_ucharBGRA;
          convertedImage.stride_bytes = image.Width * 4;
          convertedImage.pixels = bitmapData.Scan0;

          int lookIndex = ( ( null != ColorProfiles ) && ( colorProfileIndex >= 0 ) && ( colorProfileIndex < ColorProfiles.Count ) ) ? ( colorProfileIndex - 1 ) : -1;
          gltfviewer_image_convert( ref sourceImage, ref convertedImage, lookIndex, exposure );

          Marshal.FreeHGlobal( tempPixels );

          bitmap.UnlockBits( bitmapData );
          bitmap.RotateFlip( RotateFlipType.RotateNoneFlipY );

          if ( image.HasAlpha ) {
            Bitmap background = new Bitmap( (int)image.Width, (int)image.Height, PixelFormat.Format32bppRgb );
            Graphics graphics = Graphics.FromImage( background );
            graphics.Clear( Color.White );
            graphics.DrawImage( bitmap, new Point( 0, 0 ) );
            bitmap = background;
          }
        }
      }
      return bitmap;
    }

    protected virtual void Dispose( bool disposing )
    {
      if ( !_disposed ) {
        // Release unmanaged resources.
        StopRender();
        if ( _callbackContext.HasValue ) {
           _callbackContext.Value.Free();
        }
        gltfviewer_free_model( _modelHandle );
        _disposed = true;
      }
    }

    private bool LoadModel( string filename )
    {
      var bytes = Encoding.Default.GetBytes( filename );
      var utf8filename = Encoding.UTF8.GetString( bytes );
      _modelHandle = gltfviewer_load_model( utf8filename );

      _callbackContext = GCHandle.Alloc( this );

      uint variantCount = gltfviewer_get_material_variants_count( _modelHandle );
      for ( uint variantIndex = 0; variantIndex < variantCount; variantIndex++ ) {
        uint nameBufferSize = 256;
        StringBuilder nameBuffer = new StringBuilder( (int)nameBufferSize );
        gltfviewer_get_material_variant_name( _modelHandle, variantIndex, nameBuffer, nameBufferSize );
        string name = nameBuffer.ToString();
        if ( String.IsNullOrEmpty( name ) ) {
          name = "Variant " + ( 1 + variantIndex ).ToString();
        }
        MaterialVariants.Add( name );
      }

      uint sceneCount = gltfviewer_get_scene_count( _modelHandle );
      for ( uint sceneIndex = 0; sceneIndex < sceneCount; sceneIndex++ ) {
        uint nameBufferSize = 256;
        StringBuilder nameBuffer = new StringBuilder( (int)nameBufferSize );
        gltfviewer_get_scene_name( _modelHandle, sceneIndex, nameBuffer, nameBufferSize );
        string name = nameBuffer.ToString();
        if ( String.IsNullOrEmpty( name ) ) {
          name = "Scene " + ( 1 + sceneIndex ).ToString();
        }

        List<Camera>? sceneCameras = null;

        uint cameraCount = gltfviewer_get_camera_count( _modelHandle, (int)sceneIndex );
        if ( cameraCount > 0 ) {
          Camera[] cameras = new Camera[ cameraCount ];
          sceneCameras = new List<Camera>();
          gltfviewer_get_cameras( _modelHandle, DefaultSceneIndex, cameras, cameraCount );
          foreach ( var camera in cameras ) {
            sceneCameras.Add( camera );
          }
        }

        ModelScenes.Add( ( name, sceneCameras ) );
      }

      return _modelHandle != 0;
    }

    private static void RenderCallback( ref gltfviewer_image image, IntPtr context )
    {
      var handle = GCHandle.FromIntPtr( context );
      if ( null != handle.Target ) {
        Renderer renderer = (Renderer)handle.Target;
        if ( renderer.RenderUpdated != null ) {
          RenderUpdatedEventArgs args = new RenderUpdatedEventArgs();
          if ( ( image.width > 0 ) && ( image.height > 0 ) && ( gltfviewer_image_pixelformat.gltfviewer_image_pixelformat_floatRGBA == image.pixel_format ) && ( image.stride_bytes == image.width * 16 ) && ( IntPtr.Zero != image.pixels ) ) { 
            args.Image = new RenderImage( image.width, image.height, image.pixels, renderer._renderImageHasAlpha );
          }
          renderer.RenderUpdated( renderer, args );
        }
      }
    }

    private static void ProgressCallback( float progress, [MarshalAs(UnmanagedType.LPStr)] string status, IntPtr context )
    {
      var handle = GCHandle.FromIntPtr( context );
      if ( null != handle.Target ) {
        Renderer renderer = (Renderer)handle.Target;
        if ( renderer.RenderProgress!= null ) {
          RenderProgressEventArgs args = new RenderProgressEventArgs();
          args.Progress = progress;
          args.Status = status;
          renderer.RenderProgress( renderer, args );
        }
      }
    }

    private static void FinishCallback( ref gltfviewer_image original, ref gltfviewer_image denoised, IntPtr context )
    {
      var handle = GCHandle.FromIntPtr( context );
      if ( null != handle.Target ) {
        Renderer renderer = (Renderer)handle.Target;
        if ( renderer.RenderFinished != null ) {
          RenderFinishedEventArgs args = new RenderFinishedEventArgs();
          if ( ( original.width > 0 ) && ( original.height > 0 ) && ( gltfviewer_image_pixelformat.gltfviewer_image_pixelformat_floatRGBA == original.pixel_format ) && ( original.stride_bytes == original.width * 16 ) && ( IntPtr.Zero != original.pixels ) ) { 
            args.OriginalImage = new RenderImage( original.width, original.height, original.pixels, renderer._renderImageHasAlpha );
          }
          if ( ( denoised.width > 0 ) && ( denoised.height > 0 ) && ( gltfviewer_image_pixelformat.gltfviewer_image_pixelformat_floatRGBA == denoised.pixel_format ) && ( denoised.stride_bytes == denoised.width * 16 ) && ( IntPtr.Zero != denoised.pixels ) ) { 
            args.DenoisedImage = new RenderImage( denoised.width, denoised.height, denoised.pixels, renderer._renderImageHasAlpha );
          }
          renderer.RenderFinished( renderer, args );
        }
        renderer.IsRendering = false;
      }
    }

    private int _modelHandle = 0;
    private bool _renderImageHasAlpha = false;
    private readonly gltfviewer_render_callback _renderCallback = new gltfviewer_render_callback( RenderCallback );
    private readonly gltfviewer_progress_callback _progressCallback = new gltfviewer_progress_callback( ProgressCallback );
    private readonly gltfviewer_finish_callback _finishCallback = new gltfviewer_finish_callback( FinishCallback );
    private GCHandle? _callbackContext = null;
    private bool _disposed = false;


    // Interop

    private enum gltfviewer_image_pixelformat : int
    {
      gltfviewer_image_pixelformat_floatRGBA,
      gltfviewer_image_pixelformat_ucharBGRA
    }


		[StructLayout(LayoutKind.Sequential, Pack = 0)]
    private struct gltfviewer_image
    {
      public gltfviewer_image_pixelformat pixel_format;
      public uint width;
      public uint height;
      public uint stride_bytes;
      public IntPtr pixels;
    }


    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void gltfviewer_render_callback( ref gltfviewer_image image, IntPtr context );

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void gltfviewer_progress_callback( float progress, [MarshalAs(UnmanagedType.LPStr)] string status, IntPtr context );

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void gltfviewer_finish_callback( ref gltfviewer_image original_image, ref gltfviewer_image denoised_image, IntPtr context );


  	[DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
		private static extern int gltfviewer_init();

  	[DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
		private static extern void gltfviewer_free();

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern int gltfviewer_load_model( [MarshalAs(UnmanagedType.LPStr)] string filename );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void gltfviewer_free_model( int model_handle );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern uint gltfviewer_get_scene_count( int model_handle );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern uint gltfviewer_get_scene_name( int model_handle, uint scene_index, StringBuilder name_buffer, uint name_buffer_size );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern uint gltfviewer_get_camera_count( int model_handle, int scene_index );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool gltfviewer_get_cameras( int model_handle, int scene_index, [Out] Camera[] cameras, uint camera_count );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern uint gltfviewer_get_material_variants_count( int model_handle );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern uint gltfviewer_get_material_variant_name( int model_handle, uint material_variant_index, StringBuilder name_buffer, uint name_buffer_size );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool gltfviewer_start_render( 
      int model_handle,
      int scene_index,
      int material_variant_index,
      Camera camera,
      RenderSettings render_settings, 
      EnvironmentSettings environment_settings, 
      gltfviewer_render_callback render_callback,
      gltfviewer_progress_callback progress_callback,
      gltfviewer_finish_callback finish_callback,
      IntPtr context
    );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern void gltfviewer_stop_render( int model_handle );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern uint gltfviewer_get_look_count();

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern uint gltfviewer_get_look_name( uint look_index, StringBuilder name_buffer, uint name_buffer_size );

    [DllImport("gltfviewer.dll", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool gltfviewer_image_convert( ref gltfviewer_image source_image, ref gltfviewer_image target_image, int look_index, float exposure );
  }
}
