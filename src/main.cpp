#include "MainWindow.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    Gomoku::MainWindow window(instance);
    if (!window.Create())
    {
        return 0;
    }

    window.Run();
    return 0;
}
