// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright
// holders. All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   AgingLaserReductor.h
/// \author Sandor Lokos <sandor.lokos@cern.ch>, Edmundo Garcia-Solis <edmundo.garcia@cern.ch>, Andreas Molander <andreas.molander@cern.ch>
///
#ifndef QUALITYCONTROL_AGINGLASERREDUCTOR_H
#define QUALITYCONTROL_AGINGLASERREDUCTOR_H

#include "QualityControl/ReductorTObject.h"

namespace o2::quality_control_modules::ft0
{

class AgingLaserReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  AgingLaserReductor() = default;
  ~AgingLaserReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  // TODO: fetch these from somewhere?
  static constexpr int mNDetectorChannels = 208;
  static constexpr int mNChannels = 212;
  static constexpr int NChannel = 208;
  struct {
    Double_t validity1;
    Double_t validity2;
    Double_t mean1;
    Double_t mean2;
    Double_t mean[NChannel];
    Double_t stddev1;
    Double_t stddev2;
    Double_t stddev[NChannel];

  } mStats;
};

} // namespace o2::quality_control_modules::ft0

#endif // QUALITYCONTROL_AGINGLASERREDUCTOR_H