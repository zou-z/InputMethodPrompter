module;
#include <UIAutomation.h>
export module FocusedTextBoxTracker;

import std;
import PromptData;
import LogComponent;
import ReactiveComponent;
import AutomationComponent;
import TextBoxService;

using namespace Reactive;

export class FocusedTextBoxTracker
{
public:
    FocusedTextBoxTracker(
        std::shared_ptr<ILog> log,
        std::shared_ptr<IAutomation> automation,
        std::shared_ptr<ITextBoxRecognizer> textBoxRecognizer,
        std::weak_ptr<IPublisher<std::optional<TextBoxElement>>> focusedTextBox) :
        log(std::move(log)),
        automation(std::move(automation)),
        textBoxRecognizer(std::move(textBoxRecognizer)),
        focusedTextBox(std::move(focusedTextBox))
    {
    }

    HRESULT Start()
    {
        return automation->RegisterFocusChangedEvent([this](IUIAutomationElement* element)
        {
            OnFocusChanged(element);
        });
    }

    HRESULT Stop()
    {
        return automation->UnregisterFocusChangedEvent();
    }

private:
    void OnFocusChanged(IUIAutomationElement* element)
    {
        if (auto textBox = focusedTextBox.lock())
        {
            auto textBoxType = textBoxRecognizer->GetTextBoxType(element);
            if (textBoxType == TextBoxType::Unknown)
            {
                textBox->Set(std::nullopt);
                return;
            }

            auto handle = automation->GetWindowHandleOfElement(element);
            if (handle != nullptr)
            {
                TextBoxElement focusedElement
                {
                    .Type = textBoxType,
                    .WindowHandle = handle
                };
                focusedElement.Value.copy_from(element);
                textBox->Set(focusedElement);
            }
            else
            {
                log->Error("Failed to get window handle of element");
            }
        }
    }

private:
    std::shared_ptr<ILog> log;
    std::shared_ptr<IAutomation> automation;
    std::shared_ptr<ITextBoxRecognizer> textBoxRecognizer;
    std::weak_ptr<IPublisher<std::optional<TextBoxElement>>> focusedTextBox;
};