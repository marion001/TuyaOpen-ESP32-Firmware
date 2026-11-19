# ESP32_TUYA firmware project

This folder contains the embedded TuyaOpen application and the optional Tuya panel mini-app. For the complete setup guide, including SDK installation, credentials, PID, data points, and release guidance, read the [repository README](../../README.md).

## Values to configure

- `source/embedded/include/tuya_config.h`: `TUYA_PRODUCT_ID`, plus `TUYA_OPENSDK_UUID` and `TUYA_OPENSDK_AUTHKEY` only when the module does not already have a TuyaOpen license provisioned.
- `tuyaopen.project.ini`: `[product] pid`; keep it equal to `TUYA_PRODUCT_ID`.
- `source/miniapp/project.tuya.json`: `projectId` should match the product used by the panel; `appid` is a separate Tuya MiniApp/App ID and is currently blank.
- Product data points: sync from Tuya Cloud through TuyaOpen IDE. The generated DP profile/schema does not replace the application logic in `source/embedded/src/tuya_app_main.c`.

The repository contains placeholder UUID/auth key strings. For a non-provisioned module, replace them locally with a TuyaOpen authorization code issued for that device. These values are compiled into the firmware; do not commit real credentials or publish a binary containing a license you cannot redistribute. A pre-provisioned module's stored license is read first and takes precedence over these fallback macros.

## Build and flash

From the repository root, with the TuyaOpen toolchain active:

```powershell
Set-Location projects\ESP32_TUYA\source\embedded
tos.py check
tos.py build
tos.py flash --port COM4
```

Replace `COM4` with the board's current port. For manual flashing of the prebuilt 1.0.0 files, see [`source/embedded/dist/ESP32_TUYA_1.0.0/README.md`](source/embedded/dist/ESP32_TUYA_1.0.0/README.md).
