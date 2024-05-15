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
/// \file   RecPointsQcTask.h
/// \author Andreas Molander <andreas.molander@cern.ch>, Fabiola Lugo <fabiola.viridiana.lugo@cern.ch>
///

#ifndef QC_MODULE_FV0_FV0RECPOINTSQCTASK_H
#define QC_MODULE_FV0_FV0RECPOINTSQCTASK_H

#include <memory>
#include "QualityControl/TaskInterface.h"

#include <TH1.h>

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::fv0
{

/// \brief RecPoints QC task
/// \author Andreas Molander, Fabiola Lugo
class RecPointsQcTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  RecPointsQcTask() = default;
  /// Destructor
  ~RecPointsQcTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

  private:
  /// TODO: this is a dummy histogram, replace it with something meaningful
  std::unique_ptr<TH1F> mHistogram = nullptr;
};
} // namespace o2::quality_control_modules::fv0

#endif // QC_MODULE_FV0_FV0RECPOINTSQCTASK_H
