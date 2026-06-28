# SAW (Sample Audio Weaver) - Updated

## Overview

SAW is a C++ tool that reconstructs an audio track by assembling it from short sound effects (SFX). The program analyzes an original WAV file and finds the best matching SFX pieces to recreate it, producing a text-based sequence, a preview audio file, AND a Geometry Dash level file (`.gmd`)!

## How It Works

1. **Input**: The user provides:
   - An original `.wav` file (the target track)
   - Multiple short `.wav` files (SFX) stored in an `SFXs/` folder
   - A "step" parameter that determines the size of comparison chunks

2. **Process**: 
   - Divides both the original track and all SFX files into pieces of equal size (determined by the step parameter)
   - For each piece of the original, finds the SFX piece that most closely matches it
   - Builds a sequence of the best-matching SFX pieces

3. **Output**:
   - A `.txt` file with the reconstruction sequence
   - A preview `.wav` file showing the assembled result
   - A `.gmd` file (Geometry Dash level) that can be imported into the game

## Requirements

- **Audio Format**: WAV files with specific characteristics:
  - Format: PCM
  - Channels: Stereo only
  - Sample Rate: 8000 Hz
  - Bits Per Sample: 8-bit

>  The program may not work correctly with other file formats or settings

## Output Format

### TXT File
The generated `.txt` file follows this encoding:
```
[STEP]~[FILENAME:INDEX:START][FILENAME:INDEX:START]...
```

Where:
- **STEP**: The step size in samples (fixed for all SFX)
- **FILENAME**: Name of the SFX file used
- **INDEX**: The piece number from the divided SFX (starting from 0)
- **START**: The starting position in samples

### GMD File (Geometry Dash Level)
The program can export the reconstructed sequence as a Geometry Dash level file (`.gmd`). This allows you to:
- Import the audio reconstruction directly into Geometry Dash
- Use the level as a visual representation of your audio project
- Share your creations with the Geometry Dash community

**Note**: The GMD export feature creates a playable level where each trigger represents a sound effect piece from your reconstruction.

## Example Output
```
100~[s13587.wav:1:0][s22461.wav:104:0][s2842.wav:62:0][s288.wav:67:0]...
```

## File Structure
```
project/
├── SAW.exe
├── SFXs/
│   ├── sound1.wav
│   ├── sound2.wav
│   └── ...
└── output/
    ├── result.txt
    ├── result (preview).wav
    └── result.gmd
```

## Usage

1. Place all your SFX files in the `SFXs/` folder
2. Run the program
3. Enter the path to your original WAV file
4. Enter the desired output filename (will be saved as `.txt`)
5. Enter the step size (recommended range: 50-200)
6. Choose whether to export as GMD (Geometry Dash level)
7. The program will process and generate:
   - A sequence file in the `output/` folder
   - A preview WAV file for verification
   - A GMD level file (if selected)

## Technical Details

- **Comparison Method**: Uses absolute difference averaging between left and right channels to find the best-matching SFX piece
- **Normalization**: Audio is normalized to prevent clipping
- **Reconstruction**: The preview output is an 8-bit stereo WAV at 8000 Hz

## Limitations

- Only works with 8-bit stereo WAV files at 8000 Hz
- Requires all SFX files to be placed in the `SFXs/` folder
- Performance depends on the number of SFX files and the step size

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Created by: _Phan70m**

---

# Release Notes - v1.1

##  Features
- Reconstruct audio tracks from short sound effects
- Automatic SFX matching using similarity comparison
- Generates TXT sequence, WAV preview, and GMD level
- Configurable step size for fine-tuning results
- **NEW**: Geometry Dash level export for audio visualization

##  Requirements
- Windows OS
- Input: 8-bit stereo WAV files (8000 Hz)
- SFX files must be placed in `SFXs/` folder

##  How to Use
1. Place all SFX files in `SFXs/` folder
2. Run `SAW.exe`
3. Follow the prompts for:
   - Input WAV file path
   - Output TXT filename
   - Step size (recommended: 50-200)
   - GMD export option (Yes/No)

##  Output
- `output/[filename].txt` - Sequence data
- `output/[filename] (preview).wav` - Reconstructed audio preview
- `output/[filename].gmd` - Geometry Dash level file (optional)

##  Geometry Dash Integration
The new GMD export feature creates a level where:
- Each SFX trigger is placed in sequence
- The level length corresponds to your audio reconstruction
- Easy to import and share with the GD community

##  Limitations
- Only supports 8-bit stereo PCM WAV at 8000 Hz
- Performance depends on number of SFX files and step size

##  Technical Improvements
- Added WAVExporter class for better audio handling
- Improved memory management
- Better error messages and user feedback

##  Developer
_Phan70m

##  License
MIT License
