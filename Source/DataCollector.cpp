#include "DataCollector.h"
#include "MultiChannelRingBuffer.h"
#include "TriggerSource.h"
#include "TriggeredAvgNode.h"
#include <ProcessorHeaders.h>

using namespace TriggeredAverage;

// DataStore implementation
void DataStore::ResetAndResizeBuffersForTriggerSource (TriggerSource* source,
                                                       int nChannels,
                                                       int nSamples)
{
    std::scoped_lock<std::recursive_mutex> lock (m_mutex);
    if (! source)
    {
        for (auto& [key, value] : m_averageBuffers)
        {
            value.setSize (nChannels, nSamples);
        }
    }
    else
    {
        m_averageBuffers[source].setSize (nChannels, nSamples);
        m_singleTrialBuffers[source].setSize (nChannels, nSamples);
    }
}

void TriggeredAverage::DataStore::ResizeAllAverageBuffers (int nChannels, int nSamples, bool clear)
{
    auto lock = GetLock();
    for (auto& [source, buffer] : m_averageBuffers)
    {
        buffer.setSize (nChannels, nSamples);
        if (clear)
            buffer.resetTrials();
    }
}

void DataStore::setMaxTrialsToStore (int n)
{
    auto lock = GetLock();
    for (auto& [source, trialBuffer] : m_singleTrialBuffers)
    {
        trialBuffer.setMaxTrials (n);
    }
}

DataCollector::DataCollector (TriggeredAvgNode* viewer_,
                              MultiChannelRingBuffer* buffer_,
                              DataStore* datastore_)
    : Thread ("TriggeredAvg: Data Collector"),
      m_processor (viewer_),
      ringBuffer (buffer_),
      m_datastore (datastore_),
      newTriggerEvent (false)
{
    //setPriority(Thread::Priority::high);
}

DataCollector::~DataCollector() { stopThread (1000); }

void DataCollector::registerCaptureRequest (const CaptureRequest& request)
{
    const ScopedLock lock (triggerQueueLock);
    captureRequestQueue.emplace_back (request);
    newTriggerEvent.signal();
}

void DataCollector::run()
{
    constexpr double retryIntervalMs = 100.0;
    constexpr int maximumNumberOfRetries = 500;
    while (! threadShouldExit())
    {
        if (bool wasTriggered = newTriggerEvent.wait (100))
        {
            bool averageBuffersWereUpdated = false;

            while (! threadShouldExit())
            {
                // Get next request from queue (lock held briefly)
                CaptureRequest currentRequest;
                bool hasRequest = false;

                {
                    const ScopedLock lock (triggerQueueLock);
                    if (! captureRequestQueue.empty())
                    {
                        currentRequest = captureRequestQueue.front();
                        captureRequestQueue.pop_front();
                        hasRequest = true;
                    }
                }

                if (! hasRequest)
                    break;

                // Process request without holding the lock
                int iRetry = 0;
                RingBufferReadResult result = RingBufferReadResult::UnknownError;

                do
                {
                    result = processCaptureRequest (currentRequest);
                    assert (result != RingBufferReadResult::UnknownError);

                    switch (result)
                    {
                        case RingBufferReadResult::Success:
                            averageBuffersWereUpdated = true;
                            LOGD ("[TriggeredAvg] Capture Request succesfully processed ")
                            break;
                        case RingBufferReadResult::DataInRingBufferTooOld:
                            averageBuffersWereUpdated = true;
                            LOGD ("[TriggeredAvg] Catpure Request dicarded, data too old. ")
                            break;

                        case RingBufferReadResult::NotEnoughNewData:
                            if (iRetry < maximumNumberOfRetries)
                            {
                                LOGD ("[TriggeredAvg] Capture Request retry ",
                                      iRetry,
                                      " - not enough data available yet, waiting ",
                                      retryIntervalMs,
                                      " ms.")
                                wait (retryIntervalMs);
                                iRetry++;
                            }
                            else
                            {
                                LOGD ("TriggeredAvg: Capture request discarded after ",
                                      maximumNumberOfRetries,
                                      " retries - not enough data available");
                                result = RingBufferReadResult::Aborted;
                            }
                            break;

                        case RingBufferReadResult::InvalidParameters:
                        case RingBufferReadResult::UnknownError:
                        case RingBufferReadResult::Aborted:
                            assert (false);
                            break;
                    }
                } while (result == RingBufferReadResult::NotEnoughNewData
                         && iRetry < maximumNumberOfRetries && ! threadShouldExit());

                assert (RingBufferReadResult::Success == result);
            }

            if (averageBuffersWereUpdated)
            {
                // notify the processor that the data has been updated
                if (m_processor != nullptr)
                    m_processor->triggerAsyncUpdate();
            }
        }
    }
}

// process a single capture request on the ring buffer, running on the data collector thread
RingBufferReadResult DataCollector::processCaptureRequest (const CaptureRequest& request)
{
    auto result = ringBuffer->readAroundSample (
        request.triggerSample, request.preSamples, request.postSamples, m_collectBuffer);
    assert (result != RingBufferReadResult::UnknownError);
    if (result != RingBufferReadResult::Success)
    {
        return result;
    }

    // First, get buffer pointer and check size with minimal lock time
    MultiChannelAverageBuffer* avgBuffer = nullptr;
    SingleTrialBufferJuce* trialBuffer = nullptr;
    bool needsResize = false;

    {
        auto lock = m_datastore->GetLock();
        avgBuffer = m_datastore->getRefToAverageBufferForTriggerSource (request.triggerSource);
        trialBuffer = m_datastore->getRefToTrialBufferForTriggerSource (request.triggerSource);

        if (! avgBuffer)
        {
            m_datastore->ResetAndResizeBuffersForTriggerSource (request.triggerSource,
                                                                m_collectBuffer.getNumChannels(),
                                                                m_collectBuffer.getNumSamples());
            avgBuffer = m_datastore->getRefToAverageBufferForTriggerSource (request.triggerSource);
        }

        jassert (avgBuffer);

        if (! trialBuffer)
        {
            m_datastore->ResetAndResizeBuffersForTriggerSource (request.triggerSource,
                                                                m_collectBuffer.getNumChannels(),
                                                                m_collectBuffer.getNumSamples());
            trialBuffer = m_datastore->getRefToTrialBufferForTriggerSource (request.triggerSource);
        }

        jassert (trialBuffer);

        if (m_collectBuffer.getNumChannels() != avgBuffer->getNumChannels()
            || m_collectBuffer.getNumSamples() != avgBuffer->getNumSamples())
        {
            needsResize = true;
        }

    } // Lock released here

    // Resize outside the critical section if needed
    if (needsResize)
    {
        auto lock = m_datastore->GetLock();
        m_datastore->ResetAndResizeBuffersForTriggerSource (request.triggerSource,
                                                            m_collectBuffer.getNumChannels(),
                                                            m_collectBuffer.getNumSamples());
    }

    // Now add data with a separate, brief lock acquisition
    {
        auto lock = m_datastore->GetLock();
        avgBuffer = m_datastore->getRefToAverageBufferForTriggerSource (request.triggerSource);
        trialBuffer = m_datastore->getRefToTrialBufferForTriggerSource (request.triggerSource);

        jassert (avgBuffer);
        jassert (trialBuffer);
        jassert (m_collectBuffer.getNumSamples() == avgBuffer->getNumSamples());
        jassert (m_collectBuffer.getNumChannels() == avgBuffer->getNumChannels());

        // Add to average buffer
        avgBuffer->addDataToAverageFromBuffer (m_collectBuffer);

        // Add to trial buffer (uses template wrapper for AudioBuffer)
        trialBuffer->addTrial (m_collectBuffer);
    }

    return result;
}
MultiChannelAverageBuffer::MultiChannelAverageBuffer (int numChannels, int numSamples)
    : m_numChannels (numChannels),
      m_numSamples (numSamples)
{
    m_sumBuffer.setSize (numChannels, numSamples);
    m_sumSquaresBuffer.setSize (numChannels, numSamples);
    m_averageBuffer.setSize (numChannels, numSamples);
    resetTrials();
}
MultiChannelAverageBuffer::MultiChannelAverageBuffer (MultiChannelAverageBuffer&& other) noexcept
    : m_numChannels (other.m_numChannels),
      m_numSamples (other.m_numSamples)
{
    m_sumBuffer = std::move (other.m_sumBuffer);
    m_sumSquaresBuffer = std::move (other.m_sumSquaresBuffer);
    m_averageBuffer = std::move (other.m_averageBuffer);
    m_numTrials = other.m_numTrials;
}
MultiChannelAverageBuffer&
    MultiChannelAverageBuffer::operator= (MultiChannelAverageBuffer&& other) noexcept
{
    if (this != &other)
    {
        m_sumBuffer = std::move (other.m_sumBuffer);
        m_sumSquaresBuffer = std::move (other.m_sumSquaresBuffer);
        m_averageBuffer = std::move (other.m_averageBuffer);
        m_numTrials = other.m_numTrials;
        m_numChannels = other.m_numChannels;
        m_numSamples = other.m_numSamples;
    }
    return *this;
}
void MultiChannelAverageBuffer::addDataToAverageFromBuffer (const juce::AudioBuffer<float>& buffer)
{
    jassert (buffer.getNumChannels() == m_numChannels);
    jassert (buffer.getNumSamples() == m_numSamples);

    // Update sum and sum-of-squares using SIMD-optimized operations
    for (int ch = 0; ch < m_numChannels; ++ch)
    {
        auto* sumData = m_sumBuffer.getWritePointer (ch);
        auto* sumSquaresData = m_sumSquaresBuffer.getWritePointer (ch);
        auto* inputData = buffer.getReadPointer (ch);

        // Use JUCE's SIMD-optimized operations
        juce::FloatVectorOperations::add (sumData, inputData, m_numSamples);

        // For sum of squares, we need to square then add
        for (int i = 0; i < m_numSamples; ++i)
        {
            float sample = inputData[i];
            sumSquaresData[i] += sample * sample;
        }
    }

    ++m_numTrials;

    // Update the cached running average
    updateRunningAverage();
}
AudioBuffer<float> MultiChannelAverageBuffer::getAverage() const
{
    // Simply return a copy of the cached average buffer
    if (m_numTrials == 0)
    {
        AudioBuffer<float> outputBuffer;
        outputBuffer.clear();
        return outputBuffer;
    }

    // Return a copy of the cached average
    return AudioBuffer<float> (m_averageBuffer);
}
AudioBuffer<float> MultiChannelAverageBuffer::getStandardDeviation() const
{
    juce::AudioBuffer<float> outputBuffer;
    if (m_numTrials == 0)
    {
        outputBuffer.clear();
        return outputBuffer;
    }

    outputBuffer.setSize (m_numChannels, m_numSamples, false, false, true);

    for (int ch = 0; ch < m_numChannels; ++ch)
    {
        auto* sumData = m_sumBuffer.getReadPointer (ch);
        auto* sumSquaresData = m_sumSquaresBuffer.getReadPointer (ch);
        auto* outputData = outputBuffer.getWritePointer (ch);

        for (int i = 0; i < m_numSamples; ++i)
        {
            const float mean = sumData[i] / static_cast<float> (m_numTrials);
            const float meanSquares = sumSquaresData[i] / static_cast<float> (m_numTrials);
            const float variance = meanSquares - (mean * mean);
            outputData[i] = std::sqrt (
                std::max (0.0f, variance)); // Clamp to avoid negative due to float precision
        }
    }
    return outputBuffer;
}

void MultiChannelAverageBuffer::resetTrials()
{
    m_sumBuffer.clear();
    m_sumSquaresBuffer.clear();
    m_averageBuffer.clear();
    m_numTrials = 0;
}
int MultiChannelAverageBuffer::getNumTrials() const { return m_numTrials; }
int MultiChannelAverageBuffer::getNumChannels() const
{
    assert (m_sumBuffer.getNumChannels() == m_sumSquaresBuffer.getNumChannels());
    return m_sumBuffer.getNumChannels();
}
int MultiChannelAverageBuffer::getNumSamples() const
{
    assert (m_sumBuffer.getNumChannels() == m_sumSquaresBuffer.getNumChannels());
    return m_sumBuffer.getNumSamples();
}

void MultiChannelAverageBuffer::updateRunningAverage()
{
    if (m_numTrials == 0)
    {
        m_averageBuffer.clear();
        return;
    }

    const float invTrials = 1.0f / static_cast<float> (m_numTrials);

    // Use JUCE's SIMD-optimized multiply for each channel
    for (int ch = 0; ch < m_numChannels; ++ch)
    {
        juce::FloatVectorOperations::multiply (m_averageBuffer.getWritePointer (ch),
                                               m_sumBuffer.getReadPointer (ch),
                                               invTrials,
                                               m_numSamples);
    }
}
