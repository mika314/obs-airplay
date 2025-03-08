# obs-airplay

OBS plugin to capture screen mirroring from iOS and macOS devices

[![I made an AirPlay plugin for OBS](https://user-images.githubusercontent.com/1877406/173214368-17392f78-5af7-4161-a57a-e1b8002c2dd3.png)](https://youtu.be/yN5SMHl9JdY "I made an AirPlay plugin for OBS")

## Build Instructions

### Ubuntu-Like OSes 22.04

#### Install Dependencies
```bash
sudo apt-get install -y clang pkg-config libssl-dev libswscale-dev libavcodec-dev libavformat-dev libavutil-dev libswresample-dev git libobs-dev libavahi-compat-libdnssd-dev libplist-dev libfdk-aac-dev
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

### macOS

#### Install dependencies

```sh
xcode-select --install
brew install \
    fdk-aac \
    ffmpeg \
    libplist \
    openssl \
    pkg-config
```

#### Install the build tool

If you do not already have [Coddle] installed:

```sh
git clone https://github.com/coddle-cpp/coddle.git
cd coddle
./build.sh
sudo ./deploy.sh
cd ..
```

[Coddle]: https://github.com/coddle-cpp/coddle

#### Clone the OBS Studio source

```sh
# Change “30.2.3” to match your installed OBS version.
git clone --branch 30.2.3 https://github.com/obsproject/obs-studio.git
```

This project's library configuration
depends on the OBS source being located in an adjacent directory.
If you want to put the OBS source tree elsewhere,
you will need to update the `resources/macos/lib/obs` symlink accordingly.

#### Clone the plugin

```sh
git clone --recurse-submodules https://github.com/mika314/obs-airplay.git
```

#### Build

```sh
cd obs-airplay
coddle
```

#### Install the plugin

On macOS, OBS plugins are [bundles].
A template is available in the repo
at `resources/macos/obs-airplay.plugin`.
First, we'll copy that to somewhere OBS will find it;
then we'll symlink our compiled plugin into it.

```bash
cp \
    -R resources/macos/obs-airplay.plugin \
    ~/Library/Application\ Support/obs-studio/plugins/
ln -s \
    $(realpath obs-airplay.dylib) \
    ~/Library/Application\ Support/obs-studio/plugins/obs-airplay.plugin/Contents/MacOS/obs-airplay
```

[bundles]: https://en.wikipedia.org/wiki/Bundle_(macOS)
