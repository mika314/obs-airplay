# obs-airplay

OBS plugin to capture screen mirroring from iOS and macOS devices

## Build Instructions

### Ubuntu-Like OSes 22.04

#### Install Dependencies
```bash
sudo apt-get install -y clang pkg-config libavcodec-dev libavformat-dev libavutil-dev libswresample-dev git libobs-dev libavahi-compat-libdnssd-dev libplist-dev
```

#### Install Build Tool `coddle` if not Installed
```bash
git clone https://github.com/coddle-cpp/coddle.git && cd coddle && ./build.sh
sudo ./deploy.sh
cd ..
```

#### Clone the Plugin
```bash
git clone --recurse-submodules https://github.com/mika314/obs-airplay.git
```

#### Build
```bash
cd obs-airplay
coddle
```

#### Copy Plugin File into OBS Plugin Directory
I actually make a symbolic link in the `/usr/lib/x86_64-linux-gnu/obs-plugins/` directory.
```bash
sudo ln -s /home/mika/prj/obs-airplay/obs-airplay.so /usr/lib/x86_64-linux-gnu/obs-plugins/obs-airplay.so
```
