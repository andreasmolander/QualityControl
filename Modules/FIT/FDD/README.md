# FDD quality control

- [RecPoint QC](#recpoint-qc)
  - [Output](#output)

## RecPoint QC

### Output

#### Amplitude

| Name                   | Type   | Description                                |
|------------------------|--------|--------------------------------------------|
| `AmpPerChannel`        | `TH2F` | Amplitude per channel                      |
| `Amp_vs_time_channelX` | `TH2F` | Amplitude vs time, channel X;Amp;Time (ns) |

#### Time

| Name             | Type   | Description                                   |
|------------------|--------|-----------------------------------------------|
| `TimePerChannel` | `TH2F` | Time per channel                              |
| `TimeAvsTimeC`   | `TH2F` | FDD time A  vs time C;time A (ns);time C (ns) |
| `CollTimeA`      | `TH1F` | T0A;Time [ns]                                 |
| `CollTimeC`      | `TH1F` | T0C;Time [ns]                                 |

#### BC

| Name      | Type   | Description                  |
|-----------|--------|------------------------------|
| `BC`      | `TH1F` | BC;BC;counts                 |
| `BCVetex` | `TH1F` | BC Vertex trigger;BC;counts; |
| `BCorA`   | `TH1F` | BC OrA trigger;BC;counts;    |
| `BCorC`   | `TH1F` | BC OrC trigger;BC;counts;    |

#### Vertex

| Name                              | Type   | Description                                                                         |
|-----------------------------------|--------|-------------------------------------------------------------------------------------|
| `VertexVsCollTimeAllBC`           | `TH2F` | FDD vertex vs Collision time;FDD vertex (cm);  Collision Time [ns]                  |
| `VertexNsVsCollTimeAllBC`         | `TH2F` | FDD vertex vs Collision time;FDD vertex (ns);  Collision Time [ns]                  |
| `VertexVsCollTimeVertexTrigger`   | `TH2F` | FDD vertex vs Collision time (Vertex trigger);FDD vertex (cm);  Collision Time [ns] |
| `VertexNsVsCollTimeVertexTrigger` | `TH2F` | FDD vertex vs Collision time (Vertex trigger);FDD vertex (ns);  Collision Time [ns] |
