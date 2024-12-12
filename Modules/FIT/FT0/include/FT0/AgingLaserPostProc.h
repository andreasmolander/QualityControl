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
/// \file   AgingLaserPostProc.h
/// \author Andreas Molander <andreas.molander@cern.ch>
///

#ifndef QC_MODULE_FT0_AGINGLASERPOSTPROC_H
#define QC_MODULE_FT0_AGINGLASERPOSTPROC_H

#include "QualityControl/PostProcessingInterface.h"

#include <FT0Base/Constants.h>

#include <TH1F.h>

#include <memory>
#include <vector>

class TH1F;

namespace o2::quality_control_modules::ft0
{

/// \brief Post processing task for the laser aging monitoring
/// \author Andreas Molander
class AgingLaserPostProc final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  AgingLaserPostProc() = default;
  ~AgingLaserPostProc() override;

  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  // Constants

  /// Max number of FT0 channels
  constexpr static std::size_t sNCHANNELS_PM = o2::ft0::Constants::sNCHANNELS_PM;

  // Task parameters

  std::vector<uint8_t> mDetectorChIDs;  ///< Enabled detector channels
  std::vector<uint8_t> mReferenceChIDs; ///< Enabled reference channels

  double mAdcPerMip; ///< Gain as number of ADC channels per MIP

  /// Flag to indicate whether the aging correction has recently been performed.
  /// If true, the trends should be reset.
  /// If true, mAmpVsChNormStatMeanAfterLastCorrection will be published to the database, and this will be used
  /// as a normalization factor for the datapoints in the new reset trends.
  bool mReset = false;

  // Histograms

  // ### Below follows histograms that are the same for all gain and B-field settings (maybe only for testing/debug?)

  /// Statistical mean of amplitude per detector channel
  std::unique_ptr<TH1F> mAmpVsChStatMean;

  /// Statistical mean of amplitude per detector channel, normalized by the average of the reference channel amplitudes
  std::unique_ptr<TH1F> mAmpVsChNormStatMean;

  /// Statistical mean of amplitude per detector channel, normalized by the average of the reference channel amplitudes.
  /// This is stored only at a "reset" run, and is used as a normalization factor for mAmpVsChNormStatMeanCorrected
  std::unique_ptr<TH1F> mAmpVsChNormStatMeanAfterLastCorrection;

  /// Statistical mean of amplitude per detector channel, normalized by the average of the reference channel amplitudes
  /// AND by the normalized statisical mean of aplitude per detector channels after the last aging correction
  std::unique_ptr<TH1F> mAmpVsChNormStatMeanCorrected;

  // ### Below follows histograms that are specific for each gain and B-field setting. Their TObject name will be set according
  // ### to the gain and B-field setting upon task initialization. This way they will be published to the database with unique names.

  // Statistical mean of ampltiude per detector channel
  std::unique_ptr<TH1F> mAmpVsChStatMeanConfigSepcific;

  // Statistical mean of amplitude per detector channel, normalized by the average of the reference channel amplitudes
  std::unique_ptr<TH1F> mAmpVsChNormStatMeanConfigSepcific;

  // Statistical mean of amplitude per detector channel, normalized by the average of the reference channel amplitudes.
  // This is stored only at a "reset" run, and is used as a normalization factor for mAmpVsChNormStatMeanCorrectedConfigSpecific
  std::unique_ptr<TH1F> mAmpVsChNormStatMeanAfterLastCorrectionConfigSepcific;

  // Statistical mean of amplitude per detector channel, normalized by the average of the reference channel amplitudes
  /// AND by the normalized statisical mean of aplitude per detector channels after the last aging correction
  std::unique_ptr<TH1F> mAmpVsChNormStatMeanCorrectedConfigSpecific;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_AGINGLASERPOSTPROC_H