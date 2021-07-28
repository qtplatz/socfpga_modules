#-------------------------------------------------------------------------------
#-- Title      : Git describe to revision string conversion
#-- Project    : infiTOF delay/pulse generator; time event io
#-- Author     : Toshinobu Hondo, Ph.D.
#-- Copyright (C) 2018 MS-Cheminfomatics LLC
#

proc evaluate_revision_number { input_file actual_revision } {
    if { [catch {
        set inf [open $input_file ]
        while { -1 != [gets $inf line] } {
            #puts $line
            if {[regexp {^[ \t]*revision_number[ \t]*=[ \t]*32'h([0-9A-Fa-f]*);$} $line match revision_number]} {
                break
            }
        }
    } res ] } {
        return -code error $res
    } else {
        if { $revision_number == $actual_revision } {
            puts "== actual (git) revision is $actual_revision that is identical to $revision_number"
            return 1
        } else {
            puts "== actual (git) revision is $actual_revision that is NOT identical to $revision_number"
            return 0
        }
    }
}

proc create_revision_verilog { input_file actual_revision } {
    if { [catch {
        set fh [open "revision.v" w ]
        puts $fh "module revision ( output reg \[31:0\] revision_number, output reg \[31:0\] model_number );"
        puts $fh "  parameter MODEL_NUMBER = 0;"
        puts $fh "  always @* begin"
        puts $fh "    revision_number = 32'h${actual_revision};"
        puts $fh "    model_number = MODEL_NUMBER;"
        puts $fh "  end"
        puts $fh "endmodule // revision"
        close $fh
    } res ] } {
        return -code error $res
    } else {
        return 1
    }
}

if {[catch {exec git describe --dirty} describe] == 0} {
    set tweak 0
    regexp {^v([0-9]+)\.([0-9]+)\.([0-9]+)-?(.*)} $describe match major minor patch trailer
    #regexp {^v([0-9]+)\.([0-9]+)\.([0-9]+)-([0-9]+)-(.*)} $describe match major minor patch tweak hash
    regexp {^([0-9]+)} $trailer match tweak
    puts $describe
    puts "trailer= $trailer"
    puts "major= $major"
    puts "minor= $minor"
    puts "patch= $patch"
    puts "tweak= $tweak"
    if {[regexp {^.*dirty} $trailer] } {
        set major [expr $major | 0x80]
    }
    set revision [format %02x%02x%02x%02x $major $minor $patch $tweak]
} else {
    set revision 0
}

puts "$describe ==> $revision"

if { [catch {evaluate_revision_number "revision.v" $revision } res ] } {
    post_message -type critical_warning "Could not evaluate revision number: $res"
    if { [catch {create_revision_verilog "revision.v" $revision } res ] } {
        puts "revision.v cannot be created"
    } else {
        puts "The file revision.v was force created"
    }
} else {
    puts "evaluation success $res"
    if { $res } {
        puts "revision.v is clean."
    } else {
        file rename -force "revision.v" "revision.v.orig"
        if { [catch {create_revision_verilog "revision.v" $revision } res ] } {
            post_message -type critical_warning "Could not create revision.v: $res"
        } else {
            puts "revision.v updated."
        }
    }
}
