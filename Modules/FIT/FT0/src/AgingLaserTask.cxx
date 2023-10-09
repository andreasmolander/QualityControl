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

void AgingLaserTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize LaserAgingTask" << ENDM;

  // Read task parameters
  std::vector<uint8_t> vecAmpCut;
  std::vector<uint8_t> vecChIDs;
  std::vector<uint8_t> vecRefChIDs;

  if (auto param = mCustomParameters.find("ChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecChIDs = fit::helper::parseParameters<uint8_t>(chIDs, del);
  } else {
    for (std::size_t iChID = 0; iChID < sNCHANNELS_PM; iChID++)
      vecChIDs.push_back(iChID);
  }
  if (auto param = mCustomParameters.find("RefChannelIDs"); param != mCustomParameters.end()) {
    const auto chIDs = param->second;
    const std::string del = ",";
    vecRefChIDs = fit::helper::parseParameters<uint8_t>(chIDs, del);
  }
  if (auto param = mCustomParameters.find("ampCut"); param != mCustomParameters.end()) {
    const auto ampCut = param->second;
    const std::string del = ",";
    vecAmpCut = fit::helper::parseParameters<uint8_t>(ampCut, del);
  } else {
    vecAmpCut.push_back(0);
  }

  for (const auto& entry : vecAmpCut) {
    mSetAmpCut.insert(entry);
  }
  for (const auto& entry : vecChIDs) {
    mSetAllowedChIDs.insert(entry);
  }
  for (const auto& entry : vecRefChIDs) {
    mSetRefChIDs.insert(entry);
  }

  // Initialize histograms

  // BC distributions
  mHistBC = std::make_unique<TH1I>("BC", "BC;BC;", sMaxBC, 0, sMaxBC);
  mHistBCDetector = std::make_unique<TH1I>("BC_detector", "BC detector channels;BC;", sMaxBC, 0, sMaxBC);
  mHistBCReference = std::make_unique<TH1I>("BC_reference", "BC reference channels;BC;", sMaxBC, 0, sMaxBC);
  mHistBCAmpCut = std::make_unique<TH1I>("BC_ampcut", "BC (amp cut);BC;", sMaxBC, 0, sMaxBC);
  mHistBCAmpCutADC0 = std::make_unique<TH1I>("BC_ampcut_ADC0", "BC (amp cut) ADC0;BC;", sMaxBC, 0, sMaxBC);
  mHistBCAmpCutADC1 = std::make_unique<TH1I>("BC_ampcut_ADC1", "BC (amp cut) ADC1;BC;", sMaxBC, 0, sMaxBC);
  mHistBCDetectorAmpCut = std::make_unique<TH1I>("BC_detector_ampcut", "BC detector channels (amp cut);BC;", sMaxBC, 0, sMaxBC);
  mHistBCDetectorAmpCutADC0 = std::make_unique<TH1I>("BC_detector_ampcut_ADC0", "BC detector channels (amp cut) ADC0;BC;", sMaxBC, 0, sMaxBC);
  mHistBCDetectorAmpCutADC1 = std::make_unique<TH1I>("BC_detector_ampcut_ADC1", "BC detector channels (amp cut) ADC1;BC;", sMaxBC, 0, sMaxBC);
  mHistBCReferenceAmpCut = std::make_unique<TH1I>("BC_reference_ampcut", "BC reference channels (amp cut);BC;", sMaxBC, 0, sMaxBC);
  mHistBCReferenceAmpCutADC0 = std::make_unique<TH1I>("BC_reference_ampcut_ADC0", "BC reference channels (amp cut) ADC0;BC;", sMaxBC, 0, sMaxBC);
  mHistBCReferenceAmpCutADC1 = std::make_unique<TH1I>("BC_reference_ampcut_ADC1", "BC reference channels (amp cut) ADC1;BC;", sMaxBC, 0, sMaxBC);

  getObjectsManager()->startPublishing(mHistBC.get());
  getObjectsManager()->startPublishing(mHistBCDetector.get());
  getObjectsManager()->startPublishing(mHistBCReference.get());
  getObjectsManager()->startPublishing(mHistBCAmpCut.get());
  getObjectsManager()->startPublishing(mHistBCAmpCutADC0.get());
  getObjectsManager()->startPublishing(mHistBCAmpCutADC1.get());
  getObjectsManager()->startPublishing(mHistBCDetectorAmpCut.get());
  getObjectsManager()->startPublishing(mHistBCDetectorAmpCutADC0.get());
  getObjectsManager()->startPublishing(mHistBCDetectorAmpCutADC1.get());
  getObjectsManager()->startPublishing(mHistBCReferenceAmpCut.get());
  getObjectsManager()->startPublishing(mHistBCReferenceAmpCutADC0.get());
  getObjectsManager()->startPublishing(mHistBCReferenceAmpCutADC1.get());

  // Amplitude distributions for all channels
  mHistAmpVsCh = std::make_unique<TH2I>("AmpPerChannel", "Amplitude vs channel;Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmpVsChADC0 = std::make_unique<TH2I>("AmpPerChannelADC0", "Amplitude vs channel (ADC0);Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);
  mHistAmpVsChADC1 = std::make_unique<TH2I>("AmpPerChannelADC1", "Amplitude vs channel (ADC1);Channel;Amp", sNCHANNELS_PM, 0, sNCHANNELS_PM, 4200, -100, 4100);

  getObjectsManager()->startPublishing(mHistAmpVsCh.get());
  getObjectsManager()->startPublishing(mHistAmpVsChADC0.get());
  getObjectsManager()->startPublishing(mHistAmpVsChADC1.get());
  getObjectsManager()->setDefaultDrawOptions(mHistAmpVsCh.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistAmpVsChADC0.get(), "COLZ");
  getObjectsManager()->setDefaultDrawOptions(mHistAmpVsChADC1.get(), "COLZ");

  // Reference channel distributions
  for (const auto& refChId : mSetRefChIDs) {
    // Amp vs BC
    mMapHistAmpVsBC.insert({ refChId, std::make_unique<TH2I>(Form("AmpPerBC_ch%i", refChId), Form("Amplitude vs BC, channel %i;BC;Amp", refChId), sMaxBC, 0, sMaxBC, 4200, -100, 4200) });
    mMapHistAmpVsBCADC0.insert({ refChId, std::make_unique<TH2I>(Form("AmpPerBC_ch%i_ADC0", refChId), Form("Amplitude vs BC, channel %i, ADC0;BC;Amp", refChId), sMaxBC, 0, sMaxBC, 4200, -100, 4200)});
    mMapHistAmpVsBCADC1.insert({ refChId, std::make_unique<TH2I>(Form("AmpPerBC_ch%i_ADC1", refChId), Form("Amplitude vs BC, channel %i, ADC1;BC;Amp", refChId), sMaxBC, 0, sMaxBC, 4200, -100, 4200)});
    
    getObjectsManager()->startPublishing(mMapHistAmpVsBC.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpVsBCADC0.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpVsBCADC1.at(refChId).get());

    // Amplitude distributions
    mMapHistAmp.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%i", refChId), Form("Amplitude, channel %i;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpADC0.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iADC0", refChId), Form("Amplitude, channel %i, ADC0;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpADC1.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iADC1", refChId), Form("Amplitude, channel %i, ADC1;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpPeak1.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak1", refChId), Form("Amplitude, channel %i, peak 1;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpPeak2.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak2", refChId), Form("Amplitude, channel %i, peak 2;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpPeak1ADC0.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak1ADC0", refChId), Form("Amplitude, channel %i, peak 1, ADC0;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpPeak1ADC1.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak1ADC1", refChId), Form("Amplitude, channel %i, peak 1, ADC1;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpPeak2ADC0.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak2ADC0", refChId), Form("Amplitude, channel %i, peak 2, ADC0;Amp;", refChId), 4200, -100, 4100)});
    mMapHistAmpPeak2ADC1.insert({ refChId, std::make_unique<TH1I>(Form("AmpCh%iPeak2ADC1", refChId), Form("Amplitude, channel %i, peak 2, ADC1;Amp;", refChId), 4200, -100, 4100)});

    getObjectsManager()->startPublishing(mMapHistAmp.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpADC0.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpADC1.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpPeak1.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpPeak2.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpPeak1ADC0.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpPeak1ADC1.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpPeak2ADC0.at(refChId).get());
    getObjectsManager()->startPublishing(mMapHistAmpPeak2ADC1.at(refChId).get());
  }
}

void AgingLaserTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void AgingLaserTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void AgingLaserTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // AM: for now just use one value - the first
  // TODO: how to store these
  int ampCut = *(mSetAmpCut.begin());

  auto channels = ctx.inputs().get<gsl::span<o2::ft0::ChannelData>>("channels");
  auto digits = ctx.inputs().get<gsl::span<o2::ft0::Digit>>("digits");

  // Loop over digits
  for (const auto& digit : digits) {
    const int bc = digit.getIntRecord().bc;
    const auto& digitChannelData = digit.getBunchChannelData(channels);

    // Conditions wether to fill BC histograms for this BC
    // 'AmpCut' means there was at least one channel with chAmp > ampCut
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

    mHistBC->Fill(bc);

    // Loop over channels
    for (const auto& chData : digitChannelData) {
      const int chId = chData.ChId;
      const int chAmp = chData.QTCAmpl;
      const int isRef = mSetRefChIDs.find(chId) != mSetRefChIDs.end();
      const int isDet = !isRef;
      const bool isADC0 = !chData.getFlag(o2::ft0::ChannelData::kNumberADC);
      const bool isADC1 = !isADC0;
      const bool isAmpCutOk = chAmp > ampCut;

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

      // Fill amplitude per channel histograms
      mHistAmpVsCh->Fill(chId, chAmp);
      isADC0 ? mHistAmpVsChADC0->Fill(chId, chAmp) : mHistAmpVsChADC1->Fill(chId, chAmp);

      // Fill amplitude per BC for reference channels
      if (isRef && isAmpCutOk) {
        mMapHistAmpVsBC[chId]->Fill(bc, chAmp);
        isADC0 ? mMapHistAmpVsBCADC0[chId]->Fill(bc, chAmp) : mMapHistAmpVsBCADC1[chId]->Fill(bc, chAmp);
      }

      // Fill reference channel ampltidude histograms
      if (isRef) {
        // Amplitude
        mMapHistAmp.at(chId)->Fill(chAmp);
        isADC0 ? mMapHistAmpADC0.at(chId)->Fill(chAmp) : mMapHistAmpADC1.at(chId)->Fill(chAmp);

        // Ampltiude for the different peaks. The peaks are selected based on BC
        if (chAmp > ampCut) {
          // Ampltiude peak 1
          // The BCs of peak 1 is the same for all reference channels
          if (bc == 115 || bc == 1898) {
            mMapHistAmpPeak1.at(chId)->Fill(chAmp);
            isADC0 ? mMapHistAmpPeak1ADC0.at(chId)->Fill(chAmp) : mMapHistAmpPeak1ADC1.at(chId)->Fill(chAmp);
          }

          // Amplitude peak 2
          // The BCs of peak 2 depends on reference channel
          if (chId == 208 && (bc == 136 || bc == 1919)) {
            mMapHistAmpPeak2.at(chId)->Fill(chAmp);
            if (isADC0) {
              // Double check BC (not sure if this always should be the case)
              if (bc != 136) {
                LOGP(warning, "Ch 208 peak2 ADC0 is in BC %i, should be 136!!! (I think...)", bc);
              }
              mMapHistAmpPeak2ADC0.at(chId)->Fill(chAmp);
            } else {
              if (bc != 1919) {
                LOGP(warning, "Ch 208 peak2 ADC1 is in BC %i, should be 1919!!! (I think...)", bc);
              }
              mMapHistAmpPeak2ADC1.at(chId)->Fill(chAmp);
            }
          } else if (chId == 209 && (bc == 142 || bc == 1925)) {
            mMapHistAmpPeak2.at(chId)->Fill(chAmp);
            if (isADC0) {
              // Double check BC (not sure if this always should be the case)
              if (bc != 142) {
                LOGP(warning, "Ch 209 peak2 ADC0 is in BC %i, should be 142!!! (I think...)", bc);
              }
              mMapHistAmpPeak2ADC0.at(chId)->Fill(chAmp);
            } else {
              if (bc != 1925) {
                LOGP(warning, "Ch 209 peak2 ADC1 is in BC %i, should be 1925!!! (I think...)", bc);
              }
              mMapHistAmpPeak2ADC1.at(chId)->Fill(chAmp);
            }
          } else if (chId == 210 && (bc == 135 || bc == 1918)) {
            mMapHistAmpPeak2.at(chId)->Fill(chAmp);
            if (isADC0) {
              // Double check BC (not sure if this always should be the case)
              if (bc != 135) {
                LOGP(warning, "Ch 210 peak2 ADC0 is in BC %i, should be 135!!! (I think...)", bc);
              }
              mMapHistAmpPeak2ADC0.at(chId)->Fill(chAmp);
            } else {
              if (bc != 1918) {
                LOGP(warning, "Ch 210 peak2 ADC1 is in BC %i, should be 1918!!! (I think...)", bc);
              }
              mMapHistAmpPeak2ADC1.at(chId)->Fill(chAmp);
            }
          } else if (chId == 211 && (bc == 1924 || bc == 141)) {
            mMapHistAmpPeak2.at(chId)->Fill(chAmp);
            if (isADC0) {
              // Double check BC (not sure if this always should be the case)
              if (bc != 1924) {
                LOGP(warning, "Ch 211 peak2 ADC0 is in BC %i, should be 1924!!! (I think...)", bc);
              }
              mMapHistAmpPeak2ADC0.at(chId)->Fill(chAmp);
            } else {
              if (bc != 141) {
                LOGP(warning, "Ch 211 peak2 ADC1 is in BC %i, should be 141!!! (I think...)", bc);
              }
              mMapHistAmpPeak2ADC1.at(chId)->Fill(chAmp);
            }
          } 
        } // if chAmp > ampCut
      } // if isRef
    } // channel loop

    // Fill BC distributions
    if (bcHasAmpCut) {
      mHistBCAmpCut->Fill(bc);
    }
    if (bcHasAmpCutADC0) {
      mHistBCAmpCutADC0->Fill(bc);
    }
    if (bcHasAmpCutADC1) {
      mHistBCAmpCutADC1->Fill(bc);
    }
    if (bcHasDetectorCh) {
      mHistBCDetector->Fill(bc);
    }
    if (bcHasDetectorChAmpCut) {
      mHistBCDetectorAmpCut->Fill(bc);
    }
    if (bcHasDetectorChAmpCutADC0) {
      mHistBCDetectorAmpCutADC0->Fill(bc);
    }
    if (bcHasDetectorChAmpCutADC1) {
      mHistBCDetectorAmpCutADC1->Fill(bc);
    }
    if (bcHasReferenceCh) {
      mHistBCReference->Fill(bc);
    }
    if (bcHasReferenceChAmpCut) {
      mHistBCReferenceAmpCut->Fill(bc);
    }
    if
    (bcHasReferenceChAmpCutADC0) {
      mHistBCReferenceAmpCutADC0->Fill(bc);
    }
    if (bcHasReferenceChAmpCutADC1) {
      mHistBCReferenceAmpCutADC1->Fill(bc);
    }
  } // digit loop
}

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

  // BC distributions
  mHistBC->Reset();
  mHistBCDetector->Reset();
  mHistBCReference->Reset();
  mHistBCAmpCut->Reset();
  mHistBCAmpCutADC0->Reset();
  mHistBCAmpCutADC1->Reset();
  mHistBCDetectorAmpCut->Reset();
  mHistBCDetectorAmpCutADC0->Reset();
  mHistBCDetectorAmpCutADC1->Reset();
  mHistBCReferenceAmpCut->Reset();
  mHistBCReferenceAmpCutADC0->Reset();
  mHistBCReferenceAmpCutADC1->Reset();

  // Amplitude distributions
  mHistAmpVsCh->Reset();
  mHistAmpVsChADC0->Reset();
  mHistAmpVsChADC1->Reset();

  // Reference channel distributions

  // Amp vs BC
  for (auto& entry : mMapHistAmpVsBC) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpVsBCADC0) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpVsBCADC1) {
    entry.second->Reset();
  }

  // Amplitude distributions
  for (auto& entry : mMapHistAmp) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpADC0) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpADC1) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpPeak1) {
    entry.second->Reset();
  }
  for (auto& entry : mMapHistAmpPeak2) {
    entry.second->Reset();
  }
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
}

} // namespace o2::quality_control_modules::ft0
