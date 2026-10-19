# libSCT

[English](README.md) | [Русский](README.ru.md)

**A native modding library for Plants vs. Zombies 2.**

`libSCT` is a C++ native library containing custom gameplay modifications and fixes for Plants vs. Zombies 2. It is designed for the original game build **9.6.1** and uses hardcoded function addresses and memory offsets.

The source code and individual modules are free to use, modify, and integrate into other projects. No credit is required.

## Features

- **PassDamage Armor** — custom zombie armor behavior with configurable projectile interactions.
- **Umbrella Leaf** — custom plant behavior, including projectile detection and bouncing.
- **Quest Rewards Fix** — fixes plant icons displayed in quest reward UI.
- **Lawn Zoom Fix** — adjusts lawn zoom behavior.

## Compatibility

**Supported game build: Plants vs. Zombies 2 v9.6.1 (original build only).**

Function addresses and memory offsets are hardcoded for this specific game binary. The library does not automatically resolve them through signature scanning or runtime pattern matching.

Other game versions are not supported. Porting to another build requires identifying the corresponding addresses and offsets, adapting the affected hooks, and testing the modifications again.

## Installation

`libSCT` is loaded as a native Android shared library. The following method describes how to integrate it into a modified APK.

### 1. Extract the APK

Use an APK editing tool of your choice. For example, you can use [APKLab](https://github.com/APKLab/APKLab) in Visual Studio Code.

### 2. Add the library

Place `libSCT.so` in the APK's native library directory:

```text
lib/arm64-v8a/libSCT.so
```

The APK and target environment must support the ARM64 architecture.

### 3. Add the library loading call

Open:

```text
smali_classes2/com/popcap/PvZ2/PvZ2GameActivity.smali
```

Find the following code that loads the original game library:

```smali
const-string v0, "PVZ2"
invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
```

Immediately after it, insert:

```smali
const-string v0, "SCT"
invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
```

This instructs Android to load `libSCT.so` after `libPVZ2.so`.

### 4. Rebuild the APK

Rebuild and sign the APK using your preferred tool, then install it in a compatible environment.


### 5. Additionally

To ensure the functionality of the umbrella leaf and the passdamage armor, you must also recreate their instances on the OBB side.

You also need to add the corresponding texture and animation packs.

Everything required for this is located in the OBB folder.

## If the game does not start, check the library location, architecture, OBB, dependencies, and system logs.


## Building

### Requirements
- Android NDK r27d
- CMake
- Ninja


The project can be built on Windows or another supported environment using CMake and the Android NDK.

The resulting library is `libSCT.so`, built for `arm64-v8a` and Android API 26.

### Compile

Configure the project with the Android NDK toolchain:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/android-ndk/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DOBBY_GENERATE_SHARED=OFF
```

Build the library:

```bash
cmake --build build
```

The output library is named `libSCT.so`. Its exact location depends on the build configuration.

## Acknowledgements

- [Dobby](https://github.com/jmpews/Dobby) - an inline hooking framework used by this project.

## License

The original code in this repository is released under the **BSD Zero Clause License (0BSD)**.

You are free to use, copy, modify, redistribute, and incorporate this code into your own projects, including proprietary and commercial projects. Attribution is not required.

See [LICENSE](LICENSE) for the full license text. Third-party components retain their respective licenses.

## Disclaimer

This is an independent community project and is not affiliated with, endorsed by, or sponsored by Electronic Arts or PopCap Games. Plants vs. Zombies and related assets are the property of their respective owners.
