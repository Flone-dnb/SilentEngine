// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <string>
#include <vector>
#include <mutex>

// XAudio2
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <xapofx.h>

#pragma comment(lib, "xaudio2.lib")

#include <wrl.h>

// Windows Media Foundation (WMF)
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>

#include <Shlwapi.h>

#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "Propsys.lib")

#pragma comment(lib, "shlwapi.lib")



// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

// Callback structure for the WMF Source Reader.
class SourceReaderCallback : public IMFSourceReaderCallback
{
public:
    SourceReaderCallback() :
      bIsEndOfStream(FALSE), hrStatus(S_OK)
    {
        hReadSampleEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		llTimestamp = 0;
    }

    void restart()
    {
        bIsEndOfStream = false;
        hrStatus = S_OK;
    }

    virtual ~SourceReaderCallback()
    {
        CloseHandle(hReadSampleEvent);
    }

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv) override
    {
        static const QITAB qit[] =
        {
            QITABENT(SourceReaderCallback, IMFSourceReaderCallback),
            { 0 },
        };
        return QISearch(this, qit, iid, ppv);
    }

    STDMETHOD_(ULONG, AddRef)() override { return 1; }
    STDMETHOD_(ULONG, Release)() override { return 1; }

    STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
        DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample) override
    {
        std::lock_guard<std::mutex> lock(guard);

        if (SUCCEEDED(hrStatus))
        {
            if (pSample)
            {
                sample = pSample;
            }
        }
        else
        {
            // Streaming error.
            NotifyError(hrStatus);
        }

        if (MF_SOURCE_READERF_ENDOFSTREAM & dwStreamFlags)
        {
            // Reached the end of the stream.
            bIsEndOfStream = TRUE;
        }
        this->hrStatus = hrStatus;
        this->llTimestamp = llTimestamp;


        SetEvent(hReadSampleEvent);
        return S_OK;
    }

    STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) override
    {
        return S_OK;
    }

    STDMETHODIMP OnFlush(DWORD) override
    {
        return S_OK;
    }

public:
    HRESULT Wait(DWORD dwMilliseconds, BOOL *pbEOS)
    {
        *pbEOS = FALSE;

        DWORD dwResult = WaitForSingleObject(hReadSampleEvent, dwMilliseconds);
        if (dwResult == WAIT_TIMEOUT)
        {
            return E_PENDING;
        }
        else if (dwResult != WAIT_OBJECT_0)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        *pbEOS = bIsEndOfStream;
        return hrStatus;
    }

private:

    void NotifyError(HRESULT hr)
    {
        wprintf(L"Source Reader error: 0x%X\n", hr);
    }

public:
    LONGLONG            llTimestamp;
    std::mutex			guard;
    HANDLE              hReadSampleEvent;
    BOOL                bIsEndOfStream;
    HRESULT             hrStatus;
    Microsoft::WRL::ComPtr<IMFSample> sample;
};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

// Callback structure for XAudio2 voices.
struct VoiceCallback : public IXAudio2VoiceCallback
{
    HANDLE hStreamEnd;
    HANDLE hBufferEndEvent;

    VoiceCallback()
    {
        hBufferEndEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        hStreamEnd = CreateEvent( NULL, FALSE, FALSE, NULL );
    }

    virtual ~VoiceCallback()
    {
        CloseHandle( hBufferEndEvent );
        CloseHandle( hStreamEnd );
    }

    // Called when the voice has just finished playing a contiguous audio stream.
    void OnStreamEnd()
    {
        SetEvent(hStreamEnd);
        SetEvent(hBufferEndEvent);
    }

    // Unused methods.
    void OnVoiceProcessingPassEnd() { }
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired)
    {
        UNREFERENCED_PARAMETER(SamplesRequired);
    }
    void OnBufferEnd(void * pBufferContext)
    {
        UNREFERENCED_PARAMETER(pBufferContext);

        SetEvent(hBufferEndEvent);
    }
    void OnBufferStart(void * pBufferContext)
    {
        UNREFERENCED_PARAMETER(pBufferContext);
    }
    void OnLoopEnd(void * pBufferContext)
    {
        UNREFERENCED_PARAMETER(pBufferContext);
    }
    void OnVoiceError(void * pBufferContext, HRESULT Error)
    {
        UNREFERENCED_PARAMETER(pBufferContext);
        UNREFERENCED_PARAMETER(Error);
    }
};



class SSoundMix;
class SSound;
class SAudioComponent;

struct SListenerProps
{
    X3DAUDIO_VECTOR forwardVector;
    X3DAUDIO_VECTOR upVector;
    X3DAUDIO_VECTOR position;
    X3DAUDIO_VECTOR velocity; // in units per sec. (only for doppler)
};

class SAudioEngine
{

public:

    SAudioEngine();


    // Will be auto deleted in ~SAudioEngine().
    bool createSoundMix(SSoundMix*& pOutSoundMix);


    bool setMasterVolume(float fVolume);
    bool getMasterVolume(float& fVolume);


    ~SAudioEngine();

private:

    friend class SSound;
    friend class SSoundMix;
    friend class SAudioEffect;
	friend class SComponent;
	friend class SApplication;

	bool init(bool bEnableLowLatency = true);

	// called in onSpawn()
	void registerNew3DAudioComponent(SAudioComponent* pAudioComponent);
	// called in onDespawn()
	void unregister3DAudioComponent(SAudioComponent* pAudioComponent);

	// will be heard only after SSound::applyNew3DSoundProps() calls
	// should be normalized
	void applyNew3DListenerProps(SListenerProps& listenerProps);
	void apply3DPropsForComponent(SAudioComponent* pAudioComponent, float fDeltaTime = 0.0f);

	void update3DSound(class SCamera* pPlayerCamera);


    bool initXAudio2();
    bool initWMF();
    void initX3DAudio();

    bool initSourceReaderConfig(IMFAttributes*& pSourceReaderConfig);


    IXAudio2*               pXAudio2Engine;
    IXAudio2MasteringVoice* pMasteringVoice;


    X3DAUDIO_HANDLE         hX3dAudio;
    X3DAUDIO_LISTENER       x3dAudioListener;


	std::vector<SAudioComponent*> vSpawned3DAudioComponents;


    std::mutex mtxSoundMix;
    std::vector<SSoundMix*> vCreatedSoundMixes;


    bool bEngineInitialized;
    bool bEnableLowLatency;
};


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

enum class S_EFFECT_TYPE
{
    ET_REVERB = 0,
    ET_EQ = 1,
    ET_ECHO = 2
};

class SAudioEffect
{
public:

    // see 'https://docs.microsoft.com/en-us/windows/win32/xaudio2/xapofx-overview' for details.
    SAudioEffect(SAudioEngine* pAudioEngine, S_EFFECT_TYPE effectType, bool bEnable)
    {
        this->effectType = effectType;
        this->bEnable = bEnable;

        this->pAudioEngine = pAudioEngine;

		reverbParams = { 0 };
		eqParams = { 0 };
		echoParams = { 0 };
    }

    void setReverbParameters(FXREVERB_PARAMETERS params)
    {
        reverbParams = params;
    }

    void setEQParameters(FXEQ_PARAMETERS params)
    {
        eqParams = params;
    }

    void setEchoParameters(FXECHO_PARAMETERS params)
    {
        echoParams = params;
    }

    void setEnableEffect(bool bEnable)
    {
        this->bEnable = bEnable;
    }

    S_EFFECT_TYPE getEffectType() const
    {
        return effectType;
    }

    bool isEnabled() const
    {
        return bEnable;
    }

private:

    friend class SSound;
    friend class SSoundMix;


    SAudioEngine* pAudioEngine;

    S_EFFECT_TYPE effectType;

    bool bEnable;

    FXREVERB_PARAMETERS reverbParams;
    FXEQ_PARAMETERS eqParams;
    FXECHO_PARAMETERS echoParams;
};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
