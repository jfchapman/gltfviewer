using gltfviewer;
using System;
using System.Collections.Generic;
using System.Data.Common;
using System.Drawing.Imaging;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace testappWPF
{
  /// <summary>
  /// Interaction logic for MainWindow.xaml
  /// </summary>
  public partial class MainWindow : Window
  {
    public MainWindow()
    {
      InitializeComponent();

      float exposure = (float)sliderExposure.Value;
      
      gltfviewer.EnvironmentSettings environmentSettings= new gltfviewer.EnvironmentSettings();
      float elevation;
      if ( float.TryParse( textboxSunElevation.Text, out elevation ) ) {
        environmentSettings.SunElevation = elevation;
      }
      float rotation;
      if ( float.TryParse( textboxSunRotation.Text, out rotation ) ) {
        environmentSettings.SunRotation = rotation;
      }
      environmentSettings.SunIntensity = (float)sliderSunIntensity.Value;
      environmentSettings.SkyIntensity = (float)sliderSkyIntensity.Value;
      environmentSettings.TransparentBackground = checkboxTransparentBackground.IsChecked.GetValueOrDefault();

      controller = new Controller( viewportModel, imageRender, comboSamples, comboScene, comboMaterialVariant, comboColorProfile, checkboxDenoised, this, exposure, environmentSettings );
    }

    private void Window_Closed( object sender, EventArgs e )
    {
      controller.OnClose();
    }

    private readonly Controller controller;

    private void Window_MouseDown( object sender, MouseButtonEventArgs e )
    {
      UIElement element = (UIElement)sender;
      Point point = e.GetPosition( element );
      if ( MouseButtonState.Pressed == e.LeftButton ) {
        controller.OnLeftMouseDown( point );
      } else if ( MouseButtonState.Pressed == e.MiddleButton ) {
        controller.OnMiddleMouseDown( point );
      }
    }

    private void Window_MouseUp( object sender, MouseButtonEventArgs e )
    {
      controller.OnMouseUp();
    }

    private void Window_MouseMove( object sender, MouseEventArgs e )
    {
			UIElement element = (UIElement)sender;
			Point point = e.GetPosition( element );
      controller.OnMouseMove( point );
    }

    private void Window_MouseWheel( object sender, MouseWheelEventArgs e )
    {
      controller.OnMouseWheel( e.Delta );
    }

    private void Window_SizeChanged( object sender, SizeChangedEventArgs e )
    {
      controller.OnResize();
    }

    private void buttonLoadModel_Click( object sender, RoutedEventArgs e )
    {
      var dialog = new Microsoft.Win32.OpenFileDialog();
      dialog.Title = "Open glTF model";
      dialog.Filter = "glTF files|*.gltf;*.glb";

      bool? result = dialog.ShowDialog();
      if ( result == true ) {
        controller.LoadModel( dialog.FileName );
      }
    }

    private void comboColorProfile_SelectionChanged( object sender, SelectionChangedEventArgs e )
    {
      var combo = sender as ComboBox;
      if ( ( null != controller ) && ( null != combo ) ) {
        controller.OnColorProfileChanged( combo.SelectedIndex );
      }
    }

    private void comboScene_SelectionChanged( object sender, SelectionChangedEventArgs e )
    {
      var combo = sender as ComboBox;
      if ( ( null != controller ) && ( null != combo ) ) {
        controller.OnSceneChanged( combo.SelectedIndex );
      }
    }

    private void comboMaterialVariant_SelectionChanged( object sender, SelectionChangedEventArgs e )
    {
      var combo = sender as ComboBox;
      if ( ( null != controller ) && ( null != combo ) ) {
        controller.OnMaterialVariantChanged( combo.SelectedIndex );
      }
    }

    private void sliderExposure_ValueChanged( object sender, RoutedPropertyChangedEventArgs<double> e )
    {
      if ( null != controller ) {
        controller.OnExposureChanged( (float)e.NewValue );
      }
    }

    private void sliderExposure_MouseRightButtonDown( object sender, MouseButtonEventArgs e )
    {
      var slider = sender as Slider;
      if ( ( null != slider ) ) {
        slider.Value = 0;
      }
    }

    private void comboSamples_SelectionChanged( object sender, SelectionChangedEventArgs e )
    {
      var combo = sender as ComboBox;
      if ( ( null != controller ) && ( null != combo ) ) {
        controller.OnSamplesChanged( combo.SelectedIndex );
      }
    }

    private void checkboxDenoised_Checked( object sender, RoutedEventArgs e )
    {
      if ( null != controller ) {
        controller.OnDenoisedChecked( true );
      }
    }

    private void checkboxDenoised_Unchecked( object sender, RoutedEventArgs e )
    {
      if ( null != controller ) {
        controller.OnDenoisedChecked( false );
      }
    }

    private void Grid_Render_MouseEnter( object sender, MouseEventArgs e )
    {
      var grid = sender as Grid;
      if ( null != grid ) {
        foreach ( var child in grid.Children ) {
          var rectangle = child as Rectangle;
          if ( null != rectangle ) {
            rectangle.Opacity = 1;
          }
        }
      }
    }

    private void Grid_Render_MouseLeave( object sender, MouseEventArgs e )
    {
      var grid = sender as Grid;
      if ( null != grid ) {
        foreach ( var child in grid.Children ) {
          var rectangle = child as Rectangle;
          if ( null != rectangle ) {
            rectangle.Opacity = 0.5;
          }
        }
      }
    }

    private void Grid_Environment_MouseEnter( object sender, MouseEventArgs e )
    {
      var grid = sender as Grid;
      if ( null != grid ) {
        foreach ( var child in grid.Children ) {
          var rectangle = child as Rectangle;
          if ( null != rectangle ) {
            rectangle.Opacity = 1;
          }
        }
      }
    }

    private void Grid_Environment_MouseLeave( object sender, MouseEventArgs e )
    {
      var grid = sender as Grid;
      if ( null != grid ) {
        foreach ( var child in grid.Children ) {
          var rectangle = child as Rectangle;
          if ( null != rectangle ) {
            rectangle.Opacity = 0.5;
          }
        }
      }
    }

    private void sliderSunIntensity_ValueChanged( object sender, RoutedPropertyChangedEventArgs<double> e )
    {
      if ( null != controller ) {
        controller.OnSunIntensityChanged( (float)e.NewValue );
      }
    }

    private void sliderSkyIntensity_ValueChanged( object sender, RoutedPropertyChangedEventArgs<double> e )
    {
      if ( null != controller ) {
        controller.OnSkyIntensityChanged( (float)e.NewValue );
      }
    }

    private void checkboxTransparentBackground_Checked( object sender, RoutedEventArgs e )
    {
      if ( null != controller ) {
        controller.OnTransparentBackgroundChecked( true );
      }
    }

    private void checkboxTransparentBackground_Unchecked( object sender, RoutedEventArgs e )
    {
      if ( null != controller ) {
        controller.OnTransparentBackgroundChecked( false );
      }
    }

    private void textboxSunElevation_KeyUp( object sender, KeyEventArgs e )
    {
      var textbox = sender as TextBox;
      if ( ( null != textbox ) && ( null != controller ) ) {
        if ( ( e.Key == Key.Return ) || ( e.Key == Key.Enter ) || ( e.Key == Key.Tab ) ) {
          float elevation;
          if ( float.TryParse( textbox.Text, out elevation ) ) {
            controller.OnSunElevationChanged( elevation );
          }
        }
      }
    }

    private void textboxSunRotation_KeyUp( object sender, KeyEventArgs e )
    {
      var textbox = sender as TextBox;
      if ( ( null != textbox ) && ( null != controller ) ) {
        if ( ( e.Key == Key.Return ) || ( e.Key == Key.Enter ) || ( e.Key == Key.Tab ) ) {
          float rotation;
          if ( float.TryParse( textbox.Text, out rotation ) ) {
            controller.OnSunRotationChanged( rotation );
          }
        }
      }
    }

    private void textboxSunElevation_GotKeyboardFocus( object sender, RoutedEventArgs e )
    {
      var textbox = sender as TextBox;
      if ( null != textbox ) {
        textbox.Select( 0, textbox.Text.Length );
      }
    }

    private void textboxSunRotation_GotKeyboardFocus( object sender, KeyboardFocusChangedEventArgs e )
    {
      var textbox = sender as TextBox;
      if ( null != textbox ) {
        textbox.Select( 0, textbox.Text.Length );
      }
    }
  }
}
