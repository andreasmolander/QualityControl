// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   AgingLaserTask.cxx
/// \author Sandor Lokos, Edmundo Garcia-Solis, Andreas Molander
///

#include "FT0/AgingLaserTask.h"

#include "Common/Utils.h"
#include "FITCommon/HelperCommon.h"
#include "QualityControl/QcInfoLogger.h"

#include <DataFormatsFT0/ChannelData.h>
#include <DataFormatsFT0/Digit.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

#include <TH1I.h>
#include <TH2I.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace o2::quality_control_modules::ft0
{

AgingLaserTask::~AgingLaserTask()
{
}

void AgingLaserTask::initialize(o2::framework::InitContext&)
{
  // Read task parameters

  // Enabled detector channels
  const std::string detectorChannels = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "detectorChannelIDs", "");
  if (detectorChannels.size()) {
    mDetectorChIDs = fit::helper::parseParameters<uint8_t>(detectorChannels, ",");
  } else {
    // Not specified, enable all
    for (uint8_t chId = 0; chId < sNCHANNELS_PM; chId++) {
      mDetectorChIDs.push_back(chId);
    }
  }

  // Enabled reference channels
  const std::string referenceChannels = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "referenceChannelIDs", "");
  if (referenceChannels.size()) {
    mReferenceChIDs = fit::helper::parseParameters<uint8_t>(referenceChannels, ",");
  } else {
    // Not specified, enable all
    // (TODO: return with fatal if not specified, to avoid hard coded numbers?)
    for (uint8_t chId = 208; chId < 211; chId++) {
      mReferenceChIDs.push_back(chId);
    }
  }

  mDetectorAmpCut = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "detectorAmpCut", 0);
  mReferenceAmpCut = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "referenceAmpCut", 100);

  // BCs

  // Laser trigger BCs
  const std::string laserTriggerBCs = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "laserTriggerBCs", "");
  if (laserTriggerBCs.size()) {
    const auto vecParams = fit::helper::parseParameters<int>(laserTriggerBCs, ",");
    for (const int bc : vecParams) {
      mLaserTriggerBCs.push_back(bc);
      // mLaserTriggerBCs.insert(bc);
    }
  }
  if (mLaserTriggerBCs.size() == 0) {
    LOG(fatal) << "No laser trigger BCs specified in QC config!";
  }

  // BC delay for detector channels
  mDetectorBCdelay = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "detectorBCdelay", -1);
  if (mDetectorBCdelay < 0) {
    LOG(fatal) << "No detector BC delay specified in QC config!";
  }

  // BC delay for reference channels peak1
  const std::string referencePeak1BCdelays = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "referencePeak1BCdelays", "");
  if (referencePeak1BCdelays.size()) {
    const auto vecParams = fit::helper::parseParameters<int>(referencePeak1BCdelays, ",");
    if (vecParams.size() != mReferenceChIDs.size()) {
      LOG(fatal) << "Number of reference channels and reference peak 1 BC delays do not match!";
    }
    for (int i = 0; i < mReferenceChIDs.size(); i++) {
      mReferencePeak1BCdelays.insert({ mReferenceChIDs.at(i), vecParams.at(i) });
    }
  } else {
    LOG(fatal) << "No reference peak 1 BC delays specified in QC config!";
  }

  // BC delay for reference channels peak2
  const std::string referencePeak2BCdelays = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "referencePeak2BCdelays", "");
  if (referencePeak2BCdelays.size()) {
    const auto vecParams = fit::helper::parseParameters<int>(referencePeak2BCdelays, ",");
    if (vecParams.size() != mReferenceChIDs.size()) {
      LOG(fatal) << "Number of reference channels and reference peak 2 BC delays do not match!";
    }
    for (int i = 0; i < mReferenceChIDs.size(); i++) {
      mReferencePeak2BCdelays.insert({ mReferenceChIDs.at(i), vecParams.at(i) });
    }
  } else {
    LOG(fatal) << "No reference peak 2 BC delays specified in QC config!";
  }

  mDebug = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "debug", false);
  if (mDebug) {
    LOG(warning) << "Running in debug mode!";
  }

  // Initialize histograms

  // Amplitude per channel
  mHistAmpVsChADC0 = std::make_unique<TH2I>("AmpPerChannelADC0", "Amplitude vs channel (ADC0);Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmpVsChADC1 = std::make_unique<TH2I>("AmpPerChannelADC1", "Amplitude vs channel (ADC1);Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmpVsChPeak1ADC0 = std::make_unique<TH2I>("AmpPerChannelPeak1ADC0", "Amplitude vs channel (peak 1, ADC0);Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmpVsChPeak1ADC1 = std::make_unique<TH2I>("AmpPerChannelPeak1ADC1", "Amplitude vs channel (peak 1, ADC1);Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmpVsChPeak2ADC0 = std::make_unique<TH2I>("AmpPerChannelPeak2ADC0", "Amplitude vs channel (peak 2, ADC0);Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmpVsChPeak2ADC1 = std::make_unique<TH2I>("AmpPerChannelPeak2ADC1", "Amplitude vs channel (peak 2, ADC1);Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  getObjectsManager()->startPublishing(mHistAmpVsChADC0.get());
  getObjectsManager()->startPublishing(mHistAmpVsChADC1.get());
  getObjectsManager()->startPublishing(mHistAmpVsChPeak1ADC0.get());
  getObjectsManager()->startPublishing(mHistAmpVsChPeak1ADC1.get());
  getObjectsManager()->startPublishing(mHistAmpVsChPeak2ADC0.get());
  getObjectsManager()->startPublishing(mHistAmpVsChPeak2ADC1.get());
  getObjectsManager()->setDefaultDrawOptions(mHistAmpVsChADC0.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistAmpVsChADC1.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistAmpVsChPeak1ADC0.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistAmpVsChPeak1ADC1.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistAmpVsChPeak2ADC0.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistAmpVsChPeak2ADC1.get(), "COLZ");

  // Time per channel
  mHistTimeVsChADC0 = std::make_unique<TH2I>("TimePerChannelADC0", "Time vs channel (ADC0);Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  mHistTimeVsChADC1 = std::make_unique<TH2I>("TimePerChannelADC1", "Time vs channel (ADC1);Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  mHistTimeVsChPeak1ADC0 = std::make_unique<TH2I>("TimePerChannelPeak1ADC0", "Time vs channel (peak 1, ADC0);Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  mHistTimeVsChPeak1ADC1 = std::make_unique<TH2I>("TimePerChannelPeak1ADC1", "Time vs channel (peak 1, ADC1);Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  mHistTimeVsChPeak2ADC0 = std::make_unique<TH2I>("TimePerChannelPeak2ADC0", "Time vs channel (peak 2, ADC0);Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  mHistTimeVsChPeak2ADC1 = std::make_unique<TH2I>("TimePerChannelPeak2ADC1", "Time vs channel (peak 2, ADC1);Channel;Time", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4100, -2050, 2050);
  getObjectsManager()->startPublishing(mHistTimeVsChADC0.get());
  getObjectsManager()->startPublishing(mHistTimeVsChADC1.get());
  getObjectsManager()->startPublishing(mHistTimeVsChPeak1ADC0.get());
  getObjectsManager()->startPublishing(mHistTimeVsChPeak1ADC1.get());
  getObjectsManager()->startPublishing(mHistTimeVsChPeak2ADC0.get());
  getObjectsManager()->startPublishing(mHistTimeVsChPeak2ADC1.get());
  getObjectsManager()->setDefaultDrawOptions(mHistTimeVsChADC0.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistTimeVsChADC1.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistTimeVsChPeak1ADC0.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistTimeVsChPeak1ADC1.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistTimeVsChPeak2ADC0.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistTimeVsChPeak2ADC1.get(), "COLZ");

  // Reference channel histograms
  for (const uint8_t refChId : mReferenceChIDs) {
    // Ampltiude histograms for referece channel peaks
    mMapHistAmpPeak1ADC0.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak1ADC0", refChId), Form("Amplitude, channel %i, peak 1, ADC0;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpPeak1ADC1.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak1ADC1", refChId), Form("Amplitude, channel %i, peak 1, ADC1;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpPeak2ADC0.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak2ADC0", refChId), Form("Amplitude, channel %i, peak 2, ADC0;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpPeak2ADC1.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak2ADC1", refChId), Form("Amplitude, channel %i, peak 2, ADC1;Amp;", refChId), 4200, -100, 4100)});
    getObjectsManager()->startPublishing(mMapHistAmpPeak1ADC0.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpPeak1ADC1.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpPeak2ADC0.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpPeak2ADC1.at(refChId).get());

    // Time histograms for reference channel peaks
    mMapHistTimePeak1ADC0.insert({ refChId, std::make_unique<TH1I>(Form("TimeCh%iPeak1ADC0", refChId), Form("Time, channel %i, peak 1, ADC0;Time;", refChId), 4100, -2050, 2050)});
    mMapHistTimePeak1ADC1.insert({ refChId, std::make_unique<TH1I>(Form("TimeCh%iPeak1ADC1", refChId), Form("Time, channel %i, peak 1, ADC1;Time;", refChId), 4100, -2050, 2050)});
    mMapHistTimePeak2ADC0.insert({ refChId, std::make_unique<TH1I>(Form("TimeCh%iPeak2ADC0", refChId), Form("Time, channel %i, peak 2, ADC0;Time;", refChId), 4100, -2050, 2050)});
    mMapHistTimePeak2ADC1.insert({ refChId, std::make_unique<TH1I>(Form("TimeCh%iPeak2ADC1", refChId), Form("Time, channel %i, peak 2, ADC1;Time;", refChId), 4100, -2050, 2050)});
    getObjectsManager()->startPublishing(mMapHistTimePeak1ADC0.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistTimePeak1ADC1.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistTimePeak2ADC0.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistTimePeak2ADC1.at(refChId).get());
  }

  // Debug histograms
  if (mDebug) {
    // Amplitude per channel
    mDebugHistAmpVsCh = std::make_unique<TH2I>("AmpPerChannel", "Amplitude vs channel;Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
    getObjectsManager()->startPublishing(mDebugHistAmpVsCh.get());
    getObjectsManager()->setDefaultDrawOptions(mDebugHistAmpVsCh.get(), "COLZ");

    // BC
    mDebugHistBC = std::make_unique<TH1I>("BC", "BC;BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCDetector = std::make_unique<TH1I>("BC_detector", "BC detector channels;BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCReference = std::make_unique<TH1I>("BC_reference", "BC reference channels;BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCAmpCut = std::make_unique<TH1I>("BC_ampcut", "BC (amp cut);BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCAmpCutADC0 = std::make_unique<TH1I>("BC_ampcut_ADC0", "BC (amp cut) ADC0;BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCAmpCutADC1 = std::make_unique<TH1I>("BC_ampcut_ADC1", "BC (amp cut) ADC1;BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCDetectorAmpCut = std::make_unique<TH1I>("BC_detector_ampcut", "BC detector channels (amp cut);BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCDetectorAmpCutADC0 = std::make_unique<TH1I>("BC_detector_ampcut_ADC0", "BC detector channels (amp cut) ADC0;BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCDetectorAmpCutADC1 = std::make_unique<TH1I>("BC_detector_ampcut_ADC1", "BC detector channels (amp cut) ADC1;BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCReferenceAmpCut = std::make_unique<TH1I>("BC_reference_ampcut", "BC reference channels (amp cut);BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCReferenceAmpCutADC0 = std::make_unique<TH1I>("BC_reference_ampcut_ADC0", "BC reference channels (amp cut) ADC0;BC;", sMaxBC, 0, sMaxBC);
    mDebugHistBCReferenceAmpCutADC1 = std::make_unique<TH1I>("BC_reference_ampcut_ADC1", "BC reference channels (amp cut) ADC1;BC;", sMaxBC, 0, sMaxBC);
    getObjectsManager()->startPublishing(mDebugHistBC.get());
    getObjectsManager()->startPublishing(mDebugHistBCDetector.get());
    getObjectsManager()->startPublishing(mDebugHistBCReference.get());
    getObjectsManager()->startPublishing(mDebugHistBCAmpCut.get());
    getObjectsManager()->startPublishing(mDebugHistBCAmpCutADC0.get());
    getObjectsManager()->startPublishing(mDebugHistBCAmpCutADC1.get());
    getObjectsManager()->startPublishing(mDebugHistBCDetectorAmpCut.get());
    getObjectsManager()->startPublishing(mDebugHistBCDetectorAmpCutADC0.get());
    getObjectsManager()->startPublishing(mDebugHistBCDetectorAmpCutADC1.get());
    getObjectsManager()->startPublishing(mDebugHistBCReferenceAmpCut.get());
    getObjectsManager()->startPublishing(mDebugHistBCReferenceAmpCutADC0.get());
    getObjectsManager()->startPublishing(mDebugHistBCReferenceAmpCutADC1.get());

    // Reference channel histograms
    for (const uint8_t refChId : mReferenceChIDs) {
      // Amplitude histograms for reference channel peaks
      mMapDebugHistAmp.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%i", refChId), Form("Amplitude, channel %i;Amp;", refChId), 4200, -100, 4100)});
      mMapDebugHistAmpADC0.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iADC0", refChId), Form("Amplitude, channel %i, ADC0;Amp;", refChId), 4200, -100, 4100)});
      mMapDebugHistAmpADC1.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iADC1", refChId), Form("Amplitude, channel %i, ADC1;Amp;", refChId), 4200, -100, 4100)});
      mMapDebugHistAmpPeak1.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak1", refChId), Form("Amplitude, channel %i, peak 1;Amp;", refChId), 4200, -100, 4100)});
      mMapDebugHistAmpPeak2.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak2", refChId), Form("Amplitude, channel %i, peak 2;Amp;", refChId), 4200, -100, 4100)});
      getObjectsManager()->startPublishing(mMapDebugHistAmp.at(refChId).get());
      getObjectsManager()->startPublishing(mMapDebugHistAmpADC0.at(refChId).get());
      getObjectsManager()->startPublishing(mMapDebugHistAmpADC1.at(refChId).get());
      getObjectsManager()->startPublishing(mMapDebugHistAmpPeak1.at(refChId).get());
      getObjectsManager()->startPublishing(mMapDebugHistAmpPeak2.at(refChId).get());

      // Amplitude per BC
      mMapDebugHistAmpVsBC.insert({ refChId, std::make_unique<TH2I>(Form("AmpPerBC_ch%i", refChId), Form("Amplitude vs BC, channel %i;BC;Amp", refChId), sMaxBC, 0, sMaxBC, 4200, -100, 4200) });
      mMapDebugHistAmpVsBCADC0.insert({ refChId, std::make_unique<TH2I>(Form("AmpPerBC_ch%i_ADC0", refChId), Form("Amplitude vs BC, channel %i, ADC0;BC;Amp", refChId), sMaxBC, 0, sMaxBC, 4200, -100, 4200)});
      mMapDebugHistAmpVsBCADC1.insert({ refChId, std::make_unique<TH2I>(Form("AmpPerBC_ch%i_ADC1", refChId), Form("Amplitude vs BC, channel %i, ADC1;BC;Amp", refChId), sMaxBC, 0, sMaxBC, 4200, -100, 4200)});
      getObjectsManager()->startPublishing(mMapDebugHistAmpVsBC.at(refChId).get());
      getObjectsManager()->startPublishing(mMapDebugHistAmpVsBCADC0.at(refChId).get());
      getObjectsManager()->startPublishing(mMapDebugHistAmpVsBCADC1.at(refChId).get());
    }
  }
}

void AgingLaserTask::startOfActivity(const Activity& activity)
{
  reset();
}

void AgingLaserTask::startOfCycle()
{
}

void AgingLaserTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");

  // Loop over digits
  for (const auto& digit : digits) {
    const int bc = digit.getIntRecord().bc;
    const auto& digitChannelData = digit.getBunchChannelData(channels);

    // Conditions wether to fill BC histograms for this BC. For debug use only
    // 'AmpCut' means there was at least one channel with chAmp > mReferenceAmpCut
    // 'ADCX' means there was at least one channel data with ADCX
    // 'Detector' means there was at least one detector channel
    // 'Reference' means there was at least one reference channel
    bool bcHasAmpCut = false;
    bool bcHasAmpCutADC0 = false;
    bool bcHasAmpCutADC1 = false;
    bool bcHasDetectorCh = false;
    bool bcHasDetectorChAmpCut = false;
    bool bcHasDetectorChAmpCutADC0 = false;
    bool bcHasDetectorChAmpCutADC1 = false;
    bool bcHasReferenceCh = false;
    bool bcHasReferenceChAmpCut = false;
    bool bcHasReferenceChAmpCutADC0 = false;
    bool bcHasReferenceChAmpCutADC1 = false;

    if (mDebug) {
      mDebugHistBC->Fill(bc);
    }

    // Loop over channels
    for (const auto& chData : digitChannelData) {
      const int chId = chData.ChId;
      const int chAmp = chData.QTCAmpl;
      const int chTime = chData.CFDTime;
      const bool isRef = std::find(mReferenceChIDs.begin(), mReferenceChIDs.end(), chId) != mReferenceChIDs.end(); // TODO: optimize
      const bool isDet = !isRef;
      const bool isADC0 = !chData.getFlag(o2::ft0::ChannelData::kNumberADC);
      const bool isADC1 = !isADC0;
      const bool isAmpCutOk = chAmp > mReferenceAmpCut;
      const bool isDetAmpCutOk = chAmp > mDetectorAmpCut;
      const bool isRefAmpCutOk = chAmp > mReferenceAmpCut;

      if (mDebug) {
        // Use var = condition || var, so that var is never set to back to false if once true
        bcHasAmpCut = isAmpCutOk || bcHasAmpCut;
        bcHasAmpCutADC0 = (isAmpCutOk && isADC0) || bcHasAmpCutADC0;
        bcHasAmpCutADC1 = (isAmpCutOk && isADC1) || bcHasAmpCutADC1;
        bcHasDetectorCh = isDet || bcHasDetectorCh;
        bcHasDetectorChAmpCut = (isDet && isAmpCutOk) || bcHasDetectorChAmpCut;
        bcHasDetectorChAmpCutADC0 = (isDet && isAmpCutOk && isADC0) || bcHasDetectorChAmpCutADC0;
        bcHasDetectorChAmpCutADC1 = (isDet && isAmpCutOk && isADC1) || bcHasDetectorChAmpCutADC1;
        bcHasReferenceCh = isRef || bcHasReferenceCh;
        bcHasReferenceChAmpCut = (isRef && isAmpCutOk) || bcHasReferenceChAmpCut;
        bcHasReferenceChAmpCutADC0 = (isRef && isAmpCutOk && isADC0) || bcHasReferenceChAmpCutADC0;
        bcHasReferenceChAmpCutADC1 = (isRef && isAmpCutOk && isADC1) || bcHasReferenceChAmpCutADC1;
      }

      // Fill amplitude/time per channel histograms
      if (mDebug) {
        mDebugHistAmpVsCh->Fill(chId, chAmp);
      }
      isADC0 ? mHistAmpVsChADC0->Fill(chId, chAmp) : mHistAmpVsChADC1->Fill(chId, chAmp);
      isADC0 ? mHistTimeVsChADC0->Fill(chId, chTime) : mHistTimeVsChADC1->Fill(chId, chTime);

      // Fill amplitude per BC for reference channels
      if (mDebug && isRef && isRefAmpCutOk) {
        mMapDebugHistAmpVsBC[chId]->Fill(bc, chAmp);
        isADC0 ? mMapDebugHistAmpVsBCADC0[chId]->Fill(bc, chAmp) : mMapDebugHistAmpVsBCADC1[chId]->Fill(bc, chAmp);
      }

      // Fill reference channel ampltidude histograms
      if (isRef) {
        // Amplitude
        if (mDebug) {
          mMapDebugHistAmp.at(chId)->Fill(chAmp);
          isADC0 ? mMapDebugHistAmpADC0.at(chId)->Fill(chAmp) : mMapDebugHistAmpADC1.at(chId)->Fill(chAmp);
        }

        // Ampltiude for the different peaks. The peaks are selected based on BC
        if (isRefAmpCutOk) {
          // Ampltiude peak 1
          if (bcIsPeak1(bc, chId)) {
            if (mDebug) {
              mMapDebugHistAmpPeak1.at(chId)->Fill(chAmp);
            }
            if (isADC0) {
              mMapHistAmpPeak1ADC0.at(chId)->Fill(chAmp);
              mMapHistTimePeak1ADC0.at(chId)->Fill(chTime);
              mHistAmpVsChPeak1ADC0->Fill(chId, chAmp);
              mHistTimeVsChPeak1ADC0->Fill(chId, chTime);
            } else {
              mMapHistAmpPeak1ADC1.at(chId)->Fill(chAmp);
              mMapHistTimePeak1ADC1.at(chId)->Fill(chTime);
              mHistAmpVsChPeak1ADC1->Fill(chId, chAmp);
              mHistTimeVsChPeak1ADC1->Fill(chId, chTime);
            }
          }

          // Amplitude peak 2
          if (bcIsPeak2(bc, chId)) {
            if (mDebug) {
              mMapDebugHistAmpPeak2.at(chId)->Fill(chAmp);
            }
            if (isADC0) {
              mMapHistAmpPeak2ADC0.at(chId)->Fill(chAmp);
              mMapHistTimePeak2ADC0.at(chId)->Fill(chTime);
              mHistAmpVsChPeak2ADC0->Fill(chId, chAmp);
              mHistTimeVsChPeak2ADC0->Fill(chId, chTime);
            } else {
              mMapHistAmpPeak2ADC1.at(chId)->Fill(chAmp);
              mMapHistTimePeak2ADC1.at(chId)->Fill(chTime);
              mHistAmpVsChPeak2ADC1->Fill(chId, chAmp);
              mHistTimeVsChPeak2ADC1->Fill(chId, chTime);
            }
          }
        } // if isRefAmpCutOK
      } // if isRef
    } // channel loop

    if (mDebug) {
      // Fill BC distributions
      if (bcHasAmpCut) {
        mDebugHistBCAmpCut->Fill(bc);
      }
      if (bcHasAmpCutADC0) {
        mDebugHistBCAmpCutADC0->Fill(bc);
      }
      if (bcHasAmpCutADC1) {
        mDebugHistBCAmpCutADC1->Fill(bc);
      }
      if (bcHasDetectorCh) {
        mDebugHistBCDetector->Fill(bc);
      }
      if (bcHasDetectorChAmpCut) {
        mDebugHistBCDetectorAmpCut->Fill(bc);
      }
      if (bcHasDetectorChAmpCutADC0) {
        mDebugHistBCDetectorAmpCutADC0->Fill(bc);
      }
      if (bcHasDetectorChAmpCutADC1) {
        mDebugHistBCDetectorAmpCutADC1->Fill(bc);
      }
      if (bcHasReferenceCh) {
        mDebugHistBCReference->Fill(bc);
      }
      if (bcHasReferenceChAmpCut) {
        mDebugHistBCReferenceAmpCut->Fill(bc);
      }
      if
      (bcHasReferenceChAmpCutADC0) {
        mDebugHistBCReferenceAmpCutADC0->Fill(bc);
      }
      if (bcHasReferenceChAmpCutADC1) {
        mDebugHistBCReferenceAmpCutADC1->Fill(bc);
      }
    } // if mDebug
  } // digit loop
} // monitorData

void AgingLaserTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void AgingLaserTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void AgingLaserTask::reset()
{
  // Reset histograms
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;

  // Amplitude per channel
  mHistAmpVsChADC0->Reset();
  mHistAmpVsChADC1->Reset();
  mHistAmpVsChPeak1ADC0->Reset();
  mHistAmpVsChPeak1ADC1->Reset();
  mHistAmpVsChPeak2ADC0->Reset();
  mHistAmpVsChPeak2ADC1->Reset();

  // Time per channel
  mHistTimeVsChADC0->Reset();
  mHistTimeVsChADC1->Reset();
  mHistTimeVsChPeak1ADC0->Reset();
  mHistTimeVsChPeak1ADC1->Reset();
  mHistTimeVsChPeak2ADC0->Reset();
  mHistTimeVsChPeak2ADC1->Reset();

  // Amplitude histograms for reference channel peaks
  for (auto& entry : mMapHistAmpPeak1ADC0) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpPeak1ADC1) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpPeak2ADC0) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpPeak2ADC1) {
    entry.second->Reset();
  }

  // Time histograms for reference channel peaks
  for (auto& entry : mMapHistTimePeak1ADC0) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistTimePeak1ADC1) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistTimePeak2ADC0) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistTimePeak2ADC1) {
    entry.second->Reset();
  }

  if (mDebug) {
    // Amplitude per channel
    mDebugHistAmpVsCh->Reset();

    // Amplitude histograms for reference channel peaks
    for (auto& entry : mMapDebugHistAmp) {
      entry.second->Reset();
    }
    for (auto& entry : mMapDebugHistAmpADC0) {
      entry.second->Reset();
    }
    for (auto& entry : mMapDebugHistAmpADC1) {
      entry.second->Reset();
    }
    for (auto& entry : mMapDebugHistAmpPeak1) {
      entry.second->Reset();
    }
    for (auto& entry : mMapDebugHistAmpPeak2) {
      entry.second->Reset();
    }

    // BC
    mDebugHistBC->Reset();
    mDebugHistBCDetector->Reset();
    mDebugHistBCReference->Reset();
    mDebugHistBCAmpCut->Reset();
    mDebugHistBCAmpCutADC0->Reset();
    mDebugHistBCAmpCutADC1->Reset();
    mDebugHistBCDetectorAmpCut->Reset();
    mDebugHistBCDetectorAmpCutADC0->Reset();
    mDebugHistBCDetectorAmpCutADC1->Reset();
    mDebugHistBCReferenceAmpCut->Reset();
    mDebugHistBCReferenceAmpCutADC0->Reset();
    mDebugHistBCReferenceAmpCutADC1->Reset();

    // Amplitude per BC for reference channels
    for (auto& entry : mMapDebugHistAmpVsBC) {
      entry.second->Reset();
    }
    for (auto& entry : mMapDebugHistAmpVsBCADC0) {
      entry.second->Reset();
    }
    for (auto& entry : mMapDebugHistAmpVsBCADC1) {
      entry.second->Reset();
    }
  }
}

bool AgingLaserTask::bcIsPeak1(int bc, int chId) const
{
  for (const int bcTrg : mLaserTriggerBCs) {
    if (bc == bcTrg + mReferencePeak1BCdelays.at(chId)) {
      return true;
    }
  }
  return false;
}

bool AgingLaserTask::bcIsPeak2(int bc, int chId) const
{
  for (const int bcTrg : mLaserTriggerBCs) {
    if (bc == bcTrg + mReferencePeak2BCdelays.at(chId)) {
      return true;
    }
  }
  return false;
}

} // namespace o2::quality_control_modules::ft0
