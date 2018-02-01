//
//  AKFMOscillatorBank.swift
//  AudioKit
//
//  Created by Aurelius Prochazka, revision history on Github.
//  Copyright © 2017 AudioKit. All rights reserved.
//

/// Frequency Modulation Polyphonic Oscillator
///
open class AKFMOscillatorBank: AKPolyphonicNode, AKComponent {
    public typealias AKAudioUnitType = AKFMOscillatorBankAudioUnit
    /// Four letter unique description of the node
    public static let ComponentDescription = AudioComponentDescription(instrument: "fmob")

    // MARK: - Properties

    private var internalAU: AKAudioUnitType?
    private var token: AUParameterObserverToken?

    /// Waveform of the oscillator
    open var waveform: AKTable? {
        //TODO: Add error checking for table size...needs to match init()
        willSet {
            if let wf = newValue {
                for (i, sample) in wf.enumerated() {
                    self.internalAU?.setWaveformValue(sample, at: UInt32(i))
                }
            }
        }
    }
    fileprivate var carrierMultiplierParameter: AUParameter?
    fileprivate var modulatingMultiplierParameter: AUParameter?
    fileprivate var modulationIndexParameter: AUParameter?

    fileprivate var attackDurationParameter: AUParameter?
    fileprivate var decayDurationParameter: AUParameter?
    fileprivate var sustainLevelParameter: AUParameter?
    fileprivate var releaseDurationParameter: AUParameter?
    fileprivate var pitchBendParameter: AUParameter?
    fileprivate var vibratoDepthParameter: AUParameter?
    fileprivate var vibratoRateParameter: AUParameter?

    /// Ramp Time represents the speed at which parameters are allowed to change
    @objc open dynamic var rampTime: Double = AKSettings.rampTime {
        willSet {
            internalAU?.rampTime = newValue
        }
    }

    /// This multiplied by the baseFrequency gives the carrier frequency.
    @objc open dynamic var carrierMultiplier: Double = 1.0 {
        willSet {
            if carrierMultiplier != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        carrierMultiplierParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.carrierMultiplier = Float(newValue)
                }
            }
        }
    }

    /// This multiplied by the baseFrequency gives the modulating frequency.
    @objc open dynamic var modulatingMultiplier: Double = 1 {
        willSet {
            if modulatingMultiplier != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        modulatingMultiplierParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.modulatingMultiplier = Float(newValue)
                }
            }
        }
    }

    /// This multiplied by the modulating frequency gives the modulation amplitude.
    @objc open dynamic var modulationIndex: Double = 1 {
        willSet {
            if modulationIndex != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        modulationIndexParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.modulationIndex = Float(newValue)
                }
            }
        }
    }

    /// Attack time
    @objc open dynamic var attackDuration: Double = 0.1 {
        willSet {
            if attackDuration != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        attackDurationParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.attackDuration = Float(newValue)
                }
            }
        }
    }
    /// Decay time
    @objc open dynamic var decayDuration: Double = 0.1 {
        willSet {
            if decayDuration != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        decayDurationParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.decayDuration = Float(newValue)
                }
            }
        }
    }
    /// Sustain Level
    @objc open dynamic var sustainLevel: Double = 1.0 {
        willSet {
            if sustainLevel != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        sustainLevelParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.sustainLevel = Float(newValue)
                }
            }
        }
    }
    /// Release time
    @objc open dynamic var releaseDuration: Double = 0.1 {
        willSet {
            if releaseDuration != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        releaseDurationParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.releaseDuration = Float(newValue)
                }
            }
        }
    }

    /// Pitch Bend as number of semitones
    @objc open dynamic var pitchBend: Double = 0 {
        willSet {
            if pitchBend != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        pitchBendParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.pitchBend = Float(newValue)
                }
            }
        }
    }

    /// Vibrato Depth in semitones
    @objc open dynamic var vibratoDepth: Double = 0 {
        willSet {
            if vibratoDepth != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        vibratoDepthParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.vibratoDepth = Float(newValue)
                }
            }
        }
    }

    /// Vibrato Rate in Hz
    @objc open dynamic var vibratoRate: Double = 0 {
        willSet {
            if vibratoRate != newValue {
                if internalAU?.isSetUp ?? false {
                    if let existingToken = token {
                        vibratoRateParameter?.setValue(Float(newValue), originator: existingToken)
                    }
                } else {
                    internalAU?.vibratoRate = Float(newValue)
                }
            }
        }
    }

    // MARK: - Initialization

    /// Initialize the oscillator with defaults
    public convenience override init() {
        self.init(waveform: AKTable(.sine))
    }

    /// Initialize this oscillator node
    ///
    /// - Parameters:
    ///   - waveform:             The waveform of oscillation
    ///   - carrierMultiplier:    This multiplied by the baseFrequency gives the carrier frequency.
    ///   - modulatingMultiplier: This multiplied by the baseFrequency gives the modulating frequency.
    ///   - modulationIndex:      This multiplied by the modulating frequency gives the modulation amplitude.
    ///   - attackDuration:       Attack time
    ///   - decayDuration:        Decay time
    ///   - sustainLevel:         Sustain Level
    ///   - releaseDuration:      Release time
    ///   - pitchBend:          Change of pitch in semitones
    ///   - vibratoDepth:       Vibrato size in semitones
    ///   - vibratoRate:        Frequency of vibrato in Hz
    ///
    @objc public init(
        waveform: AKTable,
        carrierMultiplier: Double = 1,
        modulatingMultiplier: Double = 1,
        modulationIndex: Double = 1,
        attackDuration: Double = 0.1,
        decayDuration: Double = 0.1,
        sustainLevel: Double = 1,
        releaseDuration: Double = 0.1,
        pitchBend: Double = 0,
        vibratoDepth: Double = 0,
        vibratoRate: Double = 0) {

        self.waveform = waveform
        self.carrierMultiplier = carrierMultiplier
        self.modulatingMultiplier = modulatingMultiplier
        self.modulationIndex = modulationIndex

        self.attackDuration = attackDuration
        self.decayDuration = decayDuration
        self.sustainLevel = sustainLevel
        self.releaseDuration = releaseDuration
        self.pitchBend = pitchBend
        self.vibratoDepth = vibratoDepth
        self.vibratoRate = vibratoRate

        _Self.register()

        super.init()
        AVAudioUnit._instantiate(with: _Self.ComponentDescription) { [weak self] avAudioUnit in

            self?.avAudioNode = avAudioUnit
            self?.midiInstrument = avAudioUnit as? AVAudioUnitMIDIInstrument
            self?.internalAU = avAudioUnit.auAudioUnit as? AKAudioUnitType

            self?.internalAU?.setupWaveform(Int32(waveform.count))
            for (i, sample) in waveform.enumerated() {
                self?.internalAU?.setWaveformValue(sample, at: UInt32(i))
            }
        }

        guard let tree = internalAU?.parameterTree else {
            AKLog("Parameter Tree Failed")
            return
        }

        carrierMultiplierParameter = tree["carrierMultiplier"]
        modulatingMultiplierParameter = tree["modulatingMultiplier"]
        modulationIndexParameter = tree["modulationIndex"]

        attackDurationParameter = tree["attackDuration"]
        decayDurationParameter = tree["decayDuration"]
        sustainLevelParameter = tree["sustainLevel"]
        releaseDurationParameter = tree["releaseDuration"]
        pitchBendParameter = tree["pitchBend"]
        vibratoDepthParameter = tree["vibratoDepth"]
        vibratoRateParameter = tree["vibratoRate"]

        token = tree.token(byAddingParameterObserver: { [weak self] _, _ in

            guard let _ = self else {
                AKLog("Unable to create strong reference to self")
                return
            } // Replace _ with strongSelf if needed
            DispatchQueue.main.async {
                // This node does not change its own values so we won't add any
                // value observing, but if you need to, this is where that goes.
            }
        })

        internalAU?.carrierMultiplier = Float(carrierMultiplier)
        internalAU?.modulatingMultiplier = Float(modulatingMultiplier)
        internalAU?.modulationIndex = Float(modulationIndex)

        internalAU?.attackDuration = Float(attackDuration)
        internalAU?.decayDuration = Float(decayDuration)
        internalAU?.sustainLevel = Float(sustainLevel)
        internalAU?.releaseDuration = Float(releaseDuration)
        internalAU?.pitchBend = Float(pitchBend)
        internalAU?.vibratoDepth = Float(vibratoDepth)
        internalAU?.vibratoRate = Float(vibratoRate)
    }

    // MARK: - AKPolyphonic

    // Function to start, play, or activate the node at frequency
    open override func play(noteNumber: MIDINoteNumber, velocity: MIDIVelocity, frequency: Double) {
        internalAU?.startNote(noteNumber, velocity: velocity, frequency: Float(frequency))
    }

    /// Function to stop or bypass the node, both are equivalent
    open override func stop(noteNumber: MIDINoteNumber) {
        internalAU?.stopNote(noteNumber)
    }
}
