export module RegistryKey;

import std;
import <Windows.h>;

export class RegistryKey
{
public:
    RegistryKey(const std::wstring& registryKeyPath) : registryKeyPath(registryKeyPath)
    {
    }

    // true  ERROR_SUCCESS
    // false ERROR_FILE_NOT_FOUND
    LSTATUS HasKey(const std::wstring& name)
    {
        HKEY key;
        auto result = RegOpenKeyEx(HKEY_CURRENT_USER, registryKeyPath.c_str(), 0, KEY_READ, &key);
        if (result != ERROR_SUCCESS)
            return result;

        result = RegQueryValueEx(key, name.c_str(), nullptr, nullptr, nullptr, nullptr);

        RegCloseKey(key);

        return result;
    }

    // true ERROR_SUCCESS
    LSTATUS AddKeyValue(const std::wstring& name, const std::wstring& value)
    {
        HKEY key;
        auto result = RegOpenKeyEx(HKEY_CURRENT_USER, registryKeyPath.c_str(), 0, KEY_WRITE, &key);
        if (result != ERROR_SUCCESS)
            return result;

        result = RegSetValueEx(
            key,
            name.c_str(),
            0,
            REG_SZ,
            (const BYTE*)value.c_str(),
            (DWORD)((value.size() + 1) * sizeof(wchar_t))
        );

        RegCloseKey(key);

        return result;
    }

    // true ERROR_SUCCESS,ERROR_FILE_NOT_FOUND
    LSTATUS RemoveKey(const std::wstring& name)
    {
        HKEY key;
        auto result = RegOpenKeyEx(HKEY_CURRENT_USER, registryKeyPath.c_str(), 0, KEY_WRITE, &key);
        if (result != ERROR_SUCCESS)
            return result;

        result = RegDeleteValueW(key, name.c_str());

        RegCloseKey(key);

        return result;
    }

private:
    const std::wstring registryKeyPath;
};
