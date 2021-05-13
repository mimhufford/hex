#include <stdio.h>
#include "raylib.h"
#include "types.h"
#include "font.inl"

struct {
    const s32 default_size = 32;
    Font font = {}; // will be initialised at startup
    s32 height = 0; // will be initialised on font load
    s32 width = 0;  // will be initialised on font load
} font;

struct {
    byte *bytes = nullptr;
    u32 byte_count = 0;
} loaded_file;

struct {
    s32 row = 0;
    s32 col = 0;
} cursor;

struct {
    const s32 max_rows = 20;
    s32 row_offset = 0;
    f32 scroll_offset_buffer = 0;
} view;

struct {
    const char hex_upper[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    const char hex_lower[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char addresses[20][8];  // NOTE: first dimension must match view.max_rows
    char asciis[20][16][2]; // NOTE: first dimension must match view.max_rows
    char bytes[20][16][3];  // NOTE: first dimension must match view.max_rows
    char values[10][30];
    s32 window_padding;
    s32 width = 1060;
    s32 height = 900;
    s32 details_panel_top;
} canvas;

void UpdateAddressTexts()
{
    for (s32 i = 0; i < view.max_rows; i++)
    {
        s32 line_base_address = i * 16 + view.row_offset * 16;
        sprintf(canvas.addresses[i], "%07x", line_base_address);
    }

    canvas.addresses[cursor.row][6] = canvas.hex_lower[cursor.col];
}

void Scroll(s32 amount)
{
    view.row_offset += amount;

    // clamp to end of bytes
    s32 total_row_count = (loaded_file.byte_count - 1) / 16;
    if (view.row_offset > total_row_count) view.row_offset = total_row_count;

    // clamp to start of bytes
    if (view.row_offset < 0) view.row_offset = 0;

    UpdateAddressTexts();

    // generate bytes and ascii text
    for (s32 i = 0; i < view.max_rows; i++)
    {
        for (s32 c = 0; c < 16; c++)
        {
            s32 line_base_address = i * 16 + view.row_offset * 16;
            s32 index = line_base_address + c;

            canvas.bytes[i][c][0] = canvas.hex_upper[(loaded_file.bytes[index] & 0xF0) >> 4];
            canvas.bytes[i][c][1] = canvas.hex_upper[(loaded_file.bytes[index] & 0x0F) >> 0];

            if (index < loaded_file.byte_count)
            {
                canvas.asciis[i][c][0] = (loaded_file.bytes[index] >= 32 && loaded_file.bytes[index] <= 126) ? loaded_file.bytes[index] : '.';
            }
            else
            {
                canvas.asciis[i][c][0] = 0;
            }
        }
    }

    void MoveCursor(s32, s32);
    MoveCursor(0, 0); // trigger an int/uint/etc text update
}

void SetFontSize(s32 height)
{
    UnloadFont(font.font);
    if (height <  8) height = 8;
    if (height > 48) height = 48;
    font.font = LoadFontFromMemory(".ttf", font_bytes, font_bytes_count, height, 0, 0);
    auto size = MeasureTextEx(font.font, " ", height, 0);
    font.height = size.y;
    font.width = size.x;

    // recalculate panel positions and window size
    canvas.window_padding = font.height * 0.5f;
    canvas.details_panel_top = canvas.window_padding + font.height * 20;
    canvas.width = canvas.window_padding * 2 + font.width * 66;
    canvas.height = canvas.window_padding * 2 + font.height * 25;
    SetWindowSize(canvas.width, canvas.height);
}

void MoveCursor(s32 dx, s32 dy)
{
    s32 max_row_at_current_scroll = (loaded_file.byte_count-1) / 16 - view.row_offset;
    if (max_row_at_current_scroll >= view.max_rows) max_row_at_current_scroll = view.max_rows - 1;

    cursor.col += dx;
    if (cursor.col < 0)
    {
        if (cursor.row == 0 && view.row_offset == 0)
        {
            cursor.col = 0;
        }
        else
        {   // gone off left edge, loop back to end of previous row 
            dy -= 1;
            cursor.col = 15;
        }
    }
    if (cursor.col > 15)
    {
        if (cursor.row >= max_row_at_current_scroll)
        {
            cursor.col = 15;
        }
        else
        {   // gone off right edge, loop to start of next row 
            dy += 1;
            cursor.col = 0;
        }
    }

    cursor.row += dy;
    if (cursor.row < 0)
    {
        cursor.row = 0;
        if (view.row_offset > 0) Scroll(-1);
    }
    if (cursor.row > max_row_at_current_scroll)
    {
        cursor.row = max_row_at_current_scroll;
        if (cursor.row + view.row_offset < (loaded_file.byte_count-1) / 16) Scroll(1);
    }

    s32 byte_index = cursor.row * 16 + view.row_offset * 16 + cursor.col;

    while (byte_index >= loaded_file.byte_count)
    {
        byte_index -= 1;
        cursor.col -= 1;
    }
    
    s32 bytes_left = loaded_file.byte_count - byte_index;

    // generate value strings if there are enough bytes left to do so
    void *data = &loaded_file.bytes[byte_index];
    if (bytes_left >= 1) sprintf(canvas.values[0], "  int8: %d",   *(( s8*)data));
    else                 sprintf(canvas.values[0], "  int8: -");
    if (bytes_left >= 2) sprintf(canvas.values[1], " int16: %d",   *((s16*)data));
    else                 sprintf(canvas.values[1], " int16: -");
    if (bytes_left >= 4) sprintf(canvas.values[2], " int32: %d",   *((s32*)data));
    else                 sprintf(canvas.values[2], " int32: -");
    if (bytes_left >= 8) sprintf(canvas.values[3], " int64: %lld", *((s64*)data));
    else                 sprintf(canvas.values[3], " int64: -");
    if (bytes_left >= 4) sprintf(canvas.values[4], "   f32: %e",   *((f32*)data));
    else                 sprintf(canvas.values[4], "   f32: -");
    if (bytes_left >= 1) sprintf(canvas.values[5], " uint8: %u",   *(( u8*)data));
    else                 sprintf(canvas.values[5], " uint8: -");
    if (bytes_left >= 2) sprintf(canvas.values[6], "uint16: %u",   *((u16*)data));
    else                 sprintf(canvas.values[6], "uint16: -");
    if (bytes_left >= 4) sprintf(canvas.values[7], "uint32: %u",   *((u32*)data));
    else                 sprintf(canvas.values[7], "uint32: -");
    if (bytes_left >= 8) sprintf(canvas.values[8], "uint64: %llu", *((u64*)data));
    else                 sprintf(canvas.values[8], "uint64: -");
    if (bytes_left >= 8) sprintf(canvas.values[9], "   f64: %e",   *((f64*)data));
    else                 sprintf(canvas.values[9], "   f64: -");

    UpdateAddressTexts();
}

void HandleInput()
{
    // zoom in
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_EQUAL))
    {
        SetFontSize(font.height + 1);
    }

    // zoom out
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_MINUS))
    {
        SetFontSize(font.height - 1);
    }

    // restore default font size
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_ZERO))
    {
        SetFontSize(font.default_size);
    }

    if (IsKeyDown(KEY_LEFT_CONTROL))
    {
        if (IsKeyPressed(KEY_DOWN))
        {
            Scroll(1);
        }
        if (IsKeyPressed(KEY_UP))
        {
            Scroll(-1);
        }
        if (IsKeyPressed(KEY_LEFT))
        {
            if (cursor.col > 8) MoveCursor(8 - cursor.col, 0);
            else                MoveCursor(0 - cursor.col, 0);
        }
        if (IsKeyPressed(KEY_RIGHT))
        {
            if (cursor.col < 8) MoveCursor(8 - cursor.col, 0);
        }
    }

    if (IsKeyUp(KEY_LEFT_CONTROL))
    {
        if (IsKeyPressed(KEY_DOWN))
        {
            MoveCursor(0, 1);
        }
        if (IsKeyPressed(KEY_UP))
        {
            MoveCursor(0, -1);
        }
        if (IsKeyPressed(KEY_LEFT))
        {
            MoveCursor(-1, 0);
        }
        if (IsKeyPressed(KEY_RIGHT))
        {
            MoveCursor(1, 0);
        }
    }

    if (IsKeyPressed(KEY_HOME))
    {
        view.row_offset = 0;
        Scroll(0);
    }

    if (IsKeyPressed(KEY_END))
    {
        s32 total_row_count = (loaded_file.byte_count - 1) / 16;
        view.row_offset = total_row_count;
        Scroll(0);
    }

    if (IsKeyPressed(KEY_PAGE_UP))
    {
        Scroll(-view.max_rows);
    }

    if (IsKeyPressed(KEY_PAGE_DOWN))
    {
        Scroll(view.max_rows);
    }

    view.scroll_offset_buffer += -5 * GetMouseWheelMove();

    while (view.scroll_offset_buffer < -1.0f)
    {
        view.scroll_offset_buffer += 1.0f;
        Scroll(-1);
    }
    while (view.scroll_offset_buffer > 1.0f)
    {
        view.scroll_offset_buffer -= 1.0f;
        Scroll(1);
    }
}

void LoadFile(char *filepath)
{
    if (FileExists(filepath))
    {
        if (loaded_file.bytes)
        {
            // make sure to free any previously loaded file
            MemFree(loaded_file.bytes);
        }

        loaded_file.bytes = LoadFileData(filepath, &loaded_file.byte_count);
        view.row_offset = 0;
        cursor.row = 0;
        cursor.col = 0;
        Scroll(0); // trigger a refresh of the cached text
    }
    else
    {
        // @TODO: display error message
    }
}

void HandleDroppedFile()
{
    if (!IsFileDropped()) return;

    s32 file_count;
    char **file_paths = GetDroppedFiles(&file_count);

    if (file_count > 1)
    {
        // @TODO: display error message
    }
    else
    {
        LoadFile(file_paths[0]);
    }

    ClearDroppedFiles();
}

void main(s32 arg_count, char *args[])
{
    // If a file was dropped on to the executable, or provided on the command line
    if (arg_count == 2)
    {
        LoadFile(args[1]);
    }

    InitWindow(canvas.width, canvas.height, "Hex");
    SetTargetFPS(60);
    SetFontSize(font.default_size);

    while (!WindowShouldClose())
    {
        HandleDroppedFile();
        HandleInput();

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (loaded_file.bytes)
        {
            s32 max = 16 * view.max_rows;
            s32 offset = view.row_offset * 16;
            s32 bytes_left_to_display = (loaded_file.byte_count - offset);
            s32 count = bytes_left_to_display > max ? max : bytes_left_to_display;

            for (s32 i = 0; i < count; i++)
            {
                f32 y = canvas.window_padding + (i / 16) * (font.height);

                if (i % 16 == 0)
                {
                    bool selected = (i / 16 == cursor.row);
                    f32 x = canvas.window_padding;
                    DrawTextEx(font.font, canvas.addresses[i/16], {x, y}, font.height, 0, selected ? BLACK : GRAY);
                }

                {
                    bool selected = (i % 16 == cursor.col && i / 16 == cursor.row);
                    Color byte_colour = selected ? RAYWHITE : BLACK;

                    f32 x = canvas.window_padding + font.width * 8;
                    x += (i % 16) * (font.width * 2.5f) + ((i % 16 > 7) ? font.width : 0);
                    if (selected) DrawRectangle(x-2, y, font.width*2+4, font.height, GRAY);
                    DrawTextEx(font.font, canvas.bytes[i/16][i%16], {x, y}, font.height, 0, byte_colour);

                    x = canvas.window_padding + font.width * 50;
                    x += (i % 16) * font.width;
                    if (selected) DrawRectangle(x-1, y, font.width+2, font.height, GRAY);
                    DrawTextEx(font.font, canvas.asciis[i/16][i%16], {x, y}, font.height, 0, byte_colour);
                }
            }

            for (s32 i = 0; i < 10; i++)
            {
                f32 x = canvas.window_padding + (i > 4 ? font.width * 30 : 0);
                f32 y = canvas.details_panel_top + (i % 5) * font.height;
                DrawTextEx(font.font, canvas.values[i], {x, y}, font.height, 0, BLACK);
            };
        }
        else
        {
            auto text = "Drop a file here to display its contents...";
            auto size = MeasureTextEx(font.font, text, font.height, 0);
            f32 x = canvas.width / 2 - size.x / 2;
            f32 y = canvas.height / 2 - size.y / 2;
            DrawTextEx(font.font, text, {x, y}, font.height, 0, BLACK);
        }

        EndDrawing();
    }

    CloseWindow();
}