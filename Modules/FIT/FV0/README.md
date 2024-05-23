# FV0 quality control

## Digit QC

### Output

#### Amplitude

| Name                   | Type   | Description                                                                                |
|------------------------|--------|--------------------------------------------------------------------------------------------|
| `AmpPerChannel`        | `TH2F` | Ampltiude per channel                                                                      |
| `SumAmpA`              | `TH1F` | Sum of amplitudes from TCM                                                                 |
| `CFD_efficiency`       | `TH1F` | Fraction of events with CFD in ADC gate per channel                                        |
| `PmTcmSumAmpA`         | `TH2F` | Comparison of sum of amplitudes A from PM and TCM;Sum of amplitudes (TCM), side A;PM - TCM |
| `Amp_channelX`         | `TH1F` | Effectively projections of `AmpPerChannel`, one histo per channel bin                      |
| `Amp_vs_time_channelX` | `TH2F` | Amplitude vs time, channel X;Amp;Time                                                      |

#### Time

| Name                | Type   | Description                                                                     |
|---------------------|--------|---------------------------------------------------------------------------------|
| `TimePerChannel`    | `TH2F` | Time per channel                                                                |
| `AverageTimeA`      | `TH1F` | Average time from TCM                                                           |
| `EventsInGateTime`  | `TH1F` | Fraction of events with CFD in time gate per channel ChannelID                  |
| `PmTcmAverageTimeA` | `TH2F` | Comparison of average time A from PM and TCM;Average time(TCM), side A;PM - TCM |
| `Time_channelX`     | `TH1F` | Effectively projections of `TimePerChannel`, one histo per channel bin          |

#### Channels

| Name                     | Type   | Description                                                                            |
|--------------------------|--------|----------------------------------------------------------------------------------------|
| `StatChannelID`          | `TH1F` | Channel hits stats                                                                     |
| `NumChannelsA`           | `TH1F` | Number of fired channels from TCM                                                      |
| `PmTcmNumChannelsA`      | `TH2F` | Comparison of num. channels A from PM and TCM;Number of channels(TCM), side A;PM - TCM |
| `ChannelDataBits`        | `TH2F` | Bits per channel                                                                       |
| `EventDensityPerChannel` | `TH2F` | Event density(in BC) per Channel;Channel;BC                                            |
| `Bits_channelX`          | `TH1F` | Bits for channel X, effecticely projections of `ChannelDataBits`                       |

#### Triggers

| Name                    | Type   | Description                                                                         |
|-------------------------|--------|-------------------------------------------------------------------------------------|
| `TriggersSoftware`      | `TH1F` | Triggers from software                                                              |
| `TriggersCorrelation`   | `TH2F` | Correlation of triggers from TCM                                                    |
| `TriggersSoftwareVsTCM` | `TH2F` | TriggersSoftwareVsTCM", "Comparison of triggers from software and TCM;;Trigger name |

#### BC & orbit

| Name                         | Type   | Description                                |
|------------------------------|--------|--------------------------------------------|
| `BC`                         | `TH1F` | BC;BC;counts                               |
| `BCvsTriggers`               | `TH2F` | BC vs Triggers;BC;Trg                      |
| `BCvsFEEmodules`             | `TH2F` | BC vs FEE module;BC;FEE                    |
| `BCvsFEEmodulesForOrATrg`    | `TH2F` | BC vs FEE module for OrA trigger;BC;FEE    |
| `BCvsFEEmodulesForOrAOutTrg` | `TH2F` | BC vs FEE module for OrAOut trigger;BC;FEE |
| `BCvsFEEmodulesForNChanTrg`  | `TH2F` | BC vs FEE module for NChan trigger;BC;FEE  |
| `BCvsFEEmodulesForChargeTrg` | `TH2F` | BC vs FEE module for Charge trigger;BC;FEE |
| `BCvsFEEmodulesForOrAInTrg`  | `TH2F` | BC vs FEE module for OrAIn trigger;BC;FEE  |
| `OrbitPerBC`                 | `TH2F` | BC-Orbit map;Orbit;BC                      |
| `OrbitVsTriggers`            | `TH2F` | Orbit vs Triggers;Orbit;Trg                |
| `OrbitVsFEEmodules`          | `TH2F` | Orbit vs FEE module;Orbit;FEE              |

#### Cycles

| Name                 | Type   | Description                                   |
|----------------------|--------|-----------------------------------------------|
| `CycleDuration`      | `TH1D` | Cycle Duration;;time \[ns\]                   |
| `CycleDurationNTF`   | `TH1D` | Cycle Duration;;time \[TimeFrames\]           |
| `CycleDurationRange` | `TH1D` | Cycle Duration (total cycle range);;time [ns] |

#### ToDo

- Triggers from TCM? If `TriggersCorrelation` is 100% OK then this would be the same plot as `TriggersSoftware`, but at the moment it is not always the case, due to misconfigure trigger settings and what not in aQC so would perhaps be good to have.