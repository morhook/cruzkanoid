#!/bin/bash

dosbox-x -c "mount c $(pwd)" -c "c:" -c "call ADDPATH.BAT" -c "BUILD_R.BAT" -fs
