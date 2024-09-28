// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <board.scad>
include <box.scad>
include <gui.scad>

difference() {
  union() {
    if (show_box) DoughHeater_box(top);
    if (show_vitamins) {
      translate([wall_thickness+corner_radius,
		 wall_thickness+corner_radius,
		 space_below_board])
	translate(Z*1.6) DoughHeater_board();
    }
  }
  if (cut_front) translate([-10, -10, -10]) cube([20+outer_dims[0], 15, 70]);
}
