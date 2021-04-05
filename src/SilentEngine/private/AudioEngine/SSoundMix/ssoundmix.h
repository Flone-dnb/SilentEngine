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

class SSoundMix
{

public:


    bool setVolume(float fVolume);
    bool setFXVolume(float fFXVolume);


    // Effects
    // will reset all previous effects, pass empty vector to clear effects
    bool setAudioEffects           (std::vector<SAudioEffect>* pvEffects);
    bool setEnableAudioEffect      (unsigned int iEffectIndex, bool bEnable);
    bool setAudioEffectParameters  (unsigned int iEffectIndex, SAudioEffect* params);


    void getVolume(float& fVolume);
    void getFXVolume(float& fFXVolume);



    ~SSoundMix();

private:

    SSoundMix(SAudioEngine* pAudioEngine);

    bool init();

    friend class SAudioEngine;
    friend class SSound;


    class IXAudio2SubmixVoice* pSubmixVoice;
    class IXAudio2SubmixVoice* pSubmixVoiceFX;

    SAudioEngine* pAudioEngine;


    std::vector<bool> vEnabledEffects;


    float fFXVolume = 1.0f;


    bool bEffectsSet;
};
