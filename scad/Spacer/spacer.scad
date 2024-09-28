// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

// Spacers for supporting the OLED board cantilevered via a header from a parent board.

// These are square so you can keep them from turning when screwing an M3 screw into them
//  through the PCB from the back.

module spacer(d, l, w=1) {
  r = d / 2;
  difference()  {
    $fn=40;
    out_d = 2*(r+w);
    translate(-out_d/2*[1,1,0]) cube([out_d, out_d, l]);
    translate([0, 0, -1]) cylinder(l+2, r, r);
  }
}

nrows = 1;
ncols = 1;

for (r = [0:nrows-1]) {
  for (c = [0:ncols-1]) {
    translate(6*[r,c,0]) spacer(3, 11.5);
  }
 }
