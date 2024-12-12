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
/// \file   MeanCheck.h
/// \author My Name
///

#ifndef QC_MODULE_FV0_FV0MEANCHECK_H
#define QC_MODULE_FV0_FV0MEANCHECK_H

#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::fv0
{

/// \brief  Example QC Check
/// \author My Name
class MeanCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  MeanCheck() = default;
  /// Destructor
  ~MeanCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;
  void reset() override;
  void startOfActivity(const Activity& activity) override;
  void endOfActivity(const Activity& activity) override;

 private:
  std::shared_ptr<Activity> mActivity;
  float mLimit;

  ClassDefOverride(MeanCheck, 3);
};

} // namespace o2::quality_control_modules::fv0

#endif // QC_MODULE_FV0_FV0MEANCHECK_H
