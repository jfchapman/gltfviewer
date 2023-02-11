using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media.Media3D;
using System.Windows.Media;
using System.Windows.Controls;
using System.Drawing;
using System.Windows.Media.Imaging;
using System.Windows.Media.TextFormatting;
using System.Windows.Input;
using System.Drawing.Drawing2D;

namespace testappWPF
{
  /// <summary>
  /// Displays a (basic) 3D preview of a glTF model in the Viewport3D control.
  /// Handles mouse events to allow rotation, panning & forwards/backwards movement in the preview.
  /// </summary>
  internal class View
  {
    public View( Viewport3D viewport )
    {
      this._viewport = viewport;
    }

    public void LoadModel( string filename, int sceneIndex )
    {
      _model = new Model( filename );
      if ( ( null != _model.scenes ) && ( sceneIndex >= 0 ) && ( sceneIndex < _model.scenes.Count ) ) {
        var model3Dgroup = _model.scenes[ sceneIndex ];

        _bounds = new Rect3D();
        foreach ( var child in model3Dgroup.Children ) {
          _bounds.Union( child.Bounds );
        }

        DirectionalLight directionalLight = new DirectionalLight();
        directionalLight.Color = Colors.White;
        directionalLight.Direction = new Vector3D( 0, -1.0, 0 );
        model3Dgroup.Children.Add( directionalLight );

        ModelVisual3D modelVisual3D = new ModelVisual3D();
        modelVisual3D.Content= model3Dgroup;

        _viewport.Children.Clear();
        _viewport.Children.Add( modelVisual3D );
        _viewport.Camera = InitializeCamera();
      }
    }

		public void UpdateRotation( System.Windows.Point cursorPos )
		{
      if ( _isRotating && ( null != _xAxisRotation ) && ( null != _yAxisRotation ) ) {
			  const double kScaling = 4;

			  double deltaX = ( cursorPos.X - _lastMouseCursorPosition.X ) / kScaling;
			  double deltaY = ( cursorPos.Y - _lastMouseCursorPosition.Y ) / kScaling;

			  _lastMouseCursorPosition = cursorPos;

        AxisAngleRotation3D xAxisAngle = (AxisAngleRotation3D)_xAxisRotation.Rotation;
        xAxisAngle.Angle -= deltaY;

        AxisAngleRotation3D zAxisAngle = (AxisAngleRotation3D)_yAxisRotation.Rotation;
        zAxisAngle.Angle -= deltaX;

			  Transform3DGroup transformGroup = new Transform3DGroup();
        transformGroup.Children.Add( _originalTransform );
        transformGroup.Children.Add( _translation );
        transformGroup.Children.Add( _xAxisRotation );
        transformGroup.Children.Add( _yAxisRotation );

        _viewport.Camera.Transform = transformGroup;
      }
		}

    public void UpdateDrag( System.Windows.Point cursorPos )
    {
      if ( _isDragging && ( null != _xAxisRotation ) && ( null != _yAxisRotation ) ) {
			  const double kScaling = 1000;

			  double deltaX = cursorPos.X - _lastMouseCursorPosition.X;
			  double deltaY = cursorPos.Y - _lastMouseCursorPosition.Y;

			  _lastMouseCursorPosition = cursorPos;

        var maxExtent = Math.Max( Math.Max( _bounds.SizeX, _bounds.SizeY ), _bounds.SizeZ );

        if ( null != _translation ) {
          _translation.OffsetX -= deltaX * maxExtent / kScaling;
          _translation.OffsetY += deltaY * maxExtent / kScaling;
        }

			  Transform3DGroup transformGroup = new Transform3DGroup();
        transformGroup.Children.Add( _originalTransform );
        transformGroup.Children.Add( _translation );
        transformGroup.Children.Add( _xAxisRotation );
        transformGroup.Children.Add( _yAxisRotation );

        _viewport.Camera.Transform = transformGroup;
      }
    }

		public void StartRotation( System.Windows.Point point )
		{
      if ( !_isRotating && !_isDragging ) {
			  _lastMouseCursorPosition = point;
        _isRotating = true;
			  _viewport.CaptureMouse();
      }
		}

		public void StopRotation()
		{
      if ( _isRotating ) {
        _isRotating = false;
  		  _viewport.ReleaseMouseCapture();
      }
		}

    public void StartDrag( System.Windows.Point point )
    {
      if ( !_isDragging && !_isRotating ) {
			  _lastMouseCursorPosition = point;
        _isDragging= true;
        _viewport.CaptureMouse();
      }
    }

    public void StopDrag()
    {
      if ( _isDragging ) {
        _isDragging= false;
        _viewport.ReleaseMouseCapture();
      }
    }

    public void OnMouseWheel( int delta )
    {
      if ( !_bounds.IsEmpty ) {
			  const double kScaling = 10;

        var maxExtent = Math.Max( Math.Max( _bounds.SizeX, _bounds.SizeY ), _bounds.SizeZ );
        bool forward = delta > 0;
        var distance = ( forward ? -1 : 1 ) * maxExtent / kScaling;

        if ( null != _translation ) {
          _translation.OffsetZ += distance;
        }

			  Transform3DGroup transformGroup = new Transform3DGroup();
        transformGroup.Children.Add( _originalTransform );
        transformGroup.Children.Add( _translation );
        transformGroup.Children.Add( _xAxisRotation );
        transformGroup.Children.Add( _yAxisRotation );

        _viewport.Camera.Transform = transformGroup;
      }
    }

    private Camera? InitializeCamera()
    {
      const double horizontalFieldOfView = 45;
      const double xRotation = 0;
      const double yRotation = 0;

      PerspectiveCamera? camera = null;
      if ( null != _model ) {
        var xMin = _bounds.X;
        var xMax = xMin + _bounds.SizeX;
        var xCentre = ( xMin + xMax ) / 2;

        var yMin = _bounds.Y;
        var yMax = yMin + _bounds.SizeY;
        var yCentre = ( yMin + yMax ) / 2;

        var zMin = _bounds.Z;
        var zMax = zMin + _bounds.SizeZ;
        var zCentre = ( zMin + zMax ) / 2;

        // Place the camera at roughly the right distance to fit the model.
        double halfMaxBoundsSize = Math.Max( _bounds.SizeX, Math.Max( _bounds.SizeY, _bounds.SizeZ ) ) / 2;
        double viewAspectRatio = ( _viewport.ActualHeight > 0 ) ? ( _viewport.ActualWidth / _viewport.ActualHeight ) : 1;
        double fieldOfView = gltfviewer.Renderer.ToVerticalDegreesFieldOfView( horizontalFieldOfView, viewAspectRatio );
        double zPosition = zMax + halfMaxBoundsSize / Math.Tan( ( fieldOfView * Math.PI / 180 ) / 2 );

        Point3D centreBounds = new Point3D( xCentre, yCentre, zCentre );
        Point3D position = new Point3D( xCentre, yCentre, zPosition );
        Vector3D lookDirection = centreBounds - position;
        lookDirection.Normalize();
        Vector3D upDirection = new Vector3D( 0, 1, 0 );

        camera = new PerspectiveCamera( new Point3D( 0, 0, 0 ), lookDirection, upDirection, horizontalFieldOfView );

        var matrix = camera.Transform.Value;
        matrix.OffsetX = position.X;
        matrix.OffsetY = position.Y;
        matrix.OffsetZ = position.Z;

        _originalTransform = new MatrixTransform3D( matrix );
        _xAxisRotation = new RotateTransform3D( new AxisAngleRotation3D( new Vector3D( 1, 0, 0 ), xRotation ) );
		    _yAxisRotation = new RotateTransform3D( new AxisAngleRotation3D( new Vector3D( 0, 1, 0 ), yRotation ) );
        _translation = new TranslateTransform3D( new Vector3D( 0, 0, 0 ) );

			  Transform3DGroup transformGroup = new Transform3DGroup();
        transformGroup.Children.Add( _originalTransform );
        transformGroup.Children.Add( _translation );
        transformGroup.Children.Add( _xAxisRotation );
        transformGroup.Children.Add( _yAxisRotation );

        camera.Transform = transformGroup;

        camera.NearPlaneDistance = 1e-9;
        camera.FarPlaneDistance = 1e9;
      }
      return camera;
    }

    private Viewport3D _viewport;

    private Model? _model = null;

    private Rect3D _bounds;

		private bool _isRotating = false;
    private bool _isDragging = false;

		private System.Windows.Point _lastMouseCursorPosition = new System.Windows.Point();

    private MatrixTransform3D? _originalTransform = null;
    private RotateTransform3D? _xAxisRotation = null;
		private RotateTransform3D? _yAxisRotation = null;
    private TranslateTransform3D? _translation = null;
  }
}
