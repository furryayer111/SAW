#include "WAV2TXT.h"
#include <filesystem>
#include <conio.h>
#include <Windows.h>
#include <string>

#define SMPL std::pair<std::vector<char>, std::vector<char>>

namespace fs = std::filesystem;

bool create_directory_if_missing(const fs::path& dir_path) {
	if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
		std::cout << "Папка уже существует: " << dir_path << std::endl;
		return false;
	}

	if (fs::create_directories(dir_path)) {
		std::cout << "Создана папка: " << dir_path << std::endl;
		return true;
	}
	else {
		std::cerr << "Не удалось создать папку: " << dir_path << std::endl;
		return false;
	}
}

template<typename T>
std::vector<T> subvector(const std::vector<T>& data, int start, int end) {
	if (data.empty() || start > end) {
		return {};
	}

	int size = static_cast<int>(data.size());
	if (start < 0) start = 0;
	if (end >= size) end = size - 1;
	if (start > end) return {};

	return std::vector<T>(data.begin() + start, data.begin() + end + 1);
}

template<typename T>
std::vector<T>& operator+=(std::vector<T>& left, const std::vector<T>& right) {
	left.insert(left.end(), right.begin(), right.end());
	return left;
}

class WAVExporter {
public:
	static bool exportToWav(
		const std::pair<std::vector<char>, std::vector<char>>& stereoData,
		const std::string& outputPath,
		int sampleRate = 8000
	) {
		if (stereoData.first.empty() || stereoData.second.empty()) {
			return false;
		}

		size_t numSamples = stereoData.first.size();
		if (stereoData.second.size() != numSamples) {
			return false;
		}

		std::vector<int16_t> pcmData;
		pcmData.reserve(numSamples * 2);

		for (size_t i = 0; i < numSamples; i++) {
			int16_t left = static_cast<int16_t>(static_cast<signed char>(stereoData.first[i])) * 256;
			int16_t right = static_cast<int16_t>(static_cast<signed char>(stereoData.second[i])) * 256;
			pcmData.push_back(left);
			pcmData.push_back(right);
		}

		return writeWav(pcmData, outputPath, sampleRate, 2);
	}

	static bool exportToWav(
		const std::vector<char>& monoData,
		const std::string& outputPath,
		int sampleRate = 8000
	) {
		if (monoData.empty()) {
			return false;
		}

		std::vector<int16_t> pcmData;
		pcmData.reserve(monoData.size());

		for (char sample : monoData) {
			int16_t val = static_cast<int16_t>(static_cast<signed char>(sample)) * 256;
			pcmData.push_back(val);
		}

		return writeWav(pcmData, outputPath, sampleRate, 1);
	}

private:
	static bool writeWav(
		const std::vector<int16_t>& pcmData,
		const std::string& outputPath,
		int sampleRate,
		int channels
	) {
		std::ofstream file(outputPath, std::ios::binary);
		if (!file) return false;

		uint32_t dataSize = static_cast<uint32_t>(pcmData.size() * sizeof(int16_t));
		uint32_t byteRate = sampleRate * channels * sizeof(int16_t);
		uint16_t blockAlign = channels * sizeof(int16_t);
		uint16_t bitsPerSample = 16;

		// RIFF
		file.write("RIFF", 4);
		uint32_t chunkSize = 36 + dataSize;
		file.write(reinterpret_cast<const char*>(&chunkSize), 4);
		file.write("WAVE", 4);

		// fmt
		file.write("fmt ", 4);
		uint32_t subchunk1Size = 16;
		file.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
		uint16_t audioFormat = 1; // PCM
		file.write(reinterpret_cast<const char*>(&audioFormat), 2);
		file.write(reinterpret_cast<const char*>(&channels), 2);
		file.write(reinterpret_cast<const char*>(&sampleRate), 4);
		file.write(reinterpret_cast<const char*>(&byteRate), 4);
		file.write(reinterpret_cast<const char*>(&blockAlign), 2);
		file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

		// data
		file.write("data", 4);
		file.write(reinterpret_cast<const char*>(&dataSize), 4);
		file.write(reinterpret_cast<const char*>(pcmData.data()), dataSize);

		return true;
	}
};

struct outp {
	std::string name;
	size_t start;
	size_t index;

	outp(std::string name, size_t index, size_t start) : name(name), index(index), start(start) {}
	outp() {}
};

static void saveOut(std::string name, std::string data) {
	std::ofstream outFile(name);

	if (outFile.is_open()) {
		outFile << data << std::endl;
	}
	outFile.close();
}

struct piece {
	SMPL samples;

	void clear() {
		samples = {};
	}

	void print() {
		std::cout << "\nLeft: ";
		for (auto& el : samples.first) {
			std::cout << (int)el << " | ";
		}
		std::cout << "\nRight: ";
		for (auto& el : samples.second) {
			std::cout << (int)el << " | ";
		}
	}
};


struct namedSFX {
	std::string name;

	std::vector<piece> samplePieces;

	namedSFX(std::string name, std::vector<piece> samples) : samplePieces(samples), name(name) {};

	namedSFX(std::string namer, SMPL samples) {
		name = namer;
		samplePieces.clear();
		piece tmp;
		tmp.samples = samples;
		samplePieces.push_back(tmp);
	}
};

class MainResolver {
public:

	std::vector<piece> divideTrack(int pieceSize, SMPL track_original) {
		std::vector<piece> rt;
		piece p;

		for (size_t i = 0; i < track_original.first.size(); i++) {
			p.samples.first.push_back(track_original.first[i]);
			p.samples.second.push_back(track_original.second[i]);

			if (p.samples.first.size() == pieceSize) {
				rt.push_back(p);
				p = piece{};
			}
		}

		if (!p.samples.first.empty()) {
			while (p.samples.first.size() < pieceSize) {
				p.samples.first.push_back(0);
				p.samples.second.push_back(0);
			}
			rt.push_back(p);
		}

		return rt;
	}

	float comparePieces(SMPL a, SMPL b) {
		float leftk = 0;
		float rightk = 0;

		for (size_t i = 0; i < a.first.size(); i++) {
			leftk += std::abs(int(a.first[i]) - int(b.first[i]));
		}
		leftk /= a.first.size();

		for (size_t i = 0; i < a.second.size(); i++) {
			rightk += std::abs(int(a.second[i]) - int(b.second[i]));
		}
		rightk /= a.second.size();

		return (leftk + rightk) / 2;
	}

	SMPL normalizeStereo(
		const SMPL& input,
		char targetPeak = 127
	) {
		if (input.first.empty() || input.second.empty()) {
			return input;
		}

		int maxAbs = 0;

		for (char sample : input.first) {
			int absVal = std::abs(static_cast<signed char>(sample));
			if (absVal > maxAbs) maxAbs = absVal;
		}

		for (char sample : input.second) {
			int absVal = std::abs(static_cast<signed char>(sample));
			if (absVal > maxAbs) maxAbs = absVal;
		}

		if (maxAbs == 0) {
			return input;
		}

		float normFactor = static_cast<float>(targetPeak) / maxAbs;

		SMPL result;
		result.first.resize(input.first.size());
		result.second.resize(input.second.size());

		for (size_t i = 0; i < input.first.size(); i++) {
			float val = static_cast<signed char>(input.first[i]) * normFactor;
			result.first[i] = static_cast<char>(std::round(val));
		}

		for (size_t i = 0; i < input.second.size(); i++) {
			float val = static_cast<signed char>(input.second[i]) * normFactor;
			result.second[i] = static_cast<char>(std::round(val));
		}

		return result;
	}

	std::vector<std::string> getWAVFilesInDirectory(const std::string& directoryPath) {
		std::vector<std::string> files;

		try {
			if (!fs::exists(directoryPath)) {
				throw std::runtime_error("\nFolder does not exists: " + directoryPath);
			}

			if (!fs::is_directory(directoryPath)) {
				throw std::runtime_error("\nExpended error: " + directoryPath);
			}

			for (const auto& entry : fs::directory_iterator(directoryPath)) {
				if (fs::is_regular_file(entry.path())) {
					std::string filename = entry.path().filename().string();
					if (filename.size() >= 4) {
						std::string ext = filename.substr(filename.size() - 4);

						std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

						if (ext == ".wav") {
							files.push_back(filename);
						}
					}
				}
			}

			std::sort(files.begin(), files.end());

		}
		catch (const fs::filesystem_error& e) {
			std::cerr << "\nFilesystem error: " << e.what() << std::endl;
		}
		catch (const std::exception& e) {
			std::cerr << "\nUnexpended error: " << e.what() << std::endl;
		}

		return files;
	}
};

int main() {
	std::cout << "  .--.--.      ,---,                  .---. \n";
	std::cout << " /  /    '.   '  .' \\                /. ./| \n";
	std::cout << "|  :  /`. /  /  ;    '.          .--'.  ' ; \n";
	std::cout << ";  |  |--`  :  :       \\        /__./ \\ : | \n";
	std::cout << "|  :  ;_    :  |   /\\   \\   .--'.  '   \\' . \n";
	std::cout << " \\  \\    `. |  :  ' ;.   : /___/ \\ |    ' ' \n";
	std::cout << "  `----.   \\|  |  ;/  \\   \\;   \\  \\;      : \n";
	std::cout << "  __ \\  \\  |'  :  | \\  \\ ,' \\   ;  `      | \n";
	std::cout << " /  /`--'  /|  |  '  '--'    .   \\    .\\  ; \n";
	std::cout << "'--'.     / |  :  :           \\   \\   ' \\ | \n";
	std::cout << "  `--'---'  |  | ,'            :   '  |--\"\n";
	std::cout << "            `--''               \\   \\ ;     \n";
	std::cout << "                                 '---\"\n";
	std::cout << "                                            \n";
	std::cout << "====================================================================================================\n";
	std::cout << "\nSample Audio Weaver\n\n";
	std::cout << ">> By _Phan70m\n\n";

	system("pause");

	std::string in;
	std::string out;
	int step = 100;

	system("cls");
	std::cout << "this utility allows you to recreate a similar track from short effects or pieces.\nOnly .wav files with these characteristics are allowed:\n   Format: PCM\n   Channels: stereo only\n   BitRate: 8000\n   BitsPerSample: 8\nThe program may not work correctly with other file types.\n\n";
	std::cout << "INPUT FILE (with '.wav'):\n>> ";
	std::cin >> in;
	std::cout << "OUTPUT FILE (with '.txt'):\n>> ";
	std::cin >> out;
	std::cout << "Step (this parameter determines the size of the comparison step. the larger it is, the greater the sound match will be, and the greater the number of SFX triggers will be):\n>> ";
	std::cin >> step;
	system("cls");

	std::string stage = "connecting the parser";
	int stageN = 0;

	std::cout << "The transformation is underway...\n\n";

	std::cout << "\rCurrent stage: " << stage << " (" << ++stageN << "/8)";

	WAVConverter parser;
	MainResolver resolver;

	stage = "Parsing WAV to vectors";
	std::cout << "\rCurrent stage: " << stage << " (" << ++stageN << "/8)            ";

	SMPL original = parser.convertToChannels(in);

	stage = "Parsing SFXs to vectors";
	std::cout << "\rCurrent stage: " << stage << " (" << ++stageN << "/8)            ";

	std::vector<std::string> sfxsf = resolver.getWAVFilesInDirectory("SFXs");

	std::vector<namedSFX> sfxs;

	for (auto& sf : sfxsf) {
		namedSFX current_sfx(sf, parser.convertToChannels("SFXs/" + sf));
		sfxs.push_back(current_sfx);
	}

	stage = "dividing pieces";
	std::cout << "\rCurrent stage: " << stage << " (" << ++stageN << "/4)            ";

	std::vector<namedSFX> sfx_div;
	for (auto& el : sfxs) {
		namedSFX csf(el.name, resolver.divideTrack(step, el.samplePieces[0].samples));
		sfx_div.push_back(csf);
	}

	std::vector<piece> orig_div = resolver.divideTrack(step, original);

	system("cls");
	size_t smI = 0;
	size_t sMAX = orig_div.size();

	std::cout << "\n.";
	std::cout << "\rAssembling samples; ( " << smI << "/" << sMAX << " )            ";

	std::vector<outp> output_dat;
	SMPL preview;

	preview.first.push_back(0);
	preview.second.push_back(0);

	for (size_t i = 0; i < orig_div.size(); i++) {

		std::vector<float> comap;
		std::vector<std::string> namelist;
		std::vector<size_t> indexes;

		for (size_t j = 0; j < sfx_div.size(); j++) {
			for (size_t o = 0; o < sfx_div[j].samplePieces.size(); o++) {
				comap.push_back(resolver.comparePieces(sfx_div[j].samplePieces[o].samples, orig_div[i].samples));
				namelist.push_back(sfx_div[j].name);
				indexes.push_back(o);
			}
		}
		auto min_iter = std::min_element(comap.begin(), comap.end());
		int max = std::distance(comap.begin(), min_iter);

		outp ret(namelist[max], indexes[max], indexes[max] * step);
		output_dat.push_back(ret);
			
		SMPL vector_this = parser.convertToChannels("SFXs/" + namelist[max]);
		size_t start = indexes[max];
		std::vector<char> subvector_left = subvector(vector_this.first, start * step, ( start + 1 ) * step);
		std::vector<char> subvector_right = subvector(vector_this.second, start * step, ( start + 1 ) * step);

		preview.first += (subvector_left);
		preview.second += (subvector_right);
		smI++;
		std::cout << "\rAssembling samples; ( " << smI << "/" << sMAX << " )            ";
	}

	std::string output_string = std::to_string(step) + "~";

	for (auto& el : output_dat) {
		output_string += '[' + el.name + ':' + std::to_string(el.index) + ':' + std::to_string(el.start) + ']';
	}
	create_directory_if_missing("output");

	saveOut("output/" + out, output_string);

	system("cls");
	std::cout << "  .--.--.      ,---,                  .---. \n";
	std::cout << " /  /    '.   '  .' \\                /. ./| \n";
	std::cout << "|  :  /`. /  /  ;    '.          .--'.  ' ; \n";
	std::cout << ";  |  |--`  :  :       \\        /__./ \\ : | \n";
	std::cout << "|  :  ;_    :  |   /\\   \\   .--'.  '   \\' . \n";
	std::cout << " \\  \\    `. |  :  ' ;.   : /___/ \\ |    ' ' \n";
	std::cout << "  `----.   \\|  |  ;/  \\   \\;   \\  \\;      : \n";
	std::cout << "  __ \\  \\  |'  :  | \\  \\ ,' \\   ;  `      | \n";
	std::cout << " /  /`--'  /|  |  '  '--'    .   \\    .\\  ; \n";
	std::cout << "'--'.     / |  :  :           \\   \\   ' \\ | \n";
	std::cout << "  `--'---'  |  | ,'            :   '  |--\"\n";
	std::cout << "            `--''               \\   \\ ;     \n";
	std::cout << "                                 '---\"\n";
	std::cout << "                                            \n";
	std::cout << "====================================================================================================\n";

	std::cout << "Samples assembled!\n\nOutput file was saved: " << out;

	out.erase(out.size() - 4);
	WAVExporter::exportToWav(preview, "output/" + out + " (preview).wav");

	Sleep(1000);
	system("cls");
	std::cout << "  .--.--.      ,---,                  .---. \n";
	std::cout << " /  /    '.   '  .' \\                /. ./| \n";
	std::cout << "|  :  /`. /  /  ;    '.          .--'.  ' ; \n";
	std::cout << ";  |  |--`  :  :       \\        /__./ \\ : | \n";
	std::cout << "|  :  ;_    :  |   /\\   \\   .--'.  '   \\' . \n";
	std::cout << " \\  \\    `. |  :  ' ;.   : /___/ \\ |    ' ' \n";
	std::cout << "  `----.   \\|  |  ;/  \\   \\;   \\  \\;      : \n";
	std::cout << "  __ \\  \\  |'  :  | \\  \\ ,' \\   ;  `      | \n";
	std::cout << " /  /`--'  /|  |  '  '--'    .   \\    .\\  ; \n";
	std::cout << "'--'.     / |  :  :           \\   \\   ' \\ | \n";
	std::cout << "  `--'---'  |  | ,'            :   '  |--\"\n";
	std::cout << "            `--''               \\   \\ ;     \n";
	std::cout << "                                 '---\"\n";
	std::cout << "                                            \n";
	std::cout << "====================================================================================================\n\n";
	std::cout << "         >>> REFERENCE <<<\nThe txt file is encoded as follows:\n```STEP~[FILENAME:INDEX:START][FILENAME:INDEX:START]...```\nWhere:\n    STEP is the step size in samples(fixed for all sfx).\n    FILENAME is the name of the specific sfx.It is the same as the name of the sfx.wav file.\n    INDEX is the index of the effect piece in STEP.When the SFX is divided into parts of STEP samples, INDEX determines the number of the specific part, starting from zero. \n    START is the starting point of the specific SFX in samples.\n";

	system("pause");

}
