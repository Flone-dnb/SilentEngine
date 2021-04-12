// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>
#include <string>
#include <functional>
#include <future>

// XAudio2
#include <xaudio2.h>
#include <xaudio2fx.h>

// Custom
#include "../SAudioEngine/saudioengine.h"


class SAudioEngine;
class SSoundMix;
class SAudioComponent;

enum class SSoundState
{
    SS_NOT_PLAYING = 0,
    SS_PLAYING = 1,
    SS_PAUSED = 2
};


struct SSoundInfo
{
    unsigned long long iFileSizeInBytes;
    double         dSoundLengthInSec;
    unsigned int   iBitrate;
    unsigned short iChannels;
    unsigned long  iSampleRate;
    unsigned short iBitsPerSample;
    bool           bUsesVariableBitRate;
};

struct SEmitterProps
{
    X3DAUDIO_VECTOR position;
    X3DAUDIO_VECTOR velocity; // in units per sec. (only for doppler)
};

struct S3DSoundProps
{
	float fSoundAttenuationMultiplier = 1.0f; // also can affect vCustomVolumeCurve (all distance values will be multiplied by this value)
	// example: if distance (0 - 10) use volume (1.0f), then on distance (10 - 20) change volume to (0.0f):
	// X3DAUDIO_DISTANCE_CURVE_POINT point1, point2;
	// point1.Distance = 10.0f;
	// point1.DSPSetting = 1.0f;
	// point2.Distance = 20.0f;
	// point2.DSPSetting = 0.0f;
	// props.vCustomVolumeCurve.push_back(point1);
	// props.vCustomVolumeCurve.push_back(point2);
	std::vector<X3DAUDIO_DISTANCE_CURVE_POINT> vCustomVolumeCurve;
};

class SSound
{
public:

    SSound(SAudioEngine* pAudioEngine, bool bIs3DSound, SAudioComponent* pOwnerComponent);
    ~SSound();


    // Resets all sound effects (audio effects (reverb, echo...), volume, pan, pitch, etc.)
	// create new sound mixes using SApplication::getAudioEngine() and SAudioEngine::createSoundMix()
	// one sound mix can be used for multiple sounds, don't create sound mix per sound
	// consider sound mix as a mixer channel (like master channel)
	// all sound mixes pass the sound to the master channel (controlled by SAudioEngine)
	// TODO: what audio formats do we support?
    bool loadAudioFile(const std::wstring& sAudioFilePath, bool bStreamAudio, SSoundMix* pOutputToSoundMix = nullptr);


    // will restart the sound if playing, unpause if paused
    bool playSound    ();
    bool pauseSound   ();
    bool unpauseSound ();
    bool stopSound    ();
    bool setPositionInSec (double dPositionInSec);


    bool setVolume        (float fVolume);
    // [0.03125, 32] so [-5, 5] octaves
    bool setPitchInFreqRatio(float fRatio);
    // [-5, 5] octaves so [-60, 60]
    bool setPitchInSemitones(float fSemitones);


	void set3DSoundProps  (const S3DSoundProps& props);


    void setOnPlayEndCallback(std::function<void(SSound*)> f);


    bool getVolume        (float& fVolume);
    bool getSoundInfo     (SSoundInfo& soundInfo);
    bool getSoundState    (SSoundState& state);
    bool getPositionInSec (double& dPositionInSec);
    // returns valid value only if not using streaming
    bool getLoadedAudioDataSizeInBytes(size_t& iSizeInBytes);
    bool isSoundStoppedManually() const;

    bool readWaveData     (std::vector<unsigned char>* pvWaveData, bool& bEndOfStream);

private:

	friend class SAudioEngine;

    void init3DSound();
    bool loadFileIntoMemory(const std::wstring& sAudioFilePath, std::vector<unsigned char>& vAudioData, WAVEFORMATEX** pFormat, unsigned int& iWaveFormatSize);

    bool createAsyncReader(const std::wstring& sAudioFilePath, IMFSourceReader*& pSourceReader, WAVEFORMATEX** pFormat, unsigned int& iWaveFormatSize);
    bool streamAudioFile(IMFSourceReader* pAsyncReader);
    bool loopStream(IMFSourceReader* pAsyncReader, IXAudio2SourceVoice* pSourceVoice);

    bool createSourceReader(const std::wstring& sAudioFilePath, SourceReaderCallback** pAsyncSourceReaderCallback,
                            IMFSourceReader*& pOutSourceReader, WAVEFORMATEX** pFormat, unsigned int& iWaveFormatSize, bool bOptional = false);

	// update listener first
	bool applyNew3DSoundProps(SEmitterProps& emitterProps);

    bool waitForUnpause();


    void onPlayEnd();


    void clearSound();
    void stopStreaming();
    bool readSoundInfo(IMFSourceReader* pSourceReader, WAVEFORMATEX* pFormat);





    SAudioEngine*         pAudioEngine;
    IXAudio2SourceVoice*  pSourceVoice;
    SSoundMix*            pSoundMix;
	SAudioComponent*      pOwnerComponent;


	S3DSoundProps         sound3DProps;


    X3DAUDIO_DSP_SETTINGS x3dAudioDSPSettings;


    Microsoft::WRL::ComPtr<IMFAttributes> pSourceReaderConfig;
    Microsoft::WRL::ComPtr<IMFAttributes> pOptionalSourceReaderConfig;


    // User callbacks.
    std::function<void(SSound*)> onPlayEndCallback;


    IMFSourceReader*       pOptionalSourceReader;

    IMFSourceReader*       pAsyncSourceReader;
    SourceReaderCallback   sourceReaderCallback;
    VoiceCallback          voiceCallback;
    bool bStopStreaming = false;
    static const int iMaxBufferDuringStreaming = 3; // see XAUDIO2_MAX_QUEUED_BUFFERS


    // Used in sync mode.
    std::vector<unsigned char> vAudioData;
    std::vector<unsigned char> vSpeedChangedAudioData;
    // Used in async mode (streaming).
    size_t vSizeOfBuffers[iMaxBufferDuringStreaming] = { 0 };
    std::unique_ptr<uint8_t[]> vBuffers[iMaxBufferDuringStreaming];


    std::promise<bool> promiseStreaming;


    HANDLE         hEventUnpauseSound;


    std::mutex     mtxStreamingSwitch;
    std::mutex     mtxSoundState;
    std::mutex     mtxOptionalSourceReaderRead;
    std::mutex     mtxStreamingReadSampleSubmit;


    std::wstring   sAudioFileDiskPath;


	std::mutex     mtxUpdate3DSound;


    XAUDIO2_BUFFER audioBuffer;
    WAVEFORMATEX   soundFormat;
    unsigned int   iWaveFormatSize;
    double         dCurrentStreamingPosInSec;
    unsigned long long iSamplesPlayedOnLastSetPos;
    size_t         iLastReadSampleSize;

    SSoundInfo     soundInfo;

    SSoundState    soundState;

    size_t         iCurrentEffectIndex;

    bool           bIs3DSound;
    bool           bUseStreaming;
    bool           bCurrentlyStreaming;
    bool           bSoundLoaded;
    bool           bSoundStoppedManually;
    bool           bCalledOnPlayEnd;
    bool           bDestroyCalled;
    bool           bEffectsSet;
};
