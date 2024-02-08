/// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
/// See https://alice-o2.web.cern.ch/copyright for details of the copyright
/// holders. All rights not expressly granted are reserved.
//
/// This software is distributed under the terms of the GNU General Public
/// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
/// In applying this license CERN does not waive the privileges and immunities
/// granted to it by virtue of its status as an Intergovernmental Organization
/// or submit itself to any jurisdiction.

///
/// \file   AgingLaserReductor.cxx
/// \author Sandor Lokos <sandor.lokos@cern.ch>, Edmundo Garcia-Solis <edmundo.garcia@cern.ch>, Andreas Molander <andreas.molander@cern.ch>
///

#include "FT0/AgingLaserReductor.h"
#include "QualityControl/QcInfoLogger.h"
#include <TF2.h>
#include <TH2.h>
#include <sstream>

namespace o2::quality_control_modules::ft0
{
void* AgingLaserReductor::getBranchAddress() { return &mStats; }
const char* AgingLaserReductor::getBranchLeafList()
{
  return Form("validity1/D:validity2/D:mean1/D:mean2/D:mean[%i]/D:stddev1:stddev2:stddev[%i]", NChannel, NChannel);
}

void AgingLaserReductor::update(TObject* obj)
{
  if (auto histo = dynamic_cast<TH2F*>(obj)) {
    // Get ch ID
    int channel = -1;
    sscanf(histo->GetName(), "%*[^0-9]%d", &channel);
    
    if (channel < NChannel) {
      // Detector channels
      for (int iCh = 0; iCh < mNDetectorChannels; iCh++) {
        TH1* bc_projection = histo->ProjectionY(Form("AmpCh%i", iCh), iCh + 1, iCh + 1);
        mStats.mean[iCh] = bc_projection->GetMean();
        mStats.stddev[iCh] = bc_projection->GetStdDev();
      }
    } else {
      // Reference channels
      TH1* bc_projection = histo->ProjectionY("bc_projection", 0, -1);
      int ibc = 0;
      int ibc_max = 0;
      float stdv = 0;
      if (bc_projection->GetEntries() > 1000) {
        stdv = bc_projection->GetStdDev();
        if (stdv < 0.5) {
          stdv = 0.5;
        }
        ibc = bc_projection->GetMean() - 2. * stdv;
        ibc_max = bc_projection->GetMean() + 2. * stdv;
      }

      TH1* slice_first_peak;
      bool gotFirstPeak = false;
      mStats.mean1 = 0.;
      mStats.stddev1 = 0.;
      mStats.validity1 = 0.;
      while (!gotFirstPeak && ibc < ibc_max) {
        slice_first_peak = histo->ProjectionX(Form("first peak in BC #%d", ibc), ibc, ibc + 1);
        if (slice_first_peak->GetEntries() > 1000) {
          mStats.mean1 = slice_first_peak->GetMean();
          mStats.stddev1 = slice_first_peak->GetStdDev();
          mStats.validity1 = 1.;
          gotFirstPeak = true;
          ibc += 2;
          break;
        } else
          ibc++;
      }

      TH1* slice_second_peak;
      bool gotSecondPeak = false;
      mStats.mean2 = 0.;
      mStats.stddev2 = 0.;
      mStats.validity2 = 0.;
      while (!gotSecondPeak && gotFirstPeak && ibc < ibc_max) {
        slice_second_peak = histo->ProjectionX(Form("second peak in BC #%d", ibc), ibc, ibc + 1);
        if (slice_second_peak->GetEntries() > 1000) {
          mStats.mean2 = slice_second_peak->GetMean();
          mStats.stddev2 = slice_second_peak->GetStdDev();
          mStats.validity2 = 1.;
          gotSecondPeak = true;
          break;
        } else
          ibc++;
      }

      if (!gotSecondPeak)
        ILOG(Warning) << "TH1ReductorLaser: one of the peaks of the reference PMT is missing!" << ENDM;
      if (!gotFirstPeak && !gotSecondPeak)
        ILOG(Warning) << "TH1ReductorLaser: cannot find peaks of the reference PMT distribution at all !" << ENDM;
    }
  }
}
} // namespace o2::quality_control_modules::ft0
