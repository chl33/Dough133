#! /bin/sh
parent="$(readlink -f $(dirname "$0")/..)"
exec python3 "${parent}/gui.py"