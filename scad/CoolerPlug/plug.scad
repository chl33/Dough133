// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <BOSL/constants.scad>
include <MCAD/units.scad>
use <BOSL/shapes.scad>
use <BOSL/threading.scad>

hole_diam = 18;
hole_len = 22;
cap_depth = 5;
nut_depth = 8;
inner_diam = 9;
thread_depth = 1;

gap = 0.8;

screw_len = hole_len + cap_depth + nut_depth;
cap_diam = hole_diam + 5;

module right_side() {
  translate(-epsilon*Z) rotate(45, Z) union() {
    cube([20, 20, screw_len + 2*epsilon]);
    translate([10, 10, screw_len / 2]) {
      prismoid(size1=[20+3, 20+3], size2=[20, 20], h=3);
      translate(-3*Z) prismoid(size1=[20, 20], size2=[20+3, 20+3], h=3);
    }
  }
}

module plug() {
  difference() {
    union() {
      cylinder(cap_depth, cap_diam/2, cap_diam/2);
      translate(Z*screw_len/2)
	trapezoidal_threaded_rod(d=hole_diam-2*thread_depth, l=screw_len, pitch=8,
				 thread_depth=thread_depth,
				 thread_angle=45, left_handed=true, $fa=2, $fs=1);
    }
    translate(-Z*epsilon) {
      cylinder(screw_len + 2*epsilon, inner_diam/2, inner_diam/2, $fn=20);
    }
  }
}

module plug_left() {
  difference() {
    plug();
    right_side();    
  }
}

module plug_right() {
  intersection() {
    plug();
    right_side();    
  }
}

module nut() {
  nut_diam = hole_diam + 8;
  difference() {
    cyl(l=nut_depth + 2*epsilon, d=nut_diam, $fn=6);
      trapezoidal_threaded_rod(d=hole_diam-2*thread_depth+2*gap, l=screw_len, pitch=8,
			       thread_depth=1, thread_angle=45, left_handed=true, $fa=2, $fs=1);
  }
}


//plug_lef();
//plug_right();
//plug();
//nut();
