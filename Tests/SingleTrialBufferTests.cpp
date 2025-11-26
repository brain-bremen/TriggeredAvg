#include "gtest/gtest.h"

#include "../Source/DataCollector.h"
#include <ProcessorHeaders.h>

using namespace TriggeredAverage;

static AudioBuffer<float> makeTrial (int nChannels, int nSamples, float baseValue)
{
    AudioBuffer<float> b (nChannels, nSamples);
    for (int ch = 0; ch < nChannels; ++ch)
        for (int s = 0; s < nSamples; ++s)
            b.setSample (ch, s, baseValue + ch * 0.1f + s * 0.001f);
    return b;
}

TEST (SingleTrialBufferTests, AddAndRetrieveOrdering)
{
    SingleTrialBuffer buf;
    buf.setMaxTrials (5);
    buf.setSize (2, 4);

    auto t0 = makeTrial (2, 4, 10.0f);
    auto t1 = makeTrial (2, 4, 20.0f);
    auto t2 = makeTrial (2, 4, 30.0f);

    buf.addTrial (t0);
    buf.addTrial (t1);
    buf.addTrial (t2);

    EXPECT_EQ (buf.getNumStoredTrials(), 3);

    const auto& oldest = buf.getTrial (0);
    const auto& newest = buf.getTrial (2);

    EXPECT_EQ (oldest.getNumChannels(), t0.getNumChannels());
    EXPECT_EQ (oldest.getNumSamples(), t0.getNumSamples());
    EXPECT_EQ (newest.getNumChannels(), t2.getNumChannels());
    EXPECT_EQ (newest.getNumSamples(), t2.getNumSamples());

    for (int ch = 0; ch < oldest.getNumChannels(); ++ch)
        for (int s = 0; s < oldest.getNumSamples(); ++s)
            EXPECT_FLOAT_EQ (oldest.getSample (ch, s), t0.getSample (ch, s));

    for (int ch = 0; ch < newest.getNumChannels(); ++ch)
        for (int s = 0; s < newest.getNumSamples(); ++s)
            EXPECT_FLOAT_EQ (newest.getSample (ch, s), t2.getSample (ch, s));
}

TEST (SingleTrialBufferTests, CircularOverwrite)
{
    SingleTrialBuffer buf;
    buf.setMaxTrials (3);
    buf.setSize (1, 3);

    auto t0 = makeTrial (1, 3, 1.0f);
    auto t1 = makeTrial (1, 3, 2.0f);
    auto t2 = makeTrial (1, 3, 3.0f);
    auto t3 = makeTrial (1, 3, 4.0f);

    buf.addTrial (t0);
    buf.addTrial (t1);
    buf.addTrial (t2);
    buf.addTrial (t3); // overwrites oldest (t0)

    EXPECT_EQ (buf.getNumStoredTrials(), 3);

    const auto& oldest = buf.getTrial (0);
    const auto& newest = buf.getTrial (2);

    EXPECT_FLOAT_EQ (oldest.getSample (0, 0), t1.getSample (0, 0));
    EXPECT_FLOAT_EQ (newest.getSample (0, 0), t3.getSample (0, 0));
}

TEST (SingleTrialBufferTests, ShrinkMaxTrialsKeepsMostRecent)
{
    SingleTrialBuffer buf;
    buf.setMaxTrials (5);
    buf.setSize (1, 2);

    std::vector<AudioBuffer<float>> trials;
    for (int i = 0; i < 5; ++i)
    {
        trials.push_back (makeTrial (1, 2, 10.0f + i));
        buf.addTrial (trials.back());
    }

    EXPECT_EQ (buf.getNumStoredTrials(), 5);

    buf.setMaxTrials (3);
    EXPECT_EQ (buf.getNumStoredTrials(), 3);

    const auto& oldest = buf.getTrial (0);
    for (int s = 0; s < oldest.getNumSamples(); ++s)
        EXPECT_FLOAT_EQ (oldest.getSample (0, s), trials[2].getSample (0, s));
}

TEST (SingleTrialBufferTests, ClearResetsStorage)
{
    SingleTrialBuffer buf;
    buf.setMaxTrials (4);
    buf.setSize (2, 3);

    buf.addTrial (makeTrial (2, 3, 5.0f));
    buf.addTrial (makeTrial (2, 3, 6.0f));
    ASSERT_GT (buf.getNumStoredTrials(), 0);

    buf.clear();
    EXPECT_EQ (buf.getNumStoredTrials(), 0);
}

TEST (SingleTrialBufferTests, SetSizeResetsAndAcceptsTrials)
{
    SingleTrialBuffer buf;
    buf.setMaxTrials (3);
    buf.setSize (4, 6);

    EXPECT_EQ (buf.getNumStoredTrials(), 0);

    auto t = makeTrial (4, 6, 7.0f);
    buf.addTrial (t);
    EXPECT_EQ (buf.getNumStoredTrials(), 1);

    const auto& stored = buf.getTrial (0);
    EXPECT_EQ (stored.getNumChannels(), 4);
    EXPECT_EQ (stored.getNumSamples(), 6);
    for (int ch = 0; ch < 4; ++ch)
        for (int s = 0; s < 6; ++s)
            EXPECT_FLOAT_EQ (stored.getSample (ch, s), t.getSample (ch, s));
}
