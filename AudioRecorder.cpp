// AudioRecorder.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "framework.h"
#include "AudioRecorder.h"

#include <assert.h>

#include "mfapi.h"
#include "mfidl.h"
#include "mfreadwrite.h"
#include "mfobjects.h"
#include "mferror.h"

#define MAX_LOADSTRING 100
#define SafeRelease(o) {if(o){o->Release(); o=NULL;}}

class AudioInterface
{
private:
    IMFMediaSourceEx* m_pMediaSource = 0;
    IMFSourceReader* m_pSourceReader = 0;
    bool m_bMicRecording = false;

public:
    AudioInterface()
    {
        HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hr))
            DebugBreak();

        MFStartup(MF_VERSION, MFSTARTUP_FULL);
        GetMicrophone();
    }

    ~AudioInterface()
    {
        if (m_pSourceReader)
        {
            m_pSourceReader->Release();
            m_pSourceReader = NULL;
        }

        if (m_pMediaSource)
        {
            //m_pMediaSource->Shutdown(); //releasing source reader shuts down media source
            m_pMediaSource->Release();
            m_pMediaSource = NULL;
        }

        MFShutdown();
    }

    HRESULT GetMicrophone()
    {
        IMFAttributes* pAttributes = 0;
        IMFActivate** ppDevices = 0;
        IMFMediaSource* pDevice;
        UINT32 DeviceCount = 0;

        if (MFCreateAttributes(&pAttributes, 1) != S_OK)
        {
            DebugBreak();
        }

        pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);

        if (MFEnumDeviceSources(pAttributes, &ppDevices, &DeviceCount) != S_OK)
        {
            DebugBreak();
        }

        if (DeviceCount == 0)
        {
            DebugBreak();
        }
        else
        {
            for (DWORD i = 0; i < DeviceCount; i++)
            {
                HRESULT hr = S_OK;
                WCHAR* szFriendlyName = NULL;

                // Try to get the display name.
                UINT32 cchName;
                hr = ppDevices[i]->GetAllocatedString(
                    MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                    &szFriendlyName, &cchName);

                if (SUCCEEDED(hr))
                {
                    OutputDebugString(szFriendlyName);
                    OutputDebugString(L"\n");
                }

                CoTaskMemFree(szFriendlyName);
            }
        }

        if (ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pDevice)) != S_OK)
        {
            DebugBreak();
        }

        pDevice->QueryInterface(IID_IMFMediaSourceEx, (void**)&m_pMediaSource);

        if (m_pMediaSource == 0)
            DebugBreak();

        if (MFCreateSourceReaderFromMediaSource(m_pMediaSource, NULL, &m_pSourceReader) != S_OK)
        {
            DebugBreak();
        }
        
        IMFMediaType* pType;
        if (FAILED(m_pSourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &pType)))
            DebugBreak();

        pType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

        if ( FAILED(m_pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pType)))
            DebugBreak();
      
        SafeRelease(pAttributes);

        if (ppDevices)
        {
            for (int iSrc = 0; iSrc < (int)DeviceCount; iSrc++)
            {
                if (ppDevices[iSrc] == NULL)
                    continue;

                SafeRelease(ppDevices[iSrc]);
            }

            CoTaskMemFree(ppDevices);
        }

        return S_OK;
    }

    HRESULT StartMicRecording()
    {
        IMFPresentationDescriptor* pDescriptor;
        PROPVARIANT varStartTime;
        PropVariantInit(&varStartTime);
        varStartTime.vt = VT_EMPTY;
        HRESULT hr = S_OK;

        if (m_bMicRecording)
            return S_OK;

        /*
        hr = m_pMediaSource->CreatePresentationDescriptor(&pDescriptor);

        if (FAILED(hr))
            return E_FAIL;

        hr = m_pMediaSource->Start(pDescriptor, NULL, &varStartTime );
        */

        if (FAILED(hr))
            return E_FAIL;

        m_bMicRecording = true;

        return S_OK;
    }

    HRESULT StopMicRecording()
    {
        HRESULT hr = S_OK;

        if (m_bMicRecording)
            return S_OK;

        hr = m_pMediaSource->Stop();

        if (FAILED(hr))
            return E_FAIL;

        return S_OK;
    }

};



// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    MainProc(HWND, UINT, WPARAM, LPARAM);
BOOL                InitMainForm(HWND, HINSTANCE);


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
    LoadStringW(hInstance, IDC_AUDIORECORDER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AUDIORECORDER));

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AUDIORECORDER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_AUDIORECORDER);
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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_SYSMENU,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   if (InitMainForm(hWnd, hInstance) == FALSE)
   {
       PostQuitMessage(0);
   }

   return TRUE;
}

BOOL InitMainForm(HWND hWndParent, HINSTANCE hInstance)
{
    RECT rctWindowSize = {};

    HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), hWndParent, MainProc);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd,SW_SHOW);
    UpdateWindow(hWnd);

    //Resize the parent window to match the client size
    GetClientRect(hWnd, &rctWindowSize);
    AdjustWindowRect(&rctWindowSize, WS_OVERLAPPEDWINDOW, TRUE);
    
    
    SetWindowPos(hWndParent, NULL, 0, 0, 
        rctWindowSize.right - rctWindowSize.left, 
        rctWindowSize.bottom - rctWindowSize.top,
        SWP_NOMOVE);
    UpdateWindow(hWndParent);


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
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
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

void ToggleRecording(HWND CtrlHandle)
{
    static bool bRecording = false;

    if (!bRecording)
    {
        SetWindowText(CtrlHandle, TEXT("End Recording"));
        bRecording = true;
    }
    else
    {
        SetWindowText(CtrlHandle, TEXT("Start Recording"));
        bRecording = false;
    }

}

INT_PTR CALLBACK MainProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_DESTROY:
        EndDialog(hDlg, 0);
        break;
    case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDC_RECORD)
            {
                ToggleRecording((HWND)lParam);
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}