module;
#include <UIAutomation.h>
export module TextBoxService;

import std;
import AutomationComponent;

export enum class TextBoxType
{
    Unknown,
    Normal,
};

export struct ITextBoxRecognizer
{
    virtual ~ITextBoxRecognizer() = default;
    virtual TextBoxType GetTextBoxType(IUIAutomationElement* element) = 0;
};

export struct ITextBoxLocator
{
    virtual ~ITextBoxLocator() = default;
    virtual bool GetPosition(
        IAutomation* automation,
        IUIAutomationElement* element,
        TextBoxType type,
        RECT& position
    ) = 0;
};

export struct ITextBox : ITextBoxRecognizer, ITextBoxLocator
{
    virtual ~ITextBox() = default;
};

class NormalTextBox :public ITextBox
{
public:
    virtual TextBoxType GetTextBoxType(IUIAutomationElement* element) override
    {
        return AutomationOperation::CheckIsInputControl(element) ? TextBoxType::Normal : TextBoxType::Unknown;
    }

    virtual bool GetPosition(
        IAutomation* automation,
        IUIAutomationElement* element,
        TextBoxType type,
        RECT& position) override
    {
        return type == TextBoxType::Normal
            ? SUCCEEDED(element->get_CurrentBoundingRectangle(&position))
            : false;
    }
};



export class TextBoxService :public ITextBox
{
public:
    TextBoxService()
    {
        textBoxs.push_back(std::make_unique<NormalTextBox>());



    }

    TextBoxType GetTextBoxType(IUIAutomationElement* element) override
    {
        for (const auto& textBox : textBoxs)
        {
            auto type = textBox->GetTextBoxType(element);
            if (type != TextBoxType::Unknown)
                return type;
        }

        return TextBoxType::Unknown;
    }

    bool GetPosition(
        IAutomation* automation,
        IUIAutomationElement* element,
        TextBoxType type,
        RECT& position) override
    {
        for (const auto& textBox : textBoxs)
        {
            if (textBox->GetPosition(automation, element, type, position))
                return true;
        }

        return false;
    }

private:
    std::vector<std::unique_ptr<ITextBox>> textBoxs;
};