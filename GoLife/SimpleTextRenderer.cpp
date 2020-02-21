#include "pch.h"
#include "SimpleTextRenderer.h"
#include "GoLBasic.h"
#include <ppltasks.h>

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Core;

SimpleTextRenderer::SimpleTextRenderer() :
	m_backgroundColor(0x6495ED),//ColorF::SaddleBrown),//
	m_textPosition(0.0f, 0.0f),
	m_UniverseSize(2048.0f, 2048.0f)
{
	m_pGoL = new GoLBasic();
	m_pGoL->Initialize(2048, 2048);

	m_GolPeriod = FLT_MAX;
	m_GolCounter = m_GolPeriod;
	m_renderNeeded = true;

	m_bEcho = true;
	m_bGrid = false;
	m_Scale = 1.0f;

	m_FPSIndex = 0;
	m_FPSSum = 0;
	memset(m_pFPS, 0, sizeof(m_pFPS));

	m_SimFreqIndex = 0;
	m_SimFreqSum = 0;
	memset(m_pSimFreq, 0, sizeof(m_pSimFreq));
}

void SimpleTextRenderer::CreateDeviceIndependentResources()
{
	DirectXBase::CreateDeviceIndependentResources();

	DX::ThrowIfFailed(
		m_dwriteFactory->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_MEDIUM,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			16.0f,
			L"en-US",
			&m_textFormat
			)
		);

	DX::ThrowIfFailed(	m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING)	);
}

void SimpleTextRenderer::CreateDeviceResources()
{
	DirectXBase::CreateDeviceResources();

	DX::ThrowIfFailed(	m_d2dContext->CreateSolidColorBrush(ColorF(0xd5d5e5, 1.0f),	&m_gridBrush)	);
	DX::ThrowIfFailed(	m_d2dContext->CreateSolidColorBrush(ColorF(0x334157, 1.0f),	&m_cellBrush)	);
	DX::ThrowIfFailed(	m_d2dContext->CreateSolidColorBrush(ColorF(0xd5d5e5, 0.8f),	&m_infoBrush)	);

	ID2D1GradientStopCollection *pGradientStops = NULL;

	//D2D1_GRADIENT_STOP gradientStops[] = {
	//	{ 0.00f, ColorF(ColorF::Red, 0.0f) },
	//	{ 0.25f, ColorF(ColorF::Red, 0.5f) },
	//	{ 0.50f, ColorF(ColorF::Blue, 1.0f) },
	//	{ 1.00f, ColorF(ColorF::ForestGreen, 1.0f) },
	//};

	D2D1_GRADIENT_STOP gradientStops[] = {
		{ 0.00f, ColorF(ColorF::SaddleBrown, 0.2f) },
		{ 0.25f, ColorF(ColorF::SaddleBrown, 1.0f) },
		{ 0.33f, ColorF(ColorF::Olive, 1.0f) },
		{ 0.50f, ColorF(ColorF::Green, 1.0f) },
		{ 0.50f, ColorF(ColorF::YellowGreen, 1.0f) },
		{ 1.00f, ColorF(ColorF::White, 1.0f) },
	};
	DX::ThrowIfFailed( m_d2dContext->CreateGradientStopCollection(gradientStops, _countof(gradientStops), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStops));
	DX::ThrowIfFailed( m_d2dContext->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties( D2D1::Point2F(0, 0), D2D1::Point2F(256, 256)), pGradientStops, &m_cellGradient));

	DX::ThrowIfFailed( m_d2dContext->CreateBitmap(
		SizeU(m_pGoL->m_Width, m_pGoL->m_Height),
		BitmapProperties(PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
		&m_pBitmap
	));
}

void SimpleTextRenderer::CreateWindowSizeDependentResources()
{
	DirectXBase::CreateWindowSizeDependentResources();

	// Add code to create window size dependent objects here.
}


void SimpleTextRenderer::Update(float /*timeTotal*/, float timeDelta)
{
	m_FPSSum -= m_pFPS[m_FPSIndex];
	m_FPSSum += timeDelta;
	m_pFPS[m_FPSIndex] = timeDelta;
	if (++m_FPSIndex == _countof(m_pFPS)) m_FPSIndex = 0;

	m_FPS = (uint32)(1.0f / (m_FPSSum / _countof(m_pFPS)) + 0.5f);

	if (m_pGoL->m_actualRunFreq != UINT32_MAX)
	{
		m_SimFreqSum -= m_pSimFreq[m_SimFreqIndex];
		m_SimFreqSum += 1.0f / m_pGoL->m_actualRunFreq;
		m_pSimFreq[m_SimFreqIndex] = 1.0f / m_pGoL->m_actualRunFreq;
		if (++m_SimFreqIndex == _countof(m_pSimFreq)) m_SimFreqIndex = 0;
	}

	m_SimFreq = (uint32)(1.0f / (m_SimFreqSum / _countof(m_pSimFreq)) + 0.5f);
}

void SimpleTextRenderer::GoLStep()
{
	//m_GolCounter = 0;
	m_pGoL->Step();
	m_renderNeeded = true;

}

void SimpleTextRenderer::GoLRun()
{
	//m_GolPeriod = 0.05f;
	//m_GolCounter = 0;
	m_DrawingTasks.run([this]() { m_pGoL->Run(); });
}

void SimpleTextRenderer::GoLStop(bool bWait)
{
	//m_GolPeriod = FLT_MAX;
	//m_GolCounter = 0;
	m_pGoL->Stop();
	if (bWait)
		m_DrawingTasks.wait();
}

void SimpleTextRenderer::GoLRandomize(uint32 seed)
{
	if (m_pGoL->m_bRunning)
	{
		GoLStop(true);
		m_pGoL->Randomize(seed);
		GoLRun();
	}
	else
	{
		m_pGoL->Randomize(seed);
		//m_pGoL->Glider();
	}

	m_renderNeeded = true;
}

void SimpleTextRenderer::GoLLoad(String^ fileContent, String^ fileExtension)
{
	GoLFileFormats format = GoLFileFormats::Unknown;
	if (_wcsnicmp(fileExtension->Data(), L".rle", 4)==0) format = GoLFileFormats::RLE;
	else if (_wcsnicmp(fileExtension->Data(), L".lif", 4)==0) format = GoLFileFormats::LIFE;
	else if (_wcsnicmp(fileExtension->Data(), L".mcl", 4)==0) format = GoLFileFormats::MCELL;

	if (m_pGoL->m_bRunning)
	{
		GoLStop(true);
		m_pGoL->Load(fileContent->Data(), format);
		GoLRun();
	}
	else
	{
		m_pGoL->Load(fileContent->Data(),format);
	}

	m_renderNeeded = true;
}

void SimpleTextRenderer::GoLFaster()
{
	//if (m_pGoL->m_requiredRunFreq < m_pGoL->m_actualRunFreq+5)
		m_pGoL->m_requiredRunFreq += 5;
}

void SimpleTextRenderer::GoLSlower()
{
	if (m_pGoL->m_requiredRunFreq > 5)
		m_pGoL->m_requiredRunFreq -= 5;
}


void SimpleTextRenderer::Render()
{
	if (!m_renderNeeded) return;

	float32 invScale = 1.0f / m_Scale;

	m_d2dContext->BeginDraw();

	m_d2dContext->Clear(m_backgroundColor);

	m_d2dContext->SetTransform(Matrix3x2F::Identity());

	{
		const wchar_t fmt[] = L"FPS: %u\nRSF: %u Hz\nASF: %u Hz\nGen: %u\nDen: %u%%";
		char16 tmp[_countof(fmt) + 4*(10-2)];
		_snwprintf(tmp, _countof(tmp), fmt, m_FPS, m_pGoL->m_requiredRunFreq, m_SimFreq, m_pGoL->m_nbGenerations, (uint32)(m_pGoL->m_density*100.0f + 0.5f));
		Platform::String^ text = ref new Platform::String(tmp);

		m_d2dContext->DrawText(text->Data(), text->Length(),m_textFormat.Get(), RectF(120.5f, 140.5f, 240.0f, 260.0f), m_infoBrush.Get());
	}

	D2D1_RECT_F clipRect = RectF(120.5f, 140.5f, m_windowBounds.Width-40.5f, m_windowBounds.Height-40.5f);
	m_d2dContext->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

	{
		Matrix3x2F screenCenter = Matrix3x2F::Translation(
			120.5f + (m_windowBounds.Width-160.0f) / 2.0f + m_textPosition.X,
			140.5f + (m_windowBounds.Height-180.0f) / 2.0f + m_textPosition.Y
		);
	
		Matrix3x2F scale = Matrix3x2F::Scale(m_Scale, m_Scale);
	
		Matrix3x2F univCenter = Matrix3x2F::Translation(
			- (m_pGoL->m_Width / 2.0f),
			- (m_pGoL->m_Height / 2.0f)
		);
	
		m_d2dContext->SetTransform(univCenter * scale * screenCenter * m_orientationTransform2D);
	}

	{
		unsigned char* pBitmap = NULL;
		if (try_receive(m_pGoL->m_Bitmaps, pBitmap) && pBitmap)
		{
			m_pBitmap->CopyFromMemory(NULL, pBitmap, m_pGoL->m_Width*4);
		}
		else if (m_pGoL->m_BufferUpdated)
		{
			concurrency::create_task([this]() { m_pGoL->BuildMap(); });
			//m_DrawingTasks.run([this]() { m_pGoL->BuildMap(); });
		}
		m_d2dContext->DrawBitmap(m_pBitmap.Get(), RectF(0.0f,0.0f, (float)m_pGoL->m_Width,(float)m_pGoL->m_Height), 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

		if (m_bGrid)
		{
			for (float32 col = 0; col <= m_pGoL->m_Width; col++)
			{
				D2D1_POINT_2F point0 = { col, 0.0f };
				D2D1_POINT_2F point1 = { col, (float)m_pGoL->m_Height };
				m_d2dContext->DrawLine(point0, point1, m_gridBrush.Get(), invScale);
			}
			for (float32 row = 0; row <= m_pGoL->m_Height; row++)
			{
				D2D1_POINT_2F point0 = { 0.0f, row };
				D2D1_POINT_2F point1 = { (float)m_pGoL->m_Width, row };
				m_d2dContext->DrawLine(point0, point1, m_gridBrush.Get(), invScale);
			}

		}
	}

	m_d2dContext->PopAxisAlignedClip();

	// Ignore D2DERR_RECREATE_TARGET. This error indicates that the device
	// is lost. It will be handled during the next call to Present.
	HRESULT hr = m_d2dContext->EndDraw();
	if (hr != D2DERR_RECREATE_TARGET)
	{
		DX::ThrowIfFailed(hr);
	}

	//m_renderNeeded = false;
}

void SimpleTextRenderer::UpdateTextPosition(Point deltaTextPosition)
{
	m_textPosition.X += deltaTextPosition.X;
	m_textPosition.Y += deltaTextPosition.Y;
}

void SimpleTextRenderer::BackgroundColorNext()
{
}

void SimpleTextRenderer::BackgroundColorPrevious()
{
}

void SimpleTextRenderer::SaveInternalState(IPropertySet^ state)
{
	if (state->HasKey("m_backgroundColorIndex"))
	{
		state->Remove("m_backgroundColorIndex");
	}
	if (state->HasKey("m_textPosition"))
	{
		state->Remove("m_textPosition");
	}
	state->Insert("m_textPosition", PropertyValue::CreatePoint(m_textPosition));
}

void SimpleTextRenderer::LoadInternalState(IPropertySet^ state)
{
	if (state->HasKey("m_textPosition"))
	{
		m_textPosition = safe_cast<IPropertyValue^>(state->Lookup("m_textPosition"))->GetPoint();
	}
}
