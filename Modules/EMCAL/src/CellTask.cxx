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
/// \file   CellTask.cxx
/// \author Markus Fasel, Cristina Terrevoli
///

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <iostream>

#include <TCanvas.h>
#include <TH2.h>
#include <TProfile2D.h>

#include <DataFormatsEMCAL/TriggerRecord.h>
#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/CellTask.h"
#include "DataFormatsEMCAL/Cell.h"
#include "EMCALBase/Geometry.h"
#include "EMCALCalib/CalibDB.h"
#include "EMCALCalib/BadChannelMap.h"
#include "EMCALCalib/TimeCalibrationParams.h"
#include <Framework/ConcreteDataMatcher.h>
#include <Framework/DataRefUtils.h>
#include <Framework/DataSpecUtils.h>
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <CommonConstants/Triggers.h>

namespace o2
{
namespace quality_control_modules
{
namespace emcal
{

CellTask::~CellTask()
{
  for (auto en : mHistogramContainer) {
    en.second.clean();
  }
  if (mEvCounterTF)
    delete mEvCounterTF;
  if (mEvCounterTFPHYS)
    delete mEvCounterTFPHYS;
  if (mEvCounterTFCALIB)
    delete mEvCounterTFCALIB;
  if (mTFPerCycles)
    delete mTFPerCycles;
  if (mTFPerCyclesTOT)
    delete mTFPerCyclesTOT;
  if (mCellsMaxSM)
    delete mCellsMaxSM;
  if (mCells_ev_sm)
    delete mCells_ev_sm;
  if (mCells_ev_smThr)
    delete mCells_ev_smThr;
  if (mCells_ev)
    delete mCells_ev;
  if (mCells_ev_Thres)
    delete mCells_ev_Thres;
  if (mCells_ev_EMCAL)
    delete mCells_ev_EMCAL;
  if (mCells_ev_EMCAL_Thres)
    delete mCells_ev_EMCAL_Thres;
  if (mCells_ev_DCAL)
    delete mCells_ev_DCAL;
  if (mCells_ev_DCAL_Thres)
    delete mCells_ev_DCAL_Thres;
  if (mFracGoodCellsEvent)
    delete mFracGoodCellsEvent;
  if (mFracGoodCellsSM)
    delete mFracGoodCellsSM;
}

void CellTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::setDetector("EMC");
  ILOG(Info, Support) << "initialize CellTask" << ENDM;
  // define histograms

  auto get_bool = [](const std::string_view input) -> bool {
    return input == "true";
  };

  auto get_double = [](const std::string_view input) -> double {
    double result = 0.;
    if (input.length()) {
      try {
        result = std::stof(input.data());
      } catch (...) {
      }
    }
    return result;
  };

  mTaskSettings.mHasAmpVsCellID = get_bool(getConfigValueLower("hasAmpVsCell")),
  mTaskSettings.mHasTimeVsCellID = get_bool(getConfigValueLower("hasTimeVsCell")),
  mTaskSettings.mHasHistosCalib2D = get_bool(getConfigValueLower("hasHistCalib2D"));
  if (hasConfigValue("thresholdTimePhys"))
    mTaskSettings.mAmpThresholdTimePhys = get_double(getConfigValue("thresholdTimePhys"));
  if (hasConfigValue("thresholdTimeCalib"))
    mTaskSettings.mAmpThresholdTimeCalib = get_double(getConfigValue("thresholdTimeCalib"));
  if (hasConfigValue("thresholdCAL"))
    mTaskSettings.mThresholdCAL = get_double(getConfigValue("thresholdCAL"));
  if (hasConfigValue("thresholdPHYS"))
    mTaskSettings.mThresholdPHYS = get_double(getConfigValue("thresholdPHYS"));
  ILOG(Info, Support) << "Amplitude cut time histograms (PhysTrigger) " << mTaskSettings.mAmpThresholdTimePhys << ENDM;
  ILOG(Info, Support) << "Amplitude cut time histograms (CalibTrigger) " << mTaskSettings.mAmpThresholdTimeCalib << ENDM;
  ILOG(Info, Support) << "Amplitude cut occupancy histograms (PhysTrigger) " << mTaskSettings.mThresholdPHYS << ENDM;
  ILOG(Info, Support) << "Amplitude cut occupancy histograms (CalibTrigger) " << mTaskSettings.mThresholdCAL << ENDM;

  mIgnoreTriggerTypes = get_bool(getConfigValue("ignoreTriggers"));

  if (mTaskSettings.mHasAmpVsCellID) {
    ILOG(Debug, Support) << "Enabling histograms : Amplitude vs. cellID" << ENDM;
  }
  if (mTaskSettings.mHasTimeVsCellID) {
    ILOG(Debug, Support) << "Enabling histograms : Time vs. cellID" << ENDM;
  }
  if (mTaskSettings.mHasHistosCalib2D) {
    ILOG(Debug, Support) << "Enabling calibrated histograms" << ENDM;
  }

  // initialize geometry
  if (!mGeometry)
    mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);

  std::array<std::string, 2> triggers = { { "CAL", "PHYS" } };
  for (const auto& trg : triggers) {
    CellHistograms histos;
    histos.mGeometry = mGeometry;
    histos.initForTrigger(trg.data(), mTaskSettings);
    histos.startPublishing(*getObjectsManager());
    mHistogramContainer[trg] = histos;
  } // trigger type
  // new histos
  mTFPerCyclesTOT = new TH1D("NumberOfTFperCycles_TOT", "NumberOfTFperCycles_TOT", 100, -0.5, 99.5); //
  mTFPerCyclesTOT->GetXaxis()->SetTitle("NumberOfTFperCyclesTOT");
  mTFPerCyclesTOT->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mTFPerCyclesTOT);

  mTFPerCycles = new TH1D("NumberOfTFperCycles_1", "NumberOfTFperCycles_1", 1, -0.5, 1.5);
  mTFPerCycles->GetXaxis()->SetTitle("NumberOfTFperCycles");
  mTFPerCycles->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mTFPerCycles);

  mEvCounterTF = new TH1D("NEventsPerTF", "NEventsPerTF", 401, -0.5, 400.5);
  mEvCounterTF->GetXaxis()->SetTitle("NEventsPerTimeFrame");
  mEvCounterTF->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mEvCounterTF);

  mEvCounterTFPHYS = new TH1D("NEventsPerTFPHYS", "NEventsPerTFPHYS", 401, -0.5, 400.5);
  mEvCounterTFPHYS->GetXaxis()->SetTitle("NEventsPerTimeFrame_PHYS");
  mEvCounterTFPHYS->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mEvCounterTFPHYS);

  mEvCounterTFCALIB = new TH1D("NEventsPerTFCALIB", "NEventsPerTFCALIB", 6, -0.5, 5.5);
  mEvCounterTFCALIB->GetXaxis()->SetTitle("NEventsPerTimeFrame_CALIB");
  mEvCounterTFCALIB->GetYaxis()->SetTitle("Counts");
  getObjectsManager()->startPublishing(mEvCounterTFCALIB);

  mCellsMaxSM = new TH1D("SMMaxNumCells", "Supermodule with largest amount of cells", 20, -0.5, 19.5);
  mCellsMaxSM->GetXaxis()->SetTitle("Supermodule");
  mCellsMaxSM->GetYaxis()->SetTitle("counts");
  getObjectsManager()->startPublishing(mCellsMaxSM);

  mCells_ev_sm = new TH2D("ncellsPerEventSupermodule", "# of Cells per Events vs supermodule ID", 40, 0, 40, 20, -0.5, 19.5);
  mCells_ev_sm->GetYaxis()->SetTitle("Supermodule");
  mCells_ev_sm->GetXaxis()->SetTitle("ncellsperEvent");
  mCells_ev_sm->SetStats(0);
  getObjectsManager()->startPublishing(mCells_ev_sm);

  mCells_ev_smThr = new TH2D("ncellsPerEventSupermoduleWThr", "# of Cells per Events vs supermodule ID Threshold", 20, 0, 20, 20, -0.5, 19.5);
  mCells_ev_smThr->GetYaxis()->SetTitle("Supermodule");
  mCells_ev_smThr->GetXaxis()->SetTitle("ncellsperEvent");
  mCells_ev_smThr->SetStats(0);
  getObjectsManager()->startPublishing(mCells_ev_smThr);

  mCells_ev = new TH1D("ncellsPerEventTot", "# of Cells per event", 300, 0, 300);
  mCells_ev->GetXaxis()->SetTitle("ncellsperEvent");
  mCells_ev->SetStats(0);
  getObjectsManager()->startPublishing(mCells_ev);

  mCells_ev_Thres = new TH1D("ncellPerEventTot_Thres", "# of Cella per event Thres", 100, 0, 100);
  mCells_ev_Thres->SetStats(0);
  mCells_ev_Thres->GetXaxis()->SetTitle("ncellsperEvent");
  getObjectsManager()->startPublishing(mCells_ev_Thres);

  mCells_ev_EMCAL = new TH1D("ncellsPerEventEMCALTot", "# of Cells per events EMCAL", 300, 0, 300);
  mCells_ev_EMCAL->GetXaxis()->SetTitle("ncellsperEvent");
  mCells_ev_EMCAL->SetStats(0);
  getObjectsManager()->startPublishing(mCells_ev_EMCAL);

  mCells_ev_EMCAL_Thres = new TH1D("ncellPerEventEMCALTot_Thres", "# of Cells per event EMCAL Thres", 100, 0, 100);
  mCells_ev_EMCAL_Thres->GetXaxis()->SetTitle("ncellsperEvent");
  mCells_ev_EMCAL_Thres->SetStats(0);
  getObjectsManager()->startPublishing(mCells_ev_EMCAL_Thres);

  mCells_ev_DCAL = new TH1D("ncellsPerEventDCALTot", "# of Cells per event DCAL", 300, 0, 300);
  mCells_ev_DCAL->GetXaxis()->SetTitle("ncellsperEvent");
  mCells_ev_DCAL->SetStats(0);
  getObjectsManager()->startPublishing(mCells_ev_DCAL);

  mCells_ev_DCAL_Thres = new TH1D("ncellPerEventDCALTot_Thres", "# of Cells per event DCAL Thres", 100, 0, 100);
  mCells_ev_DCAL_Thres->GetXaxis()->SetTitle("ncellsperEvent");
  mCells_ev_DCAL_Thres->SetStats(0);
  getObjectsManager()->startPublishing(mCells_ev_DCAL_Thres);

  mFracGoodCellsEvent = new TH2D("fractionGoodCellsEvent", "Fraction of good cells / event", 3, -0.5, 2.5, 11, 0., 1.1);
  mFracGoodCellsEvent->GetXaxis()->SetBinLabel(1, "All");
  mFracGoodCellsEvent->GetXaxis()->SetBinLabel(2, "EMCAL");
  mFracGoodCellsEvent->GetXaxis()->SetBinLabel(3, "DCAL");
  mFracGoodCellsEvent->GetYaxis()->SetTitle("Fraction good");
  mFracGoodCellsEvent->SetStats(0);
  getObjectsManager()->startPublishing(mFracGoodCellsEvent);

  mFracGoodCellsSM = new TH2D("fractionGoodCellsSupermodule", "Fraction of good cells / supermodule", 20, -0.5, 19.5, 11, 0., 1.1);
  mFracGoodCellsSM->GetXaxis()->SetTitle("Supermodule ID");
  mFracGoodCellsSM->GetYaxis()->SetTitle("Fraction good");
  mFracGoodCellsSM->SetStats(0);
  getObjectsManager()->startPublishing(mFracGoodCellsSM);
}

void CellTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  reset();
}

void CellTask::startOfCycle()
{
  mTimeFramesPerCycles = 0;
  ILOG(Debug, Support) << "startOfCycle" << ENDM;
  std::map<std::string, std::string> metadata;
  mBadChannelMap = retrieveConditionAny<o2::emcal::BadChannelMap>(o2::emcal::CalibDB::getCDBPathBadChannelMap(), metadata);
  // it was EMC/BadChannelMap
  if (!mBadChannelMap)
    ILOG(Info, Support) << "No Bad Channel Map object " << ENDM;

  mTimeCalib = retrieveConditionAny<o2::emcal::TimeCalibrationParams>(o2::emcal::CalibDB::getCDBPathTimeCalibrationParams(), metadata);
  //"EMC/TimeCalibrationParams
  if (!mTimeCalib)
    ILOG(Info, Support) << " No Time Calib object " << ENDM;
}

void CellTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mTFPerCycles->Fill(1); // number of timeframe process per cycle
  mTimeFramesPerCycles++;
  // check if we have payoad
  using MaskType_t = o2::emcal::BadChannelMap::MaskType_t;

  // Handling of inputs from multiple subevents (multiple FLPs)
  // Build maps of trigger records and cells according to the subspecification
  // and combine trigger records from different maps into a single map of range
  // references and subspecifications
  std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::Cell>> cellSubEvents;
  std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::TriggerRecord>> triggerRecordSubevents;

  auto posCells = ctx.inputs().getPos("emcal-cells"),
       posTriggerRecords = ctx.inputs().getPos("emcal-triggerecords");
  auto numSlotsCells = ctx.inputs().getNofParts(posCells),
       numSlotsTriggerRecords = ctx.inputs().getNofParts(posTriggerRecords);
  for (decltype(numSlotsCells) islot = 0; islot < numSlotsCells; islot++) {
    auto celldata = ctx.inputs().getByPos(posCells, islot);
    auto subspecification = framework::DataRefUtils::getHeader<header::DataHeader*>(celldata)->subSpecification;
    // Discard message if it is a deadbeaf message (empty timeframe)
    if (subspecification == 0xDEADBEEF) {
      continue;
    }
    cellSubEvents[subspecification] = ctx.inputs().get<gsl::span<o2::emcal::Cell>>(celldata);
  }
  for (decltype(numSlotsTriggerRecords) islot = 0; islot < numSlotsTriggerRecords; islot++) {
    auto trgrecorddata = ctx.inputs().getByPos(posTriggerRecords, islot);
    auto subspecification = framework::DataRefUtils::getHeader<header::DataHeader*>(trgrecorddata)->subSpecification;
    // Discard message if it is a deadbeaf message (empty timeframe)
    if (subspecification == 0xDEADBEEF) {
      continue;
    }
    triggerRecordSubevents[subspecification] = ctx.inputs().get<gsl::span<o2::emcal::TriggerRecord>>(trgrecorddata);
  }

  auto combinedEvents = buildCombinedEvents(triggerRecordSubevents);

  //  ILOG(Info, Support) <<"Received " << cellcontainer.size() << " cells " << ENDM;
  int eventcounter = 0;
  int eventcounterCALIB = 0;
  int eventcounterPHYS = 0;
  std::array<int, 20> numCellsSM;
  std::array<int, 20> numCellsSM_Thres;
  std::array<int, 20> numCellsGood;
  std::array<int, 20> numCellsBad;
  std::fill(numCellsSM.begin(), numCellsSM.end(), 0);
  std::fill(numCellsSM_Thres.begin(), numCellsSM_Thres.end(), 0);
  for (auto trg : combinedEvents) {
    if (!trg.getNumberOfObjects()) {
      continue;
    }
    ILOG(Debug, Support) << "Next event " << eventcounter << " has " << trg.getNumberOfObjects() << " cells from " << trg.getNumberOfSubevents() << " subevent(s)" << ENDM;

    // trigger type
    auto triggertype = trg.mTriggerType;
    bool isPhysTrigger = mIgnoreTriggerTypes || (triggertype & o2::trigger::PhT),
         isCalibTrigger = (!mIgnoreTriggerTypes) && (triggertype & o2::trigger::Cal);
    std::string trgClass;
    if (isPhysTrigger) {
      trgClass = "PHYS";
      eventcounterPHYS++;
    } else if (isCalibTrigger) {
      trgClass = "CAL";
      eventcounterCALIB++;
    } else {
      ILOG(Error, Support) << " Unmonitored trigger class requested " << ENDM;
      continue;
    }

    auto bcphase = trg.mInteractionRecord.bc % 4; // to be fixed:4 histos for EMCAL, 4 histos for DCAL
    auto histos = mHistogramContainer[trgClass];
    std::fill(numCellsSM.begin(), numCellsSM.end(), 0);
    std::fill(numCellsSM_Thres.begin(), numCellsSM_Thres.end(), 0);
    std::fill(numCellsGood.begin(), numCellsGood.end(), 0);
    std::fill(numCellsBad.begin(), numCellsBad.end(), 0);

    // iterate over subevents
    for (auto& subev : trg.mSubevents) {
      auto cellsSubspec = cellSubEvents.find(subev.mSpecification);
      if (cellsSubspec == cellSubEvents.end()) {
        ILOG(Error, Support) << "No cell data found for subspecification " << subev.mSpecification << ENDM;
      } else {
        ILOG(Debug, Support) << subev.mCellRange.getEntries() << " cells in subevent from equipment " << subev.mSpecification << ENDM;
        gsl::span<const o2::emcal::Cell> eventcells(cellsSubspec->second.data() + subev.mCellRange.getFirstEntry(), subev.mCellRange.getEntries());
        for (auto cell : eventcells) {
          // int index = cell.getHighGain() ? 0 : (cell.getLowGain() ? 1 : -1);
          int index = cell.getHighGain() ? 0 : 1;
          auto timeoffset = mTimeCalib ? mTimeCalib->getTimeCalibParam(cell.getTower(), cell.getLowGain()) : 0.;
          bool goodcell = true;
          if (mBadChannelMap) {
            goodcell = mBadChannelMap->getChannelStatus(cell.getTower()) != MaskType_t::GOOD_CELL;
          }
          histos.fillHistograms(cell, goodcell, timeoffset, bcphase);
          if (isPhysTrigger) {
            auto [sm, mod, iphi, ieta] = mGeometry->GetCellIndex(cell.getTower());
            numCellsSM[sm]++;
            if (cell.getEnergy() > mTaskSettings.mThresholdPHYS)
              numCellsSM_Thres[sm]++;
            if (goodcell) {
              numCellsGood[sm]++;
            } else {
              numCellsBad[sm]++;
            }
          }
        }
      }
    }
    histos.countEvent();

    if (isPhysTrigger) {
      auto maxSM = std::max_element(numCellsSM.begin(), numCellsSM.end());
      auto indexMaxSM = maxSM - numCellsSM.begin();
      mCellsMaxSM->Fill(indexMaxSM);
      // fill histo 1)
      int mCell_all = 0, mCell_EMCAL = 0, mCell_DCAL = 0;
      int mCell_all_Thres = 0, mCell_EMCAL_Thres = 0, mCell_DCAL_Thres = 0;
      // make statistics good cells
      int nGoodAll = 0, nGoodEMCAL = 0, nGoodDCAL = 0, nBadAll = 0, nBadEMCAL = 0, nBadDCAL = 0;
      for (int ism = 0; ism < 20; ism++) {
        mCells_ev_sm->Fill(numCellsSM[ism], ism);          // for experts
        mCells_ev_smThr->Fill(numCellsSM_Thres[ism], ism); // for experts

        mCell_all += numCellsSM[ism];
        mCell_all_Thres += numCellsSM_Thres[ism];
        nGoodAll += numCellsGood[ism];
        nBadAll += numCellsBad[ism];
        if (ism < 12) {
          mCell_EMCAL += numCellsSM[ism];
          mCell_EMCAL_Thres += numCellsSM_Thres[ism];
          nGoodEMCAL += numCellsGood[ism];
          nBadEMCAL += numCellsBad[ism];
        } else {
          mCell_DCAL += numCellsSM[ism];
          mCell_DCAL_Thres += numCellsSM_Thres[ism];
          nGoodDCAL += numCellsGood[ism];
          nBadDCAL += numCellsBad[ism];
        }
        if (numCellsGood[ism] + numCellsBad[ism]) {
          mFracGoodCellsSM->Fill(ism, static_cast<double>(numCellsGood[ism]) / static_cast<double>(numCellsGood[ism] + numCellsBad[ism]));
        }
      }
      mCells_ev->Fill(mCell_all);
      mCells_ev_EMCAL->Fill(mCell_EMCAL);
      mCells_ev_DCAL->Fill(mCell_DCAL);

      mCells_ev_Thres->Fill(mCell_all_Thres);
      mCells_ev_EMCAL_Thres->Fill(mCell_EMCAL_Thres);
      mCells_ev_DCAL_Thres->Fill(mCell_DCAL_Thres);

      if (nGoodAll + nBadAll) {
        mFracGoodCellsEvent->Fill(0., static_cast<double>(nGoodAll) / static_cast<double>(nGoodAll + nBadAll));
      }
      if (nGoodEMCAL + nBadEMCAL) {
        mFracGoodCellsEvent->Fill(1., static_cast<double>(nGoodEMCAL) / static_cast<double>(nGoodEMCAL + nBadEMCAL));
      }
      if (nGoodDCAL + nBadDCAL) {
        mFracGoodCellsEvent->Fill(2., static_cast<double>(nGoodDCAL) / static_cast<double>(nGoodDCAL + nBadDCAL));
      }
    }

    eventcounter++;
  }
  mEvCounterTF->Fill(eventcounter);
  mEvCounterTFPHYS->Fill(eventcounterPHYS);
  mEvCounterTFCALIB->Fill(eventcounterCALIB);
  // event counter per TimeFrame  (range 0-100) for the moment (parameter)
}

void CellTask::endOfCycle()
{
  mTFPerCyclesTOT->Fill(mTimeFramesPerCycles); // do not reset this histo
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void CellTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Debug, Support) << "endOfActivity" << ENDM;
}

void CellTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Support) << "Resetting the histogram" << ENDM;
  for (auto cont : mHistogramContainer) {
    cont.second.reset();
  }
  if (mEvCounterTF)
    mEvCounterTF->Reset();
  if (mEvCounterTFPHYS)
    mEvCounterTFPHYS->Reset();
  if (mEvCounterTFCALIB)
    mEvCounterTFCALIB->Reset();
  if (mTFPerCycles)
    mTFPerCycles->Reset();
  if (mCellsMaxSM)
    mCellsMaxSM->Reset();
  if (mCells_ev_sm)
    mCells_ev_sm->Reset();
  if (mCells_ev_smThr)
    mCells_ev_smThr->Reset();
  if (mCells_ev)
    mCells_ev->Reset();
  if (mCells_ev_Thres)
    mCells_ev_Thres->Reset();
  if (mCells_ev_EMCAL)
    mCells_ev_EMCAL->Reset();
  if (mCells_ev_EMCAL_Thres)
    mCells_ev_EMCAL_Thres->Reset();
  if (mCells_ev_DCAL)
    mCells_ev_DCAL->Reset();
  if (mCells_ev_DCAL_Thres)
    mCells_ev_DCAL_Thres->Reset();
}

std::vector<CellTask::CombinedEvent> CellTask::buildCombinedEvents(const std::unordered_map<header::DataHeader::SubSpecificationType, gsl::span<const o2::emcal::TriggerRecord>>& triggerrecords) const
{
  std::vector<CellTask::CombinedEvent> events;

  // Search interaction records from all subevents
  std::set<o2::InteractionRecord> allInteractions;
  for (auto& [subspecification, trgrec] : triggerrecords) {
    for (auto rec : trgrec) {
      auto eventIR = rec.getBCData();
      if (allInteractions.find(eventIR) == allInteractions.end())
        allInteractions.insert(eventIR);
    }
  }

  // iterate over all subevents for all bunch crossings
  for (auto collision : allInteractions) {
    CombinedEvent nextevent;
    nextevent.mInteractionRecord = collision;
    bool first = true,
         hasSubevent = false;
    for (auto [subspecification, records] : triggerrecords) {
      auto found = std::find_if(records.begin(), records.end(), [&collision](const o2::emcal::TriggerRecord& rec) { return rec.getBCData() == collision; });
      if (found != records.end()) {
        hasSubevent = true;
        if (first) {
          nextevent.mTriggerType = found->getTriggerBits();
          first = false;
        }
        nextevent.mSubevents.push_back({ subspecification, o2::dataformats::RangeReference(found->getFirstEntry(), found->getNumberOfObjects()) });
      }
    }
    if (hasSubevent)
      events.emplace_back(nextevent);
  }
  return events;
}

bool CellTask::hasConfigValue(const std::string_view key)
{
  if (auto param = mCustomParameters.find(key.data()); param != mCustomParameters.end()) {
    return true;
  }
  return false;
}

std::string CellTask::getConfigValue(const std::string_view key)
{
  std::string result;
  if (auto param = mCustomParameters.find(key.data()); param != mCustomParameters.end()) {
    result = param->second;
  }
  return result;
}

std::string CellTask::getConfigValueLower(const std::string_view key)
{
  auto input = getConfigValue(key);
  std::string result;
  if (input.length()) {
    result = boost::algorithm::to_lower_copy(input);
  }
  return result;
}

void CellTask::CellHistograms::initForTrigger(const std::string trigger, const TaskSettings& settings)
{ // hasAmpVsCellID, bool hasTimeVsCellID, bool hasHistosCalib2D)
  if (trigger == "PYHS") {
    mCellThreshold = settings.mThresholdPHYS;
    mAmplitudeThresholdTime = settings.mAmpThresholdTimePhys;
  } else {
    mCellThreshold = settings.mThresholdCAL;
    mAmplitudeThresholdTime = settings.mAmpThresholdTimeCalib;
  }
  auto histBuilder1D = [trigger](const std::string name, const std::string title, int nbinsx, double xmin, double xmax) -> TH1* {
    std::string histname = name + "_" + trigger,
                histtitle = title + " " + trigger;
    return new TH1D(histname.data(), histtitle.data(), nbinsx, xmin, xmax);
  };
  auto histBuilder2D = [trigger](const std::string_view name, const std::string_view title, int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double ymax, bool profile) -> TH2* {
    std::string histname = std::string(name.data()) + "_" + trigger,
                histtitle = std::string(title.data()) + " " + trigger;
    if (profile)
      return new TProfile2D(histname.data(), histtitle.data(), nbinsx, xmin, xmax, nbinsy, ymin, ymax);
    return new TH2D(histname.data(), histtitle.data(), nbinsx, xmin, xmax, nbinsy, ymin, ymax);
  };

  std::map<std::string, double> maxAmps = { { "PHYS", 50. }, { "CAL", 10. } };
  double maxAmp = maxAmps[trigger];

  bool isPhysTrigger = trigger == "PHYS";
  if (isPhysTrigger) {
    if (settings.mHasAmpVsCellID) {
      mCellAmplitude = histBuilder2D("cellAmplitudeHG", "Cell Amplitude (High gain)", 80, 0, 16, 17664, -0.5, 17663.5, false);
      mCellAmplitude->SetStats(0);
      // mCellAmplitude[1] = histBuilder2D("cellAmplitudeLG", "Cell Amplitude (Low gain)", 100, 0, 100, 17664, -0.5, 17663.5, false);
      if (settings.mHasHistosCalib2D) {
        mCellAmplitudeCalib = histBuilder2D("cellAmplitudeHGCalib", "Cell Amplitude (High gain)", 80, 0, 16, 17664, -0.5, 17663.5, false);
        mCellAmplitudeCalib->SetStats(0);
        // mCellAmplitudeCalib[1] = histBuilder2D("cellAmplitudeLGCalib", "Cell Amplitude (Low gain)", 100, 0, 100, 17664, -0.5, 17663.5, false);
      }
    }
    if (settings.mHasTimeVsCellID) {
      mCellTime = histBuilder2D("cellTimeHG", "Cell Time (High gain)", 400, -200, 200, 17664, -0.5, 17663.5, false); // all cell time histo: 2D? both for CAL and Phys? CRI
      mCellTime->SetStats(0);
      // mCellTime[1] = histBuilder2D("cellTimeLG", "Cell Time (Low gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      if (settings.mHasHistosCalib2D) {
        mCellTimeCalib = histBuilder2D("cellTimeHGCalib", "Cell Time Calib (High gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
        mCellTimeCalib->SetStats(0);
        // mCellTimeCalib[1] = histBuilder2D("cellTimeLGCalib", "Cell Time Calib (Low gain)", 400, -200, 200, 17664, -0.5, 17663.5, false);
      }
    }

    mCellAmpSupermodule = histBuilder2D("cellAmplitudeSupermodule", "Cell amplitude vs. supermodule ID ", 4 * static_cast<int>(maxAmp), 0., maxAmp, 20, -0.5, 19.5, false);
    mCellAmpSupermodule->SetStats(0);
    mCellTimeSupermodule = histBuilder2D("cellTimeSupermodule", "Cell Time vs. supermodule ID ", 600, -400, 800, 20, -0.5, 19.5, false);
    mCellTimeSupermodule->SetStats(0);

    if (settings.mHasHistosCalib2D) {
      mCellAmpSupermoduleCalib = histBuilder2D("cellAmplitudeSupermoduleCalib", "Cell amplitude (Calib) vs. supermodule ID ", 4 * static_cast<int>(maxAmp), 0., maxAmp, 20, -0.5, 19.5, false);
      mCellAmpSupermoduleCalib->SetStats(0);
      mCellTimeSupermoduleCalib = histBuilder2D("cellTimeSupermoduleCalib", "Cell Time (Calib) vs. supermodule ID (High gain)", 600, -400, 800, 20, -0.5, 19.5, false);
      mCellTimeSupermoduleCalib->SetStats(0);
      mCellAmpSupermoduleBad = histBuilder2D("cellAmplitudeSupermoduleBad", "Cell amplitude (bad cells) vs. supermodule ID", 4 * static_cast<int>(maxAmp), 0., maxAmp, 20, -0.5, 19.5, false);
      mCellAmpSupermoduleBad->SetStats(0);
      mCellOccupancyGood = histBuilder2D("cellOccupancyGood", "Cell occupancy good cells", 96, -0.5, 95.5, 208, -0.5, 207.5, false);
      mCellOccupancyGood->SetStats(0);
      mCellOccupancyBad = histBuilder2D("cellOccupancyBad", "Cell occupancy bad cells", 96, -0.5, 95.5, 208, -0.5, 207.5, false);
      mCellOccupancyBad->SetStats(0);
    }
  }

  mCellOccupancy = histBuilder2D("cellOccupancyEMC", "Cell Occupancy EMCAL", 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mCellOccupancy->SetStats(0);
  mCellOccupancyThr = histBuilder2D("cellOccupancyEMCwThr", Form("Cell Occupancy EMCAL,DCAL with E>%.1f GeV/c", mCellThreshold), 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mCellOccupancyThr->SetStats(0);
  mCellOccupancyThrBelow = histBuilder2D("cellOccupancyEMCwThrBelow", Form("Cell Occupancy EMCAL,DCAL with E<%.1f GeV/c", mCellThreshold), 96, -0.5, 95.5, 208, -0.5, 207.5, false);
  mCellOccupancyThrBelow->SetStats(0);

  mIntegratedOccupancy = histBuilder2D("cellOccupancyInt", "Cell Occupancy Integrated", 96, -0.5, 95.5, 208, -0.5, 207.5, true);
  mIntegratedOccupancy->GetXaxis()->SetTitle("col");
  mIntegratedOccupancy->GetYaxis()->SetTitle("row");
  mIntegratedOccupancy->SetStats(0);
  // 1D histograms for showing the integrated spectrum

  mCellTimeSupermodule_tot = histBuilder1D("cellTime", "Cell Time EMCAL,DCAL", 600, -400, 800);
  mCellTimeSupermoduleEMCAL = histBuilder1D("cellTimeEMCAL", "Cell Time EMCAL", 600, -400, 800);
  mCellTimeSupermoduleDCAL = histBuilder1D("cellTimeDCAL", "Cell Time DCAL", 600, -400, 800);
  mCellTimeSupermoduleEMCAL_Gain[0] = histBuilder1D("cellTimeEMCAL_highGain", "Cell Time EMCAL highGain", 600, -400, 800);
  mCellTimeSupermoduleEMCAL_Gain[1] = histBuilder1D("cellTimeEMCAL_lowGain", "Cell Time EMCAL lowGain", 600, -400, 800);
  mCellTimeSupermoduleDCAL_Gain[0] = histBuilder1D("cellTimeDCAL_highGain", "Cell Time DCAL highGain", 600, -400, 800);
  mCellTimeSupermoduleDCAL_Gain[1] = histBuilder1D("cellTimeDCAL_lowGain", "Cell Time DCAL lowGain", 600, -400, 800);
  mCellAmplitude_tot = histBuilder1D("cellAmplitude", "Cell amplitude in EMCAL,DCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
  mCellAmplitudeEMCAL = histBuilder1D("cellAmplitudeEMCAL", "Cell amplitude in EMCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
  mCellAmplitudeDCAL = histBuilder1D("cellAmplitudeDCAL", "Cell amplitude in DCAL", 4 * static_cast<int>(maxAmp), 0., maxAmp);
  mnumberEvents = histBuilder1D("NumberOfEvents", "Number Of Events", 1, 0.5, 1.5);
  if (isPhysTrigger) {
    // Phys. trigger: monitor all bunch crossings
    for (auto bcID = 0; bcID < 4; bcID++) {
      std::array<TH1*, 20> cellTimeBCSM;
      for (auto smID = 0; smID < 20; smID++) {
        cellTimeBCSM[smID] = histBuilder1D(Form("cellTimeBC%dSM%d", bcID, smID), Form("Cell Time BC%d SM%d", bcID, smID), 600, -400, 800);
      }
      mCellTimeBC[bcID] = cellTimeBCSM;
    }
  } else {
    // Calib trigger: Only bc0;
    std::array<TH1*, 20> cellTimeBCSM;
    for (auto smID = 0; smID < 20; smID++) {
      cellTimeBCSM[smID] = histBuilder1D(Form("cellTimeBC0SM%d", smID), Form("Cell Time BC0 SM%d", smID), 600, -400, 800);
    }
    mCellTimeBC[0] = cellTimeBCSM;
  }
}

void CellTask::CellHistograms::fillHistograms(const o2::emcal::Cell& cell, bool goodCell, double timecalib, int bcphase)
{
  auto fillOptional1D = [](TH1* hist, double x, double weight = 1.) {
    if (hist)
      hist->Fill(x, weight);
  };
  auto fillOptional2D = [](TH2* hist, double x, double y, double weight = 1.) {
    if (hist)
      hist->Fill(x, y, weight);
  };

  fillOptional2D(mCellAmplitude, cell.getEnergy(), cell.getTower());
  // fillOptional2D(mCellAmplitude[index], cell.getEnergy(), cell.getTower());

  if (goodCell) {
    fillOptional2D(mCellAmplitudeCalib, cell.getEnergy(), cell.getTower());
    fillOptional2D(mCellTimeCalib, cell.getTimeStamp() - timecalib, cell.getTower());
    // fillOptional2D(mCellAmplitudeCalib[index], cell.getEnergy(), cell.getTower());
    // fillOptional2D(mCellTimeCalib[index], cell.getTimeStamp() - timeoffset, cell.getTower());
  }
  fillOptional2D(mCellTime, cell.getTimeStamp(), cell.getTower());
  // fillOptional2D(mCellTime[index], cell.getTimeStamp(), cell.getTower());

  try {
    auto [row, col] = mGeometry->GlobalRowColFromIndex(cell.getTower());
    if (cell.getEnergy() > 0) {
      fillOptional2D(mCellOccupancy, col, row);
    }
    if (cell.getEnergy() > mCellThreshold) {
      fillOptional2D(mCellOccupancyThr, col, row);
    } else {
      fillOptional2D(mCellOccupancyThrBelow, col, row);
    }
    if (goodCell) {
      fillOptional2D(mCellOccupancyGood, col, row);
    } else {
      fillOptional2D(mCellOccupancyBad, col, row);
    }

    fillOptional2D(mIntegratedOccupancy, col, row, cell.getEnergy());

  } catch (o2::emcal::InvalidCellIDException& e) {
    ILOG(Error, Support) << "Invalid cell ID: " << e.getCellID() << ENDM;
  };

  try {
    auto cellindices = mGeometry->GetCellIndex(cell.getTower());
    auto supermoduleID = std::get<0>(cellindices);
    fillOptional2D(mCellAmpSupermodule, cell.getEnergy(), supermoduleID);
    if (cell.getEnergy() > mAmplitudeThresholdTime)
      fillOptional2D(mCellTimeSupermodule, cell.getTimeStamp(), supermoduleID);
    if (goodCell) {
      fillOptional2D(mCellAmpSupermoduleCalib, cell.getEnergy(), supermoduleID);
      if (cell.getEnergy() > mAmplitudeThresholdTime)
        fillOptional2D(mCellTimeSupermoduleCalib, cell.getTimeStamp() - timecalib, supermoduleID);
    } else {
      fillOptional2D(mCellAmpSupermoduleBad, cell.getEnergy(), supermoduleID);
    }
    fillOptional1D(mCellAmplitude_tot, cell.getEnergy()); // EMCAL+DCAL, shifter
    if (cell.getEnergy() > mAmplitudeThresholdTime)
      fillOptional1D(mCellTimeSupermodule_tot, cell.getTimeStamp()); // EMCAL+DCAL shifter
    // check Gain
    int index = cell.getHighGain() ? 0 : 1; //(0=highGain, 1 = lowGain)
    if (supermoduleID < 12) {               // EMCAL
      if (cell.getEnergy() > mAmplitudeThresholdTime) {
        fillOptional1D(mCellTimeSupermoduleEMCAL, cell.getTimeStamp());
        fillOptional1D(mCellTimeSupermoduleEMCAL_Gain[index], cell.getTimeStamp());
      }
      fillOptional1D(mCellAmplitudeEMCAL, cell.getEnergy());
    } else {
      fillOptional1D(mCellAmplitudeDCAL, cell.getEnergy());
      if (cell.getEnergy() > mAmplitudeThresholdTime) {
        fillOptional1D(mCellTimeSupermoduleDCAL, cell.getTimeStamp());
        fillOptional1D(mCellTimeSupermoduleDCAL_Gain[index], cell.getTimeStamp());
      }
    }
    // bc phase histograms
    if (cell.getEnergy() > mAmplitudeThresholdTime) {
      auto bchistos = mCellTimeBC.find(bcphase);
      if (bchistos != mCellTimeBC.end()) {
        auto histcontainer = bchistos->second;
        histcontainer[supermoduleID]->Fill(cell.getTimeStamp());
      }
    }
  } catch (o2::emcal::InvalidCellIDException& e) {
    ILOG(Info, Support) << "Invalid cell ID: " << e.getCellID() << ENDM;
  }
}

void CellTask::CellHistograms::countEvent()
{
  mnumberEvents->Fill(1);
}

void CellTask::CellHistograms::startPublishing(o2::quality_control::core::ObjectsManager& manager)
{
  auto publishOptional = [&manager](TH1* hist) {
    if (hist)
      manager.startPublishing(hist);
  };

  publishOptional(mCellTime);
  publishOptional(mCellTimeCalib);
  publishOptional(mCellAmplitude);
  publishOptional(mCellAmplitudeCalib);
  publishOptional(mCellAmpSupermodule);
  publishOptional(mCellAmpSupermoduleCalib);
  publishOptional(mCellAmpSupermoduleBad);
  publishOptional(mCellTimeSupermodule);
  publishOptional(mCellTimeSupermodule_tot);
  publishOptional(mCellTimeSupermoduleEMCAL);
  publishOptional(mCellTimeSupermoduleDCAL);
  publishOptional(mCellTimeSupermoduleCalib);
  publishOptional(mCellAmplitude_tot);
  publishOptional(mCellAmplitudeEMCAL);
  publishOptional(mCellAmplitudeDCAL);
  publishOptional(mCellOccupancy);
  publishOptional(mCellOccupancyThr);
  publishOptional(mCellOccupancyThrBelow);
  publishOptional(mCellOccupancyGood);
  publishOptional(mCellOccupancyBad);
  publishOptional(mIntegratedOccupancy);
  publishOptional(mnumberEvents);

  for (auto histos : mCellTimeSupermoduleEMCAL_Gain) {
    publishOptional(histos);
  }
  for (auto histos : mCellTimeSupermoduleDCAL_Gain) {
    publishOptional(histos);
  }

  for (auto& [bcID, histos] : mCellTimeBC) {
    for (auto hist : histos)
      publishOptional(hist);
  }
  //  publishOptional(mEvCounterTF);
  //  publishOptional(mEvCounterTFPHYS);
  //  publishOptional(mEvCounterTFCALIB);
  //  publishOptional(mTFPerCycles);
}

void CellTask::CellHistograms::reset()
{

  //  for (auto h : mCellAmplitude) {
  //    h->Reset();
  //  }
  //  for (auto h : mCellTime) {
  //    h->Reset();
  //  }
  //  for (auto h : mCellAmplitudeCalib) {
  //    h->Reset();
  //  }
  //  for (auto h : mCellTimeCalib) {
  //    h->Reset();
  //  }

  auto resetOptional = [](TH1* hist) {
    if (hist)
      hist->Reset();
  };

  resetOptional(mCellTime);
  resetOptional(mCellTimeCalib);
  resetOptional(mCellAmplitude);
  resetOptional(mCellAmplitudeCalib);
  resetOptional(mCellAmpSupermodule);
  resetOptional(mCellAmpSupermoduleCalib);
  resetOptional(mCellAmpSupermoduleBad);
  resetOptional(mCellTimeSupermodule);
  resetOptional(mCellTimeSupermodule_tot);
  resetOptional(mCellTimeSupermoduleEMCAL);
  resetOptional(mCellTimeSupermoduleDCAL);
  resetOptional(mCellTimeSupermoduleCalib);
  resetOptional(mCellAmplitude_tot);
  resetOptional(mCellAmplitudeEMCAL);
  resetOptional(mCellAmplitudeDCAL);
  resetOptional(mCellOccupancy);
  resetOptional(mCellOccupancyThr);
  resetOptional(mCellOccupancyThrBelow);
  resetOptional(mCellOccupancyGood);
  resetOptional(mCellOccupancyBad);
  resetOptional(mIntegratedOccupancy);
  resetOptional(mnumberEvents);

  for (auto histos : mCellTimeSupermoduleEMCAL_Gain) {
    resetOptional(histos);
  }
  for (auto histos : mCellTimeSupermoduleDCAL_Gain) {
    resetOptional(histos);
  }

  for (auto& [bcID, histos] : mCellTimeBC) {
    for (auto hist : histos)
      resetOptional(hist);
  }
  //  resetOptional(mEvCounterTF);
  //  resetOptional(mEvCounterTFPHYS);
  //  resetOptional(mEvCounterTFCALIB);
  //  resetOptional(mTFPerCycles);
}

void CellTask::CellHistograms::clean()
{
  // for (auto h : mCellAmplitude) {
  //   delete h;
  // }
  //  for (auto h : mCellTime) {
  //   delete h;
  //  }
  // for (auto h : mCellAmplitudeCalib) {
  //   delete h;
  // }
  // for (auto h : mCellTimeCalib) {
  //   delete h;
  //  }

  auto cleanOptional = [](TObject* hist) {
    if (hist)
      delete hist;
  };

  cleanOptional(mCellTime);
  cleanOptional(mCellTimeCalib);
  cleanOptional(mCellAmplitude);
  cleanOptional(mCellAmplitudeCalib);
  cleanOptional(mCellAmpSupermodule);
  cleanOptional(mCellAmpSupermoduleCalib);
  cleanOptional(mCellAmpSupermoduleBad);
  cleanOptional(mCellTimeSupermodule);
  cleanOptional(mCellTimeSupermodule_tot);
  cleanOptional(mCellTimeSupermoduleEMCAL);
  cleanOptional(mCellTimeSupermoduleDCAL);
  cleanOptional(mCellTimeSupermoduleCalib);
  cleanOptional(mCellAmplitude_tot);
  cleanOptional(mCellAmplitudeEMCAL);
  cleanOptional(mCellAmplitudeDCAL);
  cleanOptional(mCellOccupancy);
  cleanOptional(mCellOccupancyThr);
  cleanOptional(mCellOccupancyThrBelow);
  cleanOptional(mCellOccupancyGood);
  cleanOptional(mCellOccupancyBad);
  cleanOptional(mIntegratedOccupancy);
  cleanOptional(mnumberEvents);

  for (auto histos : mCellTimeSupermoduleEMCAL_Gain) {
    cleanOptional(histos);
  }
  for (auto histos : mCellTimeSupermoduleDCAL_Gain) {
    cleanOptional(histos);
  }
  for (auto& [bcID, histos] : mCellTimeBC) {
    for (auto hist : histos)
      cleanOptional(hist);
  }
  //  cleanOptional(mEvCounterTF);
  //  cleanOptional(mEvCounterTFPHYS);
  //  cleanOptional(mEvCounterTFCALIB);
  //  cleanOptional(mTFPerCycles);
  //
}

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2
