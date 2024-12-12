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
/// \file   AgingLaserPostProc.cxx
/// \author Andreas Molander <andreas.molander@cern.ch>
///

#include "FT0/AgingLaserPostProc.h"

#include "Common/Utils.h"
#include "FITCommon/HelperHist.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/QcInfoLogger.h"

#include "DetectorsBase/GRPGeomHelper.h"

#include <TH1F.h>
#include <TH2.h>
#include <memory>
#include <string>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::ft0
{

AgingLaserPostProc::~AgingLaserPostProc()
{
}

void AgingLaserPostProc::initialize(Trigger, framework::ServiceRegistryRef)
{
  ILOG(Info) << "initialize" << ENDM;

  // Task parameters

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

  ILOG(Info) << "Using detector channels: ";
  for (auto chId : mDetectorChIDs) {
    ILOG(Info) << std::to_string(chId) << " ";
  }

  // Enabled reference channels
  const std::string referenceChannels = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "referenceChannelIDs", "");
  if (referenceChannels.size()) {
    mReferenceChIDs = fit::helper::parseParameters<uint8_t>(referenceChannels, ",");
  } else {
    // Not specified, enable all
    // TODO: return with fatal if not specified, to avoid hard coded numbers?
    for (uint8_t chId = 208; chId < 211; chId++) {
      mReferenceChIDs.push_back(chId);
    }
  }

  ILOG(Info) << "Using reference channels: ";
  for (auto chId : mReferenceChIDs) {
    ILOG(Info) << std::to_string(chId) << " ";
  }
  ILOG(Info) << ENDM;

  // Get HV gain and B-field configuration
  mAdcPerMip = o2::quality_control_modules::common::getFromConfig<double>(mCustomParameters, "adcPerMIP", 14);
  ILOG(Info) << "ADC per MIP: " << std::to_string(mAdcPerMip) << ENDM;

  mReset = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "reset", false);
  ILOG(Info, Support) << "Reset: " << mReset << ENDM;

  // Register histograms

  mAmpVsChStatMean = fit::helper::registerHist<TH1F>(getObjectsManager(),
                                                     quality_control::core::PublicationPolicy::ThroughStop,
                                                     "", "AmpPerChannelStatMean", "AmpPerChannelStatMean",
                                                     sNCHANNELS_PM, 0, sNCHANNELS_PM);
  mAmpVsChNormStatMean = fit::helper::registerHist<TH1F>(getObjectsManager(),
                                                         quality_control::core::PublicationPolicy::ThroughStop,
                                                         "", "AmpPerChannelNormStatMean", "AmpPerChannelNormStatMean",
                                                         sNCHANNELS_PM, 0, sNCHANNELS_PM);

  if (mReset) {
    ILOG(Warning) << "This is a resetting run and should be performed BY FIT EXPERTS ONLY! If this is not the case, please stop the run and contact the FIT expert." << ENDM;
    mAmpVsChNormStatMeanAfterLastCorrection = fit::helper::registerHist<TH1F>(getObjectsManager(),
                                                                              quality_control::core::PublicationPolicy::ThroughStop,
                                                                              "", "AmpPerChannelNormStatMeanAfterLastCorrection", "AmpPerChannelNormStatMeanAfterLastCorrection",
                                                                              sNCHANNELS_PM, 0, sNCHANNELS_PM);
  }

  mAmpVsChNormStatMeanCorrected = fit::helper::registerHist<TH1F>(getObjectsManager(),
                                                                  quality_control::core::PublicationPolicy::ThroughStop,
                                                                  "", "AmpPerChannelNormStatMeanCorrected", "AmpPerChannelNormStatMeanCorrected",
                                                                  sNCHANNELS_PM, 0, sNCHANNELS_PM);

  // const double_t nofOrbitsPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();
  // auto magField = o2::base::GRPGeomHelper::instance().getGRPMagField();
  // // ILOG(Info) << "L3 Current: " << magField->getL3Current() << ENDM;
  // // ILOG(Info) << "Dipole Current: " << magField->getDipoleCurrent() << ENDM;
  // // ILOG(Info) << "Uniform Field: " << magField->getFieldUniformity() << ENDM;
  // // ILOG(Info) << "Nominal L3 Field: " << magField->getNominalL3Field() << ENDM;
  // magField->print();
}

void AgingLaserPostProc::update(Trigger t, framework::ServiceRegistryRef services)
{
  ILOG(Info) << "update" << ENDM;

  // Reset histograms. This is needed when running async post processing over many laser scan runs.
  mAmpVsChStatMean->Reset();
  mAmpVsChNormStatMean->Reset();
  if (mReset) {
    mAmpVsChNormStatMeanAfterLastCorrection->Reset();
  }
  mAmpVsChNormStatMeanCorrected->Reset();

  auto& qcdb = services.get<quality_control::repository::DatabaseInterface>();

  // TODO: don't hard code MO path names

  ILOG(Info) << "Get histos" << ENDM;

  // Get the amplitude per channel histogram produced by the AgingLaserTask

  auto moHistAmpPerChannel = qcdb.retrieveMO("FT0/MO/AgingLaser", "AmpPerChannel", t.timestamp, t.activity);
  TH2* histAmpPerChannel = moHistAmpPerChannel ? dynamic_cast<TH2*>(moHistAmpPerChannel->getObject()) : nullptr;

  // Statistical mean of amplitude per detector channel, normalized by the average of the reference channel amplitudes, as stored after performing an aging correction.
  // This is used as a normalization factor to get the relative aging since the time of the last aging correction in "non-reset" runs.
  std::unique_ptr<TH1> histAmpPerChannelNormStatMeanAfterLastCorrection;

  if (!mReset) {
    // TODO: don't hard code provenance?
    auto validity = qcdb.getLatestObjectValidity("qc/FT0/MO/AgingLaserPostProc/AmpPerChannelNormStatMeanAfterLastCorrection");
    long timestamp = validity.getMin();
    ILOG(Info) << "timestamp = " << timestamp << ENDM;

    auto moHistAmpPerChannelNormStatMeanAfterLastCorrection = qcdb.retrieveMO("FT0/MO/AgingLaserPostProc", "AmpPerChannelNormStatMeanAfterLastCorrection", timestamp);
    if (!moHistAmpPerChannelNormStatMeanAfterLastCorrection) {
      ILOG(Fatal) << "Failed to retrieve histogram " << "FT0/MO/AgingLaserPostProc/AmpPerChannelNormStatMeanAfterLastCorrection"
                  << " for timestamp " << timestamp << "! This is not a 'resetting' run and this histogram is therefore needed."  << ENDM;
    } else {
      histAmpPerChannelNormStatMeanAfterLastCorrection = std::unique_ptr<TH1>(dynamic_cast<TH1*>(moHistAmpPerChannelNormStatMeanAfterLastCorrection->getObject()->Clone()));
    }

    if (!histAmpPerChannelNormStatMeanAfterLastCorrection) {
      ILOG(Fatal) << "Failed to retrieve histogram " << "FT0/MO/AgingLaserPostProc/AmpPerChannelNormStatMeanAfterLastCorrection"
                  << " for timestamp " << timestamp << "! This is not a 'resetting' run and this histogram is therefore needed."  << ENDM;
    } else {
      for (int i = 0; i < histAmpPerChannelNormStatMeanAfterLastCorrection->GetNbinsX(); i++) {
        ILOG(Info) << "bin " << i << " = " << std::to_string(histAmpPerChannelNormStatMeanAfterLastCorrection->GetBinContent(i)) << ENDM;
      }
    }
  }

  // Fill histograms

  ILOG(Info) << "Fill histos" << ENDM;

  if (histAmpPerChannel) {
    // Calcualate the average statistical mean of the reference channel amplitudes
    float refChMean = 0;
    int nRefCh = 0;

    // for (int chId = 208; chId <= 211; chId++) {
    for (auto chId : mReferenceChIDs) {
      // TODO: cuts
      auto refProj = histAmpPerChannel->ProjectionY("refProj", chId + 1, chId + 1);
      refChMean += refProj->GetMean();
      nRefCh++;
      ILOG(Info, Support) << "refProj->GetMean() = " << refProj->GetMean() << ENDM;
    }

    refChMean /= nRefCh;

    ILOG(Info, Support) << "refChMean = " << refChMean << ENDM;

    // Fill the statistical mean of the amplitude per detector channel, normalized by the average of the reference channel amplitudes

    // for (int chId = 0; chId < 208; chId++) {
    for (auto chId : mDetectorChIDs) {
      // TODO: cuts
      ILOG(Info) << "projeting chId = " << std::to_string(chId) << ENDM;
      auto proj = histAmpPerChannel->ProjectionY("proj", chId + 1, chId + 1);
      mAmpVsChStatMean->SetBinContent(chId + 1, proj->GetMean());
      mAmpVsChNormStatMean->SetBinContent(chId + 1, proj->GetMean() / refChMean);
      if (mReset) {
        // mAmpVsChNormStatMeanAfterLastCorrection->SetBinContent(chId + 1, proj->GetMean() / refChMean);
        mAmpVsChNormStatMeanAfterLastCorrection->SetBinContent(chId + 1, mAmpVsChNormStatMean->GetBinContent(chId + 1));
      }
      ILOG(Info) << "Normalization" << ENDM;
      // histAmpPerChannelNormStatMeanAfterLastCorrection = dynamic_cast<TH1F*>(moHistAmpPerChannelNormStatMeanAfterLastCorrection->getObject());
      if (!mReset) {
        ILOG(Info) << histAmpPerChannelNormStatMeanAfterLastCorrection->GetBinContent(chId + 1) << ENDM;
      }
      ILOG(Info) << mAmpVsChNormStatMean->GetBinContent(chId + 1) << "hi" << ENDM;

      Float_t normalization = (!mReset ? histAmpPerChannelNormStatMeanAfterLastCorrection->GetBinContent(chId + 1) : mAmpVsChNormStatMean->GetBinContent(chId + 1));
      if (normalization == 0) {
        ILOG(Error, Support) << "Normalization factor is zero for channel " << std::to_string(chId) << "!" << ENDM;
        continue;
      }
      ILOG(Info) << "Correct" << ENDM;
      mAmpVsChNormStatMeanCorrected->SetBinContent(chId + 1, mAmpVsChNormStatMean->GetBinContent(chId + 1) / normalization);
    }
  } else {
    ILOG(Error, Support) << "Failed to retrieve histogram " << "FT0/MO/AgingLaser/AmpPerChannel"
                         << " for timestamp " << t.timestamp << "!" << ENDM;
  }

  ILOG(Info) << "Update done" << ENDM;
}

void AgingLaserPostProc::finalize(Trigger, framework::ServiceRegistryRef)
{
  ILOG(Info) << "finalize" << ENDM;
}

} // namespace o2::quality_control_modules::ft0
