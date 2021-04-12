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

//@@Class
/*
This class is used for playing an audio files in 2D or 3D.
*/
class SSound
{
public:

    SSound(SAudioEngine* pAudioEngine, bool bIs3DSound, SAudioComponent* pOwnerComponent);
    ~SSound();

	//@@Function
	/*
	* desc: use to load/reload an audio file to play.
	* param "sAudioFilePath": path to the audio file. Supported audio formats are: .wav, .mp3, .ogg.
	* param "bStreamAudio": if set to 'false' the whole audio file will be uncompressed and
	loaded in the RAM (you can use getLoadedAudioDataSizeInBytes() to see the size),
	if set to 'true' the sound will be loaded in small chunks while playing and thus
	keeping the amount of used memory very low. Note that there are some cons for using streaming because the sound
	that is being streamed will apply operations (such as pause/unpause/stop and etc.) with a delay and
	the setPosition/getPosition functions will not be that precise. The general rule is this: only use streaming if the
	sound is very long (4-5 min. and longer).
	* param "pOutputToSoundMix": (optional) pass the SSoundMix (that can be created using
	SApplication::getAudioEngine()->createSoundMix()) to route this sound to a custom sound mix (i.e. mixer channel).
	Sound mixes are used for sound grouping and applying the audio effects, for example, all UI sounds may use one
	SSoundMix and this SSoundMix can be used to control overall volume of all UI sounds.
	* return: true if an error occurred, false otherwise.
	* remarks: if this function is called not the first time and there was a sound mix used pass it again,
	otherwise the sound will use master channel.
	*/
    bool loadAudioFile(const std::wstring& sAudioFilePath, bool bStreamAudio, SSoundMix* pOutputToSoundMix = nullptr);


	//@@Function
	/*
	* desc: use to start playing the sound.
	* return: true if an error occurred, false otherwise.
	* remarks: will restart the sound if it's already playing and unpause if paused.
	*/
    bool playSound    ();
	//@@Function
	/*
	* desc: pauses the sound at current position.
	* return: true if an error occurred, false otherwise.
	*/
    bool pauseSound   ();
	//@@Function
	/*
	* desc: unpauses the sound.
	* return: true if an error occurred, false otherwise.
	*/
    bool unpauseSound ();
	//@@Function
	/*
	* desc: stops the sound.
	* return: true if an error occurred, false otherwise.
	*/
    bool stopSound    ();
	//@@Function
	/*
	* desc: use to change the current playing position.
	* return: true if an error occurred, false otherwise.
	*/
    bool setPositionInSec (double dPositionInSec);


	//@@Function
	/*
	* desc: sets the volume of this sound.
	* return: true if an error occurred, false otherwise.
	*/
    bool setVolume        (float fVolume);
	//@@Function
	/*
	* desc: used to set the pitch in freq. ratio, valid range is [0.03125, 32] so it's [-5, 5] octaves.
	* return: true if an error occurred, false otherwise.
	*/
    bool setPitchInFreqRatio(float fRatio);
	//@@Function
	/*
	* desc: used to set the pitch in semitones, valid range is [-60, 60] so it's [-5, 5] octaves.
	* return: true if an error occurred, false otherwise.
	*/
    bool setPitchInSemitones(float fSemitones);

	//@@Function
	/*
	* desc: used to set the properties of this 3D sound.
	* remarks: will show an error if used on a 2D sound.
	*/
	void set3DSoundProps  (const S3DSoundProps& props);

	//@@Function
	/*
	* desc: used to set the callback that will be called when the SSound is finished playing the sound,
	or if the sound was stopped (use isSoundStoppedManually() to see the reason).
	*/
    void setOnPlayEndCallback(std::function<void(SSound*)> f);

	//@@Function
	/*
	* desc: used to retrieve the current volume.
	* return: true if an error occurred, false otherwise.
	*/
    bool getVolume        (float& fVolume);
	//@@Function
	/*
	* desc: used to retrieve the sound info (file size on disk, sound length, bitrate and etc.).
	* return: true if an error occurred, false otherwise.
	*/
    bool getSoundInfo     (SSoundInfo& soundInfo);
	//@@Function
	/*
	* desc: used to retrieve the sound state (not playing, playing, stopped and etc.).
	* return: true if an error occurred, false otherwise.
	*/
    bool getSoundState    (SSoundState& state);
	//@@Function
	/*
	* desc: used to retrieve the current sound position.
	* return: true if an error occurred, false otherwise.
	*/
    bool getPositionInSec (double& dPositionInSec);
	//@@Function
	/*
	* desc: used to retrieve the size of the loaded sound in RAM (only used without streaming).
	* return: true if an error occurred, false otherwise.
	* remarks: returns valid value only if not using streaming.
	*/
    bool getLoadedAudioDataSizeInBytes(size_t& iSizeInBytes);
	//@@Function
	/*
	* desc: tells if the sound was stopped manually (using stopSound()).
	* return: true if an error occurred, false otherwise.
	*/
    bool isSoundStoppedManually() const;
	//@@Function
	/*
	* desc: use to read raw PCM audio samples (use in cycle until bEndOfStream is true).
	* return: true if an error occurred, false otherwise.
	*/
    bool readWaveData(std::vector<unsigned char>* pvWaveData, bool& bEndOfStream);

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
