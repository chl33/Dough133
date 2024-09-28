// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <ProjectBox/project_box.scad>
include <ProjectBox/hexsheet.scad>
include <board.scad>

ones = [1, 1, 1];

wall_thickness = 1.2;
gap = 0.2;
corner_radius = 2;

space_above_board = 3;
space_below_board = 3;
inner_dims = (board_dims
	      + Z*(space_above_board+space_below_board)
	      + 2*gap*ones);
outer_dims = (inner_dims
	      + 2*ones*wall_thickness
	      + [2, 2, 0] * corner_radius);

temp_air_hole_space = 1.6;
temp_air_hole_sep = 0.6;

// Box bump for OLED
// Offset
b1o = [pad_space*10, 1*pad_space, outer_dims[2]-wall_thickness];
// Dimensions
b1d = [15, 44, 11]; // 13-1+2*wall_thickness

// Box bump for MOSFETS
// Offset
b2o = [pad_space*15.5, 4*pad_space, outer_dims[2]-wall_thickness];
// Dimensions
//  use 18mm high if mosfets directly soldered, 28mm when using headers for them.
b2d = [13, 26, 28]; // 13-1+2*wall_thickness

// Box bump for barrel jacks
// Offset
b3o = [b1o[0], 7*pad_space, outer_dims[2]-wall_thickness];
// Dimensions
b3d = [outer_dims[0]-b3o[0], 30, b1d[2]]; // 13-1+2*wall_thickness


// Square air holes for the box.
module holes_xy(x, y, z, dx, space, border) {
  // cube([x, y, z+2*epsilon]);
  nx = floor((x-2*border-space) / (dx+space));
  ny = floor((y-2*border-space) / (dx+space));
  dnx = nx * (dx+space) - space;
  dny = ny * (dx+space) - space;
  xborder = (x - dnx) / 2;
  yborder = (y - dny) / 2;
  echo(x, y, dx, space, border);
  echo(dnx, dny, nx, ny, xborder, yborder);
  translate([xborder, yborder, -epsilon]) {
    for (iy = [0:ny-1])
      for (ix = [0:nx-1])
	translate([ix*(dx+space), iy*(dx+space), 0])
	  cube([dx, dx, z+2*epsilon]);
  }
}


module in_dough_board_frame(board_height=false) {
  zoffset = wall_thickness + (board_height ? space_below_board : 0);
  in_board_frame(outer_dims, board_dims, zoffset) children();
}

module DoughHeater_box(top) {
  u = 2.54;
  wall = wall_thickness;

  hside = 1.5;
  space = 1;
  border = 1.5;

  difference() {
    union() {
      project_box(outer_dims,
		  wall_thickness=wall_thickness,
		  gap=gap,
		  snaps_on_sides=false,
		  corner_radius=corner_radius,
		  top=top);

      if (top) {
	// Bumps for OLED, MOSFET, barrel jack bumps.
	translate(b1o) rounded_box(b1d, corner_radius);
	difference() {
	  translate(b2o) rounded_box(b2d, corner_radius);
	  hside = 1.5;
	  space = 1;
	  border = 2;
	  // Air holes RHS MOSFET bump
	  translate([b2o[0]+b2d[0]-wall-1, b2o[1], b2o[2]+1]) {
	    hexes_yz(b2d[1], b2d[2], wall+1+epsilon, hside=hside, space=space, border=border);
	  }
	  translate([b2o[0]-1, b2o[1], b1o[2]+b1d[2]+1]) {
	    hexes_yz(b2d[1], b2d[2] - b1d[2], wall+1+epsilon,
		     hside=hside, space=space, border=border);
	  }
	}
	translate(b3o) rounded_box(b3d, corner_radius);

	// Screw-tabs for mounting on the cooler.
	screw_tab_d=10;
	translate([outer_dims[0]/2, epsilon, 0])
	  rotate(180,Z) screw_tab();
	translate([outer_dims[0]/2, outer_dims[1]-epsilon, 0])
	  rotate(0,Z) screw_tab();
	
      } else {
	in_dough_board_frame() at_corners(board_dims, mount_offset)
	  //	  screw_mount(space_below_board, 3, 1.5);
	  screw_mount(space_below_board, 2.5, 2.8/2);
      }

    }
    // Cut outs.
    if (top) {
      // Negative space of OLED, MOSFET, barrel jack bumps.
      translate(b1o+[wall, wall, -epsilon]) rounded_box(b1d-wall*[2, 2, 1], corner_radius);
      translate(b2o+[wall, wall, -epsilon]) rounded_box(b2d-wall*[2, 2, 1], corner_radius);
      translate(b3o+[wall, wall, -epsilon]) rounded_box(b3d-wall*[2, 2, 1], corner_radius);

      translate([b2o[0], b2o[1], b2o[2]+b2d[2]-wall-1]) {
	hexes_xy(b2d[0], b2d[1], wall+2, hside=hside, space=space, border=border);
      }

      in_dough_board_frame() doughheater_to_oled_frame()
	translate([0, 3.5, -10]) cube([12, 38-9, 30]);

      translate(Z*(space_below_board+5)) {
	// jst
	translate([21*u, 4*u, 0]) cube([8*u, 3*u, 20]);
	// air holes for temp sensor
	translate([25*u, 0.5*u, 0]) holes_xy(3*u, 3.5*u, 5,
					     temp_air_hole_space, temp_air_hole_sep, 0);
	
	// heater barrel connector
	translate([29*u, 7.5*u, 0]) cube([10*u, 5.5*u, 10]);
	// power supply barrel connector
	translate([29*u, 10*u, 0]) cube([10*u, 5.5*u, 10]);
      }

      // Hole for push-button
      translate([4.5*u, 3.3+wall, 5]) cube([2.9*u, 2.7*u, 8]);
    }
  }
}
