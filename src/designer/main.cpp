// main.cpp — entry point for the Ancient Uyghur Layout Designer (GUI editor).
#include <windows.h>
#include "DesignerWindow.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    DesignerWindow app;
    return app.Run(hInstance, nCmdShow);
}
