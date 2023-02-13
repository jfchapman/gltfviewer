using gltfviewer;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Printing;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Threading;

namespace testappWPF
{
  internal class Controller
  {
    public Controller(
      System.Windows.Controls.Viewport3D previewControl,
      System.Windows.Controls.Image renderControl,
      System.Windows.Controls.ComboBox samplesControl,
      System.Windows.Controls.ComboBox sceneControl,
      System.Windows.Controls.ComboBox materialVariantControl,
      System.Windows.Controls.ComboBox colorProfileControl,
      System.Windows.Controls.CheckBox denoisedControl,
      Window mainWindow,
      uint samplesSetting,
      uint colorProfileSetting,
      float exposure,
      bool showDenoised,
      gltfviewer.EnvironmentSettings environmentSettings )
    {
      _previewControl = previewControl;
      _renderDisplayControl = renderControl;

      _samplesControl = samplesControl;
      _sceneControl = sceneControl;
      _materialVariantControl = materialVariantControl;
      _colorProfileControl = colorProfileControl;
      _denoisedControl = denoisedControl;

      _mainWindow = mainWindow;

      _exposure = exposure;
      _environmentSettings = environmentSettings;
      _displayDenoisedImage = showDenoised;

      _preview = new View( _previewControl );
      
      _rendererAvailable = Renderer.InitLibrary();
      if ( _rendererAvailable ) {
        InitializeRenderControls( samplesSetting, colorProfileSetting );
        StartRenderTask();
        StartDisplayTask();
      }
    }

    public void LoadModel( string filename )
    {
      _modelFilename = filename;
      _sceneIndex = 0;
      _materialVariantIndex = Renderer.DefaultMaterials;

      _mainWindow.Title = "Loading " + Path.GetFileName( _modelFilename );

      _preview.LoadModel( filename, _sceneIndex );

      if ( _rendererAvailable ) {
        if ( null != _renderer ) {
          _renderer.RenderUpdated -= Render_RenderUpdated;
          _renderer.RenderFinished -= Render_RenderFinished;
          _renderer.RenderProgress -= Render_RenderProgress;
          _renderer.Dispose();
        }

        InvalidateRenderState();

        _renderer = new Renderer( filename );
        _renderer.RenderUpdated += Render_RenderUpdated;
        _renderer.RenderFinished += Render_RenderFinished;
        _renderer.RenderProgress += Render_RenderProgress;

        _camera = _previewControl.Camera.Clone();

        _renderSettings.Width = (uint)_previewControl.ActualWidth;
        _renderSettings.Height = (uint)_previewControl.ActualHeight;
      }

      ResetSceneControls();
    }
 
    public void OnClose()
    {
      if ( null != _renderTask ) {
        _cancellationToken.Cancel();
        _renderTask.Wait();
      }

      if ( null != _renderer ) {
        _renderer.RenderUpdated -= Render_RenderUpdated;
        _renderer.RenderFinished -= Render_RenderFinished;
        _renderer.RenderProgress -= Render_RenderProgress;
        _renderer.Dispose();
      }

      Renderer.FreeLibrary();
    }

    public void OnLeftMouseDown( System.Windows.Point cursorPos )
    {
      if ( null != _renderer ) {
        InvalidateRenderState();
        _preview.StartRotation( cursorPos );
      }
    }

    public void OnMiddleMouseDown( System.Windows.Point cursorPos )
    {
      if ( null != _renderer ) {
        InvalidateRenderState();
        _preview.StartDrag( cursorPos );
      }
    }

    public void OnMouseMove( System.Windows.Point cursorPos )
    {
      if ( null != _renderer ) {
		    _preview.UpdateRotation( cursorPos );
        _preview.UpdateDrag( cursorPos );
      }
    }

    public void OnMouseUp()
    {
      if ( null != _renderer ) {
        _preview.StopRotation();
        _preview.StopDrag();
        _camera = _previewControl.Camera.Clone();
        RefreshRenderState();
      }
    }

    public void OnMouseWheel( int delta )
    {
      if ( null != _renderer ) {
        InvalidateRenderState();
        _preview.OnMouseWheel( delta );
        _camera = _previewControl.Camera.Clone();
        RefreshRenderState();
      }
    }

    public void OnResize()
    {
      uint width = (uint)_previewControl.ActualWidth;
      uint height = (uint)_previewControl.ActualHeight;
      if ( ( null != _renderer ) && ( ( width != _renderSettings.Width ) || ( height != _renderSettings.Height ) ) ) {
        InvalidateRenderState();
        _renderSettings.Width = width;
        _renderSettings.Height = height;
        RefreshRenderState();
      }
    }

    public void OnSamplesChanged( int selectedIndex ) {
      if ( ( selectedIndex >= 0 ) && ( selectedIndex < _sampleSettings.Count ) ) {
        var samples = _sampleSettings[ selectedIndex ];
        if ( samples != _renderSettings.Samples ) {
          _renderSettings.Samples = samples;
          if ( null != _renderer ) {
            InvalidateRenderState( false );
            RefreshRenderState();
          }
        }
      }
    }

    public void OnSceneChanged( int selectedIndex )
    {
      if ( ( selectedIndex != _sceneIndex ) && !String.IsNullOrEmpty( _modelFilename ) ) {
        _sceneIndex = selectedIndex;
        InvalidateRenderState();
        _preview.LoadModel( _modelFilename, _sceneIndex );
        RefreshRenderState();
      }
    }

    public void OnMaterialVariantChanged( int selectedIndex )
    {
      if ( ( selectedIndex >= 0 ) && ( selectedIndex - 1 ) != _materialVariantIndex ) {
        _materialVariantIndex = selectedIndex - 1;
        InvalidateRenderState( false );
        RefreshRenderState();
      }
    }

    public void OnColorProfileChanged( int colorProfileIndex )
    {
      if ( colorProfileIndex != _colorProfileIndex ) {
        _colorProfileIndex = colorProfileIndex;
        InvalidateDisplayState();
      }
    }

    public void OnExposureChanged( float exposure )
    {
      if ( exposure != _exposure ) {
        _exposure = exposure;
        InvalidateDisplayState();
      }
    }

    public void OnDenoisedChecked( bool isChecked )
    {
      if ( isChecked != _displayDenoisedImage ) {
        _displayDenoisedImage = isChecked;
        InvalidateDisplayState();
      }
    }

    public void OnSunElevationChanged( float elevation )
    {
      if ( elevation != _environmentSettings.SunElevation ) {
        _environmentSettings.SunElevation = elevation;
        InvalidateRenderState( false );
        RefreshRenderState();
      }
    }

    public void OnSunRotationChanged( float rotation )
    {
      if ( rotation != _environmentSettings.SunRotation ) {
        _environmentSettings.SunRotation = rotation;
        InvalidateRenderState( false );
        RefreshRenderState();
      }
    }

    public void OnSunIntensityChanged( float intensity )
    {
      if ( intensity != _environmentSettings.SunIntensity ) {
        _environmentSettings.SunIntensity = intensity;
        InvalidateRenderState( false );
        RefreshRenderState();
      }
    }

    public void OnSkyIntensityChanged( float intensity )
    {
      if ( intensity != _environmentSettings.SkyIntensity ) {
        _environmentSettings.SkyIntensity = intensity;
        InvalidateRenderState( false );
        RefreshRenderState();
      }
    }

    public void OnTransparentBackgroundChecked( bool isChecked )
    {
      if ( isChecked != _environmentSettings.TransparentBackground ) {
        _environmentSettings.TransparentBackground = isChecked;
        _previewState++;
        _denoisedControl.Visibility = Visibility.Hidden;
      }
    }

    private void InitializeRenderControls( uint samplesSetting, uint colorProfileSetting )
    {
      // Samples combo
      var samplesList = new List<string>();
      for ( int i = 0; i < _sampleSettings.Count; i++ ) {
        samplesList.Add( _sampleSettings[ i ].ToString() + ( ( 1 == _sampleSettings[ i ] ) ? " Sample" : " Samples" ) );
      }
      _samplesControl.ItemsSource = samplesList;
      if ( samplesSetting < samplesList.Count ) {
        _samplesControl.SelectedIndex = (int)samplesSetting;
      } else {
        _samplesControl.SelectedIndex = 0;
      }
      _renderSettings.Samples = _sampleSettings[ _samplesControl.SelectedIndex ];

      // Color profile combo
      if ( ( null != Renderer.ColorProfiles ) && ( Renderer.ColorProfiles.Count > 0 ) ) {
        _colorProfileControl.ItemsSource = Renderer.ColorProfiles;
        _colorProfileIndex = (int)colorProfileSetting;
      } else {
        _colorProfileControl.ItemsSource = new List<string> { "No Color Profiles" };
      }
      _colorProfileControl.SelectedIndex = _colorProfileIndex;

      ResetSceneControls();
    }

    private void ResetSceneControls()
    {
      // Scene combo
      var sceneNames = new List<string>() { "Default Scene" };
      if ( ( null != _renderer ) && ( _renderer.ModelScenes.Count > 1 ) ) {
        sceneNames = new List<string>();
        foreach ( var ( sceneName, sceneCameras ) in _renderer.ModelScenes ) {
          sceneNames.Add( sceneName );
        }
      }
      _sceneControl.ItemsSource = sceneNames;
      _sceneControl.SelectedIndex = 0;

      // Material variant combo
      var materialVariants = new List<string>() { "Default Materials" };
      if ( ( null != _renderer ) && ( _renderer.MaterialVariants.Count > 0 ) ) {
        materialVariants.AddRange( _renderer.MaterialVariants );
      }
      _materialVariantControl.ItemsSource = materialVariants;
      _materialVariantControl.SelectedIndex = 0;

      // Denoised checkbox
      _denoisedControl.Visibility = Visibility.Hidden;
    }

    private void Render_RenderUpdated( object? sender, gltfviewer.RenderUpdatedEventArgs e )
    {
      if ( null != e.Image ) {
        if ( _renderState != _previewState ) {
          _renderStopEvent.Set();
        } else {
          lock ( _bitmapLock ) {
            _renderImage = e.Image;
          }
          InvalidateDisplayState();
        }
      }
    }

    private void Render_RenderProgress( object? sender, gltfviewer.RenderProgressEventArgs e )
    {
      _mainWindow.Dispatcher.BeginInvoke( () => {
        string status = String.IsNullOrEmpty( _modelFilename ) ? String.Empty : Path.GetFileName( ( _modelFilename ) + " - " );
        status += ( (uint)( e.Progress * 100 ) ).ToString() + "%";
        if ( !String.IsNullOrEmpty( e.Status ) ) {
          status += " [" + e.Status + "]";
        }
        _mainWindow.Title = status;
      } );
    }

    private void Render_RenderFinished( object? sender, gltfviewer.RenderFinishedEventArgs e )
    {
      if ( ( null != e.OriginalImage ) || ( null != e.DenoisedImage ) ) {
        lock ( _bitmapLock ) {
          if ( null != e.OriginalImage ) {
            _renderImage = e.OriginalImage;
          }
          if ( null != e.DenoisedImage ) {
            _denoisedImage = e.DenoisedImage;
            _mainWindow.Dispatcher.BeginInvoke( () => {
              _denoisedControl.Visibility = Visibility.Visible;
            } );
          }
        }
        InvalidateDisplayState();
      }
    }

    private void InvalidateRenderState( bool hideCurrentRenderImage = true )
    {
      lock ( _bitmapLock ) {
        _previewState++;
        if ( hideCurrentRenderImage ) {
          _renderDisplayControl.Visibility = Visibility.Hidden;
          _renderDisplayControl.Source = null;
          _writeableBitmap = null;
        }
        _renderBitmap = null;
        _renderImage = null;
        _denoisedImage = null;
        _denoisedControl.Visibility = Visibility.Hidden;
        _renderStopEvent.Set();
      }
    }

    private void RefreshRenderState()
    {
      _renderDisplayControl.Visibility = Visibility.Visible;
      InvalidateDisplayState();
    }

    private void InvalidateDisplayState()
    {
      _displayUpdateEvent.Set();
    }

    /// <summary>
    /// Starts the long running task which monitors the current render state and stops/restarts the renderer whenever it gets out of sync with the view state.
    /// </summary>
    private void StartRenderTask()
    {
      _renderTask = new Task( () => {
        WaitHandle[] waitHandles = new WaitHandle[] { _cancellationToken.Token.WaitHandle, _renderStopEvent };
        const int TimeoutMilliseconds = 500;
        int waitResult = 0;
        while ( ( waitResult = WaitHandle.WaitAny( waitHandles, TimeoutMilliseconds ) ) != 0 ) {
          if ( 1 == waitResult ) {
            // Stop rendering.
            if ( null != _renderer ) {
              _renderer.StopRender();
            }
            _renderStopEvent.Reset();
            _renderState = 0;
          } else {
            try {
              // Restart renderer if the view state has changed.
              if ( ( _renderState != _previewState ) && ( null != _renderer ) && ( null != _camera ) && !_renderer.IsRendering && ( _renderDisplayControl.Visibility == Visibility.Visible ) ) {
                _renderDisplayControl.Dispatcher.BeginInvoke( () => {
                  _renderState = _previewState;
                  var camera = ConvertToGltfviewerCamera( _camera, _renderSettings.Width, _renderSettings.Height );
                  _renderer.StartRender( camera, _renderSettings, _environmentSettings, _sceneIndex, _materialVariantIndex );
                } );
              }
            } catch ( Exception ex ) {
              #if DEBUG
                Debug.WriteLine( "_renderTask exception: " + ex.Message );
              #endif
            }
          }
        }
      }, _cancellationToken.Token, TaskCreationOptions.LongRunning );
      _renderTask.Start();
    }

    /// <summary>
    /// Starts the long running task which updates the currently displayed render bitmap whenever there are changes to the render image, color profile, exposure, etc.
    /// </summary>
    private void StartDisplayTask()
    {
      _displayTask = new Task( () => {
        WaitHandle[] waitHandles = new WaitHandle[] { _cancellationToken.Token.WaitHandle, _displayUpdateEvent };
        int waitResult = 0;
        while ( ( waitResult = WaitHandle.WaitAny( waitHandles, Timeout.Infinite ) ) != 0 ) {
          if ( 1 == waitResult ) {
            _displayUpdateEvent.Reset();
            lock ( _bitmapLock ) {
              var image = ( _displayDenoisedImage && ( null != _denoisedImage ) ) ? _denoisedImage : _renderImage;
              if ( null != image ) {
                _renderBitmap = Renderer.ConvertRenderImage( image, _colorProfileIndex, _exposure );
              }
            }
            _renderDisplayControl.Dispatcher.BeginInvoke( () => {
              lock ( _bitmapLock ) {
                if ( null != _renderBitmap ) {
                  if ( ( null == _writeableBitmap ) || ( ( _writeableBitmap.PixelWidth != _renderBitmap.Width ) || ( _writeableBitmap.PixelHeight != _renderBitmap.Height ) ) ) {
                    _writeableBitmap = new WriteableBitmap( _renderBitmap.Width, _renderBitmap.Height, 96, 96, System.Windows.Media.PixelFormats.Bgra32, null );
                  }
                }
                if ( ( null != _renderBitmap ) && ( null != _writeableBitmap ) ) {
                  _writeableBitmap.Lock();
                  Rectangle rect = new Rectangle( 0, 0, _renderBitmap.Width, _renderBitmap.Height );
                  var bitmapData = _renderBitmap.LockBits( rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb );
                  byte[] buffer = new byte[ bitmapData.Stride * bitmapData.Height ];
                  Marshal.Copy( bitmapData.Scan0, buffer, 0, bitmapData.Stride * bitmapData.Height );
                  _renderBitmap.UnlockBits( bitmapData );

                  Marshal.Copy( buffer, 0, _writeableBitmap.BackBuffer, bitmapData.Stride * bitmapData.Height );
                  _writeableBitmap.AddDirtyRect( new Int32Rect( 0, 0, rect.Width, rect.Height ) );
                  _writeableBitmap.Unlock();

                  if ( _renderDisplayControl.Visibility == Visibility.Visible ) {
                    _renderDisplayControl.Source = _writeableBitmap;
                  }
                }
              }
            } );
          }
        }
      }, _cancellationToken.Token, TaskCreationOptions.LongRunning );
      _displayTask.Start();
    }

    private static gltfviewer.Camera ConvertToGltfviewerCamera( System.Windows.Media.Media3D.Camera camera, uint viewWidth, uint viewHeight )
    {
      gltfviewer.Camera cam = new gltfviewer.Camera();
      var perspectiveCamera = camera as PerspectiveCamera;
      if ( null != perspectiveCamera ) {
        cam.Projection = gltfviewer.CameraProjection.Perspective;
        cam.Preset = gltfviewer.CameraPreset.None;
        cam.FarClip = (float)perspectiveCamera.FarPlaneDistance;
        cam.NearClip = (float)perspectiveCamera.NearPlaneDistance;

        double aspectRatio = ( viewHeight > 0 ) ? ( (double)viewWidth / viewHeight ) : 1;
        var verticalFOV = Renderer.ToVerticalDegreesFieldOfView( perspectiveCamera.FieldOfView, (double)viewWidth / viewHeight );
        cam.PerspectiveDegreesVerticalFieldOfView = (float)verticalFOV;

        var matrix3D = perspectiveCamera.Transform.Value;
        cam.Matrix = new float[ 16 ];
        cam.Matrix[ 0 ] = (float)matrix3D.M11;
        cam.Matrix[ 1 ] = (float)matrix3D.M12;
        cam.Matrix[ 2 ] = (float)matrix3D.M13;
        cam.Matrix[ 3 ] = (float)matrix3D.M14;

        cam.Matrix[ 4 ] = (float)matrix3D.M21;
        cam.Matrix[ 5 ] = (float)matrix3D.M22;
        cam.Matrix[ 6 ] = (float)matrix3D.M23;
        cam.Matrix[ 7 ] = (float)matrix3D.M24;

        cam.Matrix[ 8 ] = (float)matrix3D.M31;
        cam.Matrix[ 9 ] = (float)matrix3D.M32;
        cam.Matrix[ 10 ] = (float)matrix3D.M33;
        cam.Matrix[ 11 ] = (float)matrix3D.M34;

        cam.Matrix[ 12 ] = (float)matrix3D.OffsetX;
        cam.Matrix[ 13 ] = (float)matrix3D.OffsetY;
        cam.Matrix[ 14 ] = (float)matrix3D.OffsetZ;
        cam.Matrix[ 15 ] = (float)matrix3D.M44;
      }
      return cam;
    }

    private System.Windows.Media.Media3D.Camera? ConvertFromGltfviewerCamera( gltfviewer.Camera camera )
    {
      System.Windows.Media.Media3D.Camera? cam = null;
      if ( gltfviewer.CameraProjection.Perspective == camera.Projection ) {
        var horizontalFOV = Renderer.ToHorizontalDegreesFieldOfView( camera.PerspectiveDegreesVerticalFieldOfView, 1 );
        PerspectiveCamera perspectiveCamera = new PerspectiveCamera( new Point3D( 0, 0, 0 ), new Vector3D( 0, 0, -1 ), new Vector3D( 0, 1, 0 ), horizontalFOV );
        perspectiveCamera.NearPlaneDistance = camera.NearClip;
        perspectiveCamera.FarPlaneDistance = camera.FarClip;
        if ( ( null != camera.Matrix ) && ( camera.Matrix.Length == 16 ) ) {
          var matrix = new Matrix3D(
            camera.Matrix[ 0 ],
            camera.Matrix[ 1 ],
            camera.Matrix[ 2 ],
            camera.Matrix[ 3 ],

            camera.Matrix[ 4 ],
            camera.Matrix[ 5 ],
            camera.Matrix[ 6 ],
            camera.Matrix[ 7 ],

            camera.Matrix[ 8 ],
            camera.Matrix[ 9 ],
            camera.Matrix[ 10 ],
            camera.Matrix[ 11 ],

            camera.Matrix[ 12 ],
            camera.Matrix[ 13 ],
            camera.Matrix[ 14 ],
            camera.Matrix[ 15 ]
          );
          perspectiveCamera.Transform = new MatrixTransform3D(matrix);
          cam = perspectiveCamera;
        }
      }
      return cam;
    }

    private readonly System.Windows.Controls.Viewport3D _previewControl;
    private readonly System.Windows.Controls.Image _renderDisplayControl;

    private readonly System.Windows.Controls.ComboBox _samplesControl;
    private readonly System.Windows.Controls.ComboBox _sceneControl;
    private readonly System.Windows.Controls.ComboBox _materialVariantControl;
    private readonly System.Windows.Controls.ComboBox _colorProfileControl;
    private readonly System.Windows.Controls.CheckBox _denoisedControl;

    private readonly Window _mainWindow;

    private readonly bool _rendererAvailable = false;
    private readonly View _preview;
    private Renderer? _renderer = null;
    private string? _modelFilename = null;
    private System.Windows.Media.Media3D.Camera? _camera;

    private int _previewState = 0;
    private int _renderState = 0;

    private readonly List<uint> _sampleSettings = new List<uint>() { 4096, 2048, 1024, 512, 256, 128, 64, 16, 4, 1 };

    private RenderSettings _renderSettings = new RenderSettings() { Width = 0, Height = 0, Samples = 4096, TileSize = 0 };
    private EnvironmentSettings _environmentSettings = new EnvironmentSettings();

    private int _sceneIndex = Renderer.DefaultSceneIndex;
    private int _materialVariantIndex = Renderer.DefaultMaterials;
    private int _colorProfileIndex = Renderer.DefaultColorProfile;
    private float _exposure = Renderer.DefaultExposure;
    private bool _displayDenoisedImage = true;

    private readonly CancellationTokenSource _cancellationToken = new CancellationTokenSource();

    private Task? _renderTask;
    private readonly ManualResetEvent _renderStopEvent = new ManualResetEvent( false );

    private Task? _displayTask;
    private readonly ManualResetEvent _displayUpdateEvent = new ManualResetEvent( false );

    private readonly object _bitmapLock = new object();
    private System.Drawing.Bitmap? _renderBitmap = null;
    private WriteableBitmap? _writeableBitmap = null;

    private RenderImage? _renderImage = null;
    private RenderImage? _denoisedImage = null;
  }
}
