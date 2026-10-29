# Hardware connections and observed tests

This is the physical connection reference, to be completed as hardware connections are confirmed. Unknown terminals, pin numbers and component specifications remain explicit gaps. The [architecture](architecture.md) defines MCU responsibilities; [power lifecycle](power-lifecycle.md) defines ignition and retained-power requirements.

## Display power: engine-off bike test

**Reported by the user on 2026-10-24:** the display successfully powered on from the bike socket with the **engine off**.

```text
Bike socket power cables (reported 12 V)
                  |
                  v
          Buck converter input
          Output set to 5 V
                  |
                  v
          Display power input
```

| Step | User-reported setup / result |
| --- | --- |
| 1 | Bike socket 12 V power cables connected directly to the buck converter input |
| 2 | Buck converter output set to 5 V |
| 3 | Buck converter output connected to the display power input |
| 4 | Display powered on successfully; engine remained off |

This is the observed power path, not a connector pinout or an approved final power circuit. Positive/return terminals, wire colors and the display input connector have not yet been recorded. Evidence is the user's report; actual voltage measurements, instrument used, current, duration and firmware version were not supplied. Engine-start, cranking, engine-running, road use, telemetry and key-off shutdown were not validated by this test.

## Details to complete the wiring guide

### Configurable discrete input test

Latest user report on 2026-10-24 supersedes earlier gear/polarity assignments: OUT1 is **left indicator → GPIO4**, OUT2 is **right indicator → GPIO5**; active approximately **2–3.3 V**, inactive **0.05–0.2 V**. Active HIGH decoding is implemented. The lower active reading is below ESP32-S3's guaranteed HIGH threshold at a 3.3 V supply and needs electrical resolution; probe/reference/instrument and individual channel peaks remain unrecorded. The all-enabled bench allocation is N GPIO6, high beam GPIO7 and gears 1–6 GPIO8–13, using weak pull-downs so opto outputs can move between inputs without reflashing. See the authoritative [vehicle input guide](../firmware/vehicle/README.md) for configuration, voltage limits and separate conditioning requirements. Board schematic/module validation remains pending.

| Connection / component | Still to record or verify |
| --- | --- |
| Bike socket | Identity/location, connector orientation and pin numbering, positive/return identification and key-on/key-off voltage behavior |
| Buck converter | Model/photo, input range, continuous output current, adjustment method and protection specifications |
| Display supply | Exact power connector/terminal labels, polarity and revision-matched input requirements |
| Measurements | Input/output voltage under display load, current, instrument, test conditions and duration |
| Wiring / protection | Actual fuse arrangement, reverse-polarity/transient protection, wire/connector ratings, insulation and mechanical securing |
| Ignition / shutdown | Separate ignition observation, retained supply and measured shutdown margin; none confirmed by this test |
| Vehicle MCU / UART / signals | MCU identity, power/ground topology, reviewed logic levels and terminal mappings; connections pending |

Before extending testing to engine start/running, review the actual converter and supply protection and define measurements for those conditions. The reported 12 V source and 5 V setting do not establish cranking or running behavior. Initial signal integration remains passive/read-only with OEM functionality retained.

A supply that disappears at key-off cannot provide the required two-second shutdown animation without retained power. This power-on result does not establish retention; see the [shutdown power contract](power-lifecycle.md#shutdown-power-contract).

## Agreed two-MCU bench connection

Both boards are powered separately from Mac USB. User identifies display connectors UART0 (4-pin), GPIO_D (4-pin), I2C (4-pin), SPK (2-pin), and a supplied 4-pin socket-to-loose-wire cable. Connector fit does not identify signal order. Verify V3.0 schematic, orientation, logic levels and USB/UART0 bridge sharing before assigning TX/RX/GND. Leave the cable power conductor disconnected and insulated. Final installation uses separate supply branches and dedicated power control as defined in [power lifecycle](power-lifecycle.md#agreed-power-topology); the vehicle ESP coordinates rather than supplying display load current.
