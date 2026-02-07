module;
#include <winrt/base.h>
#include <UIAutomation.h>
export module UiaWrapper;

import std;

export class UiaWrapper
{
public:
    UiaWrapper() = delete;

    static bool IsEditControl(IUIAutomationElement* element)
    {
        CONTROLTYPEID controlType = 0;
        return SUCCEEDED(element->get_CurrentControlType(&controlType)) && controlType == UIA_EditControlTypeId;
    }

    static bool HasKeyboardFocus(IUIAutomationElement* element)
    {
        BOOL hasKeyboardFocus = FALSE;
        return SUCCEEDED(element->get_CurrentHasKeyboardFocus(&hasKeyboardFocus)) && hasKeyboardFocus == TRUE;
    }

    static bool GetIsReadOnly(IUIAutomationElement* element, bool& isReadOnly)
    {
        winrt::com_ptr<IUIAutomationValuePattern> valuePattern;
        if (SUCCEEDED(element->GetCurrentPatternAs(UIA_ValuePatternId, IID_PPV_ARGS(valuePattern.put()))) && valuePattern != nullptr)
        {
            BOOL value = FALSE;
            if (SUCCEEDED(valuePattern->get_CurrentIsReadOnly(&value)))
            {
                isReadOnly = value == TRUE;
                return true;
            }
        }

        return false;
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
