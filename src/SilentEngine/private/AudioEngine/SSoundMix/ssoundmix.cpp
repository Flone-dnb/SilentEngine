// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "ssoundmix.h"

// Engine
#include "SilentEngine/Private/SError/serror.h"

// Custom
#include "../SAudioEngine/saudioengine.h"

SSoundMix::SSoundMix(SAudioEngine* pAudioEngine)
{
    this->pAudioEngine = pAudioEngine;

    pSubmixVoice = nullptr;
    pSubmixVoiceFX = nullptr;

    bEffectsSet = false;
}

bool SSoundMix::init()
{
    HRESULT hr = pAudioEngine->pXAudio2Engine->CreateSubmixVoice(&pSubmixVoice, 2, 44100, 0, 0, 0, 0);
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"SSoundMix::init::CreateSubmixVoice()");
        return true;
    }

    hr = pAudioEngine->pXAudio2Engine->CreateSubmixVoice(&pSubmixVoiceFX, 2, 44100, 0, 0, 0, 0);
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"SSoundMix::init::CreateSubmixVoice() [fx]");
        return true;
    }

    pSubmixVoiceFX->SetVolume(0.0f);

    return false;
}

bool SSoundMix::setVolume(float fVolume)
{
    HRESULT hr = pSubmixVoice->SetVolume(fVolume);
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"SSoundMix::setVolume::SetVolume()");
        return true;
    }


    return false;
}

bool SSoundMix::setFXVolume(float fFXVolume)
{
    this->fFXVolume = fFXVolume;

    if (bEffectsSet)
    {
        HRESULT hr = pSubmixVoiceFX->SetVolume(fFXVolume);
        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"SSoundMix::setFXVolume::SetVolume()");
            return true;
        }
    }

	return false;
}

bool SSoundMix::setAudioEffects(std::vector<SAudioEffect> *pvEffects)
{
    if (pvEffects->size() == 0)
    {
        return true;
    }

    if (bEffectsSet)
    {
        // remove prev chain
        HRESULT hr = pSubmixVoiceFX->SetEffectChain(NULL);
        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"SSoundMix::setAudioEffects::SetEffectChain() [reset]");
            return true;
        }

        pSubmixVoiceFX->SetVolume(0.0f);

        vEnabledEffects.clear();

        if (pvEffects->size() == 0)
        {
            bEffectsSet = false;
            return false;
        }
    }


    std::vector<IUnknown*> vXAPO(pvEffects->size());
    HRESULT hr = S_OK;

    bool bEnableSound = false;

    for (size_t i = 0; i < pvEffects->size(); i++)
    {
        hr = S_OK;

        switch(pvEffects->operator[](i).effectType)
        {
        case(ET_REVERB):
        {
            hr = CreateFX(__uuidof(FXReverb), &vXAPO[i]);
            break;
        }
        case(ET_EQ):
        {
            hr = CreateFX(__uuidof(FXEQ), &vXAPO[i]);
            break;
        }
        case(ET_ECHO):
        {
            hr = CreateFX(__uuidof(FXEcho), &vXAPO[i]);
            break;
        }
        }

        if (pvEffects->operator[](i).bEnable && bEnableSound == false)
        {
            bEnableSound = true;
        }

        vEnabledEffects.push_back(pvEffects->operator[](i).bEnable);

        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"SSoundMix::setAudioEffects::CreateFX()");
            return true;
        }
    }

    if (bEnableSound)
    {
        pSubmixVoiceFX->SetVolume(fFXVolume);
    }


    std::vector<XAUDIO2_EFFECT_DESCRIPTOR> vDescriptors(vXAPO.size());

    for (size_t i = 0; i < vXAPO.size(); i++)
    {
        vDescriptors[i].InitialState = pvEffects->operator[](i).isEnabled();
        vDescriptors[i].OutputChannels = 2;
        vDescriptors[i].pEffect = vXAPO[i];
    }

    XAUDIO2_EFFECT_CHAIN chain;
	chain.EffectCount = static_cast<UINT32>(vXAPO.size());
    chain.pEffectDescriptors = &vDescriptors[0];

    hr = pSubmixVoiceFX->SetEffectChain(&chain);
    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"SSoundMix::setAudioEffects::SetEffectChain()");
        return true;
    }


    for (size_t i = 0; i < vXAPO.size(); i++)
    {
        // Releasing the client's reference to the XAPO allows XAudio2 to take ownership of the XAPO.
        vXAPO[i]->Release();
    }


    for (size_t i = 0; i < pvEffects->size(); i++)
    {
        switch(pvEffects->operator[](i).effectType)
        {
        case(ET_REVERB):
        {
            hr = pSubmixVoiceFX->SetEffectParameters(static_cast<UINT32>(i), &pvEffects->operator[](i).reverbParams, sizeof( FXREVERB_PARAMETERS ), XAUDIO2_COMMIT_NOW);
            break;
        }
        case(ET_EQ):
        {
            hr = pSubmixVoiceFX->SetEffectParameters(static_cast<UINT32>(i), &pvEffects->operator[](i).eqParams, sizeof( FXEQ_PARAMETERS ), XAUDIO2_COMMIT_NOW);
            break;
        }
        case(ET_ECHO):
        {
            hr = pSubmixVoiceFX->SetEffectParameters(static_cast<UINT32>(i), &pvEffects->operator[](i).echoParams, sizeof( FXECHO_PARAMETERS ), XAUDIO2_COMMIT_NOW);
            break;
        }
        }

        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"SSoundMix::setAudioEffects::SetEffectParameters()");
            return true;
        }
    }


    bEffectsSet = true;

    return false;
}

bool SSoundMix::setEnableAudioEffect(unsigned int iEffectIndex, bool bEnable)
{
    if (bEffectsSet == false)
    {
		SError::showErrorMessageBox(L"Sound::setEnableAudioEffect()", L"no effects added, use setAudioEffects() first.");
        return true;
    }

    if (bEnable)
    {
        HRESULT hr = pSubmixVoiceFX->EnableEffect(iEffectIndex);
        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"SSoundMix::setEnableAudioEffect::EnableEffect()");
            return true;
        }
    }
    else
    {
        HRESULT hr = pSubmixVoiceFX->DisableEffect(iEffectIndex);
        if (FAILED(hr))
        {
			SError::showErrorMessageBox(hr, L"SSoundMix::setEnableAudioEffect::DisableEffect()");
            return true;
        }
    }

    vEnabledEffects[iEffectIndex] = bEnable;

    for (size_t i = 0; i < vEnabledEffects.size(); i++)
    {
        if (vEnabledEffects[i])
        {
            pSubmixVoiceFX->SetVolume(1.0f);

            break;
        }
    }

    return false;
}

bool SSoundMix::setAudioEffectParameters(unsigned int iEffectIndex, SAudioEffect *params)
{
    if (bEffectsSet == false)
    {
		SError::showErrorMessageBox(L"SSoundMix::setAudioEffectParameters()", L"no effects added, use setAudioEffects() first.");
        return true;
    }

    HRESULT hr = S_OK;

    switch(params->effectType)
    {
    case(ET_REVERB):
    {
        hr = pSubmixVoiceFX->SetEffectParameters(iEffectIndex, &params->reverbParams, sizeof( FXREVERB_PARAMETERS ), XAUDIO2_COMMIT_NOW);
        break;
    }
    case(ET_EQ):
    {
        hr = pSubmixVoiceFX->SetEffectParameters(iEffectIndex, &params->eqParams, sizeof( FXEQ_PARAMETERS ), XAUDIO2_COMMIT_NOW);
        break;
    }
    case(ET_ECHO):
    {
        hr = pSubmixVoiceFX->SetEffectParameters(iEffectIndex, &params->echoParams, sizeof( FXECHO_PARAMETERS ), XAUDIO2_COMMIT_NOW);
        break;
    }
    }

    if (FAILED(hr))
    {
		SError::showErrorMessageBox(hr, L"SSoundMix::setAudioEffectParameters::SetEffectParameters()");
        return true;
    }

    return false;
}

void SSoundMix::getVolume(float &fVolume)
{
    pSubmixVoice->GetVolume(&fVolume);
}

void SSoundMix::getFXVolume(float &fFXVolume)
{
    fFXVolume = this->fFXVolume;
}

SSoundMix::~SSoundMix()
{
    if (pSubmixVoice)
    {
        pSubmixVoice->DestroyVoice();
    }

    if (pSubmixVoiceFX)
    {
        pSubmixVoiceFX->DestroyVoice();
    }
}
