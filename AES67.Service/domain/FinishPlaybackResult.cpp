#include "pch.h"
#include "FinishPlaybackResult.h"
#if __has_include("FinishPlaybackResult.g.cpp")
#include "FinishPlaybackResult.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::AES67Service::implementation
{
    int32_t FinishPlaybackResult::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void FinishPlaybackResult::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void FinishPlaybackResult::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        Button().Content(box_value(L"Clicked"));
    }
}
