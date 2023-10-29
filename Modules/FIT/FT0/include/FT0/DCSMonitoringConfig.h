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
/// \file   DCSMonitoringConfig.h
/// \author Andreas Molander andreas.molander@cern.ch
///

#ifndef QC_MODULE_FT0_DCSMONITORINGCONFIG_H
#define QC_MODULE_FT0_DCSMONITORINGCONFIG_H

#include "QualityControl/PostProcessingConfig.h"

#include <boost/property_tree/ptree.hpp>
#include <string>
#include <vector>

namespace o2::quality_control_modules::ft0
{

struct DCSMonitoringConfig : o2::quality_control::postprocessing::PostProcessingConfig {
  DCSMonitoringConfig() = default;
  DCSMonitoringConfig(std::string name, const boost::property_tree::ptree& config);
  ~DCSMonitoringConfig() = default;

  // TODO: allows scaling and units per datapoint
  struct Plot {
    std::string name;
    std::vector<std::string> datapoints;
    float scaling;
    std::string unit;
    float timespanHours;
  };

  std::vector<Plot> plots;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_DCSMONITORINGCONFIG_H
