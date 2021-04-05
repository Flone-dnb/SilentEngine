// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "saudioengine.h"

// STL
#include <fstream>

// Engine
#include "SilentEngine/Private/SError/serror.h"

// Custom
#include "../SSoundMix/ssoundmix.h"


SAudioEngine::SAudioEngine()
{
    pMasteringVoice = nullptr;

    bEngineInitialized = false;

    bEnableLowLatency = true;
}

bool SAudioEngine::init(bool bEnableLowLatency)
{
    this->bEnableLowLatency = bEnableLowLatency;

    bEngineInitialized = false;

    if (initXAudio2())
    {
        return true;
    }

    if (initWMF())
    {
        return true;
    }

    initX3DAudio();

    bEngineInitialized = true;

    return false;
}

bool SAudioEngine::createSoundMix(SSoundMix *&pOutSoundMix)
{
    pOutSoundMix = new SSoundMix(this);

    if (pOutSoundMix->init())
    {
        delete pOutSoundMix;

        return true;
    }


    mtxSoundMix.lock();

    vCreatedSoundMixes.push_back(pOutSoundMix);

    mtxSoundMix.unlock();


    return false;
}

bool SAudioEngine::setMasterVolume(float fVolume)
{
    if (bEngineInitialized == false)
    {
		SError::showErrorMessageBox(L"AudioEngine::setMasterVolume()", L"the audio engine is not initialized.");
        return true;
    }

    pMasteringVoice->SetVolume(fVolume);

    return false;
}

void SAudioEngine::applyNew3DListenerProps(SListenerProps &listenerProps)
{
    listenerProps.position = X3DAUDIO_VECTOR(listenerProps.position.x, listenerProps.position.z, -listenerProps.position.y);
    listenerProps.velocity = X3DAUDIO_VECTOR(listenerProps.velocity.x, listenerProps.velocity.z, -listenerProps.velocity.y);
    listenerProps.upVector = X3DAUDIO_VECTOR(listenerProps.upVector.x, listenerProps.upVector.z, -listenerProps.upVector.y);
    listenerProps.forwardVector = X3DAUDIO_VECTOR(listenerProps.forwardVector.x, listenerProps.forwardVector.z, -listenerProps.forwardVector.y);

    //listenerProps.position = X3DAUDIO_VECTOR(listenerProps.position.x, listenerProps.position.y, listenerProps.position.z);
    //listenerProps.velocity = X3DAUDIO_VECTOR(listenerProps.velocity.x, listenerProps.velocity.y, listenerProps.velocity.z);

    x3dAudioListener.OrientFront = listenerProps.forwardVector;
    x3dAudioListener.OrientTop = listenerProps.upVector;
    x3dAudioListener.Position = listenerProps.position;
    x3dAudioListener.Velocity = listenerProps.velocity;
}

bool SAudioEngine::getMasterVolume(float &fVolume)
{
    if (bEngineInitialized == false)
    {
		SError::showErrorMessageBox(L"AudioEngine::getMasterVolume()", L"the audio engine is not initialized.");
        return true;
    }

    pMasteringVoice->GetVolume(&fVolume);

    return false;
}

SAudioEngine::~SAudioEngine()
{
    mtxSoundMix.lock();

    for (size_t i = 0; i < vCreatedSoundMixes.size(); i++)
    {
        delete vCreatedSoundMixes[i];
    }

    mtxSoundMix.unlock();



    pMasteringVoice->DestroyVoice();


    pXAudio2Engine->StopEngine();


    pXAudio2Engine->Release();

    MFShutdown();
}

bool SAudioEngine::initXAudio2()
{
    UINT32 iFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
    iFlags = XAUDIO2_DEBUG_ENGINE;
#endif

    CoInitializeEx( 0, COINIT_MULTITHREADED );

    HRESULT hr = XAudio2Create(&pXAudio2Engine, iFlags);
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"AudioEngine::initXAudio2::XAudio2Create()");
        return true;
    }

#if defined(DEBUG) || defined(_DEBUG)
    XAUDIO2_DEBUG_CONFIGURATION debugConfig = {0};
    debugConfig.BreakMask = XAUDIO2_LOG_ERRORS;
    debugConfig.TraceMask = XAUDIO2_LOG_ERRORS;
    debugConfig.LogFileline = 1;

    pXAudio2Engine->SetDebugConfiguration(&debugConfig);
#endif


    hr = pXAudio2Engine->CreateMasteringVoice(&pMasteringVoice);
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"AudioEngine::initXAudio2::CreateMasteringVoice()");
        return true;
    }


    return false;
}

bool SAudioEngine::initWMF()
{
    // Initialize WMF.

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"AudioEngine::initWMF::MFStartup()");
        return true;
    }


    return false;
}

void SAudioEngine::initX3DAudio()
{
    DWORD dwChannelMask;
    pMasteringVoice->GetChannelMask(&dwChannelMask);

    X3DAudioInitialize(dwChannelMask, X3DAUDIO_SPEED_OF_SOUND, hX3dAudio);

    memset(&x3dAudioListener, 0, sizeof(X3DAUDIO_LISTENER));
    x3dAudioListener.OrientFront.y = 1.f;
    x3dAudioListener.OrientTop.z = 1.f;
}

bool SAudioEngine::initSourceReaderConfig(IMFAttributes*& pSourceReaderConfig)
{
    // Create source reader config.
    // configure to low latency
    HRESULT hr = MFCreateAttributes(&pSourceReaderConfig, 1);
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"AudioEngine::initSourceReaderConfig::MFCreateAttributes()");
        return true;
    }

    if (bEnableLowLatency)
    {
        // Enables low-latency processing
        // "Low latency is defined as the smallest possible delay from when the media data is generated (or received) to when it is rendered.
        // Low latency is desirable for real-time communication scenarios.
        // For other scenarios, such as local playback or transcoding, you typically should not enable low-latency mode, because it can affect quality."
        hr = pSourceReaderConfig->SetUINT32(MF_LOW_LATENCY, true);
        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"AudioEngine::initSourceReaderConfig::MFCreateAttributes()");
            return true;
        }
    }


    return false;
}
