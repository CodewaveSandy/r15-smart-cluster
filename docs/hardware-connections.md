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
