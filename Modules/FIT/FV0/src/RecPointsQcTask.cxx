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
/// \file   RecPointsQcTask.cxx
/// \author Andreas Molander <andreas.molander@cern.ch>, Fabiola Lugo <fabiola.viridiana.lugo@cern.ch>
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TROOT.h>

#include "QualityControl/QcInfoLogger.h"
#include "FV0/RecPointsQcTask.h"
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>

#include "DataFormatsFV0/RecPoints.h"

namespace o2::quality_control_modules::fv0
{

RecPointsQcTask::~RecPointsQcTask()
{
  // delete mHistogram;
}

void RecPointsQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize RecPointsQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

<<<<<<< Updated upstream
  mHistColTime = std::make_unique<TH1F>("Time", "FV0 collision time;Time (ps)", 500, -2050, 2050);
  mHistColFirstTime = std::make_unique<TH1F>("Time", "FV0 collision first time;Time (ps)", 500, -2050, 2050);
  mHistColGloMeanTime = std::make_unique<TH1F>("Time", "FV0 collision global mean time;Time (ps)", 500, -2050, 2050);
  mHistColSelectedMeanTime = std::make_unique<TH1F>("Time", "FV0 collision selected mean time;Time (ps)", 500, -2050, 2050);
  mHistTime2Ch = std::make_unique<TH2F>("TimePerChannel", "Time vs Channel;Channel;Time (ps)", sNCHANNELS_FV0_PLUSREF, 0, sNCHANNELS_FV0_PLUSREF, 4100, -2050, 2050);
  mHistTime2Ch->SetOption("colz");

  getObjectsManager()->startPublishing(mHistColTime.get());
  getObjectsManager()->startPublishing(mHistColFirstTime.get());
  getObjectsManager()->startPublishing(mHistColGloMeanTime.get());
  getObjectsManager()->startPublishing(mHistColSelectedMeanTime.get());
  getObjectsManager()->startPublishing(mHistTime2Ch.get());
    
    
  // mHistogram = new TH1F("example", "example", 20, 0, 30000);
  // // this will make the two histograms published at the end of each cycle. no need to use the method in monitorData
  // getObjectsManager()->startPublishing(mHistogram);
  // getObjectsManager()->startPublishing(new TH1F("example2", "example2", 20, 0, 30000));
  // try {
  //   getObjectsManager()->addMetadata(mHistogram->GetName(), "custom", "34");
  // } catch (...) {
  //   // some methods can throw exceptions, it is indicated in their doxygen.
  //   // In case it is recoverable, it is recommended to catch them and do something meaningful.
  //   // Here we don't care that the metadata was not added and just log the event.
  //   ILOG(Warning, Support) << "Metadata could not be added to " << mHistogram->GetName() << ENDM;
  // }
=======
  mHistogram = std::make_unique<TH1F>("Time", "FV0 mean time;Time (ps)", 500, -2050, 2050);
  getObjectsManager()->startPublishing(mHistogram.get());
>>>>>>> Stashed changes
}

void RecPointsQcTask::startOfActivity(const Activity& activity)
{
<<<<<<< Updated upstream
  // // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  // ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mHistColTime->Reset();
  mHistColFirstTime->Reset();
  mHistColGloMeanTime->Reset();
  mHistColSelectedMeanTime->Reset();
  mHistTime2Ch->Reset();

  // // Example: retrieve custom parameters
  // std::string parameter;
  // // first we try for the current activity. It returns an optional.
  // if (auto param = mCustomParameters.atOptional("myOwnKey", activity)) {
  //   parameter = param.value(); // we got a value
  // } else {
  //   // we did not get a value. We aDataFormatsFV0/RecPoints.hsk for the "default" runtype and beamtype and we specify a default return value.
  //   parameter = mCustomParameters.atOrDefaultValue("myOwnKey", "some default");
  // }
=======
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  mHistogram->Reset();
>>>>>>> Stashed changes
}

void RecPointsQcTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void RecPointsQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto recpoints = ctx.inputs().get<gsl::span<o2::fv0::RecPoints>>("recpoints");
  auto channels = ctx.inputs().get<gsl::span<o2::fv0::ChannelDataFloat>>("channels");

  for (const auto& recpoint : recpoints) {
    mHistColTime->Fill(recpoint.getCollisionTime());
    mHistColFirstTime->Fill(recpoint.getCollisionFirstTime());
    mHistColGloMeanTime->Fill(recpoint.getCollisionGlobalMeanTime());
    mHistColSelectedMeanTime->Fill(recpoint.getCollisionSelectedMeanTime());
      
    const auto& vecChData = recpoint.getBunchChannelData(channels);
    for (const auto& chData : vecChData) {
        time[chData.channel] = chData.time;
        mHistTime2Ch->Fill(static_cast<Double_t>(chData.channel), static_cast<Double_t>(chData.time));
    }
  }
<<<<<<< Updated upstream
  // // THIS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
    
  // // In this function you can access data inputs specified in the JSON config file, for example:
  // //   "query": "random:ITS/RAWDATA/0"
  // // which is correspondingly <binding>:<dataOrigin>/<dataDescription>/<subSpecification
  // // One can also access conditions from CCDB, via separate API (see point 3)

  // // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // // One can find additional examples at:
  // // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  // // Some examples:

  // // 1. In a loop
  // for (auto&& input : framework::InputRecordWalker(ctx.inputs())) {
  //   // get message header
  //   const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);
  //   auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
  //   // get payload of a specific input, which is a char array.
  //   // const char* payload = input.payload;

  //   // for the sake of an example, let's fill the histogram with payload sizes
  //   mHistogram->Fill(payloadSize);
  // }

  // // 2. Using get("<binding>")

  // // get the payload of a specific input, which is a char array. "random" is the binding specified in the config file.
  // //   auto payload = ctx.inputs().get("random").payload;

  // // get payload of a specific input, which is a structure array:
  // //   auto input = ctx.inputs().get("random");
  // //   auto payload = input.payload;

  // // get payload of a specific input, which is a structure array:
  // //  const auto* header = DataRefUtils::getHeader<header::DataHeader*>(input);
  // //  auto payloadSize = DataRefUtils::getPayloadSize(input);
  // //  struct s {int a; double b;};
  // //  auto array = ctx.inputs().get<s*>(input);
  // //  for (int j = 0; j < payloadSize / sizeof(s); ++j) {
  // //    int i = array.get()[j].a;
  // //  }

  // // get payload of a specific input, which is a root object
  // //   auto h = ctx.inputs().get<TH1F*>("histos");
  // //   Double_t stats[4];
  // //   h->GetStats(stats);
  // //   auto s = ctx.inputs().get<TObjString*>("string");
  // //   LOG(info) << "String is " << s->GetString().Data();

  // // 3. Access CCDB. If it is enough to retrieve it once, do it in initialize().
  // // Remember to delete the object when the pointer goes out of scope or it is no longer needed.
  // //     TObject* condition = TaskInterface::retrieveCondition("GRP/Calib/LHCClockPhase"); // put a valid condition or calibration object path here
  // //     if (condition) {
  // //       LOG(info) << "Retrieved " << condition->ClassName();
  // //       delete condition;
  // //     }
=======
>>>>>>> Stashed changes
}

void RecPointsQcTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void RecPointsQcTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void RecPointsQcTask::reset()
{
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mHistColTime->Reset();
  mHistColFirstTime->Reset();
  mHistColGloMeanTime->Reset();
  mHistColSelectedMeanTime->Reset();
  mHistTime2Ch->Reset();
}

} // namespace o2::quality_control_modules::fv0
