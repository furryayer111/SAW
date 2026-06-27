#include "WAV2TXT.h"

std::pair<std::vector<char>, std::vector<char>> WAVConverter::convertToChannels(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    char chunkID[4];
    uint32_t chunkSize;
    char format[4];

    file.read(chunkID, 4);
    file.read(reinterpret_cast<char*>(&chunkSize), 4);
    file.read(format, 4);

    if (std::string(chunkID, 4) != "RIFF" || std::string(format, 4) != "WAVE") {
        throw std::runtime_error("Not a WAVE file");
    }

    bool foundFmt = false;
    bool foundData = false;
    WAVHeader header = {};

    memcpy(header.chunkID, chunkID, 4);
    header.chunkSize = chunkSize;
    memcpy(header.format, format, 4);

    while (!foundFmt || !foundData) {
        char subchunkID[4];
        uint32_t subchunkSize;

        file.read(subchunkID, 4);
        file.read(reinterpret_cast<char*>(&subchunkSize), 4);

        if (file.eof()) break;

        std::string id(subchunkID, 4);

        if (id == "fmt ") {
            foundFmt = true;
            memcpy(header.subchunk1ID, subchunkID, 4);
            header.subchunk1Size = subchunkSize;

            file.read(reinterpret_cast<char*>(&header.audioFormat), 2);
            file.read(reinterpret_cast<char*>(&header.numChannels), 2);
            file.read(reinterpret_cast<char*>(&header.sampleRate), 4);
            file.read(reinterpret_cast<char*>(&header.byteRate), 4);
            file.read(reinterpret_cast<char*>(&header.blockAlign), 2);
            file.read(reinterpret_cast<char*>(&header.bitsPerSample), 2);

            if (subchunkSize > 16) {
                file.seekg(subchunkSize - 16, std::ios::cur);
            }
        }
        else if (id == "data") {
            foundData = true;
            memcpy(header.subchunk2ID, subchunkID, 4);
            header.subchunk2Size = subchunkSize;

            std::vector<char> audioData(subchunkSize);
            file.read(audioData.data(), subchunkSize);

            if (!file) {
                throw std::runtime_error("Error reading audio data");
            }

            if (header.audioFormat != 1) {
                throw std::runtime_error("Only PCM format supported");
            }

            if (header.numChannels != 1 && header.numChannels != 2) {
                throw std::runtime_error("Only mono or stereo supported");
            }

            if (header.bitsPerSample != 8 && header.bitsPerSample != 16) {
                throw std::runtime_error("Only 8 or 16 bit supported");
            }

            return extractChannels(audioData, header.numChannels, header.bitsPerSample);
        }
        else {
            file.seekg(subchunkSize, std::ios::cur);
        }
    }

    throw std::runtime_error("Required fmt or data chunk not found");
}

std::pair<std::vector<char>, std::vector<char>> WAVConverter::extractChannels(
    const std::vector<char>& audioData,
    uint16_t numChannels,
    uint16_t bitsPerSample) {

    std::vector<char> leftChannel;
    std::vector<char> rightChannel;

    if (numChannels == 1) {
        if (bitsPerSample == 8) {
            for (char sample : audioData) {
                char signedSample = static_cast<char>(static_cast<unsigned char>(sample) - 128);
                leftChannel.push_back(signedSample);
                rightChannel.push_back(signedSample);
            }
        }
        else if (bitsPerSample == 16) {
            for (size_t i = 0; i < audioData.size(); i += 2) {
                int16_t sample = static_cast<int16_t>(
                    static_cast<unsigned char>(audioData[i]) |
                    (static_cast<unsigned char>(audioData[i + 1]) << 8)
                    );
                char convertedSample = static_cast<char>((sample >> 8) & 0xFF);
                leftChannel.push_back(convertedSample);
                rightChannel.push_back(convertedSample);
            }
        }
    }
    else if (numChannels == 2) {
        if (bitsPerSample == 8) {
            for (size_t i = 0; i < audioData.size(); i += 2) {
                char leftSample = static_cast<char>(
                    static_cast<unsigned char>(audioData[i]) - 128
                    );
                char rightSample = static_cast<char>(
                    static_cast<unsigned char>(audioData[i + 1]) - 128
                    );
                leftChannel.push_back(leftSample);
                rightChannel.push_back(rightSample);
            }
        }
        else if (bitsPerSample == 16) {
            for (size_t i = 0; i < audioData.size(); i += 4) {
                int16_t leftSample = static_cast<int16_t>(
                    static_cast<unsigned char>(audioData[i]) |
                    (static_cast<unsigned char>(audioData[i + 1]) << 8)
                    );
                int16_t rightSample = static_cast<int16_t>(
                    static_cast<unsigned char>(audioData[i + 2]) |
                    (static_cast<unsigned char>(audioData[i + 3]) << 8)
                    );
                leftChannel.push_back(static_cast<char>((leftSample >> 8) & 0xFF));
                rightChannel.push_back(static_cast<char>((rightSample >> 8) & 0xFF));
            }
        }
    }

    return std::make_pair(leftChannel, rightChannel);
}
