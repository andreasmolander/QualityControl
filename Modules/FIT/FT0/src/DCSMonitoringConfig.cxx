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
/// \file   DCSMonitoringConfig.cxx
/// \author Andreas Molander andreas.molander@cern.ch
///

#include "FT0/DCSMonitoringConfig.h"

// O2
#include "DetectorsDCS/AliasExpander.h"

#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control_modules::ft0
{

DCSMonitoringConfig::DCSMonitoringConfig(std::string name, const boost::property_tree::ptree& config)
  : o2::quality_control::postprocessing::PostProcessingConfig(name, config)
{
  // TODO: don't allow multiple plots with the same name
  for (const auto& plotConfig : config.get_child("qc.postprocessing." + name + ".plots")) {
    const auto& plotName = plotConfig.second.get_child_optional("name");
    const auto& datapoints = plotConfig.second.get_child_optional("datapoints");
    
    if (plotName.has_value() && datapoints.has_value()) {
      // TODO: don't allow the same DP multiple times in a plot
      std::vector<std::string> aliases;
      for (const auto& dpAlias : datapoints.value()) {
        aliases.push_back(dpAlias.second.data());
      }

      plots.push_back({ plotName.value().data(),
                        o2::dcs::expandAliases(aliases),
                        plotConfig.second.get<float>("scaling", 1.),
                        plotConfig.second.get<std::string>("unit", ""),
                        plotConfig.second.get<float>("timespanHours", 24) });

    } else {
      throw std::runtime_error("Missing 'name' value or a 'datapoints' vector in the path 'qc.postprocessing." + name + ".plots'");
    }
  }
}

} // namespace o2::quality_control_modules::ft0
