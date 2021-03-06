/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/Widget.hpp"
#include "Language/Language.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Layout.hpp"


gcc_const
static WindowStyle
GetDialogStyle()
{
  WindowStyle style;
  style.Hide();
  style.ControlParent();
  return style;
}

WidgetDialog::WidgetDialog(const DialogLook &look)
  :WndForm(look),
   buttons(GetClientAreaWindow(), look.button),
   widget(GetClientAreaWindow()),
   changed(false)
{
}

void
WidgetDialog::Create(SingleWindow &parent,
                     const TCHAR *caption, const PixelRect &rc,
                     Widget *_widget,
                     DialogFooter::Listener *_listener,
                     UPixelScalar _footer_height,
                     ButtonPanel::ButtonPanelPosition button_position)
{
  full = false;
  auto_size = false;
  WndForm::Create(parent, rc, caption, GetDialogStyle());
  dialog_footer.Create(GetClientAreaWindow(), _listener, _footer_height);
  widget.Set(_widget);
  buttons.SetButtonPosition(button_position);
  widget.Move(buttons.UpdateLayout(GetNonFooterRect()));
}

void
WidgetDialog::CreateFull(SingleWindow &parent, const TCHAR *caption,
                         Widget *widget,
                         DialogFooter::Listener *_listener,
                         UPixelScalar _footer_height,
                         ButtonPanel::ButtonPanelPosition button_position)
{
  Create(parent, caption, parent.GetClientRect(), widget, _listener,
         _footer_height, button_position);
  full = true;
}

void
WidgetDialog::CreateAuto(SingleWindow &parent, const TCHAR *caption,
                         Widget *_widget)
{
  CreateFull(parent, caption, _widget);
  return;
}

void
WidgetDialog::CreatePopup(SingleWindow &parent, const TCHAR *caption,
                         Widget *_widget)
{
  full = false;
  auto_size = true;
  WndForm::Create(parent, caption, GetDialogStyle());
  dialog_footer.Create(GetClientAreaWindow(), nullptr, 0);
  widget.Set(_widget);
  widget.Move(buttons.UpdateLayout(GetNonFooterRect()));
}

void
WidgetDialog::CreatePreliminary(SingleWindow &parent, const TCHAR *caption)
{
  full = false;
  auto_size = true;
  buttons.SetButtonPosition(ButtonPanel::ButtonPanelPosition::Bottom);
  WndForm::Create(parent, parent.GetClientRect(), caption, GetDialogStyle());
}

void
WidgetDialog::CreatePreliminaryFull(SingleWindow &parent, const TCHAR *caption)
{
  auto_size = false;
  buttons.SetButtonPosition(ButtonPanel::ButtonPanelPosition::Bottom);
  WndForm::Create(parent, parent.GetClientRect(), caption, GetDialogStyle());
}

void
WidgetDialog::FinishPreliminary(Widget *_widget)
{
  assert(IsDefined());
  assert(!widget.IsDefined());
  assert(_widget != nullptr);

  widget.Set(_widget);
  widget.Move(buttons.UpdateLayout(GetNonFooterRect()));

  if (auto_size)
    AutoSize();
}

void
WidgetDialog::DialogFooter::Create(ContainerWindow &parent,
                                   Listener *_listener,
                                   UPixelScalar _height)
{
  listener = _listener;
  height = _height;
  WindowStyle style;
  PixelRect rc(0, parent.GetHeight() - height, parent.GetWidth(), parent.GetHeight());
  PaintWindow::Create(parent, rc, style);
}

void
WidgetDialog::AutoSize()
{
  const PixelRect parent_rc = GetParentClientRect();
  const PixelSize parent_size = parent_rc.GetSize();

  widget.Prepare();

  // Calculate the minimum size of the dialog
  PixelSize min_size = widget.Get()->GetMinimumSize();
  min_size.cy += GetTitleHeight();

  // Calculate the maximum size of the dialog
  PixelSize max_size = widget.Get()->GetMaximumSize();
  max_size.cy += GetTitleHeight();

  // Calculate sizes with one button row at the bottom
  const PixelScalar min_height_with_buttons =
    min_size.cy + Layout::GetMaximumControlHeight();
  const PixelScalar max_height_with_buttons =
    max_size.cy + Layout::GetMaximumControlHeight();

  if (/* need full dialog height even for minimum widget height? */
      /*landscape */
      min_height_with_buttons >= parent_size.cy ||
      /* try to avoid putting buttons left on portrait screens; try to
         comply with maximum widget height only on landscape
         screens */
      (parent_size.cx > parent_size.cy &&
       max_height_with_buttons >= parent_size.cy)) {
    /* need full height, buttons must be left */
    PixelRect rc = parent_rc;
    if (max_size.cy < parent_size.cy)
      rc.bottom = rc.top + max_size.cy;

    PixelRect remaining = buttons.LeftLayout(rc);
    PixelSize remaining_size = remaining.GetSize();
    if (remaining_size.cx > max_size.cx)
      rc.right -= remaining_size.cx - max_size.cx;

    Resize(rc.GetSize());
    rc.bottom -= dialog_footer.GetHeight();
    rc.bottom -= GetTitleHeight();
    widget.Move(buttons.LeftLayout());

    MoveToCenter();
    return;
  }

  /* see if buttons fit at the bottom */
  /* portrait */

  PixelRect rc = parent_rc;
  if (max_size.cx < parent_size.cx)
    rc.right = rc.left + max_size.cx;

  PixelRect remaining = buttons.BottomLayout(rc);
  PixelSize remaining_size = remaining.GetSize();

  if (remaining_size.cy > max_size.cy)
    rc.bottom -= remaining_size.cy - max_size.cy;

  Resize(rc.GetSize());
  rc.bottom -= dialog_footer.GetHeight();
  rc.bottom -= GetTitleHeight();
  widget.Move(buttons.BottomLayout());

  MoveToCenter();
}

int
WidgetDialog::ShowModal()
{
  if (auto_size)
    AutoSize();
  else
    widget.Move(buttons.UpdateLayout(GetNonFooterRect()));

  widget.Show();
  return WndForm::ShowModal();
}

void
WidgetDialog::OnAction(int id)
{
  if (id == mrOK) {
    if (!widget.Get()->Save(changed))
      return;
  }

  WndForm::OnAction(id);
}

void
WidgetDialog::OnDestroy()
{
  widget.Unprepare();

  WndForm::OnDestroy();
}

PixelRect
WidgetDialog::GetNonFooterRect()
{
  PixelRect rc;
  rc.left = 0;
  rc.right = GetWidth();
  rc.top = 0;
  rc.bottom = GetHeight() - dialog_footer.GetHeight() - WndForm::GetTitleHeight();
  return rc;
}

PixelRect
WidgetDialog::GetFooterRect()
{
  PixelRect rc;
  rc.left = 0;
  rc.right = GetWidth();
  rc.top = GetHeight() - dialog_footer.GetHeight() - WndForm::GetTitleHeight();
  rc.bottom = GetHeight() - WndForm::GetTitleHeight();
  return rc;
}

void
WidgetDialog::OnResize(PixelSize new_size)
{
  WndForm::OnResize(new_size);

  if (auto_size)
    return;

  widget.Move(buttons.UpdateLayout(GetNonFooterRect()));

  if (dialog_footer.IsDefined())
    dialog_footer.Move(1, new_size.cy - dialog_footer.GetHeight() - WndForm::GetTitleHeight());
}

void
WidgetDialog::ReinitialiseLayout(const PixelRect &parent_rc)
{
  if (full)
    /* make it full-screen again on the resized main window */
    Move(parent_rc);
  else
    WndForm::ReinitialiseLayout(parent_rc);
}

void
WidgetDialog::SetDefaultFocus()
{
  if (!widget.SetFocus())
    WndForm::SetDefaultFocus();
}

bool
WidgetDialog::OnAnyKeyDown(unsigned key_code)
{
  return widget.KeyPress(key_code) ||
    buttons.KeyPress(key_code) ||
    WndForm::OnAnyKeyDown(key_code);
}

bool
DefaultWidgetDialog(SingleWindow &parent, const DialogLook &look,
                    const TCHAR *caption, const PixelRect &rc, Widget &widget)
{
  WidgetDialog dialog(look);
  dialog.Create(parent, caption, rc, &widget);
  dialog.AddSymbolButton(_T("_X"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.ShowModal();

  /* the caller manages the Widget */
  dialog.StealWidget();

  return dialog.GetChanged();
}

bool
DefaultWidgetDialog(SingleWindow &parent, const DialogLook &look,
                    const TCHAR *caption, Widget &widget)
{
  WidgetDialog dialog(look);
  dialog.CreateFull(parent, caption, &widget);
  dialog.AddSymbolButton(_T("_X"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.ShowModal();

  /* the caller manages the Widget */
  dialog.StealWidget();

  return dialog.GetChanged();
}
