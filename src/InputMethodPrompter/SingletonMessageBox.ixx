export module SingletonMessageBox;

import std;
import <Windows.h>;

export class SingletonMessageBox
{
public:
    int Show(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
    {
        if (isShowing)
            return IDCANCEL;

        isShowing = true;
        auto result = MessageBox(hWnd, lpText, lpCaption, uType);
        isShowing = false;

        return result;
    }

private:
    std::atomic<bool> isShowing = false;
};