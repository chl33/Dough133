// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

include <plug.scad>

plug();
translate(X*30) plug_right();
translate(X*60) plug_left();
translate(X*90) nut();
