# Tuya panel mini-app

This is the optional Tuya panel for the ESP32_TUYA product. It builds separately from the ESP32 firmware. The repository-root README explains the Tuya product PID and TuyaOpen authorization values; this file covers panel-specific setup.

## Values to configure

Edit `project.tuya.json`:

- `appid`: enter the Tuya MiniApp/App ID assigned to your panel project. It is currently empty. This is not the device PID or the TuyaOpen UUID/auth key.
- `projectId`: keep it associated with the Tuya product used by the firmware. When changing products, update the matching PID in `tuyaopen.project.ini` and `source/embedded/include/tuya_config.h` too.
- `appVersion`: update when preparing a panel release.

Do not put Tuya account passwords, cloud access secrets, device UUID/auth keys, or local keys in this file or commit them to GitHub.

## Data points

The panel currently describes three Boolean switches (`switch_1`, `switch_2`, `switch_3`, IDs 1–3). After changing the product's data points in Tuya, open the project in TuyaOpen IDE and run **Sync DPs from Cloud**. The IDE generates `src/devices/schema.ts`, `src/config/dpCodes.ts`, and the embedded `source/embedded/include/tuya_dp_profile.h`. Do not edit generated files by hand. Update the page and embedded application logic when changing DP types or adding hardware behavior.

## Install dependencies and build

Install Node.js and npm versions supported by the Tuya Ray CLI, then run from this directory:

```powershell
npm install
npm run build
```

The package script runs `ray build --target tuya`. The configured output directory is `dist/tuya/`. Use TuyaOpen IDE/Tuya's panel workflow to preview, upload, and associate the built panel with the product; a successful local build alone does not publish it to Tuya Smart.

For local web preview, run:

```powershell
npm run start
```

See the official Tuya Ray/Tuya MiniApp documentation for account setup and release review requirements.
