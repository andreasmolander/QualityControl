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
/// \file   AgingLaserTask.h
/// \author Sandor Lokos, Edmundo Garcia-Solis, Andreas Molander
///

#ifndef QC_MODULE_FT0_AGINGLASERTASK_H
#define QC_MODULE_FT0_AGINGLASERTASK_H

#include "QualityControl/TaskInterface.h"

#include <CommonConstants/LHCConstants.h>
#include <FT0Base/Constants.h>

#include <TH1I.h>
#include <TH2I.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <type_traits>
#include <vector>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::ft0
{

class AgingLaserTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  AgingLaserTask() = default;
  /// \brief Destructor
  ~AgingLaserTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  // Constants
  constexpr static std::size_t sNCHANNELS_PM = o2::ft0::Constants::sNCHANNELS_PM;
  constexpr static std::size_t sMaxBC = o2::constants::lhc::LHCMaxBunches;

  // Task parameters
  std::set<uint8_t> mSetAllowedChIDs;
  std::set<uint8_t> mSetRefChIDs;
  std::set<uint8_t> mSetAmpCut; // TODO: one cut, or one per channel?

  // BC distributions, maybe only for debug (TODO)
  std::unique_ptr<TH1I> mHistBC;
  std::unique_ptr<TH1I> mHistBCDetector;
  std::unique_ptr<TH1I> mHistBCReference;
  std::unique_ptr<TH1I> mHistBCAmpCut;
  std::unique_ptr<TH1I> mHistBCAmpCutADC0;
  std::unique_ptr<TH1I> mHistBCAmpCutADC1;
  std::unique_ptr<TH1I> mHistBCDetectorAmpCut;
  std::unique_ptr<TH1I> mHistBCDetectorAmpCutADC0;
  std::unique_ptr<TH1I> mHistBCDetectorAmpCutADC1;
  std::unique_ptr<TH1I> mHistBCReferenceAmpCut;
  std::unique_ptr<TH1I> mHistBCReferenceAmpCutADC0;
  std::unique_ptr<TH1I> mHistBCReferenceAmpCutADC1;

  // Amplitude per channel histograms
  std::unique_ptr<TH2I> mHistAmpVsCh; // Not really needed, sum of ADC histograms could be created in post processing instead (TODO)
  std::unique_ptr<TH2I> mHistAmpVsChADC0;
  std::unique_ptr<TH2I> mHistAmpVsChADC1;

  // Amplitude per BC for reference channels (TODO: should we add for all channels, maybe only for debug?)
  std::map<uint8_t, std::unique_ptr<TH2I>> mMapHistAmpVsBC; // Not really needed, sum of ADC histograms could be created in post processing instead (TODO)
  std::map<uint8_t, std::unique_ptr<TH2I>> mMapHistAmpVsBCADC0;
  std::map<uint8_t, std::unique_ptr<TH2I>> mMapHistAmpVsBCADC1;

  // Reference channel ampltide histograms, some maybe only for debug (TODO)
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapHistAmp;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapHistAmpADC0;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapHistAmpADC1;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapHistAmpPeak1;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapHistAmpPeak2;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapHistAmpPeak1ADC0;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapHistAmpPeak1ADC1;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapHistAmpPeak2ADC0;
  std::map<uint8_t, std::unique_ptr<TH1I>> mMapHistAmpPeak2ADC1;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_AGINGLASERTASK_H
