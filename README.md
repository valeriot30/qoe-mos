
# Real-Time QoE Measurement

A real-time system for continuous video streaming quality evaluation based on the ITU-T P.1203 standard. Designed for live streaming scenarios, this tool estimates the Mean Opinion Score (MOS) with segment-level granularity and accounts for playback stalls.

## Getting started

### Prerequisites

Supported systems 

In order to run the script, you need:

- FFmpeg (only needed for Mode 2 / 3)
- Python 3.7 or higher
	* Modern Linux distributions should come with Python 3. If not, [pyenv](https://github.com/pyenv/pyenv) is recommended to get a user-level Python 3 installation.
  * Under macOS, use [Homebrew](https://brew.sh/) and `brew install python` and read the printed messages.
  * Windows is not supported at this moment
- ITU-T P.1203 python implementation, which you can get [here](https://github.com/itu-p1203/itu-p1203), installed globally or via virtualenv
- ffmpeg-debug-qp, for running in Mode 3. You can get that [here](https://github.com/slhck/ffmpeg-debug-qp.git), but it's quite tricky to compile because you need FFmpeg 4.

#### Requirements for compiling
- Make
- GCC or Clang
- libpthread (POSIX Standard)

### Installation

First of all, clone the repository:

```bash
git clone https://github.com/valeriot30/qoe-mos.git
```

Then, open the directory:

```bash
cd qoe-mos
```

Compile using Make
```bash
make 
```

You should get a executable
```bash
./main 
```


## CLI Usage

There are severals configuration in which you can start the script. Look at the help

```c
--------- EVALUATION SYSTEM -------- 
 usage: ./main [N] [p] [d] [one_step] [port] [mode] 
 N: the window size 
 p: the window step 
 d: the segments duration
 one_step: choose if you want to evaluate N segments in the window or 1 segment at time and then average N MOS 
 port: The listening port of the evaluation system 
 mode: the mode of operation for the ITU standard 

 usage: ./main [config_file] 
 [config_file] is a json file containing all the parameters. See config.example.json 

```
### Usage Examples
So you could start the script like:
```bash
./main 10 4 2 1 3000 1
```
Or composing a json file by renaming ```config.example.json``` into ```config.json```
The input JSON file (see files in `config.example.json`) must have at least the following data:
```json
{
	"N": 10,
	"p": 4,
	"d": 2,
	"one_step": 1,
	"port": 3000,
	"mode": 1
}
```
You can pass a config file via
```bash
./main config.json
```

## Resources

In order to test the live evaluation, you can use [HLS.js](https://github.com/video-dev/hls.js) as open source player supporting HLS.
To get one live streaming, you can lookup [this](https://github.com/iptv-org/iptv?) repository, containing some free IPTV contents.

## License

This repository is licensed under [MIT](https://choosealicense.com/licenses/mit/)