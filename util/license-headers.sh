#! /bin/sh
set -e
cd "$(dirname "$0")"/..
license-header-checker -r -i .git,.github,.pio,venv \
	-a ./util/license_header.txt . h cpp scad
license-header-checker -r -i .git,.github,.pio,venv \
	-a ./util/license_header_py.txt . py
