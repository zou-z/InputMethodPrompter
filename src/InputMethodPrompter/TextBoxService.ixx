module;
#include <UIAutomation.h>
export module TextBoxService;

import std;
import AutomationComponent;

export enum class TextBoxType
{
    Unknown,
    Normal,
    WebPageContent,
    VisualStudioEditor,
    VisualStudioCodeEditor,
    SublimeTextEditor,
    ZedEditor,
};

export struct ITextBoxRecognizer
{
    virtual ~ITextBoxRecognizer() = default;
    virtual TextBoxType GetTextBoxType(IUIAutomationElement* element) = 0;
};

export struct ITextBoxLocator
{
    virtual ~ITextBoxLocator() = default;
    virtual std::expected<std::optional<RECT>, HRESULT> GetPosition(
        IAutomation* automation,
        IUIAutomationElement* element,
        TextBoxType type
    ) = 0;
};

struct ElementProperties
{
    std::wstring Name;
    std::wstring ClassName;
    std::wstring FrameworkId;
};

struct TextBoxBase
{
    virtual ~TextBoxBase() = default;
    virtual bool CheckTextBoxType(IUIAutomationElement* element, const ElementProperties& properties) = 0;
    virtual std::expected<RECT, HRESULT> GetPosition(IAutomation* automation, IUIAutomationElement* element)
    {
        RECT position{};
        auto hr = element->get_CurrentBoundingRectangle(&position);
        return SUCCEEDED(hr) ? std::expected<RECT, HRESULT>(position) : std::unexpected(hr);
    }
};

struct NormalTextBox :TextBoxBase
{
    bool CheckTextBoxType(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return AutomationOperation::CheckIsInputControl(element);
    }
};

struct WebPageContentTextBox :TextBoxBase
{
    bool CheckTextBoxType(IUIAutomationElement* element, const ElementProperties& properties) override
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

struct VisualStudioEditorTextBox :TextBoxBase
{
    bool CheckTextBoxType(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return properties.Name == L"Text Editor"
            && properties.ClassName == L"WpfTextView"
            && properties.FrameworkId == L"WPF";
    }

    std::expected<RECT, HRESULT> GetPosition(IAutomation* automation, IUIAutomationElement* element) override
    {
        return automation->GetParentElement(element)
            .and_then([&](const auto& parentElement) -> std::expected<RECT, HRESULT>
            {
                return parentElement
                    ? TextBoxBase::GetPosition(automation, parentElement.get())
                    : std::unexpected(E_POINTER);
            });
    }
};

struct VisualStudioCodeEditorTextBox :TextBoxBase
{
    bool CheckTextBoxType(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return properties.ClassName == L"native-edit-context"
            && properties.FrameworkId == L"Chrome";
    }

    std::expected<RECT, HRESULT> GetPosition(IAutomation* automation, IUIAutomationElement* element) override
    {
        return automation->GetParentElement(element)
            .and_then([&](const auto& parentElement) -> std::expected<RECT, HRESULT>
            {
                return parentElement
                    ? TextBoxBase::GetPosition(automation, parentElement.get())
                    : std::unexpected(E_POINTER);
            });
    }
};

struct SublimeTextEditorTextBox :TextBoxBase
{
    bool CheckTextBoxType(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return properties.ClassName == L"PX_WINDOW_CLASS"
            && properties.FrameworkId == L"Win32";
    }
};

struct ZedEditorTextBox :TextBoxBase
{
    bool CheckTextBoxType(IUIAutomationElement* element, const ElementProperties& properties) override
    {
        return properties.ClassName == L"Zed::Window"
            && properties.FrameworkId == L"Win32";
    }
};

export class TextBoxService :public ITextBoxRecognizer, public ITextBoxLocator
{
public:
    TextBoxService()
    {
        textBoxs[TextBoxType::WebPageContent] = std::make_unique<WebPageContentTextBox>();
        textBoxs[TextBoxType::VisualStudioEditor] = std::make_unique<VisualStudioEditorTextBox>();
        textBoxs[TextBoxType::VisualStudioCodeEditor] = std::make_unique<VisualStudioCodeEditorTextBox>();
        textBoxs[TextBoxType::SublimeTextEditor] = std::make_unique<SublimeTextEditorTextBox>();
        textBoxs[TextBoxType::ZedEditor] = std::make_unique<ZedEditorTextBox>();
        textBoxs[TextBoxType::Normal] = std::make_unique<NormalTextBox>();
    }

    TextBoxType GetTextBoxType(IUIAutomationElement* element) override
    {
        ElementProperties properties
        {
            .Name = AutomationOperation::GetElementName(element),
            .ClassName = AutomationOperation::GetElementClassName(element),
            .FrameworkId = AutomationOperation::GetElementFrameworkId(element)
        };

        for (const auto& [type, textBox] : textBoxs)
        {
            if (textBox->CheckTextBoxType(element, properties))
                return type;
        }

        return TextBoxType::Unknown;
    }

    std::expected<std::optional<RECT>, HRESULT> GetPosition(
        IAutomation* automation,
        IUIAutomationElement* element,
        TextBoxType textBoxType) override
    {
        for (const auto& [type, textBox] : textBoxs)
        {
            if (type == textBoxType)
                return textBox->GetPosition(automation, element);
        }

        return std::nullopt;
    }

private:
    std::unordered_map<TextBoxType, std::unique_ptr<TextBoxBase>> textBoxs;
};