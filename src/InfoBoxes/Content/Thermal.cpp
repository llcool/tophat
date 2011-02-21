/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "InfoBoxes/Content/Thermal.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "UnitsFormatter.hpp"
#include "Interface.hpp"

#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "DeviceBlackboard.hpp"

#include "Dialogs/dlgInfoBoxAccess.hpp"
#include "Screen/Layout.hpp"

#include <tchar.h>
#include <stdio.h>

static void
SetVSpeed(InfoBoxWindow &infobox, fixed value)
{
  TCHAR buffer[32];
  Units::FormatUserVSpeed(value, buffer, 32, false);
  infobox.SetValue(buffer[0] == _T('+') ? buffer + 1 : buffer);
  infobox.SetValueUnit(Units::Current.VerticalSpeedUnit);
}

/*
 * InfoBoxContentMacCready
 *
 * Subpart Panel Edit
 */

static int InfoBoxID;

Window*
InfoBoxContentMacCready::PnlEditLoad(SingleWindow &parent, TabBarControl* wTabBar,
                                 WndForm* wf, const int id)
{
  assert(wTabBar);
  assert(wf);
//  wf = _wf;

  InfoBoxID = id;

  Window *wInfoBoxAccessEdit =
      LoadWindow(CallBackTable, wf, *wTabBar,
                 Layout::landscape ?
                     _T("IDR_XML_INFOBOXMACCREADYEDIT_L") :
                     _T("IDR_XML_INFOBOXMACCREADYEDIT"));
  assert(wInfoBoxAccessEdit);

  return wInfoBoxAccessEdit;
}

void
InfoBoxContentMacCready::PnlEditOnCloseClicked(WndButton &Sender)
{
  (void)Sender;
  dlgInfoBoxAccess::OnClose();
}

void
InfoBoxContentMacCready::PnlEditOnPlusSmall(WndButton &Sender)
{
  (void)Sender;
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("+0.1"));
}

void
InfoBoxContentMacCready::PnlEditOnPlusBig(WndButton &Sender)
{
  (void)Sender;
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("+0.5"));
}

void
InfoBoxContentMacCready::PnlEditOnMinusSmall(WndButton &Sender)
{
  (void)Sender;
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("-0.1"));
}

void
InfoBoxContentMacCready::PnlEditOnMinusBig(WndButton &Sender)
{
  (void)Sender;
  InfoBoxManager::ProcessQuickAccess(InfoBoxID, _T("-0.5"));
}

/*
 * Subpart Panel Setup
 */

Window*
InfoBoxContentMacCready::PnlSetupLoad(SingleWindow &parent, TabBarControl* wTabBar,
                                 WndForm* wf, const int id)
{
  assert(wTabBar);
  assert(wf);
//  wf = _wf;

  InfoBoxID = id;

  Window *wInfoBoxAccessEdit =
      LoadWindow(CallBackTable, wf, *wTabBar,
                 Layout::landscape ?
                     _T("IDR_XML_INFOBOXMACCREADYSETUP_L") :
                     _T("IDR_XML_INFOBOXMACCREADYSETUP"));
  assert(wInfoBoxAccessEdit);

  return wInfoBoxAccessEdit;
}

void
InfoBoxContentMacCready::PnlSetupOnSetup(WndButton &Sender) {
  (void)Sender;
  InfoBoxManager::SetupFocused(InfoBoxID);
  dlgInfoBoxAccess::OnClose();
}

/*
 * Subpart callback function pointers
 */

InfoBoxContentMacCready::InfoBoxPanelContent InfoBoxContentMacCready::pnlEdit =
{
  (*InfoBoxContentMacCready::PnlEditLoad),
  NULL
};

InfoBoxContentMacCready::InfoBoxPanelContent InfoBoxContentMacCready::pnlInfo =
{
  NULL
};

InfoBoxContentMacCready::InfoBoxPanelContent InfoBoxContentMacCready::pnlSetup =
{
  (*InfoBoxContentMacCready::PnlSetupLoad),
  NULL
};

CallBackTableEntry InfoBoxContentMacCready::CallBackTable[] = {
  DeclareCallBackEntry(InfoBoxContentMacCready::PnlEditOnPlusSmall),
  DeclareCallBackEntry(InfoBoxContentMacCready::PnlEditOnPlusBig),
  DeclareCallBackEntry(InfoBoxContentMacCready::PnlEditOnMinusSmall),
  DeclareCallBackEntry(InfoBoxContentMacCready::PnlEditOnMinusBig),

  DeclareCallBackEntry(InfoBoxContentMacCready::PnlSetupOnSetup),

  DeclareCallBackEntry(NULL)
};

InfoBoxContentMacCready::InfoBoxDlgContent InfoBoxContentMacCready::dlgContent =
{
    InfoBoxContentMacCready::pnlEdit,
    InfoBoxContentMacCready::pnlInfo,
    InfoBoxContentMacCready::pnlSetup,
    InfoBoxContentMacCready::CallBackTable
};

InfoBoxContentMacCready::InfoBoxDlgContent*
InfoBoxContentMacCready::GetInfoBoxDlgContent() {
  return &dlgContent;
}

/*
 * Subpart normal operations
 */

void
InfoBoxContentMacCready::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox,
            XCSoarInterface::Calculated().common_stats.current_risk_mc);

  // Set Comment
  if (XCSoarInterface::SettingsComputer().auto_mc)
    infobox.SetComment(_("AUTO"));
  else
    infobox.SetComment(_("MANUAL"));
}

bool
InfoBoxContentMacCready::HandleKey(const InfoBoxKeyCodes keycode)
{
  if (protected_task_manager == NULL)
    return false;

  GlidePolar polar = protected_task_manager->get_glide_polar();
  fixed mc = polar.get_mc();

  switch (keycode) {
  case ibkUp:
    mc = std::min(mc + fixed_one / 10, fixed(5));
    polar.set_mc(mc);
    protected_task_manager->set_glide_polar(polar);
    device_blackboard.SetMC(mc);
    return true;

  case ibkDown:
    mc = std::max(mc - fixed_one / 10, fixed_zero);
    polar.set_mc(mc);
    protected_task_manager->set_glide_polar(polar);
    device_blackboard.SetMC(mc);
    return true;

  case ibkLeft:
    XCSoarInterface::SetSettingsComputer().auto_mc = false;
    return true;

  case ibkRight:
    XCSoarInterface::SetSettingsComputer().auto_mc = true;
    return true;

  case ibkEnter:
    XCSoarInterface::SetSettingsComputer().auto_mc =
        !XCSoarInterface::SettingsComputer().auto_mc;
    return true;
  }
  return false;
}

bool
InfoBoxContentMacCready::HandleQuickAccess(const TCHAR *misc)
{
  if (protected_task_manager == NULL)
    return false;

  GlidePolar polar = protected_task_manager->get_glide_polar();
  fixed mc = polar.get_mc();

  if (_tcscmp(misc, _T("+0.1")) == 0) {
    HandleKey(ibkUp);
    return true;

  } else if (_tcscmp(misc, _T("+0.5")) == 0) {
    mc = std::min(mc + fixed_one / 2, fixed(5));
    polar.set_mc(mc);
    protected_task_manager->set_glide_polar(polar);
    device_blackboard.SetMC(mc);
    return true;

  } else if (_tcscmp(misc, _T("-0.1")) == 0) {
    HandleKey(ibkDown);
    return true;

  } else if (_tcscmp(misc, _T("-0.5")) == 0) {
    mc = std::max(mc - fixed_one / 2, fixed_zero);
    polar.set_mc(mc);
    protected_task_manager->set_glide_polar(polar);
    device_blackboard.SetMC(mc);
    return true;
  }
  return false;
}

void
InfoBoxContentVario::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Basic().BruttoVario);
}

void
InfoBoxContentVarioNetto::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Basic().NettoVario);
}

void
InfoBoxContentThermal30s::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Calculated().Average30s);

  // Set Color (red/black)
  infobox.SetColor(XCSoarInterface::Calculated().Average30s * fixed_two <
      XCSoarInterface::Calculated().common_stats.current_risk_mc ? 1 : 0);
}

void
InfoBoxContentThermalLastAvg::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Calculated().LastThermalAverage);
}

void
InfoBoxContentThermalLastGain::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR sTmp[32];
  Units::FormatUserAltitude(XCSoarInterface::Calculated().LastThermalGain,
                            sTmp, 32, false);
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
}

void
InfoBoxContentThermalLastTime::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR sTmp[32];
  int dd = abs((int)XCSoarInterface::Calculated().LastThermalTime) % (3600 * 24);
  int hours = dd / 3600;
  int mins = dd / 60 - hours * 60;
  int seconds = dd - mins * 60 - hours * 3600;

  if (hours > 0) { // hh:mm, ss
    _stprintf(sTmp, _T("%02d:%02d"), hours, mins);
    infobox.SetValue(sTmp);
    _stprintf(sTmp, _T("%02d"), seconds);
    infobox.SetComment(sTmp);
  } else { // mm:ss
    _stprintf(sTmp, _T("%02d:%02d"), mins, seconds);
    infobox.SetValue(sTmp);
    infobox.SetComment(_T(""));
  }
}

void
InfoBoxContentThermalAllAvg::Update(InfoBoxWindow &infobox)
{
  if (!positive(XCSoarInterface::Calculated().timeCircling)) {
    infobox.SetInvalid();
    return;
  }

  SetVSpeed(infobox, XCSoarInterface::Calculated().TotalHeightClimb /
            XCSoarInterface::Calculated().timeCircling);
}

void
InfoBoxContentThermalAvg::Update(InfoBoxWindow &infobox)
{
  SetVSpeed(infobox, XCSoarInterface::Calculated().ThermalAverage);

  // Set Color (red/black)
  infobox.SetColor(XCSoarInterface::Calculated().ThermalAverage * fixed(1.5) <
      XCSoarInterface::Calculated().common_stats.current_risk_mc ? 1 : 0);
}

void
InfoBoxContentThermalGain::Update(InfoBoxWindow &infobox)
{
  // Set Value
  TCHAR sTmp[32];
  Units::FormatUserAltitude(XCSoarInterface::Calculated().ThermalGain,
                            sTmp, 32, false);
  infobox.SetValue(sTmp);

  // Set Unit
  infobox.SetValueUnit(Units::Current.AltitudeUnit);
}

void
InfoBoxContentThermalRatio::Update(InfoBoxWindow &infobox)
{
  // Set Value
  SetValueFromFixed(infobox, _T("%2.0f%%"),
                    XCSoarInterface::Calculated().PercentCircling);
}

void
InfoBoxContentVarioDistance::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Calculated().task_stats.task_valid) {
    infobox.SetInvalid();
    return;
  }

  SetVSpeed(infobox,
            XCSoarInterface::Calculated().task_stats.total.vario.get_value());

  // Set Color (red/black)
  infobox.SetColor(negative(
      XCSoarInterface::Calculated().task_stats.total.vario.get_value()) ? 1 : 0);
}
