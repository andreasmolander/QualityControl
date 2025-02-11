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
/// \file   IncreasingEntries.cxx
/// \author Barthélémy von Haller
///

#include "Common/IncreasingEntries.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/stringUtils.h"
// ROOT
#include <TH1.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::common
{

void IncreasingEntries::configure()
{
  parseBooleanParam(mCustomParameters, "mustIncrease", mMustIncrease);

  ILOG(Debug, Support) << "mustIncrease: " << mMustIncrease << ENDM;

  if (mMustIncrease) {
    mPaveText = make_shared<TPaveText>(1, 0.125, 0.6, 0, "NDC");
    mPaveText->AddText("Number of Entries has *not* changed");
    mPaveText->AddText("in the past cycle");
    mPaveText->SetFillColor(kRed);
    mPaveText->SetMargin(0);
  } else {
    mPaveText = make_shared<TPaveText>(1, 0.125, 0.6, 0, "NDC");
    mPaveText->AddText("Number of Entries has *changed*");
    mPaveText->AddText("in the past cycle");
    mPaveText->SetFillColor(kRed);
    mPaveText->SetMargin(0);
  }
}

Quality IncreasingEntries::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Good;

  for (auto& [moName, mo] : *moMap) {

    TH1* histo = dynamic_cast<TH1*>(mo->getObject());
    if (histo == nullptr) {
      continue;
    }

    double previousNumberEntries = mLastEntries.count(moName) > 0 ? mLastEntries.at(moName) : 0;
    double currentNumberEntries = histo->GetEntries();

    if (mMustIncrease && previousNumberEntries == currentNumberEntries) {
      result = Quality::Bad;
      result.addReason(FlagReasonFactory::NoDetectorData(), "Number of entries stopped increasing.");
      histo->GetListOfFunctions()->Add(mPaveText->Clone());
    } else if (!mMustIncrease && previousNumberEntries != currentNumberEntries) {
      result = Quality::Bad;
      result.addReason(FlagReasonFactory::Unknown(), "Number of entries has increased.");
      histo->GetListOfFunctions()->Add(mPaveText->Clone());
    }

    mLastEntries[moName] = currentNumberEntries;
  }
  return result;
}

std::string IncreasingEntries::getAcceptedType() { return "TH1"; }

void IncreasingEntries::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  // as we want to add the text on the faulty histo we cannot do it here.
}

} // namespace o2::quality_control_modules::common
