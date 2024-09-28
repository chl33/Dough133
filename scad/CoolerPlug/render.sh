#! /bin/sh
to_stl() {
    local root="$1"
    openscad -o "${root}.stl" "${root}.scad"
}
for i in plug_left plug_right nut; do
    to_stl "$i"
done
