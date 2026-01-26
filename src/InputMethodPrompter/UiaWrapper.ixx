module;
#include <UIAutomation.h>
export module UiaWrapper;

import std;

export class UiaWrapper
{
public:
    UiaWrapper() = delete;

    static bool CheckIsInputControl(IUIAutomationElement* element)
    {
        CONTROLTYPEID controlType = 0;
        if (FAILED(element->get_CurrentControlType(&controlType)) || controlType != UIA_EditControlTypeId)
            return false;

        BOOL hasKeyboardFocus = FALSE;
        if (FAILED(element->get_CurrentHasKeyboardFocus(&hasKeyboardFocus)) || !hasKeyboardFocus)
            return false;

        return true;
    }

    static std::wstring GetElementName(IUIAutomationElement* element)
    {
        return GetElementProperty(element, &IUIAutomationElement::get_CurrentName);
    }

    static std::wstring GetElementClassName(IUIAutomationElement* element)
    {
        return GetElementProperty(element, &IUIAutomationElement::get_CurrentClassName);
    }

    static std::wstring GetElementFrameworkId(IUIAutomationElement* element)
    {
        return GetElementProperty(element, &IUIAutomationElement::get_CurrentFrameworkId);
    }

private:
    template<typename TType, typename TGetterFunc>
    static std::wstring GetElementProperty(TType* element, TGetterFunc getter)
    {
        std::wstring result;
        BSTR value = nullptr;
        auto hr = (element->*getter)(&value);
        if (SUCCEEDED(hr) && value != nullptr)
            result = value;

        if (value != nullptr)
            SysFreeString(value);

        return result;
    }
};
