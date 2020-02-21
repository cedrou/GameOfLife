//
// DirectXPage.xaml.cpp
// Implementation of the DirectXPage.xaml class.
//

#include "pch.h"
#include "DirectXPage.xaml.h"
#include "GoLBasic.h"
#include <time.h>
#include "ppltasks.h"
#include "agents.h"

using namespace Direct2DApp1;

using namespace Concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Input;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

DirectXPage::DirectXPage() :
	m_renderNeeded(true),
	m_lastPointValid(false)
{
	InitializeComponent();

	m_renderer = ref new SimpleTextRenderer();

	m_renderer->Initialize(
		Window::Current->CoreWindow,
		SwapChainPanel,
		DisplayProperties::LogicalDpi
		);
	EchoCheck->IsChecked = m_renderer->m_bEcho;
	GridCheck->IsChecked = m_renderer->m_bGrid;


	Window::Current->CoreWindow->SizeChanged += 
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &DirectXPage::OnWindowSizeChanged);

	DisplayProperties::LogicalDpiChanged +=
		ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnLogicalDpiChanged);

	DisplayProperties::OrientationChanged +=
        ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnOrientationChanged);

	DisplayProperties::DisplayContentsInvalidated +=
		ref new DisplayPropertiesEventHandler(this, &DirectXPage::OnDisplayContentsInvalidated);
	
	m_eventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &DirectXPage::OnRendering));

	m_timer = ref new BasicTimer();
}


void DirectXPage::OnPointerMoved(Object^ sender, PointerRoutedEventArgs^ args)
{
	auto currentPoint = args->GetCurrentPoint(nullptr);
	if (currentPoint->IsInContact)
	{
		if (m_lastPointValid)
		{
			Windows::Foundation::Point delta(
				currentPoint->Position.X - m_lastPoint.X,
				currentPoint->Position.Y - m_lastPoint.Y
				);
			m_renderer->UpdateTextPosition(delta);
			m_renderNeeded = true;
		}
		m_lastPoint = currentPoint->Position;
		m_lastPointValid = true;
	}
	else
	{
		m_lastPointValid = false;
	}
}

void DirectXPage::OnPointerReleased(Object^ sender, PointerRoutedEventArgs^ args)
{
	m_lastPointValid = false;
}

void DirectXPage::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_renderer->UpdateForWindowSizeChange();
	m_renderNeeded = true;
}

void DirectXPage::OnLogicalDpiChanged(Object^ sender)
{
	m_renderer->SetDpi(DisplayProperties::LogicalDpi);
	m_renderNeeded = true;
}

void DirectXPage::OnOrientationChanged(Object^ sender)
{
	m_renderer->UpdateForWindowSizeChange();
	m_renderNeeded = true;
}

void DirectXPage::OnDisplayContentsInvalidated(Object^ sender)
{
	m_renderer->ValidateDevice();
	m_renderNeeded = true;
}

void DirectXPage::OnRendering(Object^ sender, Object^ args)
{
	//if (m_renderNeeded)
	{
		m_timer->Update();
		m_renderer->Update(m_timer->Total, m_timer->Delta);
		m_renderer->Render();
		m_renderer->Present();
		m_renderNeeded = false;
	}
}

void DirectXPage::OnPreviousColorPressed(Object^ sender, RoutedEventArgs^ args)
{
	m_renderer->BackgroundColorPrevious();
	m_renderNeeded = true;
}

void DirectXPage::OnNextColorPressed(Object^ sender, RoutedEventArgs^ args)
{
	m_renderer->BackgroundColorNext();
	m_renderNeeded = true;
}

void DirectXPage::SaveInternalState(IPropertySet^ state)
{
	m_renderer->SaveInternalState(state);
}

void DirectXPage::LoadInternalState(IPropertySet^ state)
{
	m_renderer->LoadInternalState(state);
}


void Direct2DApp1::DirectXPage::ButtonRandomClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->GoLRandomize((uint32)time(NULL));
}

void Direct2DApp1::DirectXPage::ButtonLoadClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	FileOpenPicker^ openPicker = ref new FileOpenPicker();
	openPicker->SuggestedStartLocation = PickerLocationId::Downloads;
	openPicker->FileTypeFilter->Append(".lif");
	openPicker->FileTypeFilter->Append(".life");
	openPicker->FileTypeFilter->Append(".mcl");
	openPicker->FileTypeFilter->Append(".rle");

	create_task(openPicker->PickSingleFileAsync())
	
	.then([this] (StorageFile^ file)
	{
		if (file == nullptr) cancel_current_task();

		String^ ext = file->FileType;

		create_task(FileIO::ReadTextAsync(file))

		.then([this, ext](String^ fileContent)
		{
			m_renderer->GoLLoad(fileContent, ext);
		});
	});
	
}


void Direct2DApp1::DirectXPage::ButtonStepClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->GoLStop(true);
	m_btnRun->Visibility = Windows::UI::Xaml::Visibility::Visible;
	m_btnStop->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	m_renderer->GoLStep();
}


void Direct2DApp1::DirectXPage::ButtonStopClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->GoLStop(false);
	m_btnRun->Visibility = Windows::UI::Xaml::Visibility::Visible;
	m_btnStop->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}


void Direct2DApp1::DirectXPage::ButtonRunClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->GoLRun();
	m_btnRun->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
	m_btnStop->Visibility = Windows::UI::Xaml::Visibility::Visible;
}


void Direct2DApp1::DirectXPage::GoBack(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{

}


void Direct2DApp1::DirectXPage::EchoCheck_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->m_bEcho = EchoCheck->IsChecked->Value;
}


void Direct2DApp1::DirectXPage::GridCheck_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->m_bGrid = GridCheck->IsChecked->Value;
}


void Direct2DApp1::DirectXPage::ButtonZoomPlusClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->m_Scale *= sqrtf(2);
}


void Direct2DApp1::DirectXPage::ButtonZoomMinusClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->m_Scale /= sqrtf(2);
}


void Direct2DApp1::DirectXPage::ButtonSlowerClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->GoLSlower();
}


void Direct2DApp1::DirectXPage::ButtonFasterClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	m_renderer->GoLFaster();
}


void Direct2DApp1::DirectXPage::Page_PointerWheelChanged_1(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
	int wheelDelta = e->GetCurrentPoint(nullptr)->Properties->MouseWheelDelta;
	if (wheelDelta > 0) m_renderer->m_Scale *= sqrtf(2);
	else m_renderer->m_Scale /= sqrtf(2);
}

