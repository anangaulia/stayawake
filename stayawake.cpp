#define WINVER 0x0400  // Windows 95 compatibility
#include <windows.h>
#include <winuser.h>
#include <tchar.h>

#define KEEP_AWAKE_INTERVAL_MS 60000  // 1 menit

DWORD WINAPI AntiSleepThread(LPVOID);
volatile bool running = true;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE hThread = NULL;
    static HWND hLabel = NULL;

    switch (msg)
    {
    case WM_CREATE:
        // Membuat label teks di jendela
        hLabel = CreateWindow(
            _T("STATIC"),
            _T("\"Stay Awake\" diaktifkan!\r\nSistem tidak akan tidur.\r\nTutup jendela untuk keluar."),
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            10, 30, 230, 50,
            hwnd, NULL,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL
        );

        // Jalankan thread anti-sleep
        hThread = CreateThread(NULL, 0, AntiSleepThread, NULL, 0, NULL);
        if (!hThread)
        {
            MessageBox(hwnd, _T("Gagal membuat thread anti-sleep."), _T("Error"), MB_ICONERROR);
            PostQuitMessage(1);
            return -1;
        }
        break;

    case WM_CLOSE:
        // Saat user menutup jendela
        running = false;
        EnableWindow(hwnd, FALSE); // mencegah klik ganda
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        // Tunggu thread selesai sebelum keluar
        if (hThread)
        {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
            hThread = NULL;
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

DWORD WINAPI AntiSleepThread(LPVOID)
{
    while (running)
    {
        // Ambil posisi kursor saat ini
        POINT pt;
        if (GetCursorPos(&pt))
        {
            // Gerakkan kursor sedikit ke kanan dan balik lagi
            mouse_event(MOUSEEVENTF_MOVE, 1, 0, 0, 0);
            mouse_event(MOUSEEVENTF_MOVE, (DWORD)-1, 0, 0, 0);
        }

        // Tidur sekitar 1 menit
        for (int i = 0; i < KEEP_AWAKE_INTERVAL_MS / 100; ++i)
        {
            if (!running) break;
            Sleep(100); // interval kecil supaya bisa berhenti cepat saat ditutup
        }
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    const TCHAR CLASS_NAME[] = _T("StayAwakeWnd");

    // Daftarkan kelas jendela
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_MENU + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc))
        return 0;

    // Buat jendela utama
    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        _T("Stay Awake"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 260, 150,
        NULL, NULL, hInst, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Loop pesan utama
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    running = false;
    return (int)msg.wParam;
}
