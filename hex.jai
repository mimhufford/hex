// Font information used for layouting
font: *GUI.Dynamic_Font;
char_height: s32;
char_width:  s32;

// Window information
window               : Window_Creation.Window_Type;
window_width         : s32;
window_height        : s32;
window_padding       : s32;
window_needs_repaint := true;

// The contents of the currently loaded file
loaded_file : string;

// Colours constants used in the application
black :: Vector4.{0.153, 0.157, 0.133, 1};
white :: Vector4.{0.973, 0.973, 0.949, 1};
grey  :: Vector4.{0.459, 0.443, 0.369, 1};
red   :: Vector4.{0.976, 0.149, 0.447, 1};

// Cursor location info
row_offset := 0;
cursor_col := 0;
cursor_row := 0;

// Look-up table for converting decimal value to lower hexidecimal
lower_hex :: string.["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f"];

// How many bytes are we currently highlighting
visualisation := 0; // 0, 1, 2, 4, or 8

// Bake the font into the executable at compile time
font_bytes :: #run Compiler.add_global_data(xx read_entire_file("SourceCodePro-Regular.ttf"), .READ_ONLY);

// On windows launching the exe still opens a console window behind the
// graphical application, here we disable that at compile time
#run Windows_Resources.disable_runtime_console();

main :: () {
    args := get_command_line_arguments();

    file_path := ifx args.count > 1 then args[1] else args[0]; 
    load_file(file_path);

    font_size := 22;
    if args.count > 2 {
        val, ok := to_integer(args[2]);
        if ok font_size = val;
        font_size = clamp(font_size, 8, 50);
    }
    initialise_font(cast(s32)font_size);

    window_padding = char_width;
    window_width = window_padding * 2 + char_width * 74;
    window_height = window_padding * 2 + char_height * 26;

    // @NOTE: create_window does not expose parameters for the window style.
    // I changed WINDOWED_STYLE in windows.jai to WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME;
    window = Window_Creation.create_window(window_width, window_height, "Hex", background_color_rgb = <<(cast(*[3]float)(*black)));
    Windows.DragAcceptFiles(window, cast(Windows.BOOL) true);
    GUI.set_render_target(window);

    quit := false;
    while !quit {
        Input.update_window_events();
        
        for Input.events_this_frame {
            if it.type == .QUIT                  quit = true;
            if it.type == .DRAG_AND_DROP_FILES   load_file(it.files[0]);
            if it.type != .KEYBOARD              continue;
            if it.key_pressed && it.key_code == .ESCAPE      quit = true;
            if it.key_pressed && it.key_code == .SPACEBAR    switch_visualisation_mode();
            if it.key_pressed && it.key_code == .PAGE_UP     scroll(-1);
            if it.key_pressed && it.key_code == .PAGE_DOWN   scroll( 1);
            if it.key_pressed && it.key_code == .ARROW_DOWN  move_cursor( 0,  1);
            if it.key_pressed && it.key_code == .ARROW_UP    move_cursor( 0, -1);
            if it.key_pressed && it.key_code == .ARROW_LEFT  move_cursor(-1,  0);
            if it.key_pressed && it.key_code == .ARROW_RIGHT move_cursor( 1,  0);
        }

        mouse_wheel_buffer := Input.mouse_delta_z;
        while mouse_wheel_buffer >  100 { scroll(-1); mouse_wheel_buffer -= 100; }
        while mouse_wheel_buffer < -100 { scroll( 1); mouse_wheel_buffer += 100; }

        if !quit && window_needs_repaint {
            window_needs_repaint = false;
            draw_one_frame();
        }
        
        sleep_milliseconds(5);
    }
}

load_file :: (file : string) {
    loaded_file = read_entire_file(file);
    row_offset = 0;
    cursor_col = 0;
    cursor_row = 0;
    window_needs_repaint = true;
}

draw_one_frame :: () {
    GUI.update_window(window);
    GUI.clear_render_target(black.x, black.y, black.z, black.w);
    GUI.set_shader_for_color();

    GUI.draw_text(font, 0, 0, " ", white); // Hack to fix bug where first draw_text is skipped on first frame

    format_int := *context.print_style.default_format_int;
    format_int.base = 16;

    address_x := window_padding;
    byte_x := address_x + char_width * 8;
    char_x := byte_x + char_width * 50;
    byte_y := window_height - char_height - window_padding + 4;
    cursor_index := row_offset*16 + cursor_row*16 + cursor_col;

    for row: 0..19 {
        address := row_offset * 16 + row * 16;
        format_int.minimum_digits = 7;
        address_text := tprint("%", address);
        address_colour := grey;
        if row == cursor_row address_colour = white;
        if row == cursor_row address_text.data[6] = lower_hex[cursor_col][0];
        GUI.draw_text(font, address_x, byte_y, address_text, address_colour);
        format_int.minimum_digits = 2;

        for col: 0..15 {
            index := row_offset*16 + row*16 + col;
            if index >= loaded_file.count break;
            byte := loaded_file[index];
            text := tprint("%", byte);
            to_upper_in_place(text);
            colour := white;
            if !visualisation && index == cursor_index colour = red;
            if index >= cursor_index && index - cursor_index < visualisation colour = red;
            GUI.draw_text(font, byte_x, byte_y, text, colour);

            ascii : string = ---;
            ascii.data = loaded_file.data + index;
            ascii.count = 1;
            if ascii.data[0] < 32 || ascii.data[0] > 126 ascii = ".";
            GUI.draw_text(font, char_x, byte_y, ascii, colour);

            char_x += char_width;
            byte_x += char_width * 3;
            if col == 7 byte_x += char_width;
        }
        
        byte_x = address_x + char_width * 8;
        char_x = byte_x + char_width * 50;
        byte_y -= char_height;
    }

    byte_y -= char_height;
    left_x  := window_padding + char_width;
    centre_x := window_padding + char_width * 28;
    right_x := window_padding + char_width * 54;
    data := loaded_file.data + row_offset*16 + cursor_row*16 + cursor_col; 
    address := tprint("%", row_offset*16 + cursor_row*16 + cursor_col);
    to_upper_in_place(address);
    byte_info := tprint("Inspector for address 0x%", address);
    GUI.draw_text(font, left_x, byte_y, byte_info, white);
    format_int.base = 10;
    format_int.minimum_digits = 1;
    colour1 := ifx visualisation == 1 then red else white;
    colour2 := ifx visualisation == 2 then red else white;
    colour4 := ifx visualisation == 4 then red else white;
    colour8 := ifx visualisation == 8 then red else white;

    GUI.draw_text(font, left_x, byte_y - char_height * 1, tprint(" s8: %", <<cast(*s8) data), colour1);
    GUI.draw_text(font, left_x, byte_y - char_height * 2, ifx cursor_index > loaded_file.count - 2 then "s16: -" else tprint("s16: %", <<cast(*s16)data), colour2);
    GUI.draw_text(font, left_x, byte_y - char_height * 3, ifx cursor_index > loaded_file.count - 4 then "s32: -" else tprint("s32: %", <<cast(*s32)data), colour4);
    GUI.draw_text(font, left_x, byte_y - char_height * 4, ifx cursor_index > loaded_file.count - 8 then "s64: -" else tprint("s64: %", <<cast(*s64)data), colour8);

    GUI.draw_text(font, centre_x, byte_y - char_height * 1, tprint(" u8: %", <<cast(*u8) data), colour1);
    GUI.draw_text(font, centre_x, byte_y - char_height * 2, ifx cursor_index > loaded_file.count - 2 then "u16: -" else tprint("u16: %", <<cast(*u16)data), colour2);
    GUI.draw_text(font, centre_x, byte_y - char_height * 3, ifx cursor_index > loaded_file.count - 4 then "u32: -" else tprint("u32: %", <<cast(*u32)data), colour4);
    GUI.draw_text(font, centre_x, byte_y - char_height * 4, ifx cursor_index > loaded_file.count - 8 then "u64: -" else tprint("u64: %", <<cast(*u64)data), colour8);

    GUI.draw_text(font, right_x, byte_y - char_height * 1, tprint("bin: %", formatInt(<<cast(*u8) data, base=2, minimum_digits=8)), colour1);
    GUI.draw_text(font, right_x, byte_y - char_height * 3, ifx cursor_index > loaded_file.count - 4 then "f32: -" else tprint("f32: %", formatFloat(<<cast(*float32)data, mode=.SCIENTIFIC)), colour4);
    GUI.draw_text(font, right_x, byte_y - char_height * 4, ifx cursor_index > loaded_file.count - 8 then "f64: -" else tprint("f64: %", formatFloat(<<cast(*float64)data, mode=.SCIENTIFIC)), colour8);

    GUI.swap_buffers(window);
    reset_temporary_storage();
}

initialise_font :: (size: s32) {
    GUI.ensure_fonts_are_initted();
    font = GUI.get_font_at_size(font_bytes, size);
    char_height = cast(s32)(font.character_height * 1.3);
    char_width  = cast(s32)GUI.prepare_text(font, " ");
    assert(font != null);
}

scroll :: (offset: s8) {
    window_needs_repaint = true;
    row_offset += offset;
    row_offset = clamp(row_offset, 0, (loaded_file.count-1) / 16);
    
    // we could scroll to the end of the file with the cursor somewhere on the right,
    // but the last row of bytes might only have data on the left, so we need to make
    // sure cursor clamping happens, the easiest way is to pretend to move the cursor
    move_cursor(0, 0);
}

move_cursor :: (dx: s8, dy: s8) {
    window_needs_repaint = true;

    can_go_left := cursor_row > 0 || row_offset > 0;
    max_row_at_current_scroll := min((loaded_file.count-1) / 16 - row_offset, 19);

    cursor_col += dx;
    if cursor_col < 0 {
        if !can_go_left cursor_col = 0;
        else { dy -= 1; cursor_col = 15; }
    }
    if cursor_col > 15 {
        if cursor_row > max_row_at_current_scroll cursor_col = 15;
        else { dy += 1; cursor_col = 0; }
    } 

    cursor_row += dy;
    if cursor_row < 0 {
        cursor_row = 0;
        scroll(-1);
    }
    if cursor_row > max_row_at_current_scroll { 
        cursor_row = max_row_at_current_scroll; 
        if (cursor_row + row_offset < (loaded_file.count-1) / 16) scroll(1);
    }

    // We could be on the last row which could have fewer than
    // 16 bytes, so back up until we hit the last byte
    index := cursor_row*16 + row_offset*16 + cursor_col;
    while index >= loaded_file.count {
        index -= 1;
        cursor_col -= 1;
    }
}

switch_visualisation_mode :: () {
    window_needs_repaint = true;
    if visualisation == {
        case 0; visualisation = 1;
        case 1; visualisation = 2;
        case 2; visualisation = 4;
        case 4; visualisation = 8;
        case 8; visualisation = 0;
    }
}

// Common modules imported into global namespace
#import "Basic";
#import "File";
#import "Math";
#import "String";

// Less common modules imported with an alias
GUI :: #import "Simp";
Input :: #import "Input";
Compiler :: #import "Compiler";
Windows :: #import "Windows";
Windows_Resources :: #import "Windows_Resources";
Window_Creation :: #import "Window_Creation";