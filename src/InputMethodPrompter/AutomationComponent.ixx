module;
#include <Unknwn.h>
#include <winrt/base.h>
#include <UIAutomation.h>
#include "Macros.h"
export module AutomationComponent;

import std;

export using FocusChangedCallback = std::function<void(IUIAutomationElement*)>;

struct FocusChangedHandler : winrt::implements<FocusChangedHandler, IUIAutomationFocusChangedEventHandler>
{
public:
    FocusChangedHandler()
    {
    }

    HRESULT STDMETHODCALLTYPE HandleFocusChangedEvent(IUIAutomationElement* sender) override
    {
        if (callback != nullptr)
            callback(sender);

        return S_OK;
    }

    void SetFocusChangedCallback(FocusChangedCallback callback)
    {
        this->callback = callback;
    }

private:
    FocusChangedCallback callback;
};

export struct IAutomation
{
    virtual ~IAutomation() = default;
    virtual HRESULT RegisterFocusChangedEvent(FocusChangedCallback callback) const = 0;
    virtual HRESULT UnregisterFocusChangedEvent() const = 0;
    virtual HWND GetWindowHandleOfElement(IUIAutomationElement* element) const = 0;
    virtual HRESULT CompareElements(
       IUIAutomationElement* element1,
       IUIAutomationElement* element2,
       BOOL* areSame
    ) const = 0;
};

export class Automation :public IAutomation
{
public:
    Automation(const winrt::com_ptr<IUIAutomation>& automation) :
        automation(automation),
        focusChangedHandler(winrt::make_self<FocusChangedHandler>())
    {
    }

    static std::expected<std::shared_ptr<Automation>, HRESULT> Create()
    {
        HRESULT hr = S_OK;
        winrt::com_ptr<IUIAutomation> automation;

        RETURN_UNEXPECTED_IF_FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED), hr);
        RETURN_UNEXPECTED_IF_FAILED(
            CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(automation.put())),
            hr
        );

        return std::make_shared<Automation>(automation);
    }

    HRESULT RegisterFocusChangedEvent(FocusChangedCallback callback) const override
    {
        focusChangedHandler->SetFocusChangedCallback(callback);
        return automation->AddFocusChangedEventHandler(nullptr, focusChangedHandler.get());
    }

    HRESULT UnregisterFocusChangedEvent() const override
    {
        return automation->RemoveFocusChangedEventHandler(focusChangedHandler.get());
    }

    HWND GetWindowHandleOfElement(IUIAutomationElement* element) const override
    {
        HWND handle = nullptr;
        IUIAutomationElement* currentElement = element;
        winrt::com_ptr<IUIAutomationTreeWalker> walker;

        while (currentElement != nullptr)
        {
            auto hr = currentElement->get_CurrentNativeWindowHandle(reinterpret_cast<UIA_HWND*>(&handle));
            if (SUCCEEDED(hr) && handle != nullptr)
                return handle;

            if (walker == nullptr)
                automation->get_ControlViewWalker(walker.put());

            IUIAutomationElement* parentElement = nullptr;
            currentElement = SUCCEEDED(walker->GetParentElement(currentElement, &parentElement))
                ? parentElement
                : nullptr;
        }

        return nullptr;
    }

    HRESULT CompareElements(
        IUIAutomationElement* element1,
        IUIAutomationElement* element2,
        BOOL* areSame) const override
    {
        return automation->CompareElements(element1, element2, areSame);
    }

private:
    winrt::com_ptr<IUIAutomation> automation;
    winrt::com_ptr<FocusChangedHandler> focusChangedHandler;
};

export class AutomationOperation
{
public:
    AutomationOperation() = delete;

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

    static HRESULT HasKeyboardFocus(IUIAutomationElement* element, bool& value)
    {
        BOOL result = FALSE;
        auto hr = element->get_CurrentHasKeyboardFocus(&result);
        value = result == TRUE;
        return hr;
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