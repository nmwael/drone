# Dev Container: USB / Serial Passthrough

The dev container is designed to **always boot**, even when no ESP32, RP2040,
or other USB serial device is plugged in.

## Why hard `--device` entries break the container

Older versions of `.devcontainer/devcontainer.json` used:

```json
"mounts": [
  "source=/dev/bus/usb,target=/dev/bus/usb,type=bind",
  "source=/dev/ttyUSB0,target=/dev/ttyUSB0,type=bind"
],
"runArgs": [
  "--device=/dev/bus/usb:/dev/bus/usb",
  "--device=/dev/ttyUSB0:/dev/ttyUSB0"
]
```

On Windows + Podman (WSL2) those device paths do not exist until a device is
attached. Podman then fails at container creation:

```
no devices found in /dev/bus/usb:/dev/bus/usb:rwm: invalid argument
```

Worse, the bind-mount source `/dev/ttyUSB0` is auto-created as an empty
**directory**, which prevents the real device node from appearing later.

## Graceful approach

Mount the whole host `/dev` and run the container privileged:

```json
"mounts": [
  "source=/dev,target=/dev,type=bind"
],
"runArgs": [
  "--privileged"
]
```

`/dev` always exists, so container creation never fails. Any USB device that
appears on the Podman/WSL host later is immediately visible inside the
container. `--privileged` allows the container to open it.

This is the configuration used by `.devcontainer/devcontainer.json`.

## Attaching a board on Windows + Podman

The Podman machine is a WSL2 distribution named `podman-machine-default`.
Use [usbipd-win](https://github.com/dorssel/usbipd-win) from an **elevated**
PowerShell:

```powershell
usbipd list
usbipd bind --busid <BUSID>          # once per device
usbipd attach --wsl --distribution podman-machine-default --busid <BUSID>
```

Then verify from Windows:

```powershell
podman machine ssh "ls -l /dev/ttyUSB* /dev/ttyACM*"
```

If a stale empty directory exists from an earlier failed mount, remove it:

```powershell
podman machine ssh "sudo rmdir /dev/ttyUSB0"
```

Inside the container the device is visible as `/dev/ttyUSB0` or
`/dev/ttyACM0`. The `vscode` user is added to `dialout` by
`.devcontainer/setup.sh`, so `idf.py flash` and `picotool` work without sudo.

## Notes

- `postCreateCommand` runs as the `vscode` user, so `setup.sh` is invoked with
  `sudo` (it installs packages and writes to `/opt` and `/etc`).
- Do **not** add `--device=/dev/bus/usb:/dev/bus/usb`: Podman interprets a
  directory device as a device list and fails when it is empty.
- The same recipe works for Docker Desktop with a Linux engine, as long as
  `/dev` is visible to the engine.
