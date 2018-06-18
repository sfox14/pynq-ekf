# Vivado script to extract block design .tcl

set XPR_ROOT [lindex $argv 0]; # .xpr file
set BD_ROOT [lindex $argv 1]; # .bd file
set DEST_TCL [lindex $argv 2]; # destination of .tcl

open_project $XPR_ROOT
open_bd_design $BD_ROOT
write_bd_tcl -force $DEST_TCL
q
