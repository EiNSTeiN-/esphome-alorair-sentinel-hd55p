# ESPHome for AlorAir Sentinel HD55P / HD55-family CAN monitoring

This repository collects community notes and a starter ESPHome configuration for monitoring and controlling an AlorAir Sentinel HD55P / HD55-family dehumidifier through its optional remote-control CAN port.

The example is written for an [Ebyte ECAN-E02 ESPHome board package](https://github.com/EiNSTeiN-/esphome-ecan-e02), which provides an ESP32, Ethernet, and isolated CAN interface in one inexpensive enclosure. The intended Home Assistant experience is:

1. Flash the ECAN-E02 with ESPHome.
2. Wire the ECAN-E02 CANH/CANL/GND to the dehumidifier remote port.
3. Connect the ECAN-E02 to Ethernet.
4. Adopt the ESPHome device in Home Assistant.

This is community reverse-engineering, not vendor documentation. Confirm wiring on your unit before connecting hardware, and start with the board powered off.

## Current status

The CAN port and protocol vary across the HD55 family:

| Source | Reported model/context | CAN bitrate | Status/request notes |
| --- | --- | --- | --- |
| ECAN-E02 bench validation in this repo | HD55P connected to an Ebyte ECAN-E02 | 50KBPS | Remote-panel idle frame on `0x123` produces repeated status-like frames on `0x123`. |
| Tinymicros HD55 wiki | Original HD55 remote reverse engineering | 50KBPS | Remote and display protocol documented around IDs `0x123` and `0x3b0`. |
| Home Assistant thread, 2023+ | HD55 manufactured April 2023, serial starting `S38` | 125KBPS | Request/status format reported as `0x123` request, status bytes `RH actual, RH set, Temp, D3, Status, D5, D6, D7`. |
| Home Assistant thread, 2024-2026 | HD55, HD55S, HD35P, HDi-family attempts | 50KBPS or 125KBPS | Several working examples, several wiring/CAN-enable gotchas, and some model-specific differences. |

The example config in [`configs/alorair-hd55p-ecan-e02.yaml`](configs/alorair-hd55p-ecan-e02.yaml) defaults to the confirmed ECAN-E02 bench behavior:

- `can_bit_rate: 50KBPS`
- send a periodic remote-panel idle frame on CAN ID `0x123`, every `5s` by default
- decode HD55P remote-panel responses received on `0x123`
- expose humidity, setpoint, temperature, status text, power/continuous/pumping flags, setpoint/continuous controls, and remote-button-style diagnostic controls

If you see no frames, try `can_bit_rate: 125KBPS` with the zero-poll diagnostic before changing hardware. Also verify CANH/CANL are not swapped and that the CAN interface is in `NORMAL` mode when you expect to transmit requests.

You can tune the normal remote-panel polling cadence with:

```yaml
substitutions:
  hd55_poll_interval: 5s
```

## Hardware

Recommended parts:

- Ebyte ECAN-E02 flashed with ESPHome.
- Ethernet connection for the ECAN-E02.
- CAN cable from ECAN-E02 to the dehumidifier remote jack.
- Optional RJ45 breakout or sacrificial CAT5/CAT6 cable.

Do not connect the dehumidifier remote-port VCC pin to the ECAN-E02 unless you have verified the voltage, current capacity, and grounding plan. The ECAN-E02 should normally be powered through its own intended supply, not from the dehumidifier remote jack.

## Validated ECAN-E02 bench wiring

A live HD55P bench setup was validated with the ECAN-E02 connected to the dehumidifier remote/CAN port as follows:

| ECAN-E02 terminal | HD55P cable color | Function |
| --- | --- | --- |
| `G` | Brown | Ground |
| `H` | White/blue | CANH |
| `L` | Blue | CANL |

The connected circuit measured about `119 ohm` across CANH/CANL without adding a bench resistor. On the short bench cable this behaved as a valid terminated CAN bus.

The ECAN-E02 must also be powered from its normal power input. USB-UART power alone is not enough for the isolated CAN side of the ECAN-E02.

## Remote RJ45 / CAN pinout

The Tinymicros wiki documents the HD55 remote RJ45 receptacle as follows, looking into the RJ45 receptacle with the tab down and pin 1 on the left:

| RJ45 pin | Function |
| --- | --- |
| 1 | Not connected |
| 2 | Not connected |
| 3 | Not connected |
| 4 | CANL |
| 5 | CANH |
| 6 | Not connected |
| 7 | VCC, reported as 5 V on Tinymicros and discussed as remote-port power in the thread |
| 8 | GND |

Community wiring notes for common Ethernet cable colors:

| RJ45 pin | Common T568B wire color | Function |
| --- | --- | --- |
| 4 | Blue | CANL |
| 5 | White/blue | CANH |
| 7 | White/brown | VCC |
| 8 | Brown | GND |

Cable colors are not a reliable source of truth. Use pin numbers on the RJ45 connector or continuity-test your cable.

## Manual / terminal block notes

The manual and thread discuss the screw-terminal control block separately from the remote/CAN jack. These terminals are useful context, but the example config in this repository uses the CAN remote jack, not the A1-A9 terminal block.

| Terminal | Name | Description |
| --- | --- | --- |
| A1 | NO | Normally-open relay output |
| A2 | COM | Common for the relay output |
| A3 | ON-D | External input, 24 VAC/DC |
| A4 | COM | Common for the external input |
| A5 | FLOAT | External low-voltage float switch or water sensor input |
| A6 | FLOAT | External low-voltage float switch or water sensor input |
| A7 | VCC | Humidity transducer positive |
| A8 | DATA | Humidity transducer data |
| A9 | GND | Humidity transducer ground |

Thread notes add that A1/A2 close when the dehumidifier is running and could be monitored as a dry-contact indicator. A3/A4 can be used as an external 24 VAC/DC run input on some models, but the thread notes that this may require enabling central/external control on the unit. A5/A6 are intended for a normally-open condensate pump or water-level safety switch.

The official HD55/HD55S manual also states that the machine has a one minute fan delay after shutdown. That is one reason the CAN/control-panel path is preferable to simply cutting mains power when you want normal shutdown behavior.

## Protocol notes

### Confirmed ECAN-E02 / HD55P bench behavior

The tested HD55P responded at `50KBPS` when the ECAN-E02 transmitted an idle remote-panel frame. Early bench validation used one frame per second; the main config now defaults to one frame every five seconds to reduce CAN traffic while keeping Home Assistant state fresh enough for normal control:

| Direction | CAN ID | Data |
| --- | --- | --- |
| ECAN-E02 to HD55P | `0x123` | `01 00 00 32 14 00 00 00` |
| HD55P to ECAN-E02 | `0x123` | examples: `2F 2A 32 18 08 01 14 20`, `2F 2A 32 18 07 01 14 20`, `34 24 32 14 04 01 14 20`, `33 24 32 14 03 01 14 20` |

TWAI status stayed clean during this test: no TX/RX errors, missed frames, overruns, arbitration losses, or bus errors. That confirms the ECAN-E02 CAN pins, isolated transceiver path, HD55P wiring, termination, and `50KBPS` bitrate for this setup.

Two negative tests are also useful:

- `125KBPS` active polling on ID `0x123` produced TWAI TX/RX/bus errors on the same wiring, so it is not the correct bitrate for the tested port.
- `50KBPS` polling with `id=0x123 data=00 00 00 00 00 00 00 00` stayed electrically clean but did not elicit frames.

The byte interpretation for the confirmed `0x123` response is still provisional, but the HD55P config intentionally favors the observed 50KBPS HD55P behavior. The current ESPHome example treats byte `0` as humidity, byte `1` as setpoint, byte `3` as Celsius temperature, and byte `5` as the display/status candidate. Byte `4` changes while the real machine state appears steady, so it is exposed as a raw diagnostic byte and is not used for power or running state.

For the current HD55P 50KBPS remote-panel response:

| Byte | Meaning used by this config |
| --- | --- |
| 0 | Current relative humidity, percent |
| 1 | Humidity setpoint, percent |
| 2 | Remote/display humidity echo |
| 3 | Temperature, Celsius |
| 4 | Raw/volatile diagnostic byte; not a state bitfield |
| 5 | Display/status candidate |
| 6 | Raw/status variant |
| 7 | Raw/status variant |

The current display/status candidate interpretation is:

| Bit | Mask | Meaning used by this config |
| --- | --- | --- |
| 0 | `0x01` | Powered on / idle capable |
| 3 | `0x08` | Continuous mode candidate |
| 4 | `0x10` | Pumping / drain-pump candidate |

The compressor-running bit has not been confirmed for the HD55P 50KBPS display frame. The config therefore does not claim the dehumidifier is running from CAN alone. If running state is needed, prefer a confirmed dry-contact signal from A1/A2 or a new capture taken while the compressor is known to be running.

### Newer HD55 / HD55P-style notes

These reports are still useful for comparison, but they are not the default in this repo after the ECAN-E02 HD55P validation:

| Direction | CAN ID | Data |
| --- | --- | --- |
| Poll/request status | `0x123` | `00 00 00 00 00 00 00 00` |
| Set humidity | `0x123` | first byte is desired RH + `0x80`; remaining bytes `00` |
| Toggle power | `0x123` | `00 00 01 00 00 00 00 00` |
| Toggle drain pump | `0x123` | `00 00 02 01 00 00 00 00` |
| Enable continuous mode | `0x123` | `00 00 04 00 00 00 00 00` |
| Disable continuous mode | `0x123` | send a normal setpoint frame |

Status frames reported in the thread for newer/125KBPS-style setups use this byte layout:

| Byte | Meaning |
| --- | --- |
| 0 | Current relative humidity, percent |
| 1 | Humidity setpoint, percent |
| 2 | Temperature, Celsius |
| 3 | Unknown |
| 4 | Status byte |
| 5 | Unknown/status variant by model |
| 6 | Unknown/status variant by model |
| 7 | Unknown/status variant by model |

The common newer/125KBPS status-byte interpretation is:

| Bit | Mask | Meaning |
| --- | --- | --- |
| 0 | `0x01` | Powered on / idle capable |
| 1 | `0x02` | Running |
| 3 | `0x08` | Continuous mode |
| 4 | `0x10` | Pumping / drain-pump related |

Observed status byte examples from the thread:

| Value | Meaning reported by users |
| --- | --- |
| `0x00` | Normal/off |
| `0x01` | Normal/idle |
| `0x03` | Normal/running |
| `0x08` | Continuous/off |
| `0x09` | Continuous/idle |
| `0x0b` | Continuous/running |
| `0x10` | Off/pumping |
| `0x11` | Idle/pumping |
| `0x13` | Running/pumping |

### Older Tinymicros HD55 notes

Tinymicros reports the dehumidifier and optional remote communicating at 50KBPS.

Remote-to-unit button frames are documented on CAN ID `0x123`:

| Byte | Meaning |
| --- | --- |
| 0 | Always `0x01` |
| 1 | Button status: power, pump, continuous, mode, up/down, etc. |
| 2 | Button-dependent value |
| 3 | RH at remote |
| 4 | Temperature at remote, Celsius |
| 5-7 | Always `0x00` in their capture |

Unit-to-remote display frames are documented on CAN ID `0x3b0`:

| Byte | Meaning |
| --- | --- |
| 0 | RH value to display |
| 3 | Temperature to display, Celsius |
| 5 | Display/status bits |
| 7 | Error/status bits |

The Tinymicros page includes more complete bit tables for remote display symbols and error indications.

The main HD55P config decodes the observed `0x123` 50KBPS response and does not apply the older `0x3b0` display-frame layout to those frames. Setpoint, power, drain-pump, and continuous-mode controls use the HD55P direct command format because the Tinymicros remote-button continuous frame was observed to transmit but not change the tested HD55P state. The remaining remote-button-style mode button is exposed as a diagnostic and should be treated as experimental until you have verified your own unit's response in Home Assistant and the raw CAN logs.

## Flash and use the example

In ESPHome Builder, create/import a device configuration based on:

```yaml
github://EiNSTeiN-/esphome-alorair-sentinel-hd55p/configs/alorair-hd55p-ecan-e02.yaml@main
```

For local development with ESPHome installed:

```sh
esphome config configs/alorair-hd55p-ecan-e02.yaml
esphome run configs/alorair-hd55p-ecan-e02.yaml
```

Development diagnostics are available under [`dev/configs/`](dev/configs/):

```sh
esphome run dev/configs/ecan-e02-hd55-remote-emulator.yaml
esphome run dev/configs/ecan-e02-hd55p-zero-poll.yaml
```

Use the remote-emulator diagnostic first on an ECAN-E02 plus HD55P setup matching the wiring above. It sends the confirmed `50KBPS` idle remote frame and logs raw received CAN frames. Use the zero-poll diagnostic only when comparing against newer `125KBPS` reports.

After adoption in Home Assistant, enable the disabled-by-default debug entities if needed:

- `CAN Last Frame`
- `CAN Last Command Frame`
- `CAN RX Logging`
- `Raw Byte 4`
- ECAN-E02 heap/version/IP diagnostics

## Troubleshooting

If Home Assistant sees the ECAN-E02 but no dehumidifier data arrives:

1. Confirm the dehumidifier is powered.
2. Confirm ECAN-E02 `can_mode` is `NORMAL`.
3. Start with `can_bit_rate: 50KBPS`; try the `125KBPS` zero-poll diagnostic only for variants that do not respond.
4. Swap CANH/CANL if you have never seen a valid frame.
5. Confirm RJ45 pins 4/5/8 at the dehumidifier end, not just wire colors.
6. If using a board other than ECAN-E02, confirm any CAN transceiver enable, standby, or 5 V enable pins. Several thread reports were fixed by enabling CAN support pins on LilyGO T-CAN-style boards.
7. Keep a raw frame log and compare against the byte tables above.

If controls work but switch states appear backward or stale, remember that the power control is a command toggle. The continuous switch sends an explicit enable command and disables continuous mode by sending the current normal humidity setpoint. If status frames are missing, avoid using the switches until receiving is working.

## Existing repositories and sources

I did not find a dedicated public ESPHome repository for the HD55 family while preparing this repo. The Home Assistant thread mentions `disruptivepatternmaterial/esphome_config`, but that repository was not publicly accessible when checked.

Useful references:

- [Home Assistant Community thread: Automate AlorAir Sentinel HD55 dehumidifier](https://community.home-assistant.io/t/automate-alorair-sentinel-hd55-dehumidifier/306084)
- [Tinymicros wiki: AlorAir Sentinel HD55 Dehumidifier](https://tinymicros.com/wiki/AlorAir_Sentinel_HD55_Dehumidifier)
- [AlorAir HD55 wiring diagram PDF](https://www.alorair.com/pdf_files/product_guides/11/Sentinel-HD55-Dehumidifier-Wiring-Diagram.pdf)
- [AlorAir HD55/HD55S user manual PDF](https://www.alorair.com/pdf_files/product_guides/248/AlorAir-Sentinel-HD55S-User-Manual-20240815.pdf)
- [Ebyte ECAN-E02 ESPHome package](https://github.com/EiNSTeiN-/esphome-ecan-e02)
