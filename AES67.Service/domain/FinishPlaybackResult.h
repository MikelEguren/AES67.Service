#pragma once

#include "FinishPlaybackResult.g.h"

namespace winrt::AES67Service::implementation
{
    struct FinishPlaybackResult : FinishPlaybackResultT<FinishPlaybackResult>
    {
        FinishPlaybackResult() 
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::AES67Service::factory_implementation
{
    struct FinishPlaybackResult : FinishPlaybackResultT<FinishPlaybackResult, implementation::FinishPlaybackResult>
    {
    };
}
