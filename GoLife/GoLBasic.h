#pragma once

#include <ppl.h>
#include <agents.h>


#define GOL_NB_BUFFERS 3

enum GoLFileFormats
{
	Unknown,
	RLE,
	LIFE,
	MCELL,
};

class GoLBasic
{
public:
	GoLBasic();
	~GoLBasic();

	void Initialize(uint32 width, uint32 height);
	
	void	Step();
	void	Run();
	void	Stop();

	void	Clear();
	void	Randomize(uint32 seed);
	void	Glider();

	bool	Load(const wchar_t* pFileBuffer, GoLFileFormats format);
	bool	LoadRLE(const wchar_t* pFileBuffer);
	bool	LoadLIFE(const wchar_t* pFileBuffer);
	bool	LoadMCELL(const wchar_t* pFileBuffer);


	enum Rules {
		Conway,
		HighLife,
		Diamoeba
	};
	void	SetRules(Rules rules);
	void	SetRules(uint32 Survival, uint32 Birth);

	void	BuildMap();

public:
	uint32	m_Width;
	uint32	m_Height;
	uint32	m_Area;

	bool	m_bCylinder;
	bool	m_bUseParallel;

	concurrency::unbounded_buffer<unsigned char*> m_Bitmaps;
	bool	m_BufferUpdated;

	uint32	m_requiredRunFreq;
	uint32	m_actualRunFreq;
	uint64	m_nbGenerations;
	float	m_density;

	bool	m_bRunning;

private:

	struct 
	{
		bool*							pData;
		uint64							NbCells;
		concurrency::reader_writer_lock	Lock;
	}
	m_Buffers[GOL_NB_BUFFERS];

	uint32*	m_pBitmap;

	char	m_BufferIndex;

	bool	m_pRulesLut[2][9];

	concurrency::event	m_evtStop;
};

