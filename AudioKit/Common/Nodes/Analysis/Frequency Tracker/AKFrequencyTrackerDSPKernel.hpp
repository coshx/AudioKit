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
        Yin_init(&yin, 2048, 0.20); // Tony uses 0.2
        bufIdx = 0;
    }

    void start() {
        started = true;
    }

    void stop() {
        started = false;
    }

    void destroy() {
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

    void process(AUAudioFrameCount frameCount, AUAudioFrameCount bufferOffset) override {
        // Meekohi: This is the wrong place to do any filtering, because it will be called many times
        // before hopsize actually fills up and a new frequency is estimated.
        // Think of this as just a front-end for shovelling data into ptrack.
        for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {

            int frameOffset = int(frameIndex + bufferOffset);

            for (int channel = 0; channel < 1; ++channel) { // don't use the right stereo.
                
                // This is just one sample!!!!!
                float *in  = (float *)inBufferListPtr->mBuffers[channel].mData  + frameOffset;
                float temp = *in;
                float *out = (float *)outBufferListPtr->mBuffers[channel].mData + frameOffset;
                if (started) {
                    if(bufIdx == 0) {
                        trackedFrequency = Yin_getPitch(&yin, &(analysisBlock[bufIdx]));
                    }
                    
                    analysisBlock[bufIdx] = temp;
                    analysisBlock[bufIdx+2048] = temp;
                    bufIdx++;
                    if(bufIdx >= 2048) {
                        bufIdx = 0;
                    }

                    printf("%f %f #blep\n", temp, trackedFrequency);
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

    int hopSize = 256; // estimate every 5.8 ms // I don't know what makes sense here. This is how often you're willing to re-estimate a frame and will effect performance.
    float analysisBlock[4096]; // use a 46ms block of data but 2x space so you always have 46ms (2048) filled up
    int bufIdx = 0;

    Yin yin;

public:
    float trackedAmplitude = 0.0;
    float trackedFrequency = 0.0;
    bool started = true;
};

