/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "CrossSectionWindow.hpp"
#include "Components.hpp"
#include "GlideComputer.hpp"
#include "Interface.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Chart.hpp"
#include "Screen/Graphics.hpp"
#include "Airspace/AirspaceIntersectionVisitor.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Units.hpp"

#define AIRSPACE_SCANSIZE_X 16

class AirspaceIntersectionVisitorSlice: public AirspaceIntersectionVisitor
{
public:
  AirspaceIntersectionVisitorSlice(Canvas &_canvas, Chart &_chart,
                                   const SETTINGS_MAP &_settings_map,
                                   const GeoPoint _start,
                                   const ALTITUDE_STATE& _state) :
    canvas(_canvas), chart(_chart), settings_map(_settings_map),
    start(_start), state(_state) {}

  void
  Render(const AbstractAirspace& as)
  {
    int type = as.get_type();
    if (type <= 0)
      return;

    // No intersections for this airspace
    if (m_intersections.empty())
      return;

    // Select pens and brushes
    if (settings_map.bAirspaceBlackOutline)
      canvas.black_pen();
    else
      canvas.select(Graphics::hAirspacePens[type]);

    canvas.select(Graphics::GetAirspaceBrushByClass(type, settings_map));
    canvas.set_text_color(Graphics::GetAirspaceColourByClass(type, settings_map));

    RECT rcd;
    // Calculate top and bottom coordinate
    rcd.top = chart.screenY(as.get_top_altitude(state));
    if (as.is_base_terrain())
      rcd.bottom = chart.screenY(fixed_zero);
    else
      rcd.bottom = chart.screenY(as.get_base_altitude(state));

    // Iterate through the intersections
    for (AirspaceIntersectionVector::const_iterator it = m_intersections.begin();
         it != m_intersections.end(); ++it) {
      const GeoPoint p_start = it->first;
      const GeoPoint p_end = it->second;
      const fixed distance_start = start.distance(p_start);
      const fixed distance_end = start.distance(p_end);

      // Determine left and right coordinate
      rcd.left = chart.screenX(distance_start);
      rcd.right = chart.screenX(distance_end);

      // only one edge found, next edge must be beyond screen
      if ((rcd.left == rcd.right) && (p_start == p_end)) {
        rcd.right = chart.screenX(chart.getXmax());
      }

      // Draw the airspace
      canvas.rectangle(rcd.left, rcd.top, rcd.right, rcd.bottom);
    }
  }

  void
  Visit(const AirspaceCircle& as)
  {
    Render(as);
  }

  void
  Visit(const AirspacePolygon& as)
  {
    Render(as);
  }

private:
  Canvas& canvas;
  Chart& chart;
  const SETTINGS_MAP& settings_map;
  const GeoPoint start;
  const ALTITUDE_STATE& state;
};

CrossSectionWindow::CrossSectionWindow() :
  terrain_brush(Chart::GROUND_COLOUR),
  terrain(NULL), airspace_database(NULL),
  start(Angle::native(fixed_zero), Angle::native(fixed_zero)),
  vec(fixed(50000), Angle::native(fixed_zero)),
  background_color(Color::WHITE), text_color(Color::BLACK) {}

void
CrossSectionWindow::ReadBlackboard(const NMEA_INFO &_gps_info,
                                   const DERIVED_INFO &_calculated_info,
                                   const SETTINGS_MAP &_settings_map)
{
  gps_info = _gps_info;
  calculated_info = _calculated_info;
  settings_map = _settings_map;
}

void
CrossSectionWindow::Paint(Canvas &canvas, const RECT rc)
{
  fixed hmin = max(fixed_zero, gps_info.GPSAltitude - fixed(3300));
  fixed hmax = max(fixed(3300), gps_info.GPSAltitude + fixed(1000));

  Chart chart(canvas, rc);
  chart.ResetScale();
  chart.ScaleXFromValue(fixed_zero);
  chart.ScaleXFromValue(vec.Distance);
  chart.ScaleYFromValue(hmin);
  chart.ScaleYFromValue(hmax);

  PaintAirspaces(canvas, chart);
  PaintTerrain(canvas, chart);
  PaintGlide(chart);
  PaintAircraft(canvas, chart, rc);
  PaintGrid(canvas, chart);
}

void
CrossSectionWindow::PaintAirspaces(Canvas &canvas, Chart &chart)
{
  if (airspace_database == NULL)
    return;

  AirspaceIntersectionVisitorSlice ivisitor(canvas, chart, settings_map,
                                            start, ToAircraftState(gps_info));
  airspace_database->visit_intersecting(start, vec, ivisitor);
}

void
CrossSectionWindow::PaintTerrain(Canvas &canvas, Chart &chart)
{
  if (terrain == NULL)
    return;

  const GeoPoint p_diff = vec.end_point(start) - start;

  RasterTerrain::Lease map(*terrain);

  RasterPoint points[2 + AIRSPACE_SCANSIZE_X];

  points[0].x = chart.screenX(vec.Distance);
  points[0].y = chart.screenY(fixed_zero);
  points[1].x = chart.screenX(fixed_zero);
  points[1].y = chart.screenY(fixed_zero);

  unsigned i = 2;
  for (unsigned j = 0; j < AIRSPACE_SCANSIZE_X; ++j) {
    const fixed t_this = fixed(j) / (AIRSPACE_SCANSIZE_X - 1);
    const GeoPoint p_this = start + p_diff * t_this;

    short h = map->GetField(p_this);
    if (RasterBuffer::is_special(h)) {
      if (RasterBuffer::is_water(h))
        /* water is at 0m MSL */
        /* XXX paint in blue? */
        h = 0;
      else
        /* skip "unknown" values */
        continue;
    }

    RasterPoint p;
    p.x = chart.screenX(t_this * vec.Distance);
    p.y = chart.screenY(fixed(h));

    points[i++] = p;
  }

  if (i >= 4) {
    canvas.null_pen();
    canvas.select(terrain_brush);
    canvas.polygon(&points[0], i);
  }
}

void
CrossSectionWindow::PaintGlide(Chart &chart)
{
  if (gps_info.GroundSpeed > fixed(10)) {
    fixed t = vec.Distance / gps_info.GroundSpeed;
    chart.DrawLine(fixed_zero, gps_info.GPSAltitude, vec.Distance,
                   gps_info.GPSAltitude + calculated_info.Average30s * t,
                   Chart::STYLE_BLUETHIN);
  }
}

void
CrossSectionWindow::PaintAircraft(Canvas &canvas, const Chart &chart,
                                  const RECT rc)
{
  Brush brush(text_color);
  canvas.select(brush);

  Pen pen(1, text_color);
  canvas.select(pen);

  RasterPoint line[4];
  line[0].x = chart.screenX(fixed_zero);
  line[0].y = chart.screenY(gps_info.GPSAltitude);
  line[1].x = rc.left;
  line[1].y = line[0].y;
  line[2].x = line[1].x;
  line[2].y = line[0].y - (line[0].x - line[1].x) / 2;
  line[3].x = (line[1].x + line[0].x) / 2;
  line[3].y = line[0].y;
  canvas.polygon(line, 4);
}

void
CrossSectionWindow::PaintGrid(Canvas &canvas, Chart &chart)
{
  canvas.set_text_color(text_color);

  chart.DrawXGrid(Units::ToSysDistance(fixed(5)), fixed_zero,
                  Chart::STYLE_THINDASHPAPER, fixed(5), true);
  chart.DrawYGrid(Units::ToSysAltitude(fixed(1000)), fixed_zero,
                  Chart::STYLE_THINDASHPAPER, fixed(1000), true);

  chart.DrawXLabel(_T("D"));
  chart.DrawYLabel(_T("h"));
}

bool
CrossSectionWindow::on_create() {
  PaintWindow::on_create();
  return true;
}

void
CrossSectionWindow::on_paint(Canvas &canvas)
{
  canvas.clear(background_color);
  canvas.set_text_color(text_color);
  canvas.select(Fonts::Map);

  const RECT rc = get_client_rect();

  Paint(canvas, rc);
}
