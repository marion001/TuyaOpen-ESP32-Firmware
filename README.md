# TuyaOpen ESP32 Firmware

TuyaOpen firmware project for an ESP32 device, with a companion Tuya panel mini-app. The embedded project was previously built for the classic ESP32 target. The source tree contains a three-switch data-point example; adjust the product identifiers and data-point mapping for your own Tuya product before building.

## Repository contents

- `projects/ESP32_TUYA/source/embedded/` — ESP32 firmware source and build configuration.
- `projects/ESP32_TUYA/source/miniapp/` — optional Tuya panel mini-app.
- `projects/ESP32_TUYA/source/embedded/dist/ESP32_TUYA_1.0.0/README.md` — prebuilt package flash command and verified flash/partition offsets.

The TuyaOpen SDK and toolchain are installed separately. Do not copy the SDK repository into this project.

## Install TuyaOpen SDK and build tools

Install TuyaOpen IDE/CLI and the ESP32 toolchain using the SDK's official instructions. If cloning the SDK manually, run this from the parent folder of this repository so the SDK is a sibling of the project folder:

```powershell
git clone --branch master --recurse-submodules https://github.com/tuya/TuyaOpen.git TuyaOpenSDK
```

Use the TuyaOpen IDE SDK manager or the SDK's setup instructions to install and activate the required Python/toolchain environment. In a TuyaOpen-enabled terminal, check that `tos.py` is available:

```powershell
tos.py --help
```

The previous successful build used TuyaOpen SDK commit `d624665af44887500e1a9642614069568d751462`. SDK `master` changes over time; record the SDK commit and submodule revisions used for each release.

## Configure your Tuya product and authorization

Create/configure a product in the Tuya IoT developer platform using the [Tuya product-creation guide](https://developer.tuya.com/en/docs/iot-device-dev/application-creation?id=Kbxw7ket3aujc), then use that product's PID and data-point definitions. The project currently repeats the product ID in these files; update them consistently:

| File | Setting | What to enter |
|---|---|---|
| `projects/ESP32_TUYA/source/embedded/include/tuya_config.h` | `TUYA_PRODUCT_ID` | PID of the Tuya product |
| `projects/ESP32_TUYA/tuyaopen.project.ini` | `[product] pid` | The same product PID |
| `projects/ESP32_TUYA/source/miniapp/project.tuya.json` | `projectId` | Keep aligned with the product used by the panel |

The Tuya panel's `appid` in `project.tuya.json` is a separate MiniApp/App ID. Fill it only when Tuya gives you an App ID for the panel; it is currently blank. The panel can be omitted if you only need the embedded firmware.

### TuyaOpen UUID and auth key

`TUYA_OPENSDK_UUID` and `TUYA_OPENSDK_AUTHKEY` are TuyaOpen-specific device authorization values. They are not a Tuya IoT access ID/secret and are not a LocalTuya `local_key`. Obtain a TuyaOpen authorization code for the device/module through the TuyaOpen process described in the [TuyaOpen SDK cloud-app guide](https://github.com/tuya/TuyaOpen/tree/master/apps/tuya_cloud); the guide currently links to Tuya's [authorization-code page](https://platform.tuya.com/purchase/index?type=6).

In `tuya_app_main.c`, startup first calls `tuya_authorize_read(&license)`. If the module already has a TuyaOpen authorization code provisioned, that stored code is used and the fallback macros below are ignored. If no authorization is stored, the code uses these macros from `tuya_config.h`:

```c
#define TUYA_OPENSDK_UUID    "uuidxxxxxxxxxxxxxxxx"
#define TUYA_OPENSDK_AUTHKEY "keyxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
```

Those strings in the repository are placeholders, not working credentials. For a local build on a module without pre-provisioned authorization, replace them with the UUID/auth key issued for that module. The values are compiled into the firmware image. Keep real values out of commits and do not publish a firmware binary that contains a license you are not authorized to redistribute. Use a private working copy/build for device-specific credentials, then confirm the public source still contains placeholders before committing.

The firmware provisions Wi-Fi through the Tuya Wi-Fi AP flow in the app; this project does not require a Wi-Fi password to be hard-coded in source.

### Data points (DPs)

The example currently handles three Boolean switches with IDs 1, 2, and 3. The embedded DP header and mini-app schema are generated from the Tuya product data model:

- `projects/ESP32_TUYA/source/embedded/include/tuya_dp_profile.h`
- `projects/ESP32_TUYA/source/miniapp/src/devices/schema.ts`
- `projects/ESP32_TUYA/source/miniapp/src/config/dpCodes.ts`

After changing product DPs in Tuya, use the TuyaOpen IDE **Sync DPs from Cloud** / DP generation workflow to regenerate those files. Do not hand-edit generated files. Update `source/embedded/src/tuya_app_main.c` when your product uses different DP IDs, types, or hardware behavior; changing the generated profile alone does not implement new device behavior.

## Build the ESP32 firmware

Open a TuyaOpen-enabled PowerShell terminal at the repository root:

```powershell
Set-Location projects\ESP32_TUYA\source\embedded
tos.py check
tos.py build
```

The build output is written under `projects/ESP32_TUYA/source/embedded/dist/`. Build products are ignored by Git. If the installed CLI reports an unsupported option, check `tos.py --help` for that SDK version.

## Flash the ESP32

For the usual source-build workflow, connect the board, find its current COM port in Device Manager, then run from `source/embedded`:

```powershell
tos.py flash --port COM4
```

Replace `COM4` with the actual port. Flashing overwrites data on the connected device. Check the board, port, product configuration, and any stored data before proceeding.

For the prebuilt 1.0.0 binaries, use the detailed [firmware package instructions](projects/ESP32_TUYA/source/embedded/dist/ESP32_TUYA_1.0.0/README.md). They explain when to flash the combined QIO image at `0x0000` versus the separate files at their individual offsets. Do not flash the combined QIO image at the application offset.

## Build the optional Tuya panel mini-app

See [`projects/ESP32_TUYA/source/miniapp/README.md`](projects/ESP32_TUYA/source/miniapp/README.md). The panel needs its own Tuya App ID, a product association, and synchronized data points. Build the panel separately from the ESP32 firmware.

## Before publishing

- Check `git diff` and staged files to ensure local UUID/auth keys, account credentials, and device logs are absent.
- Review whether the configured product PID should be public; it identifies a Tuya product even though it is not an authorization secret.
- Add a project `LICENSE` and required TuyaOpen/third-party notices before public distribution. The SDK and components keep their own licenses.
- Publish firmware binaries as GitHub Release assets if needed. The source repository ignores generated `.bin`, `.elf`, `.map`, and build-cache files by default.

