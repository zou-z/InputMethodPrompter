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
            auto currentElement = textBox->Get();
            if (currentElement.has_value() && currentElement->Value)
            {
                BOOL areSame = FALSE;
                auto hr = automation->CompareElements(element, currentElement->Value.get(), &areSame);
                if (FAILED(hr))
                {
                    log->Error("Failed to compare elements, hr: " + HResultToString(hr));
                    return;
                }

                if (areSame == TRUE)
                    return;
            }

            auto textBoxType = textBoxRecognizer->GetTextBoxType(element);
            if (textBoxType == TextBoxType::Unknown)
            {
                if (!textBox->Set(std::nullopt))
                    log->Error("Failed to set text box type");
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
                if (!textBox->Set(focusedElement))
                    log->Error("Failed to set text box type");
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