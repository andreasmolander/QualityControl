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
/// \file   MeanCheck.cxx
/// \author My Name
///

#include "FV0/MeanCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>
#include "Common/Utils.h"

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::fv0
{

void MeanCheck::configure()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  // This method is called whenever CustomParameters are set.

  // Example of retrieving a custom parameter
  std::string parameter = mCustomParameters.atOrDefaultValue("myOwnKey1", "default");

  mLimit = o2::quality_control_modules::common::getFromConfig<float>(mCustomParameters, "limit", 50);
}

Quality MeanCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  Quality result = Quality::Null;

  // You can get details about the activity via the object mActivity:
  ILOG(Debug, Devel) << "Run " << mActivity->mId << ", type: " << mActivity->mType << ", beam: " << mActivity->mBeamType << ENDM;
  // and you can get your custom parameters:
  ILOG(Debug, Devel) << "custom param physics.pp.myOwnKey1 : " << mCustomParameters.atOrDefaultValue("myOwnKey1", "default_value", "physics", "pp") << ENDM;

  // This is an example of accessing the histogram 'example' created by FV0Task
  for (auto& [moName, mo] : *moMap) {
    if (mo->getName() == "Time") {
      auto* h = dynamic_cast<TH1*>(mo->getObject());
      if (h == nullptr) {
        ILOG(Error, Support) << "Could not cast `example` to TH1*, skipping" << ENDM;
        continue;
      }
      // unless we find issues, we assume the quality is good
      result = Quality::Good;
      
      if (TMath::Abs(h->GetMean()) > mLimit) {
        result = Quality::Bad;
      }
    }
  }
  return result;
}

std::string MeanCheck::getAcceptedType()
{
  // This method is a remnant of early interface prototype and will be removed in the scope of ticket QC-373
  return "TH1";
}

void MeanCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.

  // This method lets you decorate the checked object according to the computed Quality
  if (mo->getName() == "Time") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    if (h == nullptr) {
      ILOG(Error, Support) << "Could not cast `example` to TH1*, skipping" << ENDM;
      return;
    }

    

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

void MeanCheck::reset()
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "MeanCheck::reset" << ENDM;
  // please reset the state of the check here to allow for reuse between consecutive runs.
}

void MeanCheck::startOfActivity(const Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "MeanCheck::start : " << activity.mId << ENDM;
  mActivity = make_shared<Activity>(activity);
}

void MeanCheck::endOfActivity(const Activity& activity)
{
  // THUS FUNCTION BODY IS AN EXAMPLE. PLEASE REMOVE EVERYTHING YOU DO NOT NEED.
  ILOG(Debug, Devel) << "MeanCheck::end : " << activity.mId << ENDM;
}

} // namespace o2::quality_control_modules::fv0
