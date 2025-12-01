#pragma once
#include "MultiChannelRingBuffer.h"
#include "SingleTrialBuffer.h"

#include <JuceHeader.h>
#include <ProcessorHeaders.h>

namespace TriggeredAverage
{
class MultiChannelAverageBuffer;
class TriggeredAvgNode;
class TriggerSource;
class MultiChannelRingBuffer;

struct CaptureRequest
{
    TriggerSource* triggerSource;
    SampleNumber triggerSample;
    int preSamples;
    int postSamples;
};

/** JUCE-aware wrapper around SingleTrialBuffer that provides AudioBuffer convenience methods */
class SingleTrialBufferJuce : public SingleTrialBuffer
{
public:
    SingleTrialBufferJuce() = default;
    using SingleTrialBuffer::addTrial;  // Expose raw pointer version
    using SingleTrialBuffer::addTrialChannel;
    using SingleTrialBuffer::getChannelTrials;
    using SingleTrialBuffer::getSample;
    using SingleTrialBuffer::getTrial;  // Expose raw pointer version
    using SingleTrialBuffer::getNumStoredTrials;
    using SingleTrialBuffer::getMaxTrials;
    using SingleTrialBuffer::getNumChannels;
    using SingleTrialBuffer::getNumSamples;
    using SingleTrialBuffer::setMaxTrials;
    using SingleTrialBuffer::setSize;
    using SingleTrialBuffer::clear;
    
    /** Add a trial from a JUCE AudioBuffer (convenience wrapper) */
    template<typename SampleType>
    void addTrial(const juce::AudioBuffer<SampleType>& buffer)
    {
        SingleTrialBuffer::addTrial(buffer.getArrayOfReadPointers(), 
                                     buffer.getNumChannels(), 
                                     buffer.getNumSamples());
    }
    
    /** Copy a specific trial into a JUCE AudioBuffer (convenience wrapper) */
    template<typename SampleType>
    void getTrial(int trialIndex, juce::AudioBuffer<SampleType>& destination) const
    {
        SingleTrialBuffer::getTrial(trialIndex, 
                                     destination.getArrayOfWritePointers(),
                                     destination.getNumChannels(), 
                                     destination.getNumSamples());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SingleTrialBufferJuce)
};

// Thread-safe storage of average buffers
class DataStore
{
public:
    void ResetAndResizeBuffersForTriggerSource (TriggerSource* source, int nChannels, int nSamples);
    void ResizeAllAverageBuffers (int nChannels, int nSamples, bool clear = true);

    MultiChannelAverageBuffer* getRefToAverageBufferForTriggerSource (TriggerSource* source)
    {
        if (m_averageBuffers.contains (source))
            return &m_averageBuffers.at (source);
        return nullptr;
    }

    SingleTrialBufferJuce* getRefToTrialBufferForTriggerSource (TriggerSource* source)
    {
        if (m_singleTrialBuffers.contains (source))
            return &m_singleTrialBuffers.at (source);
        return nullptr;
    }

    std::scoped_lock<std::recursive_mutex> GetLock()
    {
        return std::scoped_lock<std::recursive_mutex> (m_mutex);
    }

    void Clear()
    {
        auto lock = GetLock();
        m_averageBuffers.clear();
        m_singleTrialBuffers.clear();
    }

    void setMaxTrialsToStore (int n);

private:
    std::recursive_mutex m_mutex;
    std::unordered_map<TriggerSource*, MultiChannelAverageBuffer> m_averageBuffers;
    std::unordered_map<TriggerSource*, SingleTrialBufferJuce> m_singleTrialBuffers;
};

class DataCollector : public Thread
{
public:
    DataCollector (TriggeredAvgNode*, MultiChannelRingBuffer*, DataStore*);
    ~DataCollector() override;
    void run() override;
    void registerTriggerSource (const TriggerSource*);
    void registerCaptureRequest (const CaptureRequest&);

private:
    // dependencies
    TriggeredAvgNode* m_processor;
    MultiChannelRingBuffer* ringBuffer;
    DataStore* m_datastore;

    // data
    std::deque<CaptureRequest> captureRequestQueue;
    AudioBuffer<float> m_collectBuffer;

    // synchronization
    CriticalSection triggerQueueLock;
    WaitableEvent newTriggerEvent;

    RingBufferReadResult processCaptureRequest (const CaptureRequest&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataCollector)
    JUCE_DECLARE_NON_MOVEABLE (DataCollector)
};

class MultiChannelAverageBuffer
{
public:
    MultiChannelAverageBuffer() = default;
    MultiChannelAverageBuffer (int numChannels, int numSamples);
    MultiChannelAverageBuffer (MultiChannelAverageBuffer&& other) noexcept;
    MultiChannelAverageBuffer& operator= (MultiChannelAverageBuffer&& other) noexcept;
    ~MultiChannelAverageBuffer() = default;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiChannelAverageBuffer)

    void addDataToAverageFromBuffer (const juce::AudioBuffer<float>& buffer);
    AudioBuffer<float> getAverage() const;
    AudioBuffer<float> getStandardDeviation() const;

    void resetTrials();
    int getNumTrials() const;
    int getNumChannels() const;
    int getNumSamples() const;
    void setSize (int nChannels, int nSamples)
    {
        m_numChannels = nChannels;
        m_numSamples = nSamples;
        m_sumBuffer.setSize (nChannels, nSamples);
        m_sumSquaresBuffer.setSize (nChannels, nSamples);
        m_averageBuffer.setSize (nChannels, nSamples);
        resetTrials();
    }

private:
    juce::AudioBuffer<float> m_sumBuffer;
    juce::AudioBuffer<float> m_sumSquaresBuffer;
    juce::AudioBuffer<float> m_averageBuffer;
    int m_numTrials = 0;
    int m_numChannels;
    int m_numSamples;

    void updateRunningAverage();
};

} // namespace TriggeredAverage
