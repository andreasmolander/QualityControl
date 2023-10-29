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
/// \file   DCSMonitoring.cxx
/// \author Andreas Molander andreas.molander@cern.ch
///

#include "FT0/DCSMonitoring.h"
#include "FT0/DCSMonitoringConfig.h"
// #include "QualityControl/DatabaseInterface.h"
// #include "QualityControl/PostProcessingConfig.h"
#include "QualityControl/QcInfoLogger.h"
// #include "QualityControl/RepoPathUtils.h"

// O2
#include "CCDB/CcdbApi.h"
// #include "CCDB/CCDBTimeStampUtils.h"
#include "DataFormatsFIT/DCSDPValues.h"
#include "DetectorsDCS/AliasExpander.h"
#include "DetectorsDCS/DataPointIdentifier.h"
#include "DetectorsDCS/DeliveryType.h"

// #include "TH1F.h"
#include "TAxis.h"
#include "TGraph.h"
#include "TMultiGraph.h"

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::ft0
{

DCSMonitoring::~DCSMonitoring()
{
  for (auto& canvas : mCanvases) {
    delete canvas.second;
  }
}

void DCSMonitoring::configure(std::string name, const boost::property_tree::ptree& config)
{
  mConfig = DCSMonitoringConfig(name, config);
}

void DCSMonitoring::initialize(Trigger, framework::ServiceRegistryRef services)
{
  ILOG(Info, Support) << "Initializing" << ENDM;

  mCcdbApi.init(mConfig.ccdbUrl);

  for (const auto& plot : mConfig.plots) {
    for (const std::string& datapoint : plot.datapoints) {
      mDPtimeValues[plot.name + datapoint] = std::deque<uint64_t>();
      mDPValues[plot.name + datapoint] = std::deque<int64_t>();
    }
  }
}

void DCSMonitoring::update(Trigger t, framework::ServiceRegistryRef)
{
  // TODO: usage of t.timestamp when async
  ILOG(Info, Support) << "Updating" << ENDM;
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;
  // mHistogram->Fill(t.timestamp % 30000);

  const std::map<std::string, std::string> metadata;
  auto map = mCcdbApi.retrieveFromTFileAny<std::unordered_map<o2::dcs::DataPointIdentifier, o2::fit::DCSDPValues>>("FT0/Calib/DCSDPs", metadata, t.timestamp);

  o2::dcs::DataPointIdentifier dpIdTmp;
  o2::fit::DCSDPValues* ccdbDPValuesPointer = nullptr;

  for (const auto& plot : mConfig.plots) {
    for (const std::string& dpAlias : plot.datapoints) {
      // Find the datapoint in the CCDB object, TODO: make better
      o2::dcs::DataPointIdentifier::FILL(dpIdTmp, dpAlias, o2::dcs::DeliveryType::DPVAL_DOUBLE);
      if (map->find(dpIdTmp) != map->end()) {
        ccdbDPValuesPointer = &map->at(dpIdTmp);
      } else {
        o2::dcs::DataPointIdentifier::FILL(dpIdTmp, dpAlias, o2::dcs::DeliveryType::DPVAL_UINT);
        if (map->find(dpIdTmp) != map->end()) {
          ccdbDPValuesPointer = &map->at(dpIdTmp);
        } else {
          ccdbDPValuesPointer = nullptr;
        }
      }

      if (ccdbDPValuesPointer && !ccdbDPValuesPointer->empty()) {
        // Check if there are newer values in the CCDB object
        if (mDPtimeValues[plot.name + dpAlias].empty() || (mDPtimeValues[plot.name + dpAlias].back() < ccdbDPValuesPointer->values.back().first)) {
          for (const auto& dpValue : ccdbDPValuesPointer->values) {
            // Only add the newer values
            if (mDPtimeValues[plot.name + dpAlias].empty() || (dpValue.first > mDPtimeValues[plot.name + dpAlias].back())) {
              mDPtimeValues[plot.name + dpAlias].push_back(dpValue.first);
              mDPValues[plot.name + dpAlias].push_back(dpValue.second);
            }
          }
        }
      } else {
        ILOG(Error, Support) << "Values for DP '" << dpAlias << "' not found in CCDB object for timestamp " << t.timestamp << ENDM;
      }

      // Store (and plot) only values from the latest time period of plot.timespanHours hours, remove older values
      if (!mDPtimeValues[plot.name + dpAlias].empty()) {
        // Set a datapoint at 'now' with the latest known value
        // This is to have the trend start at 'now' and span plot.timespanHours backwards
        if (mDPtimeValues[plot.name + dpAlias].back() < t.timestamp) {
          mDPtimeValues[plot.name + dpAlias].push_back(t.timestamp);
          mDPValues[plot.name + dpAlias].push_back(mDPValues[plot.name + dpAlias].back());
        }
        
        uint64_t firstValue;
        bool remove = mDPtimeValues[plot.name + dpAlias].front() < mDPtimeValues[plot.name + dpAlias].back() - plot.timespanHours * o2::ccdb::CcdbObjectInfo::MINUTE;
        bool removed = remove;
        while (remove) {
          ILOG(Info, Devel) << "Removing" << ENDM; // TODO remove
          firstValue = mDPValues[plot.name + dpAlias].front();
          mDPtimeValues[plot.name + dpAlias].pop_front();
          mDPValues[plot.name + dpAlias].pop_front();
          remove = mDPtimeValues[plot.name + dpAlias].front() < mDPtimeValues[plot.name + dpAlias].back() - plot.timespanHours * o2::ccdb::CcdbObjectInfo::MINUTE;
        }
        if (removed && mDPtimeValues[plot.name + dpAlias].front() > mDPtimeValues[plot.name + dpAlias].back() - plot.timespanHours * o2::ccdb::CcdbObjectInfo::MINUTE) {
          mDPtimeValues[plot.name + dpAlias].push_front(mDPtimeValues[plot.name + dpAlias].back() - plot.timespanHours * o2::ccdb::CcdbObjectInfo::MINUTE);
          mDPValues[plot.name + dpAlias].push_front(firstValue);
        }
      }
    }
  }

  generatePlots();
}

void DCSMonitoring::finalize(Trigger, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Finalizing" << ENDM;
  // Only if you don't want it to be published after finalisation.
  // getObjectsManager()->stopPublishing(mHistogram);
  // getObjectsManager()->stopPublishing(mGraph.get());
  // getObjectsManager()->startPublishing(mGraph.get());
  // getObjectsManager()->stopPublishing(mGraph.get());
  // getObjectsManager()->stopPublishing(mMultiGraph.get());
  // getObjectsManager()->stopPublishing(mCanvas.get());
}

void DCSMonitoring::generatePlots()
{
  ILOG(Info, Devel) << "Generating " << mConfig.plots.size() << " plots." << ENDM;

  for (const auto& plot : mConfig.plots) {
    ILOG(Info, Devel) << "Generating plot " << plot.name << ENDM;

    // From TrendingTask.cxx:
    // Before we generate any new plots, we have to delete existing under the same names.
    // It seems that ROOT cannot handle an existence of two canvases with a common name in the same process.
    if (mCanvases.count(plot.name)) {
      getObjectsManager()->stopPublishing(plot.name);
      // The TCanvas owns the TMultiGraph which owns the TGraph's, so this should delete everything
      delete mCanvases[plot.name];
    }

    TMultiGraph* multiGraph = new TMultiGraph();

    for (const auto& graphName : plot.datapoints) {
      ILOG(Info, Devel) << "Generating graph " << graphName << ENDM;

      // TODO: check if values

      TGraph* graph = new TGraph();
      graph->SetName((plot.name + graphName).c_str());

      Int_t iPoint = 0;

      for (Int_t i = 0; i < mDPtimeValues[plot.name + graphName].size(); i++) {
        // ROOT uses seconds for time

        // At each time, add an extra point with the previous value, to keep the trend lines horizontal.
        // Beteen two points, the real value is the latest know, and not a value on the straight diagonal line between two points.
          if (i > 0) {
            graph->SetPoint(iPoint++, mDPtimeValues[plot.name + graphName].at(i) / 1000., mDPValues[plot.name + graphName].at(i - 1) * plot.scaling);
          }
        graph->SetPoint(iPoint++, mDPtimeValues[plot.name + graphName].at(i) / 1000., mDPValues[plot.name + graphName].at(i) * plot.scaling);
      }

      multiGraph->Add(graph);
    }

    TCanvas* canvas = new TCanvas();
    canvas->SetName(plot.name.c_str());
    canvas->SetTitle(plot.name.c_str());

    multiGraph->SetName(plot.name.c_str());
    multiGraph->SetTitle(plot.name.c_str());
    multiGraph->GetXaxis()->SetTimeDisplay(1);
    multiGraph->GetXaxis()->SetNdivisions(505);
    multiGraph->GetXaxis()->SetTimeOffset(0.0);
    multiGraph->GetXaxis()->SetTimeFormat("#splitline{%d.%m.%y}{%H:%M:%S}");
    multiGraph->Draw("apl pmc plc");

    canvas->BuildLegend();

    mCanvases[plot.name] = canvas;
    getObjectsManager()->startPublishing(canvas);
  }
}

} // namespace o2::quality_control_modules::ft0
