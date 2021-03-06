//
//  AKLowShelfParametricEqualizerFilterDSPKernel.hpp
//  AudioKit
//
//  Created by Aurelius Prochazka, revision history on Github.
//  Copyright (c) 2016 Aurelius Prochazka. All rights reserved.
//

#pragma once

#import "DSPKernel.hpp"
#import "ParameterRamper.hpp"

#import <AudioKit/AudioKit-Swift.h>

extern "C" {
#include "soundpipe.h"
}

enum {
    cornerFrequencyAddress = 0,
    gainAddress = 1,
    qAddress = 2
};

class AKLowShelfParametricEqualizerFilterDSPKernel : public AKSporthKernel, public AKBuffered {
public:
    // MARK: Member Functions

    AKLowShelfParametricEqualizerFilterDSPKernel() {}

    void init(int _channels, double _sampleRate) override {
        AKSporthKernel::init(_channels, _sampleRate);

        sp_pareq_create(&pareq);
        sp_pareq_init(sp, pareq);
        pareq->fc = 1000;
        pareq->v = 1.0;
        pareq->q = 0.707;
        pareq->mode = 1;

        cornerFrequencyRamper.init();
        gainRamper.init();
        qRamper.init();
    }

    void start() {
        started = true;
    }

    void stop() {
        started = false;
    }

    void destroy() {
        sp_pareq_destroy(&pareq);
        AKSporthKernel::destroy();
    }

    void reset() {
        resetted = true;
        cornerFrequencyRamper.reset();
        gainRamper.reset();
        qRamper.reset();
    }

    void setCornerFrequency(float value) {
        cornerFrequency = clamp(value, 12.0f, 20000.0f);
        cornerFrequencyRamper.setImmediate(cornerFrequency);
    }

    void setGain(float value) {
        gain = clamp(value, 0.0f, 10.0f);
        gainRamper.setImmediate(gain);
    }

    void setQ(float value) {
        q = clamp(value, 0.0f, 2.0f);
        qRamper.setImmediate(q);
    }


    void setParameter(AUParameterAddress address, AUValue value) {
        switch (address) {
            case cornerFrequencyAddress:
                cornerFrequencyRamper.setUIValue(clamp(value, 12.0f, 20000.0f));
                break;

            case gainAddress:
                gainRamper.setUIValue(clamp(value, 0.0f, 10.0f));
                break;

            case qAddress:
                qRamper.setUIValue(clamp(value, 0.0f, 2.0f));
                break;

        }
    }

    AUValue getParameter(AUParameterAddress address) {
        switch (address) {
            case cornerFrequencyAddress:
                return cornerFrequencyRamper.getUIValue();

            case gainAddress:
                return gainRamper.getUIValue();

            case qAddress:
                return qRamper.getUIValue();

            default: return 0.0f;
        }
    }

    void startRamp(AUParameterAddress address, AUValue value, AUAudioFrameCount duration) override {
        switch (address) {
            case cornerFrequencyAddress:
                cornerFrequencyRamper.startRamp(clamp(value, 12.0f, 20000.0f), duration);
                break;

            case gainAddress:
                gainRamper.startRamp(clamp(value, 0.0f, 10.0f), duration);
                break;

            case qAddress:
                qRamper.startRamp(clamp(value, 0.0f, 2.0f), duration);
                break;

        }
    }

    void process(AUAudioFrameCount frameCount, AUAudioFrameCount bufferOffset) override {

        for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {

            int frameOffset = int(frameIndex + bufferOffset);

            cornerFrequency = cornerFrequencyRamper.getAndStep();
            pareq->fc = (float)cornerFrequency;
            gain = gainRamper.getAndStep();
            pareq->v = (float)gain;
            q = qRamper.getAndStep();
            pareq->q = (float)q;

            for (int channel = 0; channel < channels; ++channel) {
                float *in  = (float *)inBufferListPtr->mBuffers[channel].mData  + frameOffset;
                float *out = (float *)outBufferListPtr->mBuffers[channel].mData + frameOffset;

                if (started) {
                    sp_pareq_compute(sp, pareq, in, out);
                } else {
                    *out = *in;
                }
            }
        }
    }

    // MARK: Member Variables

private:

    sp_pareq *pareq;

    float cornerFrequency = 1000;
    float gain = 1.0;
    float q = 0.707;

public:
    bool started = true;
    bool resetted = false;
    ParameterRamper cornerFrequencyRamper = 1000;
    ParameterRamper gainRamper = 1.0;
    ParameterRamper qRamper = 0.707;
};

