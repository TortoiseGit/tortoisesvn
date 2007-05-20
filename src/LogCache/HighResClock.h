#pragma once

///////////////////////////////////////////////////////////////
//
// CHighResClock
//
//		high resolution clock for performance measurement.
//		Depending on the hardware it will provide µsec 
//		resolution and accuracy.
//
//		May not be available on all machines.
//
///////////////////////////////////////////////////////////////

class CHighResClock
{
private:

	LARGE_INTEGER start;
	LARGE_INTEGER taken;

public:

	// construction (starts measurement) / destruction

	CHighResClock() 
	{
		taken.QuadPart = 0;
		Start();
	}

	~CHighResClock()
	{
	}

	// (re-start)

	void Start()
	{
		QueryPerformanceCounter(&start);
	}

	// set "taken" to time since last Start()

	void Stop()
	{
		QueryPerformanceCounter(&taken);
		taken.QuadPart -= start.QuadPart;
	}

	// time in microseconds between last Start() and last Stop()

	DWORD GetMusecsTaken() const
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		return (DWORD)((taken.QuadPart * 1000000) / frequency.QuadPart);
	}
};
