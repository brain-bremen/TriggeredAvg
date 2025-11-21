# Triggered Average Plugin for OpenEphys GUI

A work-in-progress plugin for the Open Ephys GUI that averages continuous
signals triggered by TTL events and/or messages, similar to a triggered display on an oscilloscope.
The plugin is based on the [Online PSTH plugin](https://github.com/open-ephys-plugins/online-psth).

## TODO

- [ ] Fix not updating average anymore after a few trials
- [ ] Show number of averages collected per trigger source
- [X] Handle changes of capture parameters (pre/post samples), i.e. reset resize average buffers
- [ ] Add option to save averages to disk
- [ ] Add option to reset average
- [ ] Handle multiple data streams (currently always using the first)


## Installation

...

## Usage

### Basic Setup

...

### Trigger Configuration

...

### Display Options

...

## Development

### Architecture

The plugin uses a multi-threaded architecture to ensure real-time processing and responsive visualization:

#### Threads

1. **Audio/Processing Thread** (`TriggeredAvgNode::process`)
   - Runs in the OpenEphys audio callback
   - Continuously writes incoming continuous data to a `MultiChannelRingBuffer`
   - Monitors TTL events and broadcast messages
   - When a trigger event matches a configured trigger source, creates a `CaptureRequest` and queues it for the Data Collector thread
   - Operates lock-free for audio data writing to avoid blocking real-time processing

2. **Data Collector Thread** (`DataCollector::run`)
   - Background thread named "TriggeredAvg: Data Collector"
   - Processes queued `CaptureRequest` objects by:
     - Reading the requested pre/post-trigger window from the ring buffer
     - Adding the captured data to the appropriate `MultiChannelAverageBuffer`
   - Notifies the message thread via `AsyncUpdater` when average buffers are updated

3. **Message/GUI Thread** (`TriggeredAvgNode::handleAsyncUpdate` & `TriggeredAvgCanvas`)
   - JUCE message thread that handles all UI operations
   - Triggered asynchronously by the Data Collector thread when new data is available
   - Refreshes the canvas display to show updated averages
   - Handles user interactions with the editor and canvas

#### Key Components

- **MultiChannelRingBuffer**: Thread-safe circular buffer that stores ~10 seconds of continuous data with sample-accurate indexing
- **DataStore**: Thread-safe storage for `MultiChannelAverageBuffer` objects, one per trigger source
- **MultiChannelAverageBuffer**: Accumulates sum and sum-of-squares for computing running averages and standard deviations
- **TriggerSources**: Manages multiple trigger conditions (TTL, message, or combined triggers)
- **CaptureRequest**: Data structure containing trigger sample number, trigger source, and pre/post sample counts

#### Thread Synchronization

- Ring buffer uses recursive mutex (`writeLock`) and atomic variables for sample indexing
- Data Collector uses `CriticalSection` (`triggerQueueLock`) for the capture request queue
- Data Store uses recursive mutex for accessing average buffers
- Asynchronous updates via `AsyncUpdater` ensure GUI updates happen on the message thread

