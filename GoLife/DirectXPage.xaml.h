//
// BlankPage.xaml.h
// Declaration of the BlankPage.xaml class.
//

#pragma once

#include "DirectXPage.g.h"
#include "SimpleTextRenderer.h"
#include "BasicTimer.h"

namespace Direct2DApp1
{
	/// <summary>
	/// A DirectX page that can be used on its own.  Note that it may not be used within a Frame.
	/// </summary>
    [Windows::Foundation::Metadata::WebHostHidden]
	public ref class DirectXPage sealed
	{
	public:
		DirectXPage();

		void OnPreviousColorPressed(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnNextColorPressed(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		void SaveInternalState(Windows::Foundation::Collections::IPropertySet^ state);
		void LoadInternalState(Windows::Foundation::Collections::IPropertySet^ state);

	private:
		void OnPointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args);
		void OnPointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ args);
		void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
		void OnLogicalDpiChanged(Platform::Object^ sender);
		void OnOrientationChanged(Platform::Object^ sender);
		void OnDisplayContentsInvalidated(Platform::Object^ sender);
		void OnRendering(Object^ sender, Object^ args);

		Windows::Foundation::EventRegistrationToken m_eventToken;

		SimpleTextRenderer^ m_renderer;
		bool m_renderNeeded;


		Windows::Foundation::Point m_lastPoint;
		bool m_lastPointValid;
		
		BasicTimer^ m_timer;
		void Button_Click_1(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Button_Click_2(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonRandomClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonStepClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonStopClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonRunClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void GoBack(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void EchoCheck_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void GridCheck_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonZoomPlusClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonZoomMinusClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonSlowerClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void ButtonFasterClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Page_PointerWheelChanged_1(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
		void ButtonLoadClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
