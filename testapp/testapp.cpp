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
}

// Updates the camera menu with commands for any cameras contained in the current model.
void UpdateCameraMenu( HMENU menu )
{
  for ( uint32_t i = ID_CAMERA_MODEL_LAST; i > ID_CAMERA_MODEL_FIRST; i-- ) {
    RemoveMenu( menu, i, MF_BYCOMMAND );
  }
  ModifyMenu( menu, ID_CAMERA_MODEL_FIRST, MF_BYCOMMAND | MF_STRING | MF_GRAYED, ID_CAMERA_MODEL_FIRST, L"No Model Cameras" );

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

                  gCurrentCameraPreset = ID_CAMERA_DEFAULT;
                  UpdateCameraMenu( GetMenu( hWnd ) );
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

            default:
                if ( ( wmId >= ID_CAMERA_MODEL_FIRST ) && ( wmId <= ID_CAMERA_MODEL_LAST ) ) {
                  if ( gTestViewer )
                    gTestViewer->SetCameraFromModel( wmId - ID_CAMERA_MODEL_FIRST );
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
        UpdateCameraMenu( GetMenu( hWnd ) );
        break;
    case WM_SIZE:
        if ( gTestViewer && ( SIZE_MINIMIZED != wParam ) )
          gTestViewer->OnSize( LOWORD( lParam ), HIWORD( lParam ) );
        break;
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
