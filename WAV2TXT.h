#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <cstdint>
#include <algorithm>
#include <cmath>

struct WAVHeader {
    char chunkID[4];        // "RIFF"
    uint32_t chunkSize;     // 8
    char format[4];         // "WAVE"

    char subchunk1ID[4];    // "fmt "
    uint32_t subchunk1Size; // 16 PCM
    uint16_t audioFormat;   // 1 PCM
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;      // sampleRate * numChannels * bitsPerSample/8
    uint16_t blockAlign;    // numChannels * bitsPerSample/8
    uint16_t bitsPerSample; // 8, 16, 24, 32

    char subchunk2ID[4];    // "data"
    uint32_t subchunk2Size;
};

class WAVConverter {
public:

    std::pair<std::vector<char>, std::vector<char>> convertToChannels(const std::string& filename);

private:

    std::pair<std::vector<char>, std::vector<char>> extractChannels(
        const std::vector<char>& audioData,
        uint16_t numChannels,
        uint16_t bitsPerSample);
};
