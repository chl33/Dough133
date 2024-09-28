// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <MCAD/units.scad>

include <ProjectBox/project_box.scad>
include <ProjectBox/screw_terminal.scad>

board_thickness = 1.6;
pad_space = 2.54;
board_dims = [27.5*pad_space, 17.5*pad_space, board_thickness];

mount_offset = pad_space;

module oled() {
  s = inch * 0.1;
  pin_len = 4;
  color("silver") {
    for (i = [0: 3]) {
      translate([s*i+pad_space, pad_space/2, -pin_len]) cube([0.25, 0.25, pin_len]);
    }
  }
  color("gray") cube([12, 38, 1]);
  translate([0, 4, 1]) color("black") cube([12, 28, 1]);
}

module doughheater_to_oled_frame() {
  u = pad_space;
  translate([u*9.3, 2, 11]) children();
}

module DoughHeater_board() {
  u = pad_space;
  bdz = board_dims[2];

  // Board imported from KiCad (VRML) -> Blender
  translate([board_dims[0]/2, board_dims[1]/2, 0.6]) color("white")
    import(file="DoughHeater.stl", convexity=3);

  doughheater_to_oled_frame() oled();
}
