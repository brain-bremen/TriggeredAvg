#include "SingleTrialBuffer.h"

namespace TriggeredAverage
{

void SingleTrialBuffer::addTrial(const float* const* trialData, int nChannels, int nSamples)
{
    // Initialize vector on first use or if resized
    if (data.size() != static_cast<size_t>(numChannels * maxTrials * numSamples))
    {
        data.resize(static_cast<size_t>(numChannels) * maxTrials * numSamples, 0.0f);
    }

    assert(nChannels == numChannels && "Channel count mismatch");
    assert(nSamples == numSamples && "Sample count mismatch");

    // Copy trial data into the circular buffer using channel-major layout
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* sourceData = trialData[ch];
        const size_t destOffset = getIndex(ch, writeIndex, 0);
        std::memcpy(&data[destOffset], sourceData, numSamples * sizeof(float));
    }

    // Update circular buffer indices
    writeIndex = (writeIndex + 1) % maxTrials;
    numStored = std::min(numStored + 1, maxTrials);
}

void SingleTrialBuffer::addTrialChannel(const float* channelData, int nSamples, int channelIndex)
{
    assert(channelIndex >= 0 && channelIndex < numChannels && "Channel index out of range");
    assert(nSamples == numSamples && "Sample count mismatch");
    
    if (data.size() != static_cast<size_t>(numChannels * maxTrials * numSamples))
    {
        data.resize(static_cast<size_t>(numChannels) * maxTrials * numSamples, 0.0f);
    }
    
    const size_t destOffset = getIndex(channelIndex, writeIndex, 0);
    std::memcpy(&data[destOffset], channelData, numSamples * sizeof(float));
}

std::span<const float> SingleTrialBuffer::getChannelTrials(int channelIndex) const
{
    assert(channelIndex >= 0 && channelIndex < numChannels && "Channel index out of range");
    
    if (numStored == 0)
        return {};
        
    // Return span to all trials for this channel
    // Note: This includes the full circular buffer space, not just stored trials
    const size_t offset = static_cast<size_t>(channelIndex) * maxTrials * numSamples;
    const size_t count = static_cast<size_t>(numStored) * numSamples;
    
    return std::span<const float>(&data[offset], count);
}

float SingleTrialBuffer::getSample(int channelIndex, int trialIndex, int sampleIndex) const
{
    assert(channelIndex >= 0 && channelIndex < numChannels && "Channel index out of range");
    assert(trialIndex >= 0 && trialIndex < numStored && "Trial index out of range");
    assert(sampleIndex >= 0 && sampleIndex < numSamples && "Sample index out of range");
    
    int physicalTrial = getPhysicalTrialIndex(trialIndex);
    return data[getIndex(channelIndex, physicalTrial, sampleIndex)];
}

void SingleTrialBuffer::getTrial(int trialIndex, float** destination, int nChannels, int nSamples) const
{
    assert(trialIndex >= 0 && trialIndex < numStored && "Trial index out of range");
    assert(nChannels <= numChannels && "Requested more channels than available");
    assert(nSamples <= numSamples && "Requested more samples than available");

    int physicalTrial = getPhysicalTrialIndex(trialIndex);
    
    for (int ch = 0; ch < nChannels; ++ch)
    {
        const size_t sourceOffset = getIndex(ch, physicalTrial, 0);
        std::memcpy(destination[ch], &data[sourceOffset], nSamples * sizeof(float));
    }
}

void SingleTrialBuffer::setMaxTrials(int n)
{
    int newMaxTrials = std::max(1, n);

    if (newMaxTrials == maxTrials)
        return;

    // Save existing trials in order (oldest to newest) in temporary storage
    const int trialsToKeep = std::min(numStored, newMaxTrials);
    const int startTrial = std::max(0, numStored - newMaxTrials);
    
    std::vector<float> tempData(
        static_cast<size_t>(numChannels) * trialsToKeep * numSamples);
    
    // Copy trials we want to keep
    for (int t = 0; t < trialsToKeep; ++t)
    {
        int sourceTrial = getPhysicalTrialIndex(startTrial + t);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const size_t sourceOffset = getIndex(ch, sourceTrial, 0);
            const size_t destOffset = (static_cast<size_t>(ch) * trialsToKeep + t) * numSamples;
            std::memcpy(&tempData[destOffset], &data[sourceOffset], numSamples * sizeof(float));
        }
    }

    // Resize and rebuild with new layout
    maxTrials = newMaxTrials;
    data.resize(static_cast<size_t>(numChannels) * maxTrials * numSamples, 0.0f);
    
    // Copy back the trials
    for (int t = 0; t < trialsToKeep; ++t)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const size_t sourceOffset = (static_cast<size_t>(ch) * trialsToKeep + t) * numSamples;
            const size_t destOffset = getIndex(ch, t, 0);
            std::memcpy(&data[destOffset], &tempData[sourceOffset], numSamples * sizeof(float));
        }
    }
    
    writeIndex = trialsToKeep % maxTrials;
    numStored = trialsToKeep;
}

void SingleTrialBuffer::clear()
{
    writeIndex = 0;
    numStored = 0;
    std::fill(data.begin(), data.end(), 0.0f);
}

void SingleTrialBuffer::setSize(int nChannels, int nSamples, int nTrials)
{
    numChannels = nChannels;
    numSamples = nSamples;
    maxTrials = std::max(1, nTrials);
    
    data.clear();
    data.resize(static_cast<size_t>(numChannels) * maxTrials * numSamples, 0.0f);
    
    writeIndex = 0;
    numStored = 0;
}

bool SingleTrialBuffer::getChannelMinMax(int channelIndex, int startTrialIndex, int endTrialIndex,
                                          float& outMin, float& outMax) const
{
    assert(channelIndex >= 0 && channelIndex < numChannels && "Channel index out of range");
    assert(startTrialIndex >= 0 && "Start trial index must be non-negative");
    
    // Clamp to valid range
    startTrialIndex = std::max(0, startTrialIndex);
    endTrialIndex = std::min(endTrialIndex, numStored);
    
    if (startTrialIndex >= endTrialIndex || numStored == 0 || numSamples == 0)
    {
        return false;
    }
    
    // Initialize with first sample
    int firstPhysicalTrial = getPhysicalTrialIndex(startTrialIndex);
    outMin = data[getIndex(channelIndex, firstPhysicalTrial, 0)];
    outMax = outMin;
    
    // Iterate through all trials and samples
    for (int trialIdx = startTrialIndex; trialIdx < endTrialIndex; ++trialIdx)
    {
        int physicalTrial = getPhysicalTrialIndex(trialIdx);
        const size_t trialOffset = getIndex(channelIndex, physicalTrial, 0);
        
        for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
        {
            float value = data[trialOffset + sampleIdx];
            outMin = std::min(outMin, value);
            outMax = std::max(outMax, value);
        }
    }
    
    return true;
}

} // namespace TriggeredAverage
