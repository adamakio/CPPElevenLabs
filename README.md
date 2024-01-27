# ElevenLabs TTS C++ API Wrapper

This project is a C++ wrapper for the ElevenLabs Text-to-Speech (TTS) API. It provides an interface to interact with the ElevenLabs API, allowing users to list available models, voices, and manage voice settings programmatically.

## Features

- List available TTS models and voices
- Get and set voice settings
- Perform TTS operations using ElevenLabs API

## Requirements

- C++17 compatible compiler
- [CMake](https://cmake.org/) for building the project
- [Curl](https://curl.se/) for making API requests
- [PortAudio](http://www.portaudio.com/) for audio playback (if needed)

## Installation

To build the project, use the following CMake commands:

```bash
cmake -S . -B build
cmake --build build
```

## Usage
To use the ElevenLabs TTS API wrapper, instantiate the API with your API key and perform operations as needed. For example:

```
#include "ElevenLabsTTS.h"

int main() {
    auto& ElevenLabs = elevenlabs::start("your_api_key_here");
    
    // List models
    auto models = listModels();
    
    // List voices
    auto voices = listVoices();
    
    // Get default voice settings
    auto defaultVoiceSettings = getDefaultVoiceSettings();
    
    // Get voice settings for a specific voice
    auto voiceSettings = getVoiceSettings("voice_id_here");

    // More operations...
    return 0;
}
```
## Documentation
For detailed API usage and available methods, refer to the ElevenLabsTTS.h header file.

## Contributing
Contributions are welcome. Please feel free to fork the repository and submit pull requests.

## License
This project is licensed under the MIT License

## Acknowledgments
This project utilizes the ElevenLabs API. For more information on the API and its capabilities, visit the ElevenLabs API Documentation.

## Disclaimer
This project is not affiliated with ElevenLabs but serves as an interface to interact with the ElevenLabs TTS API.

Please replace `your_api_key_here` and `voice_id_here` with actual values you would use 