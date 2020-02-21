#include "pch.h"
#include "GoLBasic.h"

#include <stdlib.h>
#include <string.h>
#include <ppl.h>
#include <chrono>

using namespace Platform;


GoLBasic::GoLBasic()
{
	m_Width = 0;
	m_Height = 0;
	m_Area = 0;

	for (uint32 i = 0; i < GOL_NB_BUFFERS; i++)
	{
		m_Buffers[i].pData = NULL;
		m_Buffers[i].NbCells = 0;
	}
	m_pBitmap = 0;
	m_BufferIndex = 0;
	m_bCylinder = true;
	m_bUseParallel = true;
	m_requiredRunFreq = 240;
	m_actualRunFreq = 0;
	m_nbGenerations = 0;
	m_bRunning = false;
}


GoLBasic::~GoLBasic()
{
	for (uint32 i = 0; i < GOL_NB_BUFFERS; i++)
	{
		delete[] m_Buffers[i].pData;
		m_Buffers[i].pData = NULL;
		m_Buffers[i].NbCells = 0;
	}

	delete[] m_pBitmap;
}

void GoLBasic::Initialize(uint32 width, uint32 height)
{
	uint32 area = width * height;

	for (uint32 i = 0; i < GOL_NB_BUFFERS; i++)
	{
		delete[] m_Buffers[i].pData; m_Buffers[i].pData = NULL;
		m_Buffers[i].NbCells = 0;
	}
	delete[] m_pBitmap; m_pBitmap = NULL;

	for (uint32 i = 0; i < GOL_NB_BUFFERS; i++)
	{
		m_Buffers[i].pData = new bool[area];
		memset(m_Buffers[i].pData, 0, area * sizeof(bool));
	}
	m_pBitmap = new uint32[area];

	m_BufferIndex = 0;

	m_Width = width;
	m_Height = height;
	m_Area = area;

	m_BufferUpdated = true;

	SetRules(Conway);

	m_actualRunFreq = UINT32_MAX;
	m_bRunning = false;
}


void GoLBasic::SetRules(Rules rules)
{
	memset(m_pRulesLut, 0, sizeof(m_pRulesLut));

	switch(rules)
	{
	case HighLife:	SetRules(23,36); break;
	case Diamoeba:	SetRules(5678,35678); break;
	case Conway:	
	default:		SetRules(23,3); break;
	}
}

void GoLBasic::SetRules(uint32 Survival, uint32 Birth)
{
	memset(m_pRulesLut, 0, sizeof(m_pRulesLut));

	while (Birth)
	{
		uint32 a = Birth;
		Birth /= 10;
		
		m_pRulesLut[0][a-Birth*10] = true;
	}

	while (Survival)
	{
		uint32 a = Survival;
		Survival /= 10;
		
		m_pRulesLut[1][a-Survival*10] = true;
	}
}

void GoLBasic::Clear()
{
	if (m_Area == 0)
		return;

	concurrency::reader_writer_lock::scoped_lock(m_Buffers[m_BufferIndex].Lock);
	bool* pDstBuffer = m_Buffers[m_BufferIndex].pData;
	memset(pDstBuffer, 0, m_Width * m_Height * sizeof(bool));

	m_BufferUpdated = true;
	m_nbGenerations = 0;
}

static float Sigmoid(float x)
{
	static float cache[1024] = {0};
	if (x < -16.0f) return 0.0f;
	if (x > 16.0f) return 1.0f;
	if (cache[_countof(cache)-1]==0)
	{
		for (uint32 i = 0; i < _countof(cache); i++)
		{
			float a = -16.0f + 32.0f * i / _countof(cache);
			cache[i] = 1.0f - 1.0f / (1.0f + expf(a));
		}
	}

	uint32 index = (uint32)((x + 16.0f) * _countof(cache) / 32.0f + 0.5f);
	return cache[index];
}

void GoLBasic::Randomize(uint32 seed)
{
	Clear();

	srand(seed);

	concurrency::reader_writer_lock::scoped_lock(m_Buffers[m_BufferIndex].Lock);
	bool* pMap = m_Buffers[m_BufferIndex].pData;
	//for (uint32 i = 0; i < m_Width * m_Height; i++)
	//{
	//	pMap[i] = rand() < RAND_MAX / 2;
	//}

	uint32 min = min(m_Width, m_Height);

	for (uint32 y = 0; y < m_Height; y++)
		for (uint32 x = 0; x < m_Width; x++)
		{
			const static float r2Max = (float)(min*min) / 4.0f; //20000.0f - 50000.0f * logf(1/(RAND_MAX/2-1));
			const static float Z = logf(1.0f/(RAND_MAX/2-1));
			const static float B = r2Max / (0.4f - Z);
			const static float A = 2*B/5;

			float r2 = (float)((x - m_Width/2)*(x - m_Width/2) + (y - m_Height/2)*(y - m_Height/2));
			if (r2 > r2Max) continue;

			float p = Sigmoid((A-r2)/B);
			pMap[y*m_Width + x] = rand() < (int32)(RAND_MAX * p) / 2;
		}

	m_BufferUpdated = true;
	m_nbGenerations = 0;
}

void GoLBasic::Glider()
{
	Clear();

	concurrency::reader_writer_lock::scoped_lock(m_Buffers[m_BufferIndex].Lock);
	bool* pMap = m_Buffers[m_BufferIndex].pData;

	pMap[          0] = false; pMap[          1] = true;  pMap[          2] = false;
	pMap[  m_Width+0] = false; pMap[  m_Width+1] = false; pMap[  m_Width+2] = true;
	pMap[2*m_Width+0] = true;  pMap[2*m_Width+1] = true;  pMap[2*m_Width+2] = true;

	m_BufferUpdated = true;
	m_nbGenerations = 0;
}

bool GoLBasic::Load(const wchar_t* pFileBuffer, GoLFileFormats format)
{
	switch (format)
	{
	case LIFE: return LoadLIFE(pFileBuffer);
	case MCELL: return LoadMCELL(pFileBuffer);
	case RLE: return LoadRLE(pFileBuffer);
	}

	if (wcsncmp(pFileBuffer, L"#Life", 5) == 0)
	{
		return LoadLIFE(pFileBuffer);
	}
	else if (wcsncmp(pFileBuffer, L"#MCell", 5) == 0)
	{
		return LoadMCELL(pFileBuffer);
	}
	else
	{ 
		return LoadRLE(pFileBuffer);
	}
}

bool GoLBasic::LoadRLE(const wchar_t* pFileBuffer)
{
	const wchar_t* pCurrent = pFileBuffer;
	uint32 survivalRule = 23, birthRule = 3;
	
	// Consume pre-header lines
	while (*pCurrent == '#') 
	{
		pCurrent++;

		switch (*pCurrent++)
		{
		case 'r': 
			if (swscanf(pCurrent, L" %u / %u", &survivalRule, &birthRule) != 2)
				return false;
			break;

		case 'C': case 'c': // comment
		case 'O':			// author
		case 'R': case 'P':	// top-left coords
		default:
			break;
		}

		// goto next line
		while (*pCurrent++ != '\n');
	}

	// Consume header
	uint32 x = 0, y = 0;
	int offset = 0;
	if (swscanf(pCurrent, L"x = %u , y = %u%n", &x, &y, &offset) != 2)
		return false;

	pCurrent += offset;
	if (swscanf(pCurrent, L" , rule"))
		return false;

	// goto next line
	while (*pCurrent++ != '\n');


	Clear();
	if (x>m_Width || y>m_Height)
		Initialize(x, y);

	SetRules(survivalRule, birthRule);


	concurrency::reader_writer_lock::scoped_lock(m_Buffers[m_BufferIndex].Lock);

	uint32 firstCol = m_Width / 2 - x / 2;
	uint32 firstRow = m_Height / 2 - y / 2;
	bool* pMap = m_Buffers[m_BufferIndex].pData + firstRow*m_Width + firstCol;

	bool* pMapLine = pMap;

	while (true)
	{
		int n = 1;
		wchar_t c = 0;
		bool* pMapColumn = pMapLine;

		while (true)
		{
			while (*pCurrent == '\r' || *pCurrent == '\n') pCurrent++;

			c = *pCurrent;
			if (c=='$' || c=='!') 
			{
				pCurrent++;
				break;
			}

			n = 1;
			offset = 0;
			swscanf(pCurrent, L"%d%n", &n, &offset);

			pCurrent += offset;
			c = *pCurrent++;

			for (int i = 0; i < n; i++)
				*pMapColumn++ = (c!='b');
		}

		if (c=='!') break;
		pMapLine += m_Width;
	}

	m_BufferUpdated = true;
	m_nbGenerations = 0;

	return true;
}

bool GoLBasic::LoadLIFE(const wchar_t* pFileBuffer)
{
	const wchar_t* pCurrent = pFileBuffer;
	uint32 survivalRule = 23, birthRule = 3;
	uint32 x = 0, y = 0;
	float version = 1.05f;
	
	// Consume pre-header lines
	while (*pCurrent == '#') 
	{
		pCurrent++;

		switch (*pCurrent++)
		{
		case 'R': 
			if (swscanf(pCurrent, L" %u / %u", &survivalRule, &birthRule) != 2)
				return false;
			break;

		case 'L': 
			if (swscanf(pCurrent, L"ife %f", &version) != 1)
				return false;
			if (version != 1.05f && version != 1.06f)
				return false;
			break;

		case 'P':	// top-left coords
			if (swscanf(pCurrent, L" %u %u", &x, &y) != 2)
				return false;
			break;

		case 'D':	// comment
		default:
			break;
		}

		// goto next line
		while (*pCurrent++ != '\n');
	}

	uint32 firstCol = m_Width / 2 - x;
	uint32 firstRow = m_Height / 2 - y;
	bool* pMap = m_Buffers[m_BufferIndex].pData + firstRow*m_Width + firstCol;

	if (version == 1.05f)
	{
		bool* pMapLine = pMap;
		bool bNewLine = true;

		while (true)
		{
			char c = *pCurrent++;
			if (c == '\r')
			{
				c = *pCurrent++;

			}
			while (*pCurrent == '\r' || *pCurrent == '\n') pCurrent++;

			//char c = *pCurrent++;

			//bool* pMapColumn = pMapLine;



			pMapLine += m_Width;
		}

	}
	else
	{
	}

	return false;
}

bool GoLBasic::LoadMCELL(const wchar_t* /*pFileBuffer*/)
{
	return false;
}

void GoLBasic::Step()
{
	char currentBufferIndex = m_BufferIndex;
	m_Buffers[currentBufferIndex].Lock.lock_read();
	bool* pSrcBuffer = m_Buffers[currentBufferIndex].pData;

	char nextBufferIndex = (currentBufferIndex+1) % GOL_NB_BUFFERS;
	while (!m_Buffers[nextBufferIndex].Lock.try_lock()) nextBufferIndex = (nextBufferIndex+1) % GOL_NB_BUFFERS;
	bool* pDstBuffer = m_Buffers[nextBufferIndex].pData;

	if (!m_bUseParallel)
	{
		for (uint32 row = 1; row < m_Height-1; row++)
		{
			uint32 offset = row * m_Width;
			bool* pSrc = pSrcBuffer + offset;
			bool* pDst = pDstBuffer + offset;
			for (uint32 col = 1; col < m_Width-1; col++)
			{
				uint32 N = pSrc[col - 1 - m_Width] + pSrc[col - m_Width] + pSrc[col + 1 - m_Width]
						 + pSrc[col - 1          ]                       + pSrc[col + 1          ]
						 + pSrc[col - 1 + m_Width] + pSrc[col + m_Width] + pSrc[col + 1 + m_Width];

				pDst[col] = m_pRulesLut[pSrc[col]][N];
			}
		}
	}
	else
	{
		concurrency::parallel_for<uint32> (1, m_Height-1, [this,pSrcBuffer,pDstBuffer](uint32 row)
		{
			uint32 offset = row * m_Width;
			bool* pSrc = pSrcBuffer + offset;
			bool* pDst = pDstBuffer + offset;
			for (uint32 col = 1; col < m_Width-1; col++)
			{
				uint32 N = pSrc[col - 1 - m_Width] + pSrc[col - m_Width] + pSrc[col + 1 - m_Width]
						 + pSrc[col - 1          ]                       + pSrc[col + 1          ]
						 + pSrc[col - 1 + m_Width] + pSrc[col + m_Width] + pSrc[col + 1 + m_Width];

				pDst[col] = m_pRulesLut[pSrc[col]][N];
			}
		});
	}

	if (m_bCylinder)
	{
		// Row #0
		{
			bool* pSrc = pSrcBuffer;
			bool* pDst = pDstBuffer;
			for (uint32 col = 1; col < m_Width-1; col++)
			{
				uint32 N = pSrc[col - 1 - m_Width + m_Area] + pSrc[col - m_Width + m_Area] + pSrc[col + 1 - m_Width + m_Area]
				         + pSrc[col - 1                   ]                                + pSrc[col + 1                   ]
				         + pSrc[col - 1 + m_Width         ] + pSrc[col + m_Width         ] + pSrc[col + 1 + m_Width         ];

				pDst[col] = m_pRulesLut[pSrc[col]][N];
			}
		}
		// Row #N-1
		{
			uint32 offset = (m_Height-1) * m_Width;
			bool* pSrc = pSrcBuffer + offset;
			bool* pDst = pDstBuffer + offset;
			for (uint32 col = 1; col < m_Width-1; col++)
			{
				uint32 N = pSrc[col - 1 - m_Width         ] + pSrc[col - m_Width         ] + pSrc[col + 1 - m_Width         ]
				         + pSrc[col - 1                   ]                                + pSrc[col + 1                   ]
				         + pSrc[col - 1 + m_Width - m_Area] + pSrc[col + m_Width - m_Area] + pSrc[col + 1 + m_Width - m_Area];

				pDst[col] = m_pRulesLut[pSrc[col]][N];
			}
		}

		// Col #0
		for (uint32 row = 1; row < m_Height-1; row++)
		{
			uint32 offset = row * m_Width;
			bool* pSrc = pSrcBuffer + offset;
			bool* pDst = pDstBuffer + offset;
			uint32 col = 0;
			uint32 N = pSrc[col - 1 + m_Width - m_Width] + pSrc[col - m_Width] + pSrc[col + 1 - m_Width]
			         + pSrc[col - 1 + m_Width          ]                         + pSrc[col + 1          ]
			         + pSrc[col - 1 + m_Width + m_Width] + pSrc[col + m_Width] + pSrc[col + 1 + m_Width];

			pDst[col] = m_pRulesLut[pSrc[col]][N];
		}

		// Col #N-1
		for (uint32 row = 1; row < m_Height-1; row++)
		{
			uint32 offset = row * m_Width;
			bool* pSrc = pSrcBuffer + offset;
			bool* pDst = pDstBuffer + offset;
			
			uint32 col = m_Width-1;
			uint32 N = pSrc[col - 1 - m_Width] + pSrc[col - m_Width] + pSrc[col + 1 - m_Width - m_Width]
			         + pSrc[col - 1          ]                       + pSrc[col + 1 - m_Width          ]
			         + pSrc[col - 1 + m_Width] + pSrc[col + m_Width] + pSrc[col + 1 - m_Width + m_Width];

			pDst[col] = m_pRulesLut[pSrc[col]][N];
		}

		// (0,0)
		{
			bool* pSrc = pSrcBuffer;
			bool* pDst = pDstBuffer;
			uint32 index = 0;
			uint32 N = pSrc[ index - 1 + m_Width - m_Width + m_Area] + pSrc[ index - m_Width + m_Area] + pSrc[ index + 1 - m_Width + m_Area]
			         + pSrc[ index - 1 + m_Width                   ]                                   + pSrc[ index + 1                   ]
			         + pSrc[ index - 1 + m_Width + m_Width         ] + pSrc[ index + m_Width         ] + pSrc[ index + 1 + m_Width         ];

			pDst[index] = m_pRulesLut[pSrc[index]][N];
		}

		// (N-1,0)
		{
			bool* pSrc = pSrcBuffer;
			bool* pDst = pDstBuffer;
			uint32 index = m_Width-1;
			uint32 N = pSrc[ index - 1 - m_Width + m_Area] + pSrc[ index - m_Width + m_Area] + pSrc[ index + 1 - m_Width - m_Width + m_Area]
			         + pSrc[ index - 1                   ]                                   + pSrc[ index + 1           - m_Width         ]
			         + pSrc[ index - 1 + m_Width         ] + pSrc[ index + m_Width         ] + pSrc[ index + 1 + m_Width - m_Width         ];

			pDst[index] = m_pRulesLut[pSrc[index]][N];
		}

		// (0,N-1)
		{
			bool* pSrc = pSrcBuffer;
			bool* pDst = pDstBuffer;
			uint32 index = m_Width * (m_Height-1);
			uint32 N = pSrc[ index - 1 + m_Width - m_Width         ] + pSrc[ index - m_Width         ] + pSrc[ index + 1 - m_Width         ]
			         + pSrc[ index - 1 + m_Width                   ]                                   + pSrc[ index + 1                   ]
			         + pSrc[ index - 1 + m_Width + m_Width - m_Area] + pSrc[ index + m_Width - m_Area] + pSrc[ index + 1 + m_Width - m_Area];

			pDst[index] = m_pRulesLut[pSrc[index]][N];
		}

		// (N-1,N-1)
		{
			bool* pSrc = pSrcBuffer;
			bool* pDst = pDstBuffer;
			uint32 index = m_Area - 1;
			uint32 N = pSrc[ index - 1 - m_Width         ] + pSrc[ index - m_Width         ] + pSrc[ index + 1 - m_Width - m_Width         ]
			         + pSrc[ index - 1                   ]                                   + pSrc[ index + 1           - m_Width         ]
			         + pSrc[ index - 1 + m_Width - m_Area] + pSrc[ index + m_Width - m_Area] + pSrc[ index + 1 + m_Width - m_Width - m_Area];

			pDst[index] = m_pRulesLut[pSrc[index]][N];
		}
	}

	m_BufferIndex = nextBufferIndex;
	m_BufferUpdated = true;
	m_Buffers[nextBufferIndex].Lock.unlock();
	m_Buffers[currentBufferIndex].Lock.unlock();

	m_nbGenerations++;
}

void GoLBasic::Run()
{
	m_evtStop.reset();
	m_bRunning = true;

	uint32 stopTimeout = 0;
	while (m_evtStop.wait(stopTimeout) == concurrency::COOPERATIVE_WAIT_TIMEOUT)
	{
		auto startTime = std::chrono::high_resolution_clock::now();

		Step();

		auto endTime = std::chrono::high_resolution_clock::now();
 
		float elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime).count() / 1000.0f;

		float requiredRunPeriod = 1.0f / m_requiredRunFreq; 
		float actualRunPeriod = requiredRunPeriod;

		if (requiredRunPeriod > elapsedTime)
		{
			stopTimeout = (uint32)(1000.0f * (requiredRunPeriod - elapsedTime) + 0.5f);
			actualRunPeriod = requiredRunPeriod;
		}
		else
		{
			stopTimeout = 0;
			actualRunPeriod = elapsedTime;
		}

		m_actualRunFreq = (uint32)(1.0f / actualRunPeriod + 0.5f);

		//{
		//	//concurrency::reader_writer_lock::scoped_lock_read(m_Buffers[m_BufferIndex].Lock);
		//	m_density = (float)m_Buffers[m_BufferIndex].NbCells / m_Area;
		//}
	}

	m_actualRunFreq = UINT32_MAX;
	m_bRunning = false;
}

void GoLBasic::Stop()
{
	m_evtStop.set();
}

void GoLBasic::BuildMap()
{
	if (m_BufferUpdated)
	{
		char bufferIndex = m_BufferIndex;
		concurrency::reader_writer_lock::scoped_lock_read(m_Buffers[bufferIndex].Lock);
		if (!m_bUseParallel)
		{
			bool* pSrc = m_Buffers[bufferIndex].pData;
			uint32* pDst = m_pBitmap;

			for (uint32 row = 0; row < m_Height; row++)
			{
				for (uint32 col = 0; col < m_Width; col++)
				{
					*pDst++ = *pSrc++ ? 0xFFFFFFFF : 0x20000000;
				}
			}
		}
		else
		{
			concurrency::parallel_for<uint32> (0, m_Height, [this,bufferIndex](uint32 row)
			{
				bool* pSrc = m_Buffers[bufferIndex].pData + row * m_Width;
				uint32* pDst = m_pBitmap + row * m_Width;

				for (uint32 col = 0; col < m_Width; col++)
				{
					*pDst++ = *pSrc++ ? 0xFFFFFFFF : 0x20000000;
				}
			});
		}
	}

	send(m_Bitmaps, (unsigned char*)m_pBitmap);
}

