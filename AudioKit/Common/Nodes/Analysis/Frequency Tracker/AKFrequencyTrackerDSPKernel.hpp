//
//  AKFrequencyTrackerDSPKernel.hpp
//  AudioKit
//
//  Created by Aurelius Prochazka, revision history on Github.
//  Copyright © 2017 Aurelius Prochazka. All rights reserved.
//

#pragma once

#import "AKSoundpipeKernel.hpp"

class AKFrequencyTrackerDSPKernel : public AKSoundpipeKernel, public AKBuffered {
public:
    // MARK: Member Functions

    AKFrequencyTrackerDSPKernel() {}

    void init(int _channels, double _sampleRate) override {
        printf("AKFrequencyTrackerDSPKernel init\n");
        AKSoundpipeKernel::init(_channels, _sampleRate);
        sp_ptrack_create(&ptrack);
        sp_ptrack_init(sp, ptrack, hopSize, peakCount);
    }

    void start() {
        started = true;
    }

    void stop() {
        started = false;
    }

    void destroy() {
        sp_ptrack_destroy(&ptrack);
        AKSoundpipeKernel::destroy();
    }

    void reset() {
    }


    void setParameter(AUParameterAddress address, AUValue value) {
        switch (address) {
        }
    }

    AUValue getParameter(AUParameterAddress address) {
        switch (address) {
            default: return 0.0f;
        }
    }

    void startRamp(AUParameterAddress address, AUValue value, AUAudioFrameCount duration) override {
        switch (address) {
        }
    }
    
    float median3(float a, float b, float c)
    {
        if ((a <= b) && (a <= c))
        {
            return (b <= c) ? b : c;
        }
        else if ((b <= a) && (b <= c))
        {
            return (a <= c) ? a : c;
        }
        else
        {
            return (a <= b) ? a : b;
        }
    }

    void process(AUAudioFrameCount frameCount, AUAudioFrameCount bufferOffset) override {
        // Meekohi: This is the wrong place to do any filtering, because it will be called many times
        // before hopsize actually fills up and a new frequency is estimated.
        // Think of this as just a front-end for shovelling data into ptrack.
        for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {

            int frameOffset = int(frameIndex + bufferOffset);

            for (int channel = 0; channel < channels; ++channel) {
                float *in  = (float *)inBufferListPtr->mBuffers[channel].mData  + frameOffset;
                float temp = *in;
                float *out = (float *)outBufferListPtr->mBuffers[channel].mData + frameOffset;
                if (started) {
                    sp_ptrack_compute(sp, ptrack, in, &trackedFrequency, &trackedAmplitude);
                } else {
                    trackedAmplitude = 0;
                    trackedFrequency = 0;
                }
                *out = temp;
            }
        }
    }

    // MARK: Member Variables

private:

    int hopSize = 4096;
    int peakCount = 20;

    sp_ptrack *ptrack = nullptr;

public:
    float trackedAmplitude = 0.0;
    float trackedFrequency = 0.0;
    bool started = true;
};

