// testapp.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "testapp.h"

#include "testviewer.h"
#include <map>
#include <memory>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

std::unique_ptr<TestViewer> gTestViewer;        // Test object that consumes the gltfviewer library

///////////////////////////////////////////////////////////////////////////////
// Scene menu

// A limit to the range of command IDs reserved for scenes contained in the current model.
#define ID_SCENE_LAST    ID_SCENE_FIRST + 100

// Currently selected scene.
int gCurrentScene = ID_SCENE_DEFAULT;

// Updates the check states for the scene menu.
void UpdateSceneMenuCheckState( HMENU menu )
{
  CheckMenuItem( menu, ID_SCENE_DEFAULT, ( ID_SCENE_DEFAULT == gCurrentScene ) ? MF_CHECKED : MF_UNCHECKED );

  if ( !gTestViewer )
    return;

  const size_t scene_count = gTestViewer->GetSceneNames().size();
  for ( int id = ID_SCENE_FIRST; id < ID_SCENE_FIRST + scene_count; id++ ) {
    CheckMenuItem( menu, id, ( id == gCurrentScene ) ? MF_CHECKED : MF_UNCHECKED );
  }
}

// Updates the scene menu with commands for any scenes contained in the current model.
void UpdateSceneMenu( HMENU menu )
{
  for ( uint32_t i = ID_SCENE_LAST; i > ID_SCENE_FIRST; i-- ) {
    RemoveMenu( menu, i, MF_BYCOMMAND );
  }
  ModifyMenu( menu, ID_SCENE_FIRST, MF_BYCOMMAND | MF_STRING | MF_GRAYED, ID_SCENE_FIRST, L"No Scenes" );

  UpdateSceneMenuCheckState( menu );

  if ( !gTestViewer )
    return;

  const auto& scenes = gTestViewer->GetSceneNames();
  if ( !scenes.empty() ) {
    const int scene_count = static_cast<int>( min( scenes.size(), ID_SCENE_LAST - ID_SCENE_FIRST ) );

    std::wstring scene_name = TestViewer::FromUTF8( scenes.back() );
    ModifyMenu( menu, ID_SCENE_FIRST, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SCENE_FIRST + scene_count - 1, scene_name.c_str() );

    for ( int i = scene_count - 1; i > 0; i-- ) {
      scene_name = TestViewer::FromUTF8( scenes[ i - 1 ] );
      InsertMenu( menu, ID_SCENE_FIRST + i, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_SCENE_FIRST + i - 1, scene_name.c_str() );
    }
  }

  UpdateSceneMenuCheckState( menu );
}

///////////////////////////////////////////////////////////////////////////////
// Variant menu

// A limit to the range of command IDs reserved for material variants contained in the current model.
#define ID_MATERIAL_VARIANT_LAST    ID_MATERIAL_VARIANT_FIRST + 100

// Currently selected material variant.
int gCurrentMaterialVariant = ID_MATERIAL_VARIANT_NONE;

// Updates the check states for the material variants menu.
void UpdateVariantMenuCheckState( HMENU menu )
{
  CheckMenuItem( menu, ID_MATERIAL_VARIANT_NONE, ( ID_MATERIAL_VARIANT_NONE == gCurrentMaterialVariant ) ? MF_CHECKED : MF_UNCHECKED );

  if ( !gTestViewer )
    return;

  const size_t material_variant_count = gTestViewer->GetMaterialVariants().size();
  for ( int id = ID_MATERIAL_VARIANT_FIRST; id < ID_MATERIAL_VARIANT_FIRST + material_variant_count; id++ ) {
    CheckMenuItem( menu, id, ( id == gCurrentMaterialVariant ) ? MF_CHECKED : MF_UNCHECKED );
  }
}

// Updates the variant menu with commands for any material variants contained in the current model.
void UpdateVariantMenu( HMENU menu )
{
  for ( uint32_t i = ID_MATERIAL_VARIANT_LAST; i > ID_MATERIAL_VARIANT_FIRST; i-- ) {
    RemoveMenu( menu, i, MF_BYCOMMAND );
  }
  ModifyMenu( menu, ID_MATERIAL_VARIANT_FIRST, MF_BYCOMMAND | MF_STRING | MF_GRAYED, ID_MATERIAL_VARIANT_FIRST, L"No Material Variants" );

  UpdateVariantMenuCheckState( menu );

  if ( !gTestViewer )
    return;

  const auto& material_variants = gTestViewer->GetMaterialVariants();
  if ( !material_variants.empty() ) {
    const int variant_count = static_cast<int>( min( material_variants.size(), ID_MATERIAL_VARIANT_LAST - ID_MATERIAL_VARIANT_FIRST ) );

    std::wstring variant_name = TestViewer::FromUTF8( material_variants.back() );
    ModifyMenu( menu, ID_MATERIAL_VARIANT_FIRST, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_MATERIAL_VARIANT_FIRST + variant_count - 1, variant_name.c_str() );

    for ( int i = variant_count - 1; i > 0; i-- ) {
      variant_name = TestViewer::FromUTF8( material_variants[ i - 1 ] );
      InsertMenu( menu, ID_MATERIAL_VARIANT_FIRST + i, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_MATERIAL_VARIANT_FIRST + i - 1, variant_name.c_str() );
    }
  }

  UpdateVariantMenuCheckState( menu );
}

///////////////////////////////////////////////////////////////////////////////
// Camera menu

// A limit to the range of command IDs reserved for cameras contained in the current model.
#define ID_CAMERA_MODEL_LAST    ID_CAMERA_MODEL_FIRST + 100

// Camera menu command IDs for the gltfviewer camera presets.
const std::map<int, gltfviewer_camera_preset> kCameraPresets = {
  { ID_CAMERA_DEFAULT, gltfviewer_camera_preset_default },
  { ID_CAMERA_FRONT, gltfviewer_camera_preset_front },
  { ID_CAMERA_BACK, gltfviewer_camera_preset_back },
  { ID_CAMERA_LEFT, gltfviewer_camera_preset_left },
  { ID_CAMERA_RIGHT, gltfviewer_camera_preset_right },
  { ID_CAMERA_TOP, gltfviewer_camera_preset_top },
  { ID_CAMERA_BOTTOM, gltfviewer_camera_preset_bottom }              
};

// Currently selected camera preset.
int gCurrentCameraPreset = ID_CAMERA_DEFAULT;

// Updates the check states for the camera menu.
void UpdateCameraMenuCheckState( HMENU menu )
{
  CheckMenuItem( menu, ID_CAMERA_DEFAULT, ( ID_CAMERA_DEFAULT == gCurrentCameraPreset ) ? MF_CHECKED : MF_UNCHECKED );
  CheckMenuItem( menu, ID_CAMERA_FRONT, ( ID_CAMERA_FRONT == gCurrentCameraPreset ) ? MF_CHECKED : MF_UNCHECKED );
  CheckMenuItem( menu, ID_CAMERA_BACK, ( ID_CAMERA_BACK == gCurrentCameraPreset ) ? MF_CHECKED : MF_UNCHECKED );
  CheckMenuItem( menu, ID_CAMERA_LEFT, ( ID_CAMERA_LEFT == gCurrentCameraPreset ) ? MF_CHECKED : MF_UNCHECKED );
  CheckMenuItem( menu, ID_CAMERA_RIGHT, ( ID_CAMERA_RIGHT == gCurrentCameraPreset ) ? MF_CHECKED : MF_UNCHECKED );
  CheckMenuItem( menu, ID_CAMERA_TOP, ( ID_CAMERA_TOP == gCurrentCameraPreset ) ? MF_CHECKED : MF_UNCHECKED );
  CheckMenuItem( menu, ID_CAMERA_BOTTOM, ( ID_CAMERA_BOTTOM == gCurrentCameraPreset ) ? MF_CHECKED : MF_UNCHECKED );

  if ( !gTestViewer )
    return;

  const int model_camera_count = static_cast<int>( gTestViewer->GetModelCameraCount() );
  for ( int id = ID_CAMERA_MODEL_FIRST; id < ID_CAMERA_MODEL_FIRST + model_camera_count; id++ ) {
    CheckMenuItem( menu, id, ( id == gCurrentCameraPreset ) ? MF_CHECKED : MF_UNCHECKED );
  }
}

// Updates the camera menu with commands for any cameras contained in the current model.
void UpdateCameraMenu( HMENU menu )
{
  for ( uint32_t i = ID_CAMERA_MODEL_LAST; i > ID_CAMERA_MODEL_FIRST; i-- ) {
    RemoveMenu( menu, i, MF_BYCOMMAND );
  }
  ModifyMenu( menu, ID_CAMERA_MODEL_FIRST, MF_BYCOMMAND | MF_STRING | MF_GRAYED, ID_CAMERA_MODEL_FIRST, L"No Model Cameras" );

  UpdateCameraMenuCheckState( menu );

  if ( !gTestViewer )
    return;

  const uint32_t model_camera_count = static_cast<uint32_t>( gTestViewer->GetModelCameraCount() );
  if ( model_camera_count > 0 ) {
    const std::wstring menuStr = L"Model Camera " + std::to_wstring( model_camera_count - 1 );
    ModifyMenu( menu, ID_CAMERA_MODEL_FIRST, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_CAMERA_MODEL_FIRST + model_camera_count - 1, menuStr.c_str() );

    for ( uint32_t i = model_camera_count - 1; i > 0; i-- ) {
      std::wstring menuStr = L"Model Camera " + std::to_wstring( i - 1 );
      InsertMenu( menu, ID_CAMERA_MODEL_FIRST + i, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_CAMERA_MODEL_FIRST + i - 1, menuStr.c_str() );
    }
  }

  UpdateCameraMenuCheckState( menu );
}

///////////////////////////////////////////////////////////////////////////////
// Display menu

// A limit to the range of color conversion command IDs.
#define ID_DISPLAY_LOOK_LAST    ID_DISPLAY_LOOK_FIRST + 100

// Currently selected color conversion look.
int gCurrentDisplayLook = ID_DISPLAY_DEFAULT;

// Currently selected exposure.
int gCurrentExposure = ID_EXPOSURE_DEFAULT;

// Maps the exposure menu command ID to the corresponding exposure value.
const std::map<int, float> kExposureValues = {
  { ID_EXPOSURE_PLUS_3_0, 3.0f },
  { ID_EXPOSURE_PLUS_2_5, 2.5f },
  { ID_EXPOSURE_PLUS_2_0, 2.0f },
  { ID_EXPOSURE_PLUS_1_5, 1.5f },
  { ID_EXPOSURE_PLUS_1_0, 1.0f },
  { ID_EXPOSURE_PLUS_0_5, 0.5f },
  { ID_EXPOSURE_DEFAULT, 0.0f },
  { ID_EXPOSURE_MINUS_0_5, -0.5f },
  { ID_EXPOSURE_MINUS_1_0, -1.0f },
  { ID_EXPOSURE_MINUS_1_5, -1.5f },
  { ID_EXPOSURE_MINUS_2_0, -2.0f },
  { ID_EXPOSURE_MINUS_2_5, -2.5f },
  { ID_EXPOSURE_MINUS_3_0, -3.0f }
};

// Updates the check states for the exposure sub menu.
void UpdateExposureMenuCheckState( HMENU menu )
{
  for ( const auto& [ commandID, value ] : kExposureValues ) {
    CheckMenuItem( menu, commandID, ( commandID == gCurrentExposure ) ? MF_CHECKED : MF_UNCHECKED );
  }
}

// Updates the check states for the display menu.
void UpdateDisplayMenuCheckState( HMENU menu )
{
  CheckMenuItem( menu, ID_DISPLAY_DEFAULT, ( ID_DISPLAY_DEFAULT == gCurrentDisplayLook ) ? MF_CHECKED : MF_UNCHECKED );

  if ( !gTestViewer )
    return;

  const size_t look_count = gTestViewer->GetColorConversionLooks().size();
  for ( int id = ID_DISPLAY_LOOK_FIRST; id < ID_DISPLAY_LOOK_FIRST + look_count; id++ ) {
    CheckMenuItem( menu, id, ( id == gCurrentDisplayLook ) ? MF_CHECKED : MF_UNCHECKED );
  }

  UpdateExposureMenuCheckState( menu );
}

// Updates the display menu with commands for color conversion.
void UpdateDisplayMenu( HMENU menu )
{
  for ( uint32_t i = ID_DISPLAY_LOOK_LAST; i > ID_DISPLAY_LOOK_FIRST; i-- ) {
    RemoveMenu( menu, i, MF_BYCOMMAND );
  }
  ModifyMenu( menu, ID_DISPLAY_LOOK_FIRST, MF_BYCOMMAND | MF_STRING | MF_GRAYED, ID_DISPLAY_LOOK_FIRST, L"No Color Looks" );

  UpdateDisplayMenuCheckState( menu );

  if ( !gTestViewer )
    return;

  const auto& looks = gTestViewer->GetColorConversionLooks();
  if ( !looks.empty() ) {
    const int look_count = static_cast<int>( min( looks.size(), ID_DISPLAY_LOOK_LAST - ID_DISPLAY_LOOK_FIRST ) );

    std::wstring look_name = TestViewer::FromUTF8( looks.back() );
    ModifyMenu( menu, ID_DISPLAY_LOOK_FIRST, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_DISPLAY_LOOK_FIRST + look_count - 1, look_name.c_str() );

    for ( int i = look_count - 1; i > 0; i-- ) {
      look_name = TestViewer::FromUTF8( looks[ i - 1 ] );
      InsertMenu( menu, ID_DISPLAY_LOOK_FIRST + i, MF_BYCOMMAND | MF_STRING | MF_ENABLED, ID_DISPLAY_LOOK_FIRST + i - 1, look_name.c_str() );
    }
  }

  UpdateDisplayMenuCheckState( menu );
}


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TESTAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTAPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTAPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = nullptr;
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TESTAPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case ID_FILE_OPEN: {
                OPENFILENAME ofn = {};
                ofn.lStructSize = sizeof( OPENFILENAME );
                ofn.hwndOwner = hWnd;
                ofn.lpstrTitle = L"Select glTF file";
                ofn.lpstrFilter = L"glTF files (.gltf, .glb)\0*.gltf;*.glb\0\0";
                ofn.Flags = OFN_EXPLORER;
                WCHAR result[ 1024 ] = {};
                ofn.nMaxFile = 1024;
                ofn.lpstrFile = result;
                if ( GetOpenFileName( &ofn ) ) {
                  gTestViewer.reset();
                  gTestViewer = std::make_unique<TestViewer>( hWnd );
                  gTestViewer->LoadFile( result );

                  gCurrentScene = ID_SCENE_DEFAULT;
                  UpdateSceneMenu( GetMenu( hWnd ) );

                  gCurrentMaterialVariant = ID_MATERIAL_VARIANT_NONE;
                  UpdateVariantMenu( GetMenu( hWnd ) );

                  gCurrentCameraPreset = ID_CAMERA_DEFAULT;
                  UpdateCameraMenu( GetMenu( hWnd ) );

                  gCurrentDisplayLook = ID_DISPLAY_DEFAULT;
                  gCurrentExposure = ID_EXPOSURE_DEFAULT;
                  UpdateDisplayMenu( GetMenu( hWnd ) );
                }
                break;
            }
            case ID_CAMERA_DEFAULT:
            case ID_CAMERA_FRONT:
            case ID_CAMERA_BACK:
            case ID_CAMERA_LEFT:
            case ID_CAMERA_RIGHT:
            case ID_CAMERA_TOP:
            case ID_CAMERA_BOTTOM: {
              if ( const auto it = kCameraPresets.find( wmId ); kCameraPresets.end() != it ) {
                if ( gTestViewer )
                  gTestViewer->SetCameraPreset( it->second );
                gCurrentCameraPreset = it->first;
                UpdateCameraMenuCheckState( GetMenu( hWnd ) );
              }
              break;
            }
            case ID_SCENE_DEFAULT: {
              if ( gTestViewer ) {
                gTestViewer->SetSceneIndex( gltfviewer_default_scene_index );
                gCurrentScene = ID_SCENE_DEFAULT;
                UpdateSceneMenuCheckState( GetMenu( hWnd ) );
              }
              break;
            }
            case ID_MATERIAL_VARIANT_NONE: {
              if ( gTestViewer ) {
                gTestViewer->SetMaterialVariantIndex( gltfviewer_default_scene_materials );
                gCurrentMaterialVariant = ID_MATERIAL_VARIANT_NONE;
                UpdateVariantMenuCheckState( GetMenu( hWnd ) );
              }
              break;
            }
            case ID_DISPLAY_DEFAULT: {
              if ( gTestViewer ) {
                gTestViewer->SetColorConversionLookIndex( gltfviewer_image_convert_linear_to_srgb );
                gCurrentDisplayLook = ID_DISPLAY_DEFAULT;
                UpdateDisplayMenuCheckState( GetMenu( hWnd ) );
              }
              break;
            }
            case ID_EXPOSURE_PLUS_3_0:
            case ID_EXPOSURE_PLUS_2_5:
            case ID_EXPOSURE_PLUS_2_0:
            case ID_EXPOSURE_PLUS_1_5:
            case ID_EXPOSURE_PLUS_1_0:
            case ID_EXPOSURE_PLUS_0_5:
            case ID_EXPOSURE_DEFAULT:
            case ID_EXPOSURE_MINUS_0_5:
            case ID_EXPOSURE_MINUS_1_0:
            case ID_EXPOSURE_MINUS_1_5:
            case ID_EXPOSURE_MINUS_2_0:
            case ID_EXPOSURE_MINUS_2_5:
            case ID_EXPOSURE_MINUS_3_0: {
              if ( gTestViewer ) {
                gCurrentExposure = wmId;
                UpdateExposureMenuCheckState( GetMenu( hWnd ) );
                if ( const auto it = kExposureValues.find( wmId ); kExposureValues.end() != it ) {
                  gTestViewer->SetExposure( it->second );
                }
              }
              break;
            }

            default:
              if ( ( wmId >= ID_SCENE_FIRST ) && ( wmId <= ID_SCENE_LAST ) ) {
                if ( gTestViewer ) {
                  gTestViewer->SetSceneIndex( wmId - ID_SCENE_FIRST );
                  gCurrentScene = wmId;
                  UpdateSceneMenuCheckState( GetMenu( hWnd ) );
                }
                break;
              } else if ( ( wmId >= ID_MATERIAL_VARIANT_FIRST ) && ( wmId <= ID_MATERIAL_VARIANT_LAST ) ) {
                if ( gTestViewer ) {
                  gTestViewer->SetMaterialVariantIndex( wmId - ID_MATERIAL_VARIANT_FIRST );
                  gCurrentMaterialVariant = wmId;
                  UpdateVariantMenuCheckState( GetMenu( hWnd ) );
                }
                break;
              } else if ( ( wmId >= ID_CAMERA_MODEL_FIRST ) && ( wmId <= ID_CAMERA_MODEL_LAST ) ) {
                if ( gTestViewer ) {
                  gTestViewer->SetCameraFromModel( wmId - ID_CAMERA_MODEL_FIRST );
                  gCurrentCameraPreset = wmId;
                  UpdateCameraMenuCheckState( GetMenu( hWnd ) );
                }
                break;
              } else if ( ( wmId >= ID_DISPLAY_LOOK_FIRST ) && ( wmId <= ID_DISPLAY_LOOK_LAST ) ) {
                if ( gTestViewer ) {
                  gTestViewer->SetColorConversionLookIndex( wmId - ID_DISPLAY_LOOK_FIRST );
                  gCurrentDisplayLook = wmId;
                  UpdateDisplayMenuCheckState( GetMenu( hWnd ) );
                }
                break;
              } else {
                return DefWindowProc(hWnd, message, wParam, lParam);
              }
           }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            if ( gTestViewer ) {
              gTestViewer->Redraw();
            } else {
              FillRect( hdc, &ps.rcPaint, GetSysColorBrush( COLOR_WINDOW ) );
            }
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        gTestViewer.reset();
        PostQuitMessage(0);
        break;
    case WM_CREATE:
        UpdateSceneMenu( GetMenu( hWnd ) );
        UpdateCameraMenu( GetMenu( hWnd ) );
        UpdateVariantMenu( GetMenu( hWnd ) );
        UpdateDisplayMenu( GetMenu( hWnd ) );
        break;
    case WM_SIZE:
        if ( gTestViewer && ( SIZE_MINIMIZED != wParam ) )
          gTestViewer->OnSize( LOWORD( lParam ), HIWORD( lParam ) );
        break;
    case MSG_UPDATE_PROGRRESS:
        if ( gTestViewer )
          gTestViewer->OnUpdateProgress();
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
