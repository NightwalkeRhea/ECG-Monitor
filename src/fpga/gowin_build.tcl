set script_dir    [file normalize [file dirname [info script]]]
set repo_dir      [file normalize [file join $script_dir .. ..]]
set project_name  ecg_monitor
set build_root    [file join $repo_dir build gowin]
set project_dir   [file join $build_root $project_name]
set reports_dir   [file join $repo_dir logs fpga]

file mkdir $build_root

create_project -name $project_name -dir $build_root -pn GW1NR-LV9QN88PC6/I5 -device_version C -force

add_file [file join $repo_dir src fpga ecg_top.v]
add_file [file join $repo_dir src fpga dsp_pipeline_qrs.v]
add_file [file join $repo_dir src fpga fir_filter.v]
add_file [file join $repo_dir src fpga qrs_actuator.v]
add_file [file join $repo_dir src fpga page_buffer_256B.v]
add_file [file join $repo_dir src fpga flash_page.v]
add_file [file join $repo_dir src fpga spi_shifter.v]
add_file [file join $repo_dir src fpga spi_slave_rx.v]
add_file [file join $repo_dir src fpga constraints.cst]
add_file [file join $repo_dir src fpga timing.sdc]

set_option -top_module ecg_top
set_option -output_base_name ecg_top
set_option -verilog_std sysv2017
set_option -use_mspi_as_gpio 0
set_option -use_sspi_as_gpio 0

run all

file mkdir $reports_dir

set report_files [list \
    [file join $project_dir impl gwsynthesis ecg_top_syn.rpt.html] \
    [file join $project_dir impl pnr ecg_top.log] \
    [file join $project_dir impl pnr ecg_top.rpt.txt] \
    [file join $project_dir impl pnr ecg_top.rpt.html] \
    [file join $project_dir impl pnr ecg_top.pin.html] \
    [file join $project_dir impl pnr ecg_top.power.html] \
    [file join $project_dir impl pnr ecg_top.tr.html] \
    [file join $project_dir impl pnr ecg_top_tr_cata.html] \
    [file join $project_dir impl pnr ecg_top_tr_content.html] \
]

foreach report_file $report_files {
    if {[file exists $report_file]} {
        file copy -force $report_file $reports_dir
    }
}

set manifest_path [file join $reports_dir gowin_reports.txt]
set manifest [open $manifest_path w]
puts $manifest "Generated: [clock format [clock seconds] -format {%Y-%m-%d %H:%M:%S}]"
puts $manifest "Project directory: $project_dir"
puts $manifest "Copied reports:"
foreach report_file $report_files {
    if {[file exists $report_file]} {
        puts $manifest "  [file tail $report_file]"
    }
}
close $manifest

exit
