// main.cpp — process entry point.
//
// A GUI (WIN32 subsystem) application: no console window, runs in the tray.
// All real work lives in Application.

#include <windows.h>
#include "Application.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrev*/,
                    LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    Application app;
    return app.Run(hInstance);
}
