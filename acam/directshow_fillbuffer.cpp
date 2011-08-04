#include "stdafx.h"
#include "acam.h"


extern IReferenceClock *globalClock;

//
// FillBuffer
//
// Stuffs the buffer with data
// "they" call this, every so often...
// you probably should fill the entire buffer...I think...hmm...
HRESULT CVCamStream::FillBuffer(IMediaSample *pms) 
{
    CheckPointer(pms,E_POINTER);
    BYTE *pData;
    HRESULT hr = pms->GetPointer(&pData);
    if (FAILED(hr)) {
        return hr;
    }

	LONG totalWrote = -1;
	hr = LoopbackCaptureTakeFromBuffer(pData, pms->GetSize(), NULL, &totalWrote); // the real meat
	if(FAILED(hr)) {
		return hr;
	}

	hr = pms->SetActualDataLength(totalWrote);
	if(FAILED(hr)) {
		return hr;
	}

	WAVEFORMATEX* pwfexCurrent = (WAVEFORMATEX*)m_mt.Format();
	IReferenceClock* pClock;
	m_pParent->GetSyncSource(&pClock);
	REFERENCE_TIME now;
	pClock->GetTime(&now);
	REFERENCE_TIME previousEnd = m_rtSampleEndTime;
	pClock->Release(); // TODO


    // Set the sample's start and end time stamps...
	// disabled for now...
    REFERENCE_TIME rtStart = now - latestGraphStart; // unused
	if(now < latestGraphStart) {
	  int a = 3;
	}
	rtStart = m_rtSampleEndTime; // re-use <sigh>. TODO re-sync it if it's too far in the past [?] // compare end  "now"
    m_rtSampleEndTime = rtStart + (REFERENCE_TIME)(UNITS * pms->GetActualDataLength()) / 
                     (REFERENCE_TIME)pwfexCurrent->nAvgBytesPerSec;

    REFERENCE_TIME oldStartTime = now - latestGraphStart;


	// by not setting this, we're instructing it to "immediately display"
	// TODO set it using the time passed in to the [start] method [?] http://msdn.microsoft.com/en-us/library/aa451394.aspx Run method?
	/*
	REFERENCE_TIME a;
	if(previousEnd > rtStart) {
		a = rtStart;
		rtStart = previousEnd;
	}*/
	if(!(rtStart < m_rtSampleEndTime)) {
	  assert(rtStart < m_rtSampleEndTime); // sanity check my math...
	}


   // sniff hr = pms->SetTime((REFERENCE_TIME*)&rtStart, (REFERENCE_TIME*)&m_rtSampleEndTime);

    if (FAILED(hr)) {
        return hr;
    }

    // Set the sample's properties.

    hr = pms->SetPreroll(FALSE); // tell it that this isn't preroll, so actually use it
    if (FAILED(hr)) {
        return hr;
    }

    hr = pms->SetMediaType(NULL);
    if (FAILED(hr)) {
        return hr;
    }
   
    hr = pms->SetDiscontinuity(!bFirstPacket);
    if (FAILED(hr)) {
        return hr;
    }
    
    hr = pms->SetSyncPoint(!bFirstPacket);
	if (FAILED(hr)) {
        return hr;
    }

	// MediaTime is our own internal "offset" so fill it in starting from 0...http://msdn.microsoft.com/en-us/library/aa451394.aspx
	// which actually might work for us :)
    LONGLONG llMediaTimeStart = m_llSampleMediaTimeStart;
    DWORD dwNumAudioSamplesInPacket = (pms->GetActualDataLength() * BITS_PER_BYTE) /
                                      (pwfexCurrent->nChannels * pwfexCurrent->wBitsPerSample);
    LONGLONG llMediaTimeStop = m_llSampleMediaTimeStart + dwNumAudioSamplesInPacket;
    hr = pms->SetMediaTime(&llMediaTimeStart, &llMediaTimeStop);
    if (FAILED(hr)) {
        return hr;
    }

    m_llSampleMediaTimeStart = llMediaTimeStop;
    return NOERROR;
} // end FillBuffer
