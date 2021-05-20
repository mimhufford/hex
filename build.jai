#run {
    build_options := get_build_options();
    build_options.output_type = .NO_OUTPUT;
    set_build_options(build_options);

    workspace := compiler_create_workspace("Hex");
    build_options.output_type = .EXECUTABLE;
    build_options.output_executable_name = "hex";
    build_options.use_custom_link_command = true;
    user, system := compiler_get_command_line_arguments();
    for system {
        if it == "-release"  {
            set_optimization_level(*build_options, 2, 0);
            build_options.stack_trace = false;
        }
    }
    set_build_options(build_options, workspace);

    compiler_begin_intercept(workspace);
    font := read_entire_file("SourceCodePro-Regular.ttf");
    builder : String_Builder;
    init_string_builder(*builder);
    print_to_builder(*builder, "font_bytes :: u8.[");
    for cast([]u8)font print_to_builder(*builder, "%,", it);
    print_to_builder(*builder, "];");
    add_build_string(builder_to_string(*builder), workspace);
    add_build_file("hex.jai", workspace);

    while true {
        message := compiler_wait_for_message();
        if !message || message.workspace != workspace continue;
        if message.kind == .COMPLETE break;
        if message.kind == .COMPILATION_PHASE {
            phase_message := cast (*Message_Phase) message;
            if phase_message.phase == .READY_FOR_CUSTOM_LINK_COMMAND
                run_custom_link_command(message.workspace, phase_message);
        }
    }

    compiler_end_intercept(workspace);
}

run_custom_link_command :: (w: Workspace, m: *Message_Phase) {

    options := get_build_options(w);


    kit_root := find_windows_kit_root();
    vc_path, linker_path := find_visual_studio_in_a_ridiculous_garbage_way();
    if !kit_root    compiler_report(#file, #line, 0, "Unable to find Windows Kit root; can't compile.\n");
    if !vc_path     compiler_report(#file, #line, 0, "Unable to find Visual Studio runtime library folder; can't compile.\n");
    if !linker_path compiler_report(#file, #line, 0, "Could not find link.exe path; can't compile.\n");
    system_linker := tprint("%\\%", linker_path, "link.exe");

    arguments: [..] string;
    array_add(*arguments, system_linker);
    array_add(*arguments, "/nologo");

    // Object files.
    for m.compiler_generated_object_files array_add(*arguments, it);
    for m.support_object_files array_add(*arguments, it);

    // Target filename.
    target_filename := tprint("%1%2.exe", options.output_path, options.output_executable_name);
    array_add(*arguments, tprint("/OUT:%", target_filename));
    array_add(*arguments, "/MACHINE:AMD64");
    array_add(*arguments, "/INCREMENTAL:NO");
    array_add(*arguments, "/DEBUG");
    array_add(*arguments, "/OPT:REF");

    // Make a Windows application instead of a command line program.
    array_add(*arguments, "/SUBSYSTEM:WINDOWS");
    array_add(*arguments, "/ENTRY:wWinMain");
    array_add(*arguments, "/IGNORE:4216");

    // Create lib and exp file inside intermediate directory.
    array_add(*arguments, tprint("/IMPLIB:%1%2.lib", options.intermediate_path, path_strip_extension(options.output_executable_name)));

    array_add(*arguments, tprint("/libpath:\"%\"", vc_path));

    // kit_root does not have a slash at the end
    array_add(*arguments, tprint("/libpath:\"%/um/x64\"", kit_root));
    array_add(*arguments, tprint("/libpath:\"%/ucrt/x64\"", kit_root));

    array_add(*arguments, "-nodefaultlib");

    for m.libraries array_add(*arguments, it);

    success, exit_code, out, err := os_run_command(..arguments, capture_and_return_output=true);

    if (!success) || (exit_code != 0) {
        print("\n\n\e[1;31mError: Link step failed! (Exit code %).\n", exit_code);
        if err != ""  print("%\n", err);
        if out != ""  print("%\n", out);
        print("\e[0;36m");
    }
    else {
        displayed_something := false;
        for split(out, "\n") {
            line := trim(it, " \t\n\r");
            if line != "" && !starts_with(line, "Creating library") {
                if !displayed_something {
                    print("\n\n");
                    displayed_something = true;
                }
                print("%\n", line);
            }
        }
    }
    print("\e[0;36m");

    compiler_custom_link_command_is_complete(w);
}

#import "Basic";
#import "File";
#import "Compiler";
#import "String";
#import "System";
#import "Process";
#import "Windows_Resources";