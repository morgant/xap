#!/bin/sh
# the next line restart wish \
exec wish "$0" "$@"

set inf_file	"setup.inf"

# default values
set inf_title				"GNU Setup"
set inf_info				"Setup: No INF-file found, using defaults.."
set inf_view				"README INSTALL"
set inf_makefile			"Makefile"
set inf_create_makefile_cmd	"configure"
set inf_compile_cmd			"make"
set inf_clean_cmd			"make clean"
set inf_install_cmd			"make install"
set inf_test_install_cmd	"make -n install"

set inf_makefile_options	"--prefix /usr/local/X11"

set width	620
set height	400
set x_off	310
set y_off	340
set top		".top"
set logo_data {\
R0lGODlhbAKQAaUAAAICBBEREU5OTmpqaIqKjKampMDAwM7OztjY2ODg4Obm5Orq7B0dHu7u
7AIChP7+BCsrKvLy9Pb29H19fjk5OWBgYPr6/I6OjJeXmERERFhYV7CwsHR0dP7+/KCgoIaG
hP//////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////yH+Dk1hZGUgd2l0
aCBHSU1QACH5BAEKACAALAAAAABsApABAAb+wI5wSCwaj8ikcslsOp/QqHRKrVqv2Kx2y+16
v+CweEwum8/otHrNbrvf8Lh8Tq/b7/i8fs/v+/+AgYKDhIWGh4iJiouMjY6PkJGSk5SVhw4O
AJqbnACYlqChoqOkipgPqKmqD5gbpa+wsbKzZZgJt7i5Ca20vb6/wMEdn0q8wsfIycqJtrq6
xsvR0tPUb8RJ0NXa29zdVc3OuNne5OXm3ddI4+fs7e6/mPHy8w6u7/f4+aLyBv3+BvLs6RtI
sCAifpj+BTTIsKFDPQgdKIwn8KHFixjPRJy4LqPHjyCpbPS3MKTJkyiTbIj3jyTFlDBjntxA
s6bNmzJz6tzJs6f+z59AkVmwIEFChKNIjxa1UGWoUyxPlzhlemWqmKFFkyZdGrQrPgseBgjo
pEmAAAwHFixoEEECVSYGCsj14AFBgwZupcSda7ftWyFzC3gwwDZvlMCDC//NYsGAWAZkKWT4
cEDBWr9eM5MzMJasZwAQJiBIcHnxEQ2dINxSwNa0EtSbMCRgXZRIJwG31hp2cjs3XtdSEgyA
/Dk1h9GlNSufZoBC8ecMBoxO8DuJhc6cBhg4QDoCcCPXOVE+gGCBdyEWOlHYPvq8k/Sc1pNP
4N4Kh+fFA3wYTfv78v+yDIDfgBAUwJ0CbSWBHScX9FPebkssyEEBG5C3QF7wbUIBhQb+IICg
f0RkqMmGG3T4YRUJODdgcQJsRxqEZ0wFIoA0npEidAzkyEAA+RHQIXUSHBEeWQxcUKJdQTIx
pCYVXIDBdgi6JUFqH1xQgAHUzSjElJxAUOWVWVKRAHGe6Zgjj59RUKGH9ZFxgQAamLXBXW3W
aKeNZHISAAVm9SlABmZBgCZZGFQI5F9ELchJBgQYipdUEiyYAQcfDGbXUBF0wkAFEzyZVpJS
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
fx766KWfnvrqrb8e++y135777r3/HsggAAA7
}

# read the inf-file to get some installation parameters
#
catch [source $inf_file]

#
wm title . $inf_title

image create photo logo -data $logo_data
canvas	$top -background white -border 0 -width $width -height $height
frame	$top.panel
button	$top.panel.compile -text "Compile" -command "m_compile $top"
button	$top.panel.install -text "Install" -command "install $top"
button	$top.panel.edit -text "Make $inf_makefile" -command \
			"m_makefile $top $inf_makefile $inf_create_makefile_cmd"
label	$top.panel.space -text "-"

foreach view $inf_view {
	set view_w $top.panel.v_$view
	button $view_w -text "$view" -command "m_view $top $view"
}
button	$top.quit  -text "Quit" -command exit

catch "$top create image 0 0 -image logo -anchor nw"
$top create window $x_off $y_off -window $top.panel
$top create text $x_off [expr $y_off+30] \
		-text "GNU Setup - Version 0.3, (c) Rasca, published under the GNU GPL"
$top create text $x_off 98 -text $inf_info -anchor n

$top create window $width $height -window $top.quit -anchor se

pack $top
pack $top.panel.edit -side left
pack $top.panel.compile -side left
pack $top.panel.install -side left
pack $top.panel.space -side left
foreach view $inf_view {
	set view_w $top.panel.v_$view
	pack $view_w -side left
}
event add <<Press>> <Enter> <ButtonPress-1> <ButtonRelease-1>
# end of main()


# view a file inside a text box
#
proc m_view {top what} {
	set box [newframe $top]
	set text [newtext $box]
	set id [open $what]
	button $box.close -text "Close" -command "destroy $box"

	pack $text
	pack $box.close -side right
	$text.text insert 1.0 [read $id]
	$text.text config -state disabled
	close $id
}

#
# run a command and display result in "out"
#
proc run {out command btn} {
	$out insert end "$command\n"
	if [catch {open "|$command |& cat"} input] {
		$out insert end $input\n
	} else {
		set label [$btn cget -text]
		fileevent $input readable \
			"cont $out $input $btn \"$label\" \"$command\""
		$btn config -text "Stop" \
			-command "stop \"$out\" $input $btn \"$label\" \"$command\""
	}
}

#
proc stop {out input btn label command} {
	catch {close $input}
	$btn config -text $label -command "run \"$out\" \"$command\" $btn"
}

proc cont {out input btn label cmd} {
	if [eof $input] {
		stop $out $input $btn "$label" "$cmd"
	} else {
		gets $input line
		$out insert end $line\n
		$out see end
	}
}


#
# run configure command and before read options from the named entry
#
proc run_opts {out cmd btn opts} {
	global inf_makefile_options
	set inf_makefile_options [$opts get]
	set command [concat $cmd $inf_makefile_options]
	$out insert end "$command\n"
	if [catch {open "|$command |& cat"} input] {
		$out insert end $input\n
	} else {
		set label [$btn cget -text]
		fileevent $input readable \
			"cont_opts $out $input $btn \"$label\" \"$cmd\" $opts"
		$btn config -text "Stop" \
			-command "stop_opts \"$out\" $input $btn \"$label\" \"$cmd\" $opts"
	}
}

#
# stop running configure
#
proc stop_opts {out input btn label command opts} {
	catch {close $input}
	$btn config -text $label -command "run_opts \"$out\" \"$command\" $btn $opts"
}


# continue running configure
#
proc cont_opts {out input btn label cmd opts} {
	if [eof $input] {
		stop_opts $out $input $btn "$label" "$cmd" $opts
	} else {
		gets $input line
		$out insert end $line\n
		$out see end
	}
}

#
# menu for creating a makefile
#
proc m_makefile {top makefile cmd} {
	global inf_makefile_options
	set box [newframe $top]
	set text [newtext $box]

	entry $box.opts -width 24
	$box.opts insert 0 $inf_makefile_options

	button $box.run -text "Run $cmd" -command \
			"run_opts $text.text \"$cmd\" $box.run $box.opts"

	button $box.help -text "Options:" -command \
			"run $text.text \"$cmd --help\" $box.help"

	button $box.edit -text "Edit $makefile" -command "edit $top $makefile"
	button $box.close -text "Close" -command "destroy $box"

	pack $text
	pack $box.run -side left
	pack $box.help -side left
	pack $box.opts -side left -fill x -expand true
	pack $box.edit -side left
	pack $box.close -side right
	return $box.run
}

#
proc save {text file} {
	set id [open $file w]
	puts $id [$text get 0.0 end]
	close $id
}

#
proc edit {top file} {
	set box [newframe $top]
	set text [newtext $box]
	set id [open $file]
	button $box.close -text "Close" -command "destroy $box"
	button $box.save -text "Save" -command "save $text.text $file"

	pack $text
	pack $box.save -side left
	pack $box.close -side right
	$text.text insert 1.0 [read $id]
	close $id
}

#
proc m_compile {top} {
	global inf_compile_cmd
	global inf_clean_cmd
	global inf_makefile
	global inf_create_makefile_cmd
	if {[file exists $inf_makefile] != 1} {
		set btn [m_makefile $top $inf_makefile $inf_create_makefile_cmd]
		tkwait visibility $btn
		event generate $btn <Enter> 
		event generate $btn <ButtonPress-1>
		event generate $btn <ButtonRelease-1>
		return
	}
	set box [newframe $top]
	set text [newtext $box]
	button $box.close -text "Close" -command "destroy $box"
	button $box.compile -text "Compile" -command \
				"run $text.text \"$inf_compile_cmd\" $box.compile"
	button $box.clean -text "Clean" -command \
				"run $text.text \"$inf_clean_cmd\" $box.clean"

	pack $text
	pack $box.compile -side left
	pack $box.clean -side left
	pack $box.close -side right
	return $box.compile
}

#
proc install {top} {
	global inf_install_cmd
	global inf_test_install_cmd
	global inf_makefile
	if {[file exists $inf_makefile] != 1} {
		set btn [m_compile $top]
		tkwait visibility $btn
		event generate $btn <Enter> 
		event generate $btn <ButtonPress-1>
		event generate $btn <ButtonRelease-1>
		return
	}
	set box [newframe $top]
	set out [newtext $box]
	button $box.close -text "Close" -command "destroy $box"
	button $box.install -text "Install" -command \
				"run \"$out.text\" \"$inf_install_cmd\" $box.install"
	button $box.test -text "Test" -command \
				"run \"$out.text\" \"$inf_test_install_cmd\" \ $box.test"
	pack $out
	pack $box.install -side left
	pack $box.test -side left
	pack $box.close -side right
}

# create a new container frame inside the canvas
#
proc newframe {top} {
	if {[winfo exists $top.f] == 1} {destroy $top.f}
	frame $top.f
	$top create window 310 215 -window $top.f
	return $top.f
}

#
#
proc newtext {top} {
	set box $top.text_box
	frame $box
	text $box.text -wrap word -yscrollcommand "$box.yscroll set" -height 13
	scrollbar $box.yscroll -command "$box.text yview" -orient vertical
	pack $box.text -side left
	pack $box.yscroll -side right -fill y
	return $box
}

