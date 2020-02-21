#pragma once

#include "DirectXBase.h"
#include <ppl.h>
class GoLBasic;

// This class renders simple text with a colored background.
ref class SimpleTextRenderer sealed : public DirectXBase
{
public:
	SimpleTextRenderer();

	// DirectXBase methods.
	virtual void CreateDeviceIndependentResources() override;
	virtual void CreateDeviceResources() override;
	virtual void CreateWindowSizeDependentResources() override;
	virtual void Render() override;

	// Method for updating time-dependent objects.
	void Update(float timeTotal, float timeDelta);

	void GoLStep();
	void GoLRun();
	void GoLStop(bool bWait);
	void GoLRandomize(uint32 seed);
	void GoLLoad(Platform::String^ fileContent, Platform::String^ fileExtension);
	void GoLFaster();
	void GoLSlower();

	// Method to change the text position based on input events.
	void UpdateTextPosition(Windows::Foundation::Point deltaTextPosition);

	// Methods to adjust the window background color.
	void BackgroundColorNext();
	void BackgroundColorPrevious();

	// Methods to save and load state in response to suspend.
	void SaveInternalState(Windows::Foundation::Collections::IPropertySet^ state);
	void LoadInternalState(Windows::Foundation::Collections::IPropertySet^ state);

public:
	property bool m_bEcho;
	property bool m_bGrid;

	property float32 m_Scale;

private:
	D2D1::ColorF m_backgroundColor;

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_gridBrush;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_cellBrush;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_infoBrush;
	//Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_grayBrush[256];
	Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> m_cellGradient;
	Microsoft::WRL::ComPtr<ID2D1Bitmap> m_pBitmap;

	Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;
	Microsoft::WRL::ComPtr<IDWriteTextLayout> m_textLayout;
	DWRITE_TEXT_METRICS m_textMetrics;

	Windows::Foundation::Point m_textPosition;
	bool m_renderNeeded;

	Windows::Foundation::Size m_UniverseSize;

	GoLBasic* m_pGoL;
	float32   m_GolPeriod;
	float32   m_GolCounter;

	uint32 m_FPS;
	uint32 m_FPSIndex;
	float m_FPSSum;
	float m_pFPS[32];

	uint32 m_SimFreq;
	uint32 m_SimFreqIndex;
	float m_SimFreqSum;
	float m_pSimFreq[32];

	concurrency::task_group m_DrawingTasks;

};
