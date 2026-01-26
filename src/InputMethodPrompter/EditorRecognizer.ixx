module;
#include <winrt/base.h>
#include <UIAutomation.h>
export module EditorRecognizer;

import std;
import UiaWrapper;

struct ElementProperties
{
    std::wstring Name;
    std::wstring ClassName;
    std::wstring FrameworkId;
};


struct EditorRecognizer
{
    virtual ~EditorRecognizer() = default;

    virtual bool IsEditorElement(IUIAutomationElement* element, const ElementProperties& properties) = 0;

    virtual bool GetElementPosition(IUIAutomation* automation, IUIAutomationElement* element, RECT& position)
    {
        return SUCCEEDED(element->get_CurrentBoundingRectangle(&position));
    }
};


struct DefaultEditorRecognizer : EditorRecognizer
{
    bool IsEditorElement(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return UiaWrapper::CheckIsInputControl(element);
    }
};


struct WebPageContentEditableEditorRecognizer : EditorRecognizer
{
    bool IsEditorElement(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        if (properties.FrameworkId != L"Chrome")
            return false;

        if (properties.ClassName == L"EdgePopupRowContentView")
            return false;

        BOOL hasKeyboardFocus = FALSE;
        if (FAILED(element->get_CurrentHasKeyboardFocus(&hasKeyboardFocus)) || !hasKeyboardFocus)
            return false;

        CONTROLTYPEID controlType = 0;
        if (FAILED(element->get_CurrentControlType(&controlType)) || controlType != UIA_GroupControlTypeId)
            return false;

        return true;
    }
};


struct VisualStudioEditorRecognizer : EditorRecognizer
{
    bool IsEditorElement(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return properties.Name == L"Text Editor"
            && properties.ClassName == L"WpfTextView"
            && properties.FrameworkId == L"WPF";
    }

    bool GetElementPosition(IUIAutomation* automation, IUIAutomationElement* element, RECT& position) override
    {
        winrt::com_ptr<IUIAutomationTreeWalker> walker;
        auto hr = automation->get_ControlViewWalker(walker.put());
        if (FAILED(hr))
            return false;

        winrt::com_ptr<IUIAutomationElement> parentElement;
        hr = walker->GetParentElement(element, parentElement.put());
        if (FAILED(hr) || !parentElement)
            return false;

        return EditorRecognizer::GetElementPosition(automation, parentElement.get(), position);
    }
};


struct VisualStudioCodeEditorRecognizer : EditorRecognizer
{
    bool IsEditorElement(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return properties.ClassName == L"native-edit-context"
            && properties.FrameworkId == L"Chrome";
    }

    bool GetElementPosition(IUIAutomation* automation, IUIAutomationElement* element, RECT& position) override
    {
        winrt::com_ptr<IUIAutomationTreeWalker> walker;
        auto hr = automation->get_ControlViewWalker(walker.put());
        if (FAILED(hr))
            return false;

        winrt::com_ptr<IUIAutomationElement> parentElement;
        hr = walker->GetParentElement(element, parentElement.put());
        if (FAILED(hr) || !parentElement)
            return false;

        return EditorRecognizer::GetElementPosition(automation, parentElement.get(), position);
    }
};


struct SublimeTextEditorRecognizer : EditorRecognizer
{
    bool IsEditorElement(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return properties.ClassName == L"PX_WINDOW_CLASS"
            && properties.FrameworkId == L"Win32";
    }
};


struct ZedEditorRecognizer : EditorRecognizer
{
    bool IsEditorElement(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return properties.ClassName == L"Zed::Window"
            && properties.FrameworkId == L"Win32";
    }
};


export class EditorRecognizerService
{
public:
    EditorRecognizerService()
    {
        recognizers.push_back(std::make_unique<VisualStudioEditorRecognizer>());
        recognizers.push_back(std::make_unique<VisualStudioCodeEditorRecognizer>());
        recognizers.push_back(std::make_unique<SublimeTextEditorRecognizer>());
        recognizers.push_back(std::make_unique<ZedEditorRecognizer>());
        recognizers.push_back(std::make_unique<WebPageContentEditableEditorRecognizer>());
        recognizers.push_back(std::make_unique<DefaultEditorRecognizer>());
    }

    bool GetEditorPosition(IUIAutomation* automation, IUIAutomationElement* element, RECT& position)
    {
        ElementProperties properties
        {
            .Name = UiaWrapper::GetElementName(element),
            .ClassName = UiaWrapper::GetElementClassName(element),
            .FrameworkId = UiaWrapper::GetElementFrameworkId(element)
        };

        for (const auto& recognizer : recognizers)
        {
            if (recognizer->IsEditorElement(element, properties))
                return recognizer->GetElementPosition(automation, element, position);
        }

        return false;
    }

private:
    std::vector<std::unique_ptr<EditorRecognizer>> recognizers;
};
