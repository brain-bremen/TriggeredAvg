# TriggeredAvg Plugin Tests

This directory contains unit tests for the TriggeredAvg plugin using Google Test.

## Building Tests

The tests are independent of the Open Ephys executable and only require JUCE headers from the plugin-GUI repository. Tests only cover classes that don't depend on Open Ephys-specific code (like `ProcessorHeaders.h`).

## Tested Components

- **SingleTrialBuffer** - JUCE-independent trial storage with raw pointer interface
- **MultiChannelRingBuffer** - Lock-free circular buffer for multi-channel audio data

**Note:** Components that depend on Open Ephys classes (like `DataCollector`, `TriggeredAvgNode`, etc.) are not tested here as they require linking to the Open Ephys executable.

### Prerequisites

- CMake 3.23.0 or higher
- C++20 compatible compiler
- Access to the plugin-GUI repository (automatically detected from `../plugin-GUI` or `GUI_BASE_DIR` environment variable)

### Build Instructions

#### Windows

```powershell
# Configure with tests enabled
cmake -B build -DBUILD_TESTS=ON

# Build the tests
cmake --build build --target triggered-avg_tests --config Debug

# Run the tests
ctest --test-dir build -C Debug --output-on-failure
```

#### Linux

```bash
# Configure with tests enabled
cmake -B build -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# Build the tests
cmake --build build --target triggered-avg_tests

# Run the tests
ctest --test-dir build --output-on-failure
```

#### macOS

```bash
# Configure with tests enabled  
cmake -B build -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# Build the tests
cmake --build build --target triggered-avg_tests

# Run the tests
ctest --test-dir build --output-on-failure
```

### Running Tests Directly

You can also run the test executable directly for more verbose output:

```bash
# Windows
.\build\Tests\Debug\triggered-avg_tests.exe

# Linux/macOS
./build/Tests/triggered-avg_tests
```

## Test Structure

- **SingleTrialBufferTests.cpp** - Tests for the `SingleTrialBuffer` class (basic functionality with JUCE AudioBuffer)
- **test_SingleTrialBuffer_RawPointers.cpp** - Tests for `SingleTrialBuffer` using raw pointer interface (JUCE-independent)
- **test_MultiChannelRingBuffer.cpp** - Tests for the `MultiChannelRingBuffer` class

## Writing New Tests

To add a new test file:

1. Create a new `.cpp` file in the `Tests/` directory
2. Include the necessary headers:
   ```cpp
   #include <gtest/gtest.h>
   #include <JuceHeader.h>  // Only if you need JUCE types
   #include "../Source/YourClassToTest.h"
   ```
3. Add your test file to `Tests/CMakeLists.txt` in the `TRIGGERED_AVG_TEST_SOURCES` list
4. If testing with JUCE types (like `AudioBuffer`), use the `ScopedJuceInitialiser_GUI` pattern:
   ```cpp
   class MyTest : public ::testing::Test {
   protected:
       void SetUp() override {
           scopedJuceInit = std::make_unique<juce::ScopedJuceInitialiser_GUI>();
       }
       void TearDown() override {
           scopedJuceInit.reset();
       }
       std::unique_ptr<juce::ScopedJuceInitialiser_GUI> scopedJuceInit;
   };
   ```

**Important:** Avoid including `<ProcessorHeaders.h>` or any Open Ephys-specific headers as they require linking to the Open Ephys executable.

## Dependencies

- Google Test v1.14.0 (automatically fetched via CMake FetchContent)
- JUCE headers from plugin-GUI (no linking required)
- No Open Ephys linking required
