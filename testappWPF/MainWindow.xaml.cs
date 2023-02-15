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
using testappWPF.Properties;

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

      Width = Settings.Default.WindowWidth;
      Height = Settings.Default.WindowHeight;
      WindowState = Settings.Default.WindowMaximized ? WindowState.Maximized : WindowState.Normal;
      if ( Settings.Default.WindowLeft >= 0 ) {
        Left = Settings.Default.WindowLeft;
      }
      if ( Settings.Default.WindowTop >= 0 ) {
        Top = Settings.Default.WindowTop;
      }

      sliderExposure.Value= Settings.Default.Exposure;
      checkboxDenoised.IsChecked = Settings.Default.ShowDenoised;

      gltfviewer.EnvironmentSettings environmentSettings= new gltfviewer.EnvironmentSettings() {
        SkyIntensity = Settings.Default.SkyIntensity,
        SunIntensity = Settings.Default.SunIntensity,
        SunElevation = Settings.Default.SunElevation,
        SunRotation = Settings.Default.SunRotation,
        TransparentBackground = Settings.Default.TransparentBackground
      };

      sliderSkyIntensity.Value = environmentSettings.SkyIntensity;
      sliderSunIntensity.Value = environmentSettings.SunIntensity;
      textboxSunElevation.Text = environmentSettings.SunElevation.ToString();
      textboxSunRotation.Text = environmentSettings.SunRotation.ToString();
      checkboxTransparentBackground.IsChecked = environmentSettings.TransparentBackground;

      float exposure = (float)sliderExposure.Value;
      
      controller = new Controller( 
        viewportModel, 
        imageRender, 
        comboSamples, 
        comboScene, 
        comboMaterialVariant, 
        comboColorProfile, 
        checkboxDenoised, 
        this, 
        Settings.Default.Samples, 
        Settings.Default.ColorProfile, 
        exposure,
        Settings.Default.ShowDenoised,
        environmentSettings );
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
        Settings.Default.ColorProfile = (uint)combo.SelectedIndex;
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
        float exposure = (float)e.NewValue;
        controller.OnExposureChanged( exposure );
        Settings.Default.Exposure = exposure;
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
        Settings.Default.Samples = (uint)combo.SelectedIndex;
      }
    }

    private void checkboxDenoised_Checked( object sender, RoutedEventArgs e )
    {
      if ( null != controller ) {
        controller.OnDenoisedChecked( true );
        Settings.Default.ShowDenoised = true;
      }
    }

    private void checkboxDenoised_Unchecked( object sender, RoutedEventArgs e )
    {
      if ( null != controller ) {
        controller.OnDenoisedChecked( false );
        Settings.Default.ShowDenoised = false;
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
        float intensity = (float)e.NewValue;
        controller.OnSunIntensityChanged( intensity );
        Settings.Default.SunIntensity = intensity;
      }
    }

    private void sliderSkyIntensity_ValueChanged( object sender, RoutedPropertyChangedEventArgs<double> e )
    {
      if ( null != controller ) {
        float intensity = (float)e.NewValue;
        controller.OnSkyIntensityChanged( intensity );
        Settings.Default.SkyIntensity = intensity;
      }
    }

    private void checkboxTransparentBackground_Checked( object sender, RoutedEventArgs e )
    {
      if ( null != controller ) {
        controller.OnTransparentBackgroundChecked( true );
        Settings.Default.TransparentBackground = true;
      }
    }

    private void checkboxTransparentBackground_Unchecked( object sender, RoutedEventArgs e )
    {
      if ( null != controller ) {
        controller.OnTransparentBackgroundChecked( false );
        Settings.Default.TransparentBackground = false;
      }
    }

    private void textboxSunElevation_KeyUp( object sender, KeyEventArgs e )
    {
      var textbox = sender as TextBox;
      if ( ( null != textbox ) && ( null != controller ) ) {
        if ( ( e.Key == Key.Return ) || ( e.Key == Key.Enter ) ) {
          float elevation;
          if ( float.TryParse( textbox.Text, out elevation ) ) {
            controller.OnSunElevationChanged( elevation );
            Settings.Default.SunElevation = elevation;
          }
        }
      }
    }

    private void textboxSunRotation_KeyUp( object sender, KeyEventArgs e )
    {
      var textbox = sender as TextBox;
      if ( ( null != textbox ) && ( null != controller ) ) {
        if ( ( e.Key == Key.Return ) || ( e.Key == Key.Enter ) ) {
          float rotation;
          if ( float.TryParse( textbox.Text, out rotation ) ) {
            controller.OnSunRotationChanged( rotation );
            Settings.Default.SunRotation = rotation;
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

    private void Window_Closing( object sender, System.ComponentModel.CancelEventArgs e )
    {
      Settings.Default.WindowWidth = ActualWidth;
      Settings.Default.WindowHeight = ActualHeight;
      Settings.Default.WindowMaximized = WindowState.Maximized == WindowState;
      Settings.Default.WindowLeft = Left;
      Settings.Default.WindowTop = Top;
      Settings.Default.Save();
    }

    private void textboxSunElevation_LostKeyboardFocus( object sender, KeyboardFocusChangedEventArgs e )
    {
      var textbox = sender as TextBox;
      if ( ( null != textbox ) && ( null != controller ) ) {
        float elevation;
        if ( float.TryParse( textbox.Text, out elevation ) ) {
          controller.OnSunElevationChanged( elevation );
          Settings.Default.SunElevation = elevation;
        }
      }
    }

    private void textboxSunRotation_LostKeyboardFocus( object sender, KeyboardFocusChangedEventArgs e )
    {
      var textbox = sender as TextBox;
      if ( ( null != textbox ) && ( null != controller ) ) {
        float rotation;
        if ( float.TryParse( textbox.Text, out rotation ) ) {
          controller.OnSunRotationChanged( rotation );
          Settings.Default.SunRotation = rotation;
        }
      }
    }
  }
}
