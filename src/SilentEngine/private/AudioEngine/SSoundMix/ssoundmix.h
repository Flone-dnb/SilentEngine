// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// STL
#include <vector>

class SAudioEngine;
class SAudioEffect;
class IXAudio2SubmixVoice;

//@@Class
/*
This class is only used for sound grouping or applying audio effects.
*/
class SSoundMix
{

public:


	//@@Function
	/*
	* desc: sets the volume of this SSoundMix (i.e. mixer channel) and thus controls
	the volume of all sounds that use this SSoundMix.
	* return: true if an error occurred, false otherwise.
	*/
    bool setVolume(float fVolume);
	//@@Function
	/*
	* desc: sets the output volume of this SSoundMix's effect chain (if effects are used).
	* return: true if an error occurred, false otherwise.
	*/
    bool setFXVolume(float fFXVolume);


	//@@Function
	/*
	* desc: sets the audio effects chain for this SSoundMix, so all sounds that use this SSoundMix
	will have the specified audio effects, note that the order of the effects matters.
	* return: true if an error occurred, false otherwise.
	* remarks: calling this function will reset all previous effects, pass empty vector to clear effects.
	*/
    bool setAudioEffects           (std::vector<SAudioEffect>* pvEffects);
	//@@Function
	/*
	* desc: use to enable/disable an audio effect.
	* return: true if an error occurred, false otherwise.
	*/
    bool setEnableAudioEffect      (unsigned int iEffectIndex, bool bEnable);
	//@@Function
	/*
	* desc: use to set/override the parameters of the audio effect.
	* return: true if an error occurred, false otherwise.
	*/
    bool setAudioEffectParameters  (unsigned int iEffectIndex, SAudioEffect* params);


	//@@Function
	/*
	* desc: use to get the volume of this SSoundMix.
	*/
    void getVolume(float& fVolume);
	//@@Function
	/*
	* desc: use to get the volume of this SSoundMix.
	*/
    void getFXVolume(float& fFXVolume);



    ~SSoundMix();

private:

    SSoundMix(SAudioEngine* pAudioEngine);

    bool init();

    friend class SAudioEngine;
    friend class SSound;


    IXAudio2SubmixVoice* pSubmixVoice;
    IXAudio2SubmixVoice* pSubmixVoiceFX;

    SAudioEngine* pAudioEngine;


    std::vector<bool> vEnabledEffects;


    float fFXVolume = 1.0f;


    bool bEffectsSet;
};
