// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "ssound.h"

// STL
#include <fstream>

// Engine
#include "SilentEngine/Private/SError/serror.h"

// Custom
#include "../SSoundMix/ssoundmix.h"

// Other
#include <Mferror.h>


SSound::SSound(SAudioEngine* pAudioEngine, bool bIs3DSound)
{
    this->pAudioEngine = pAudioEngine;

    pAsyncSourceReader = nullptr;
    pOptionalSourceReader = nullptr;

    pSourceVoice = nullptr;

    bSoundLoaded = false;
    bUseStreaming = false;
    bSoundStoppedManually = false;
    bCurrentlyStreaming = false;

    sAudioFileDiskPath = L"";

    dCurrentStreamingPosInSec = 0.0;
    iSamplesPlayedOnLastSetPos = 0;

    bCalledOnPlayEnd = false;
    bDestroyCalled = false;
    bEffectsSet = false;
    this->bIs3DSound = bIs3DSound;

    iCurrentEffectIndex = 0;


    soundState = SS_NOT_PLAYING;


    hEventUnpauseSound = CreateEvent(NULL, FALSE, FALSE, NULL);
}

SSound::~SSound()
{
    bDestroyCalled = true;

    clearSound();

    CloseHandle(hEventUnpauseSound);
}

bool SSound::loadAudioFile(const std::wstring &sAudioFilePath, bool bStreamAudio, SSoundMix* pOutputToSoundMix)
{
    clearSound();

    bSoundLoaded = false;
    iCurrentEffectIndex = 0;
    bEffectsSet = false;


    this->pSoundMix = pOutputToSoundMix;


    IMFAttributes* pConfig = nullptr;
    pAudioEngine->initSourceReaderConfig(pConfig);
    pSourceReaderConfig = pConfig;
    pAudioEngine->initSourceReaderConfig(pConfig);
    pOptionalSourceReaderConfig = pConfig;


    WAVEFORMATEX* waveFormatEx;

    if (bStreamAudio == false)
    {
        if (loadFileIntoMemory(sAudioFilePath, vAudioData, &waveFormatEx, iWaveFormatSize))
        {
            return true;
        }

        soundFormat = *waveFormatEx;
        CoTaskMemFree(waveFormatEx);


        // Create source voice.


        XAUDIO2_VOICE_SENDS sendTo = {0};

		HRESULT hr = S_OK;

		if (pSoundMix)
		{
			XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2];
			sendDescriptors[0].Flags = 0;
			sendDescriptors[0].pOutputVoice = pSoundMix->pSubmixVoice;
			sendDescriptors[1].Flags = 0;
			sendDescriptors[1].pOutputVoice = pSoundMix->pSubmixVoiceFX;

			const XAUDIO2_VOICE_SENDS sendList = { 2, sendDescriptors };

            hr = pAudioEngine->pXAudio2Engine->CreateSourceVoice(&pSourceVoice, &soundFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback, &sendList, NULL);
		}
		else
		{
			XAUDIO2_SEND_DESCRIPTOR sendDescriptors[1];
			sendDescriptors[0].Flags = 0;
			sendDescriptors[0].pOutputVoice = pAudioEngine->pMasteringVoice;

			const XAUDIO2_VOICE_SENDS sendList = { 1, sendDescriptors };

            hr = pAudioEngine->pXAudio2Engine->CreateSourceVoice(&pSourceVoice, &soundFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback, &sendList, NULL);
		}

        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"Sound::loadAudioFile::CreateSourceVoice()");
            return true;
        }

        ZeroMemory(&audioBuffer, sizeof(XAUDIO2_BUFFER));
        audioBuffer.AudioBytes = static_cast<UINT32>(vAudioData.size());
        audioBuffer.pAudioData = &vAudioData[0];
        audioBuffer.pContext = nullptr;
    }
    else
    {
        if (createAsyncReader(sAudioFilePath, pAsyncSourceReader, &waveFormatEx, iWaveFormatSize))
        {
            return true;
        }

        soundFormat = *waveFormatEx;
        CoTaskMemFree(waveFormatEx);


        readSoundInfo(pAsyncSourceReader, &soundFormat);



		HRESULT hr = S_OK;

        if (pSoundMix)
        {
			XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2];
			sendDescriptors[0].Flags = 0;
			sendDescriptors[0].pOutputVoice = pSoundMix->pSubmixVoice;
			sendDescriptors[1].Flags = 0;
			sendDescriptors[1].pOutputVoice = pSoundMix->pSubmixVoiceFX;

			const XAUDIO2_VOICE_SENDS sendList = { 2, sendDescriptors };

            hr = pAudioEngine->pXAudio2Engine->CreateSourceVoice(&pSourceVoice, &soundFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback, &sendList, NULL);
        }
        else
        {
            XAUDIO2_SEND_DESCRIPTOR sendDescriptors[1];
			sendDescriptors[0].Flags = 0;
			sendDescriptors[0].pOutputVoice = pAudioEngine->pMasteringVoice;

			const XAUDIO2_VOICE_SENDS sendList = { 1, sendDescriptors };

            hr = pAudioEngine->pXAudio2Engine->CreateSourceVoice(&pSourceVoice, &soundFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback, &sendList, NULL);
        }

        // create the source voice
        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"SSound::loadAudioFile::CreateSourceVoice() [async]");
            return true;
        }
    }

    if (soundInfo.iChannels != 2)
    {
		SError::showErrorMessageBox(L"SSound::loadAudioFile()", L"Unsupported channel format (expected 2 channels, received "
                                    + std::to_wstring(soundInfo.iChannels) + L" channels.");
        return true;
    }


    // Create source reader for readWaveData().
    WAVEFORMATEX* pWaveFormat;
    unsigned int iWaveSize;
    createSourceReader(sAudioFilePath, nullptr, pOptionalSourceReader, &pWaveFormat, iWaveSize, true);
    CoTaskMemFree(pWaveFormat);


    this->sAudioFileDiskPath = sAudioFilePath;


    if (bIs3DSound)
    {
        init3DSound();
    }


    bSoundLoaded = true;
    bUseStreaming = bStreamAudio;


    soundState = SS_NOT_PLAYING;

    return false;
}

bool SSound::playSound()
{
    if (bSoundLoaded == false)
    {
		SError::showErrorMessageBox(L"Sound::playSound()", L"no sound is loaded.");
        return true;
    }


    if (bCurrentlyStreaming && bUseStreaming)
    {
        bStopStreaming = true;
    }

    if (soundState == SS_PLAYING)
    {
        stopSound();
    }


    bStopStreaming = false;
    bSoundStoppedManually = false;
    bCurrentlyStreaming = false;

    dCurrentStreamingPosInSec = 0.0;
    iSamplesPlayedOnLastSetPos = 0;


    if (onPlayEndCallback)
    {
        // Start callback wait.
        std::thread t(&SSound::onPlayEnd, this);
        t.detach();
    }


    if (bUseStreaming)
    {
        std::thread t(&SSound::streamAudioFile, this, pAsyncSourceReader);
        t.detach();
        //streamAudioFile(pAsyncSourceReader);
    }
    else
    {
        audioBuffer.Flags = XAUDIO2_END_OF_STREAM; // OnStreamEnd is triggered when XAudio2 processes an XAUDIO2_BUFFER with the XAUDIO2_END_OF_STREAM flag set.

        // Submit the audio buffer to the source voice.
        // don't delete buffer until stop.
        HRESULT hr = pSourceVoice->SubmitSourceBuffer(&audioBuffer);
        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"Sound::playSound::SubmitSourceBuffer()");
            return true;
        }


        hr = pSourceVoice->Start();
        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"Sound::playSound::Start()");
            return true;
        }
    }


    soundState = SS_PLAYING;


    return false;
}

bool SSound::pauseSound()
{
    if (bSoundLoaded == false)
    {
		SError::showErrorMessageBox(L"Sound::pauseSound()", L"no sound is loaded.");
        return true;
    }

    if (soundState == SS_PAUSED || soundState == SS_NOT_PLAYING)
    {
        return false;
    }

    mtxSoundState.lock();

    soundState = SS_PAUSED;

    mtxSoundState.unlock();

    HRESULT hr = pSourceVoice->Stop();
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"Sound::pauseSound::Stop()");
        return true;
    }

    return false;
}

bool SSound::unpauseSound()
{
    if (bSoundLoaded == false)
    {
		SError::showErrorMessageBox(L"Sound::unpauseSound()", L"no sound is loaded.");
        return true;
    }

    if (soundState == SS_PLAYING || soundState == SS_NOT_PLAYING)
    {
        return false;
    }

    HRESULT hr = pSourceVoice->Start();
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"Sound::unpauseSound::Stop()");
        return true;
    }


    mtxSoundState.lock();

    soundState = SS_PLAYING;

    mtxSoundState.unlock();

    SetEvent(hEventUnpauseSound);

    return false;
}

bool SSound::stopSound()
{
    if (bSoundLoaded == false)
    {
		SError::showErrorMessageBox(L"Sound::stopSound()", L"no sound is loaded.");
        return true;
    }

    if (soundState == SS_NOT_PLAYING)
    {
        return false;
    }


    bSoundStoppedManually = true;

    if (onPlayEndCallback && bCalledOnPlayEnd == false)
    {
        SetEvent(voiceCallback.hStreamEnd);
    }


    if (pSourceVoice)
    {
        HRESULT hr = pSourceVoice->Stop();
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"Sound::stopSound::Stop()");
            return true;
        }
    }


    if (bCurrentlyStreaming && bUseStreaming)
    {
        bStopStreaming = true;

        SetEvent(voiceCallback.hBufferEndEvent);

        if (soundState == SS_PAUSED)
        {
            mtxSoundState.lock();

            soundState = SS_PLAYING;

            mtxSoundState.unlock();

            SetEvent(hEventUnpauseSound);
        }

        stopStreaming();
    }


    if (bUseStreaming)
    {
        // Restart the stream.
        sourceReaderCallback.restart();

        PROPVARIANT var = { 0 };
        var.vt = VT_I8;

        HRESULT hr = pAsyncSourceReader->SetCurrentPosition(GUID_NULL, var);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"Sound::playSound::SetCurrentPosition() [restart]");
            return true;
        }
    }

    dCurrentStreamingPosInSec = 0.0;


    pSourceVoice->FlushSourceBuffers();


    audioBuffer.PlayBegin = 0;


    soundState = SS_NOT_PLAYING;


    return false;
}

bool SSound::setPositionInSec(double dPositionInSec)
{
    if (bSoundLoaded == false)
    {
        SError::showErrorMessageBox(L"Sound::setPositionInSec()", L"no sound is loaded.");
        return true;
    }

    if ((dPositionInSec > soundInfo.dSoundLengthInSec) || (dPositionInSec < 0.0))
    {
        SError::showErrorMessageBox(L"Sound::setPositionInSec()", L"the specified position is invalid.");
        return true;
    }



    if (bUseStreaming)
    {
        LONGLONG pos = static_cast<LONGLONG>(dPositionInSec * 10000000);



        PROPVARIANT var;
        HRESULT hr = InitPropVariantFromInt64(pos, &var);
        if (SUCCEEDED(hr))
        {
            //dCurrentStreamingPosInSec = dPositionInSec;

            std::lock_guard<std::mutex> lock(mtxStreamingReadSampleSubmit);

            hr = pAsyncSourceReader->Flush((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM);
            if (FAILED(hr))
            {
                SError::showErrorMessageBox(hr, L"Sound::setPositionInSec::Flush()");
                return true;
            }

            hr = pAsyncSourceReader->SetCurrentPosition(GUID_NULL, var);
            PropVariantClear(&var);

            hr = pSourceVoice->Stop();
            if (FAILED(hr))
            {
                SError::showErrorMessageBox(hr, L"Sound::setPositionInSec::Stop()");
                return true;
            }

            hr = pSourceVoice->FlushSourceBuffers();
            if (FAILED(hr))
            {
                SError::showErrorMessageBox(hr, L"Sound::setPositionInSec::FlushSourceBuffers()");
                return true;
            }

            SetEvent(voiceCallback.hBufferEndEvent);

            hr = pSourceVoice->Start();
            if (FAILED(hr))
            {
                SError::showErrorMessageBox(hr, L"Sound::setPositionInSec::Start()");
                return true;
            }
        }
        else
        {
            SError::showErrorMessageBox(hr, L"Sound::setPositionInSec::SetCurrentPosition()");
            return true;
        }
    }
    else
    {
        HRESULT hr = pSourceVoice->Stop();
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"Sound::setPositionInSec::Stop()");
            return true;
        }

        hr = pSourceVoice->FlushSourceBuffers();
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"Sound::setPositionInSec::FlushSourceBuffers()");
            return true;
        }

        XAUDIO2_VOICE_STATE state;
        pSourceVoice->GetState(&state);

        iSamplesPlayedOnLastSetPos = state.SamplesPlayed;


        size_t iSampleCount = static_cast<size_t>(soundInfo.iSampleRate * soundInfo.dSoundLengthInSec);
        double dPercent = dPositionInSec / soundInfo.dSoundLengthInSec;

        audioBuffer.PlayBegin = static_cast<UINT32>(dPercent * iSampleCount);

        hr = pSourceVoice->SubmitSourceBuffer(&audioBuffer);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"Sound::setPositionInSec::SubmitSourceBuffer()");
            return true;
        }

        hr = pSourceVoice->Start();
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"Sound::setPositionInSec::Start()");
            return true;
        }
    }


    return false;
}

void SSound::setOnPlayEndCallback(std::function<void (SSound *)> f)
{
    onPlayEndCallback = f;
}

bool SSound::setVolume(float fVolume)
{
    if (bSoundLoaded == false)
    {
        SError::showErrorMessageBox(L"Sound::setVolume()", L"no sound is loaded.");
        return true;
    }


    HRESULT hr = pSourceVoice->SetVolume(fVolume);
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"Sound::setVolume::SetVolume()");
        return true;
    }


    return false;
}

bool SSound::setPitchInFreqRatio(float fRatio)
{
    if (bSoundLoaded == false)
    {
        SError::showErrorMessageBox(L"Sound::setPitchInFreqRatio()", L"no sound is loaded.");
        return true;
    }

    if (fRatio > 32.0f)
    {
        fRatio = 32.0f;
    }
    else if (fRatio < 0.03125f)
    {
        fRatio = 0.03125f;
    }

    HRESULT hr = pSourceVoice->SetFrequencyRatio(fRatio);
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"Sound::setPitchInFreqRatio::SetFrequencyRatio()");
        return true;
    }

    return false;
}

bool SSound::setPitchInSemitones(float fSemitones)
{
    if (bSoundLoaded == false)
    {
        SError::showErrorMessageBox(L"Sound::setPitchInFreqRation()", L"no sound is loaded.");
        return true;
    }

    if (fSemitones > 60.0f)
    {
        fSemitones = 60.0f;
    }
    else if (fSemitones < -60.0f)
    {
        fSemitones = -60.0f;
    }

    HRESULT hr = pSourceVoice->SetFrequencyRatio(powf(2.0f, fSemitones / 12.0f));
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"Sound::setPitchInSemitones::SetFrequencyRatio()");
        return true;
    }

    return false;
}

bool SSound::applyNew3DSoundProps(SEmitterProps &emitterProps, bool bCalcDopplerEffect)
{
    if (bIs3DSound)
    {
        if (bSoundLoaded == false)
        {
            SError::showErrorMessageBox(L"SSound::applyNew3DSoundProps()", L"The sound is not loaded.");
            return true;
        }

        X3DAUDIO_EMITTER emitter;
        float emitterAzimuths[XAUDIO2_MAX_AUDIO_CHANNELS] = {};
        std::memset(&emitter, 0, sizeof(X3DAUDIO_EMITTER));

        emitter.OrientFront.y = 1.f;

        emitter.OrientTop.z =
            emitter.ChannelRadius =
            emitter.CurveDistanceScaler =
            emitter.DopplerScaler = 1.f;

        emitter.ChannelCount = 1;
        emitter.pChannelAzimuths = emitterAzimuths;

        emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;


        emitterProps.position = X3DAUDIO_VECTOR(emitterProps.position.x, emitterProps.position.z, -emitterProps.position.y);
        emitterProps.velocity = X3DAUDIO_VECTOR(emitterProps.velocity.x, emitterProps.velocity.z, -emitterProps.velocity.y);
        //emitterProps.forwardVector = X3DAUDIO_VECTOR(emitterProps.forwardVector.x, emitterProps.forwardVector.z, -emitterProps.forwardVector.y);
        //emitterProps.topVector = X3DAUDIO_VECTOR(emitterProps.topVector.x, emitterProps.topVector.z, -emitterProps.topVector.y);

        //emitterProps.position = X3DAUDIO_VECTOR(emitterProps.position.x, emitterProps.position.y, emitterProps.position.z);
        //emitterProps.velocity = X3DAUDIO_VECTOR(emitterProps.velocity.x, emitterProps.velocity.y, emitterProps.velocity.z);


        emitter.Position = emitterProps.position;
        emitter.Velocity = emitterProps.velocity;


        UINT32 iFlags = X3DAUDIO_CALCULATE_MATRIX;
        if (bCalcDopplerEffect) iFlags |= X3DAUDIO_CALCULATE_DOPPLER;

        float matrix[XAUDIO2_MAX_AUDIO_CHANNELS * 8] = {};
        x3dAudioDSPSettings.pMatrixCoefficients = matrix;




        X3DAudioCalculate(pAudioEngine->hX3dAudio, &pAudioEngine->x3dAudioListener, &emitter,
                          iFlags,
                          &x3dAudioDSPSettings);


        x3dAudioDSPSettings.pMatrixCoefficients = nullptr;


        HRESULT hr = S_OK;


        if (bCalcDopplerEffect)
        {
            hr = pSourceVoice->SetFrequencyRatio(x3dAudioDSPSettings.DopplerFactor);
            if (FAILED(hr))
            {
                SError::showErrorMessageBox(hr, L"SSound::applyNew3DSoundProps::SetFrequencyRatio()");
                return true;
            }
        }


        if (pSoundMix)
        {
            hr = pSourceVoice->SetOutputMatrix(pSoundMix->pSubmixVoice,
                                               x3dAudioDSPSettings.SrcChannelCount,
                                               x3dAudioDSPSettings.DstChannelCount, matrix);
            hr = pSourceVoice->SetOutputMatrix(pSoundMix->pSubmixVoiceFX,
                                               x3dAudioDSPSettings.SrcChannelCount,
                                               x3dAudioDSPSettings.DstChannelCount, matrix);
        }
        else
        {
            hr = pSourceVoice->SetOutputMatrix(pAudioEngine->pMasteringVoice,
                                               x3dAudioDSPSettings.SrcChannelCount,
                                               x3dAudioDSPSettings.DstChannelCount, matrix);
        }

        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"SSound::applyNew3DSoundProps::SetOutputMatrix()");
            return true;
        }


        return false;
    }
    else
    {
        SError::showErrorMessageBox(L"SSound::applyNew3DSoundProps()", L"you can use this function only on a 3D sound (see SSound constructor).");
        return true;
    }
}

bool SSound::getVolume(float &fVolume)
{
    if (bSoundLoaded == false)
    {
        SError::showErrorMessageBox(L"Sound::getVolume()", L"no sound is loaded.");
        return true;
    }


    pSourceVoice->GetVolume(&fVolume);


    return false;
}

bool SSound::getSoundInfo(SSoundInfo &soundInfo)
{
    if (bSoundLoaded == false)
    {
        SError::showErrorMessageBox(L"Sound::getSoundFormat()", L"no sound is loaded.");
        return true;
    }

    soundInfo = this->soundInfo;

    return false;
}

bool SSound::getSoundState(SSoundState &state)
{
    if (bSoundLoaded == false)
    {
        SError::showErrorMessageBox(L"Sound::getSoundFormat()", L"no sound is loaded.");
        return true;
    }

    state = soundState;

    return false;
}

bool SSound::getPositionInSec(double &dPositionInSec)
{
    if (bSoundLoaded == false)
    {
        SError::showErrorMessageBox(L"Sound::getSoundFormat()", L"no sound is loaded.");
        return true;
    }


    if (bUseStreaming)
    {
        dPositionInSec = dCurrentStreamingPosInSec;
    }
    else
    {
        XAUDIO2_VOICE_STATE state;
        pSourceVoice->GetState(&state);

        size_t iSampleCount = static_cast<size_t>(soundInfo.iSampleRate * soundInfo.dSoundLengthInSec);

        double dPercent = 0.0;

        if (audioBuffer.PlayBegin != 0)
        {
            dPercent = (state.SamplesPlayed - iSamplesPlayedOnLastSetPos + audioBuffer.PlayBegin) / static_cast<double>(iSampleCount);
        }
        else
        {
            dPercent = (state.SamplesPlayed + audioBuffer.PlayBegin) / static_cast<double>(iSampleCount);
        }

        dPositionInSec = dPercent * soundInfo.dSoundLengthInSec;
    }


    return false;
}

bool SSound::getLoadedAudioDataSizeInBytes(size_t& iSizeInBytes)
{
    if (bSoundLoaded == false)
    {
        SError::showErrorMessageBox(L"Sound::getLoadedAudioDataSizeInBytes()", L"no sound is loaded.");
        return true;
    }


    if (bUseStreaming)
    {
        return true;
    }
    else
    {
        iSizeInBytes = vAudioData.size();
    }


    return false;
}

bool SSound::isSoundStoppedManually() const
{
    return bSoundStoppedManually;
}

bool SSound::readWaveData(std::vector<unsigned char>* pvWaveData, bool& bEndOfStream)
{
    std::lock_guard<std::mutex> lock(mtxOptionalSourceReaderRead);

    if (pOptionalSourceReader == nullptr)
    {
        return true;
    }


    Microsoft::WRL::ComPtr<IMFSample> pSample = nullptr;
    Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer = nullptr;
    unsigned char* pLocalAudioData = nullptr;
    DWORD iLocalAudioDataLength = 0;


    // Audio stream index.
    DWORD iStreamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;
    HRESULT hr = S_OK;


    DWORD flags = 0;
    hr = pOptionalSourceReader->ReadSample(iStreamIndex, 0, nullptr, &flags, nullptr, pSample.GetAddressOf());
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::readWaveData::ReadSample()");
        return true;
    }

    // Check for eof.
    if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
    {
        // Restart the stream.
        PROPVARIANT var = { 0 };
        var.vt = VT_I8;

        HRESULT hr = pOptionalSourceReader->SetCurrentPosition(GUID_NULL, var);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"Sound::readWaveData::SetCurrentPosition()");
            return true;
        }

        bEndOfStream = true;

        return false;
    }



    // Convert data to contiguous buffer.
    hr = pSample->ConvertToContiguousBuffer(pBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::readWaveData::ConvertToContiguousBuffer()");
        return true;
    }

    // Lock buffer and copy data to local memory.
    hr = pBuffer->Lock(&pLocalAudioData, nullptr, &iLocalAudioDataLength);
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::readWaveData::Lock()");
        return true;
    }

    for (size_t i = 0; i < iLocalAudioDataLength; i++)
    {
        pvWaveData->push_back(pLocalAudioData[i]);
    }

    // Unlock the buffer.
    hr = pBuffer->Unlock();
    pLocalAudioData = nullptr;

    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::readWaveData::Unlock()");
        return true;
    }


    return false;
}

void SSound::init3DSound()
{
    XAUDIO2_VOICE_DETAILS masterVoiceDetails;
    pAudioEngine->pMasteringVoice->GetVoiceDetails(&masterVoiceDetails);

    memset(&x3dAudioDSPSettings, 0, sizeof(X3DAUDIO_DSP_SETTINGS));
    x3dAudioDSPSettings.SrcChannelCount = soundInfo.iChannels;
    x3dAudioDSPSettings.DstChannelCount = masterVoiceDetails.InputChannels;
}

void SSound::clearSound()
{
    if (bSoundLoaded)
    {
        stopSound();


        sAudioFileDiskPath = L"";

        if (pSourceVoice)
        {
            pSourceVoice->DestroyVoice();
        }

        vAudioData.clear();
        vSpeedChangedAudioData.clear();

        if (pAsyncSourceReader)
        {
            pAsyncSourceReader->Release();
            pAsyncSourceReader = nullptr;
        }

        mtxOptionalSourceReaderRead.lock();
        if (pOptionalSourceReader)
        {
            pOptionalSourceReader->Release();
            pOptionalSourceReader = nullptr;
        }
        mtxOptionalSourceReaderRead.unlock();
    }
}

void SSound::stopStreaming()
{
    if (bUseStreaming)
    {
        bStopStreaming = true;

        mtxStreamingSwitch.lock();

        if (bCurrentlyStreaming)
        {
            std::future<bool> future = promiseStreaming.get_future();
            mtxStreamingSwitch.unlock();
            future.get();
        }
        else
        {
            mtxStreamingSwitch.unlock();
        }
    }
}

bool SSound::readSoundInfo(IMFSourceReader* pSourceReader, WAVEFORMATEX* pFormat)
{
    soundInfo.iChannels      = pFormat->nChannels;
    soundInfo.iSampleRate    = pFormat->nSamplesPerSec;
    soundInfo.iBitsPerSample = pFormat->wBitsPerSample;


    // Get audio length.

    LONGLONG duration; // in 100-nanosecond units

    PROPVARIANT var;
    HRESULT hr = pSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"SSound::readSoundInfo::GetPresentationAttribute() [duration]");
        return true;
    }

    hr = PropVariantToInt64(var, &duration);
    PropVariantClear(&var);

    soundInfo.dSoundLengthInSec = duration / 10000000.0;




    // Get audio bitrate.

    LONG bitrate;

    hr = pSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_AUDIO_ENCODING_BITRATE, &var);
    if (FAILED(hr))
    {
        // Not critical (may fail on .ogg).

        //SError::showErrorMessageBox(hr, L"SSound::readSoundInfo::GetPresentationAttribute() [bitrate]");
        //return true;

        soundInfo.iBitrate = 0;
    }
    else
    {
        hr = PropVariantToInt32(var, &bitrate);
        PropVariantClear(&var);


        soundInfo.iBitrate = static_cast<unsigned int>(bitrate);
    }



    // Get VBR.

    LONG vbr;

    hr = pSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_AUDIO_ISVARIABLEBITRATE, &var);
    if (FAILED(hr))
    {
        // Not critical (may fail on .wav).

        //SError::showErrorMessageBox(hr, L"SSound::readSoundInfo::GetPresentationAttribute() [vbr]");
        //return true;

        soundInfo.bUsesVariableBitRate = false;
    }
    else
    {
        hr = PropVariantToInt32(var, &vbr);
        PropVariantClear(&var);

        soundInfo.bUsesVariableBitRate = static_cast<bool>(vbr);
    }



    // Get file size.

    LONGLONG size;

    hr = pSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_TOTAL_FILE_SIZE, &var);
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"SSound::readSoundInfo::GetPresentationAttribute() [size]");
        return true;
    }

    hr = PropVariantToInt64(var, &size);
    PropVariantClear(&var);


    soundInfo.iFileSizeInBytes = static_cast<unsigned long long>(size);



    return false;
}

bool SSound::loadFileIntoMemory(const std::wstring &sAudioFilePath, std::vector<unsigned char> &vAudioData, WAVEFORMATEX **pFormat, unsigned int &iWaveFormatSize)
{
    if (pAudioEngine->bEngineInitialized == false)
    {
        SError::showErrorMessageBox(L"AudioEngine::loadFileIntoMemory()", L"the audio engine is not initialized.");
        return true;
    }


    // Check if the file exists.

    std::ifstream file(sAudioFilePath, std::ios::binary);
    if (file.is_open())
    {
        file.close();
    }
    else
    {
        SError::showErrorMessageBox(L"AudioEngine::loadFileIntoMemory()", L"the specified file (" + sAudioFilePath +L") does not exist.");
        return true;
    }


    IMFSourceReader* pSourceReaderOut;
    createSourceReader(sAudioFilePath, nullptr, pSourceReaderOut, pFormat, iWaveFormatSize);

    readSoundInfo(pSourceReaderOut, *pFormat);

    Microsoft::WRL::ComPtr<IMFSourceReader> pSourceReader(pSourceReaderOut);


    // Copy data into byte vector.

    Microsoft::WRL::ComPtr<IMFSample> pSample = nullptr;
    Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer = nullptr;
    unsigned char* pLocalAudioData = NULL;
    DWORD iLocalAudioDataLength = 0;


    // Audio stream index.
    DWORD iStreamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;
    HRESULT hr = S_OK;

    while (true)
    {
        DWORD flags = 0;
        hr = pSourceReader->ReadSample(iStreamIndex, 0, nullptr, &flags, nullptr, pSample.GetAddressOf());
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::loadFileIntoMemory::ReadSample()");
            return true;
        }

        // Check whether the data is still valid.
        if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
        {
            break;
        }

        // Check for eof.
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            break;
        }

        if (pSample == nullptr)
        {
            continue;
        }

        // Convert data to contiguous buffer.
        hr = pSample->ConvertToContiguousBuffer(pBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::loadFileIntoMemory::ConvertToContiguousBuffer()");
            return true;
        }

        // Lock buffer and copy data to local memory.
        hr = pBuffer->Lock(&pLocalAudioData, nullptr, &iLocalAudioDataLength);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::loadFileIntoMemory::Lock()");
            return true;
        }

        for (size_t i = 0; i < iLocalAudioDataLength; i++)
        {
            vAudioData.push_back(pLocalAudioData[i]);
        }

        // Unlock the buffer.
        hr = pBuffer->Unlock();
        pLocalAudioData = nullptr;

        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::loadFileIntoMemory::Unlock()");
            return true;
        }
    }

    pSourceReader.Reset();

    return false;
}

bool SSound::createAsyncReader(const std::wstring &sAudioFilePath, IMFSourceReader *&pSourceReader, WAVEFORMATEX **pFormat, unsigned int &iWaveFormatSize)
{
    if (pAudioEngine->bEngineInitialized == false)
    {
        SError::showErrorMessageBox(L"SSound::createAsyncReader()", L"the audio engine is not initialized.");
        return true;
    }


    // Check if the file exists.

    std::ifstream file(sAudioFilePath, std::ios::binary);
    if (file.is_open())
    {
        file.close();
    }
    else
    {
        SError::showErrorMessageBox(L"SSound::loadFileIntoMemory()", L"the specified file (" + sAudioFilePath + L") does not exist.");
        return true;
    }


    SourceReaderCallback* pCallback = &sourceReaderCallback;
    createSourceReader(sAudioFilePath, &pCallback, pSourceReader, pFormat, iWaveFormatSize);


    return false;
}

bool SSound::streamAudioFile(IMFSourceReader *pAsyncReader)
{
    mtxStreamingSwitch.lock();

    if (bStopStreaming)
    {
        return false;
    }

    bCurrentlyStreaming = true;

    mtxStreamingSwitch.unlock();


    promiseStreaming = std::promise<bool>();


    DWORD iStreamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;


    pSourceVoice->Start();


    if (loopStream(pAsyncReader, pSourceVoice))
    {
        mtxStreamingSwitch.lock();
        bCurrentlyStreaming = false;
        mtxStreamingSwitch.unlock();
        promiseStreaming.set_value(false);

        return true;
    }


    pAsyncReader->Flush(iStreamIndex);

    pSourceVoice->Stop();


    mtxStreamingSwitch.lock();
    bCurrentlyStreaming = false;
    mtxStreamingSwitch.unlock();
    promiseStreaming.set_value(false);



    return false;
}

bool SSound::loopStream(IMFSourceReader *pAsyncReader, IXAudio2SourceVoice *pSourceVoice)
{
    DWORD streamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;
    HRESULT hr = S_OK;

    DWORD iCurrentStreamBufferIndex = 0;

    while(true)
    {
        if (bStopStreaming)
        {
            // Exit.
            break;
        }


        if (waitForUnpause())
        {
            break;
        }


        mtxStreamingReadSampleSubmit.lock();

        hr = pAsyncReader->ReadSample(streamIndex, 0, nullptr, nullptr, nullptr, nullptr);
        if (FAILED(hr))
        {
            mtxStreamingReadSampleSubmit.unlock();

            if (hr == MF_E_NOTACCEPTING)
            {
                continue;
            }

            SError::showErrorMessageBox(hr, L"AudioEngine::loopStream::ReadSample()");
            return true;
        }

        WaitForSingleObject(sourceReaderCallback.hReadSampleEvent, INFINITE);

        mtxStreamingReadSampleSubmit.unlock();


        dCurrentStreamingPosInSec = sourceReaderCallback.llTimestamp / 10000000.0;

        if (sourceReaderCallback.bIsEndOfStream)
        {
            // Notify about onPlayEnd.

            XAUDIO2_VOICE_STATE state;
            pSourceVoice->GetState(&state);
            while(state.BuffersQueued > 0)
            {
                WaitForSingleObject(voiceCallback.hBufferEndEvent, INFINITE);

                pSourceVoice->GetState(&state);
            }

            if (onPlayEndCallback)
            {
                SetEvent(voiceCallback.hStreamEnd);
            }

            break;
        }



        // Sample to contiguous buffer.

        Microsoft::WRL::ComPtr<IMFMediaBuffer> pMediaBuffer;
        hr = sourceReaderCallback.sample->ConvertToContiguousBuffer(pMediaBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::loopStream::ConvertToContiguousBuffer()");
            return true;
        }



        // Read buffer.

        unsigned char* pAudioData = nullptr;
        DWORD iSampleBufferSize = 0;

        hr = pMediaBuffer->Lock(&pAudioData, nullptr, &iSampleBufferSize);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::loopStream::Lock()");
            return true;
        }



        // Recreate proper buffer if needed.

        if (vSizeOfBuffers[iCurrentStreamBufferIndex] < iSampleBufferSize)
        {
            vBuffers[iCurrentStreamBufferIndex].reset(new uint8_t[iSampleBufferSize]);
            vSizeOfBuffers[iCurrentStreamBufferIndex] = iSampleBufferSize;
        }



        // Copy data to our buffer.

        std::memcpy(vBuffers[iCurrentStreamBufferIndex].get(), pAudioData, iSampleBufferSize);


        hr = pMediaBuffer->Unlock();
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::loopStream::Unlock()");
            return true;
        }



        // Wait until there is 'iMaxBufferDuringStreaming - 1' buffers in the queue (leave 1 buffer for reader).

        XAUDIO2_VOICE_STATE state;
        while(true)
        {
            pSourceVoice->GetState(&state);

            if (state.BuffersQueued < iMaxBufferDuringStreaming - 1)
            {
                break;
            }

            WaitForSingleObject(voiceCallback.hBufferEndEvent, INFINITE);

            if (waitForUnpause())
            {
                return false;
            }
        }



        // Play audio.

        XAUDIO2_BUFFER buf = { 0 };
        buf.AudioBytes = iSampleBufferSize;
        buf.pAudioData = vBuffers[iCurrentStreamBufferIndex].get();

        mtxStreamingReadSampleSubmit.lock();
        pSourceVoice->SubmitSourceBuffer(&buf);
        mtxStreamingReadSampleSubmit.unlock();



        // Next buffer.

        iCurrentStreamBufferIndex++;
        iCurrentStreamBufferIndex %= iMaxBufferDuringStreaming;
    }

    return false;
}

bool SSound::createSourceReader(const std::wstring &sAudioFilePath, SourceReaderCallback** pAsyncSourceReaderCallback,
                                IMFSourceReader *& pOutSourceReader, WAVEFORMATEX **pFormat, unsigned int &iWaveFormatSize, bool bOptional)
{
    HRESULT hr = S_OK;

    if (pAsyncSourceReaderCallback)
    {
        // Set the source reader to asyncronous mode.
        hr = pSourceReaderConfig->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, *pAsyncSourceReaderCallback);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::SetUnknown() [async]");
            return true;
        }
    }

    if (bOptional)
    {
        hr = MFCreateSourceReaderFromURL(sAudioFilePath.c_str(), pOptionalSourceReaderConfig.Get(), &pOutSourceReader);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::MFCreateSourceReaderFromURL()");
            return true;
        }
    }
    else
    {
        hr = MFCreateSourceReaderFromURL(sAudioFilePath.c_str(), pSourceReaderConfig.Get(), &pOutSourceReader);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::MFCreateSourceReaderFromURL()");
            return true;
        }
    }


    // Disable all other streams except for the audio stream (read only from audio stream).

    // Disable all streams.
    hr = pOutSourceReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, false);
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::SetStreamSelection()");
        return true;
    }

    // Audio stream index.
    DWORD iStreamIndex = (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM;

    // Enable audio stream.
    hr = pOutSourceReader->SetStreamSelection(iStreamIndex, true);
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::SetStreamSelection()");
        return true;
    }



    // Query information about the media file.
    Microsoft::WRL::ComPtr<IMFMediaType> pNativeMediaType;
    hr = pOutSourceReader->GetNativeMediaType(iStreamIndex, 0, pNativeMediaType.GetAddressOf());
    if(FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::GetNativeMediaType()");
        return true;
    }


    // Make sure that this is really an audio file.
    GUID majorType{};
    hr = pNativeMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
    if (majorType != MFMediaType_Audio)
    {
        SError::showErrorMessageBox(L"AudioEngine::createSourceReader::GetGUID() [MF_MT_MAJOR_TYPE]", L"the requested file is not an audio file.");
        return true;
    }

    // Check whether the audio file is compressed.
    GUID subType{};
    hr = pNativeMediaType->GetGUID(MF_MT_MAJOR_TYPE, &subType);
    if (subType != MFAudioFormat_Float && subType != MFAudioFormat_PCM)
    {
        // The audio file is compressed. We have to decompress it first.
        // Tell the source reader to decompress it for us. Create the media type for that.

        Microsoft::WRL::ComPtr<IMFMediaType> pPartialType = nullptr;
        hr = MFCreateMediaType(pPartialType.GetAddressOf());
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::MFCreateMediaType()");
            return true;
        }

        // Set the media type to "audio".
        hr = pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::SetGUID() [MF_MT_MAJOR_TYPE]");
            return true;
        }

        // Request uncompressed audio data.
        hr = pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::SetGUID() [MF_MT_SUBTYPE]");
            return true;
        }

        // Submit our request to the source reader.

        hr = pOutSourceReader->SetCurrentMediaType(iStreamIndex, NULL, pPartialType.Get());
        if (FAILED(hr))
        {
            SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::SetCurrentMediaType()");
            return true;
        }
    }


    // Uncompress the data.
    Microsoft::WRL::ComPtr<IMFMediaType> pUncompressedAudioType = nullptr;
    hr = pOutSourceReader->GetCurrentMediaType(iStreamIndex, pUncompressedAudioType.GetAddressOf());
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::GetCurrentMediaType()");
        return true;
    }

    hr = MFCreateWaveFormatExFromMFMediaType(pUncompressedAudioType.Get(), pFormat, &iWaveFormatSize);
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::MFCreateWaveFormatExFromMFMediaType()");
        return true;
    }


    // Ensure the stream is selected.
    hr = pOutSourceReader->SetStreamSelection(iStreamIndex, true);
    if (FAILED(hr))
    {
        SError::showErrorMessageBox(hr, L"AudioEngine::createSourceReader::SetStreamSelection()");
        return true;
    }


    return false;
}

bool SSound::waitForUnpause()
{
    mtxSoundState.lock();
    if (soundState == SS_PAUSED)
    {
        mtxSoundState.unlock();

        WaitForSingleObject(hEventUnpauseSound, INFINITE);
    }
    else
    {
        mtxSoundState.unlock();
    }


    if (bStopStreaming)
    {
        // Exit.
        return true;
    }

    return false;
}

void SSound::onPlayEnd()
{
    do
    {
       WaitForSingleObject(voiceCallback.hStreamEnd, INFINITE);

       if (bDestroyCalled) return;

       if (bSoundStoppedManually) break;

       if (bUseStreaming == false)
       {
           break;
       }
       else if (sourceReaderCallback.bIsEndOfStream)
       {
           break;
       }
    }while(true);


    bCalledOnPlayEnd = true;
    onPlayEndCallback(this);
    bCalledOnPlayEnd = false;
}
