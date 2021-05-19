#run {
    w := compiler_create_workspace("Hex");

    options := get_build_options(w);
    options.output_executable_name = "hex";

    user, system := compiler_get_command_line_arguments();
    for system {
        if it == "-release"  {
            set_optimization_level(*options, 2, 0);
            options.stack_trace = false;
        }
    }
    set_build_options(options, w);

    compiler_begin_intercept(w);

    font := read_entire_file("SourceCodePro-Regular.ttf");
    builder : String_Builder;
    init_string_builder(*builder);
    print_to_builder(*builder, "font_bytes :: u8.[");
    for cast([]u8)font print_to_builder(*builder, "%,", it);
    print_to_builder(*builder, "];");
    add_build_string(builder_to_string(*builder), w);
    add_build_file("hex.jai", w);
    
    while true {
        message := compiler_wait_for_message();
        if !message || message.kind == .COMPLETE break;
    }

    compiler_end_intercept(w);

    // Don't build an executable for the compilation workspace
    options = get_build_options();
    options.output_type = .NO_OUTPUT;
    set_build_options(options);
}

#import "File";
#import "Basic";
#import "Compiler";