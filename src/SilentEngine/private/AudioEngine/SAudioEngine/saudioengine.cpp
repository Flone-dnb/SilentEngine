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
#include "SilentEngine/Public/EntityComponentSystem/SAudioComponent/SAudioComponent.h"
#include "SilentEngine/Public/SCamera/SCamera.h"
#include "../SSoundMix/ssoundmix.h"
#include "../SSound/ssound.h"


SAudioEngine::SAudioEngine()
{
    pMasteringVoice = nullptr;

    bEngineInitialized = false;
    bEnableLowLatency = true;

	memset(&x3dAudioListener, 0, sizeof(X3DAUDIO_LISTENER));
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
		SError::showErrorMessageBoxAndLog("the audio engine is not initialized.");
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

void SAudioEngine::apply3DPropsForComponent(SAudioComponent* pAudioComponent, float fDeltaTime)
{
	SVector vComponentLocation = pAudioComponent->getLocationInWorld();

	SEmitterProps emitterProps;
	X3DAUDIO_VECTOR currentPosition = { vComponentLocation.getX(), vComponentLocation.getY(), vComponentLocation.getZ() };

	emitterProps.position = currentPosition;
	emitterProps.velocity = { 0.0f, 0.0f, 0.0f };

	pAudioComponent->pSound->applyNew3DSoundProps(emitterProps);
}

bool SAudioEngine::getMasterVolume(float &fVolume)
{
    if (bEngineInitialized == false)
    {
		SError::showErrorMessageBoxAndLog("the audio engine is not initialized.");
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

void SAudioEngine::registerNew3DAudioComponent(SAudioComponent* pAudioComponent)
{
	// called in onSpawn() (in spawn mutex)

	vSpawned3DAudioComponents.push_back(pAudioComponent);
}

void SAudioEngine::unregister3DAudioComponent(SAudioComponent* pAudioComponent)
{
	// called in onSpawn() (in spawn mutex)

	bool bFound = false;

	for (size_t i = 0; i < vSpawned3DAudioComponents.size(); i++)
	{
		if (vSpawned3DAudioComponents[i] == pAudioComponent)
		{
			bFound = true;
			break;
		}
	}

	if (bFound == false)
	{
		SError::showErrorMessageBoxAndLog("can't find specified audio component as a registered one.");
	}
}

void SAudioEngine::update3DSound(SCamera* pPlayerCamera)
{
	SVector vCameraLocation = pPlayerCamera->getCameraLocationInWorld();
	SVector vCameraForwardVector;
	SVector vCameraUpVector;
	pPlayerCamera->getCameraBasicVectors(&vCameraForwardVector, nullptr, &vCameraUpVector);

	SListenerProps listenerProps;
	listenerProps.position = { vCameraLocation.getX(), vCameraLocation.getY(), vCameraLocation.getZ() };
	listenerProps.upVector = { vCameraUpVector.getX(), vCameraUpVector.getY(), vCameraUpVector.getZ() };
	listenerProps.forwardVector = { vCameraForwardVector.getX(), vCameraForwardVector.getY(), vCameraForwardVector.getZ() };
	listenerProps.velocity = { 0.0f, 0.0f, 0.0f };


	applyNew3DListenerProps(listenerProps);



	for (size_t i = 0; i < vSpawned3DAudioComponents.size(); i++)
	{
		apply3DPropsForComponent(vSpawned3DAudioComponents[i]);
	}
}

bool SAudioEngine::initXAudio2()
{
    UINT32 iFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
    iFlags = XAUDIO2_DEBUG_ENGINE;
#endif

	HRESULT hr = CoInitializeEx( 0, COINIT_MULTITHREADED );
	if (FAILED(hr))
	{
		SError::showErrorMessageBoxAndLog(hr);
		return true;
	}


    hr = XAudio2Create(&pXAudio2Engine, iFlags);
    if (FAILED(hr))
    {
		SError::showErrorMessageBoxAndLog(hr);
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
		SError::showErrorMessageBoxAndLog(hr);
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
		SError::showErrorMessageBoxAndLog(hr);
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
		SError::showErrorMessageBoxAndLog(hr);
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
			SError::showErrorMessageBoxAndLog(hr);
            return true;
        }
    }


    return false;
}
