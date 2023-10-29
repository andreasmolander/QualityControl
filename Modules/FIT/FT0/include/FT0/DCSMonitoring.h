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
/// \file   DCSMonitoring.h
/// \author My Name
///

#ifndef QC_MODULE_FT0_DCSMONITORING_H
#define QC_MODULE_FT0_DCSMONITORING_H

// QualityControl
#include "FT0/DCSMonitoringConfig.h"
#include "QualityControl/PostProcessingConfig.h"
#include "QualityControl/PostProcessingInterface.h"
  
// O2
#include "CCDB/CcdbApi.h"

// ROOT
#include "TCanvas.h"
#include "TGraph.h"
#include "TMultiGraph.h"

#include <deque>
#include <memory>

// class TH1F;

namespace o2::quality_control_modules::ft0
{

/// \brief Example Quality Control Postprocessing Task
/// \author My Name
class DCSMonitoring final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  DCSMonitoring() = default;
  /// \brief Destructor
  ~DCSMonitoring() override;

  /// \brief Configuration of a post-processing task.
  /// Configuration of a post-processing task. Can be overridden if user wants to retrieve the configuration of the task.
  /// \param name     Name of the task
  /// \param config   ConfigurationInterface with prefix set to ""
  void configure(std::string name, const boost::property_tree::ptree& config) override;
  /// \brief Initialization of a post-processing task.
  /// Initialization of a post-processing task. User receives a Trigger which caused the initialization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::SOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  /// \brief Update of a post-processing task.
  /// Update of a post-processing task. User receives a Trigger which caused the update and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::Period
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistryRef) override;

 private:
  void generatePlots();

  DCSMonitoringConfig mConfig;
  o2::ccdb::CcdbApi mCcdbApi;
  // std::string mDataUrl;
  // TH1F* mHistogram = nullptr;
  // std::unique_ptr<TCanvas> mCanvas;
  // std::unique_ptr<TMultiGraph> mMultiGraph;
  // std::unique_ptr<TGraph> mGraph;
  // int mPointCounter;

  // std::map<std::string, std::vector<std::string>> mDataPointTrendNames;

  std::map<std::string, std::deque<uint64_t>> mDPtimeValues;
  std::map<std::string, std::deque<int64_t>> mDPValues;

  std::map<std::string, TCanvas*> mCanvases;
  // std::map<std::string, TGraph*> mGraphs;
  // std::map<std::string, TMultiGraph*> mMultiGraphs;
};

} // namespace o2::quality_control_modules::ft0

#endif // QC_MODULE_FT0_DCSMONITORING_H