module;
#include <Windows.h>
#include "Macros.h"
export module SingletonMessageBox;

import std;


export class SingletonMessageBox
{
public:
    int Show(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
    {
        RETURN_IF(isShowing, IDCANCEL);

        isShowing = true;
        auto result = MessageBox(hWnd, lpText, lpCaption, uType);
        isShowing = false;

        return result;
    }

private:
    std::atomic<bool> isShowing = false;
};