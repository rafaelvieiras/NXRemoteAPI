<p align="center">
  <img src="logo.png" alt="NXRemoteAPI Logo" width="150">
</p>

# NXRemoteAPI — Nintendo Switch CFW Remote API

[![GitHub Release](https://img.shields.io/github/release/rafaelvieiras/NXRemoteAPI.svg?style=flat-square)](https://github.com/rafaelvieiras/NXRemoteAPI/releases)
[![Downloads (Current release)](https://img.shields.io/github/downloads/rafaelvieiras/NXRemoteAPI/latest/switch_cfw.zip?label=Downloads%20(Current%20release)&style=flat-square)](https://github.com/rafaelvieiras/NXRemoteAPI/releases)
[![License](https://img.shields.io/github/license/rafaelvieiras/NXRemoteAPI.svg?style=flat-square)](LICENSE)
[![hacs](https://img.shields.io/badge/HACS-custom-orange.svg?style=flat-square)](https://hacs.xyz)
[![CI](https://github.com/rafaelvieiras/NXRemoteAPI/actions/workflows/ci-orchestrator.yml/badge.svg)](https://github.com/rafaelvieiras/NXRemoteAPI/actions/workflows/ci-orchestrator.yml)

A low-memory-footprint remote API and telemetry sysmodule for Nintendo Switch consoles
running Atmosphère Custom Firmware. It exposes console health, current game, storage,
and remote control (buttons, launch, sleep/reboot/shutdown) over a small local HTTP
API, designed to be consumed by whatever automates or logs your life - Home Assistant
today, a Prometheus/Grafana bridge and a lightweight client SDK on the roadmap.

## 🧭 Quick Links

| | | | |
| :--- | :--- | :--- | :--- |
| [✨ Features](#-features) | [📦 Installation](#-installation) | [⚙️ Configuration](#️-configuration) | [🤖 Automations](#-automation-examples) |
| [🛡️ Security](#️-security) | [🧑‍💻 Development](#-development--releases) | [📄 License](#-license) | |

## ✨ Features

- **System Monitoring**:
  - **Firmware Version**: Tracks the currently installed HOS version.
  - **Battery Level**: Precise battery percentage sensor.
  - **Charging Status**: Binary sensor for charging/not charging.
  - **Dock Status**: Detects if the console is docked or in handheld mode.
  - **Sleep Mode**: Detects if the console is in sleep mode (keeps last known values active).
  - **Temperatures**: CPU, GPU, and Skin temperature sensors (disabled by default).
  - **Fan Speed**: Real-time fan RPM monitoring (disabled by default).
  - **Storage**: Monitor free/total space on SD Card and NAND (disabled by default).
- **Game Management**:
  - **Current Game**: Shows the title of the game currently running.
  - **Title ID**: Attribute for the current game's Title ID.
  - **Remote Launch**: Select and launch any installed game directly from your dashboard (wakes the console if needed).
- **Remote Control & Input**:
  - **Remote Entity**: A full Home Assistant `remote` platform to send controller button presses (A, B, X, Y, Home, etc.) in the background.
  - **Reboot**: Button to reboot the console (optionally to a specific payload).
  - **Shutdown**: Button to safely power off the console.
- **Update Management**:
  - **System Update**: Notifies you when a new firmware version is available.
  - **Homebrew App Update**: Monitor and trigger automatic updates for the sysmodule and companion app directly from HA.
  - **Daybreak Integration**: Trigger firmware updates directly via Daybreak (experimental).
- **Modern Standards**:
  - Full support for **Auto-discovery** via Zeroconf/mDNS.
  - **Device Bundling**: All entities grouped under a single Switch device.
  - **Configuration URL**: "Visit" button links directly to the sysmodule status API.
  - **Localization**: English and German translations included.
  - **Energy Efficient**: Optimized polling and dynamic interval during sleep.

## 🙏 Credits & Project History

This project started as a fork of [FaserF/ha-NintendoSwitchCFW](https://github.com/FaserF/ha-NintendoSwitchCFW)
— all credit for the original Home Assistant integration, sysmodule, and companion app
goes to FaserF.

NXRemoteAPI is now a **permanent, definitive fork**, no longer tracked against upstream.
This isn't a comment on the original project — it's that the scope of changes needed
(a polyglot monorepo restructure, splitting the sysmodule into an orchestrator+core pair,
and generalizing the HTTP API into a low-memory telemetry/automation surface meant to
serve clients well beyond Home Assistant) made merging back upstream impractical. The
Home Assistant integration in this repo continues to work as one client of that API,
alongside whatever else gets built against it.

### Firmware Updates
The integration checks for the latest firmware versions. By default, it uses [THZoria/NX_Firmware](https://github.com/THZoria/NX_Firmware).

> [!TIP]
> You can change the update repository in the **Integration Options**.

> [!WARNING]
> Support is only provided for the default repository. Custom repositories must follow a similar release structure (tag names as versions, `.zip` or `.nro` assets) to be compatible.

## 🎮 Supported Hardware

This integration is designed for Nintendo Switch consoles running Atmosphere Custom Firmware.

| Model | Supported | Notes |
| :--- | :---: | :--- |
| **Nintendo Switch V1 (Erista)** | ✅ | **Primary Test Platform.** Full support. |
| **Nintendo Switch V2 (Mariko)** | ⚠️ | Should work with a modchip, but not actively tested. |
| **Nintendo Switch Lite** | ⚠️ | Should work with a modchip, but not actively tested. |
| **Nintendo Switch OLED** | ⚠️ | Should work with a modchip, but not actively tested. |

> [!IMPORTANT]
> The developer primarily tests this integration on a **Nintendo Switch V1**. While it should theoretically work on all models running Atmosphere, support and troubleshooting will be prioritized for the V1 model.

## 📦 Installation

### 1. Nintendo Switch (Sysmodule & Config App)

To allow the API to talk to your Switch, you must install the background sysmodule and the companion configuration app.
1. Download the latest `main` (background service), `main.npdm` (boot descriptor), `boot2.flag` (autorun flag), and `nxremoteapi.nro` (config app) from the [Releases page](https://github.com/rafaelvieiras/NXRemoteAPI/releases).
2. On your SD card, create the folder: `/atmosphere/contents/010000000000CAFE/exefs/` and `/atmosphere/contents/010000000000CAFE/flags/`.
3. Copy `main` and `main.npdm` to: `/atmosphere/contents/010000000000CAFE/exefs/`.
4. Copy `boot2.flag` to: `/atmosphere/contents/010000000000CAFE/flags/`. (Without this file, the service will not start!)
5. Copy `nxremoteapi.nro` to: `/switch/nxremoteapi.nro`.
6. Reboot your Switch.

### ❓ Why both an .NSO and an .NRO?

Users often ask why this requires two separate files. This is due to technical limitations of the Nintendo Switch Operating System (Horizon):

- **The NSO (Sysmodule)**: This is a background service. Standard Switch applications (.nro) are automatically suspended or closed when you launch a game. To keep monitoring your Switch **while you are playing**, a sysmodule is required as it runs in the background at all times.
- **The NRO (Config App)**: Sysmodules cannot have a graphical user interface (GUI). The NRO provides a user-friendly way to see your console's IP address, generate secure API tokens, and check the status of the background service without having to manually edit configuration files on your SD card.

Together, they provide both the persistent background connectivity and a simple setup experience.

### 2. Home Assistant (Integration)

#### HACS (Recommended)

This integration is fully compatible with [HACS](https://hacs.xyz/).

[![Open your Home Assistant instance and open a repository inside the Home Assistant Community Store.](https://my.home-assistant.io/badges/hacs_repository.svg)](https://my.home-assistant.io/redirect/hacs_repository?owner=rafaelvieiras&repository=NXRemoteAPI&category=integration)

1. Open HACS in Home Assistant.
2. Click on the three dots in the top right corner and select **Custom repositories**.
3. Add `rafaelvieiras/NXRemoteAPI` with category **Integration**.
4. Search for "Nintendo Switch CFW" and install.
5. Restart Home Assistant.

### Manual Installation

1. Download the latest release from the [Releases page](https://github.com/rafaelvieiras/NXRemoteAPI/releases).
2. Extract the `custom_components/switch_cfw` folder into your Home Assistant's `custom_components` directory.
3. Restart Home Assistant.

## ⚙️ Configuration

1. Navigate to **Settings > Devices & Services**.
2. If your Switch is on the same network, it should be **automatically discovered**. Click **Configure**.
3. If not discovered, click **Add Integration** and search for **Nintendo Switch CFW**.
4. Enter the IP address and the **API Token** shown in the Switch app.

## 🛡️ Security

The connection to the Switch is secured via a **secure API Token**.

- On first launch, the Switch app generates a unique random token.
- This token is saved in `sdmc:/config/NXRemoteAPI/settings.json`.
- The background service validates every request against this token.
- You can regenerate the token at any time within the Switch app.

## 🤖 Automation Examples

<details>
<summary><b>Notify when Battery is Low (< 15%)</b></summary>

```yaml
alias: "Switch: Low Battery Notification"
trigger:
  - platform: numeric_state
    entity_id: sensor.nintendo_switch_battery_level
    below: 15
condition:
  - condition: state
    entity_id: binary_sensor.nintendo_switch_charging
    state: "off"
action:
  - service: notify.mobile_app_your_phone
    data:
      title: "Nintendo Switch"
      message: "Battery is low! Please dock the console."
```
</details>

<details>
<summary><b>Alert when a Specific Game is started</b></summary>

```yaml
alias: "Switch: Mario Kart Time!"
trigger:
  - platform: state
    entity_id: sensor.nintendo_switch_current_game
    to: "Mario Kart 8 Deluxe"
action:
  - service: light.turn_on
    target:
      entity_id: light.gaming_room_leds
    data:
      color_name: red
```
</details>

<details>
<summary><b>Notify on Firmware Update</b></summary>

```yaml
alias: "Switch: Firmware Update Available"
trigger:
  - platform: state
    entity_id: update.nintendo_switch_firmware
    to: "on"
action:
  - service: notify.persistent_notification
    data:
      title: "Switch Update"
      message: "A new HOS version {{ state_attr('update.nintendo_switch_firmware', 'latest_version') }} is available!"
```
</details>

<details>
<summary><b>Turn off Lights when Switch is Shutdown</b></summary>

```yaml
alias: "Switch: Power Off Scene"
trigger:
  - platform: state
    entity_id: binary_sensor.nintendo_switch_sleep_mode
    to: "on"
action:
  - service: light.turn_off
    target:
      entity_id: light.living_room
```
</details>

<details>
<summary><b>Pause Media when Switch is Shutdown</b></summary>

```yaml
alias: "Switch: Multi-Command Remote Macro"
trigger:
  - platform: state
    entity_id: sensor.nintendo_switch_current_game
    to: "YouTube"
action:
  - service: remote.send_command
    target:
      entity_id: remote.nintendo_switch
    data:
      command:
        - "UP"
        - "UP"
        - "A"
```
</details>

---

## 🧑‍💻 Development & USB Debugging

For developers or advanced users troubleshooting connection issues, the companion app includes a **Developer Mode** that streams live logs over USB.

### Enabling Developer Mode
1. Connect your Nintendo Switch to your PC via a USB-C cable.
2. Open the **NXRemoteAPI** app on your console.
3. Press **MINUS (-)**.
4. You will see a pink `[ DEV MODE ACTIVE (USB) ]` status on the screen and a message in the logs.

### Accessing Live Logs (PC Side)
Once Developer Mode is active, you can view the live console output on your PC using the `nxlink` tool from the **devkitPro** toolchain:

```bash
# Listen for logs over USB
nxlink -u -s
```

> [!NOTE]
> Developer Mode is intended for advanced troubleshooting. It may slightly increase battery consumption while active.

## 🧑‍💻 Project Infrastructure

This project uses an automated release workflow.
- Releases are tagged automatically.
- The workflow builds the C++ components (NSP & NRO) using DevkitPro.
- Packages the HA integration and generates a dynamic changelog.

See [`docs/api.md`](docs/api.md) for the HTTP API contract shared by every client
(Home Assistant included), and [`AGENTS.md`](AGENTS.md) for the repo's monorepo layout
and contribution conventions.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
