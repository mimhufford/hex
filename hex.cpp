#include <stdio.h>
#include "raylib.h"
#include "types.h"
#include "font.inl"

const s32 default_font_size = 32;
Font font = {}; // will be initialised at startup
s32 glyph_height = 0; // will be initialised on font load
s32 glyph_width = 0; // will be initialised on font load

u32 byte_count = 0;
byte *bytes = nullptr;

s32 selected_row = 0;
s32 selected_col = 0;

s32 max_rows = 20;
s32 row_offset = 0;
f32 scroll_offset_buffer = 0;

void Scroll(s32 amount)
{
    row_offset += amount;

    // clamp to end of bytes
    s32 total_row_count = (byte_count - 1) / 16;
    if (row_offset > total_row_count)
    {
        row_offset = total_row_count;
    }

    // clamp to start of bytes
    if (row_offset < 0)
    {
        row_offset = 0;
    }
}

void SetFontSize(s32 height)
{
    if (height <  8) height = 8;
    if (height > 48) height = 48;
    font = LoadFontFromMemory(".ttf", font_bytes, font_bytes_count, height, 0, 0);
    auto size = MeasureTextEx(font, " ", height, 0);
    glyph_height = size.y;
    glyph_width = size.x;
}

void HandleInput()
{
    // zoom in
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_EQUAL))
    {
        SetFontSize(glyph_height + 1);
    }

    // zoom out
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_MINUS))
    {
        SetFontSize(glyph_height - 1);
    }

    // restore default font size
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_ZERO))
    {
        SetFontSize(default_font_size);
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
    }

    if (IsKeyUp(KEY_LEFT_CONTROL))
    {
        if (IsKeyPressed(KEY_DOWN))
        {
            // @TODO: check col, might be moving to last column which could be shorter
            // @TODO: this does scroll down but it gets a bit messy at the end of the file
            s32 total_row_count = (byte_count - 1) / 16;
            if (selected_row == max_rows - 1 && row_offset < total_row_count)
            {
                Scroll(1);
            }
            selected_row = (selected_row < max_rows - 1) ? selected_row + 1 : selected_row;
        }
        if (IsKeyPressed(KEY_UP))
        {
            if (selected_row == 0 && row_offset > 0)
            {
                Scroll(-1);
            }
            selected_row = (selected_row > 0) ? selected_row - 1 : selected_row;
        }
        if (IsKeyPressed(KEY_LEFT))
        {
            // @TODO: loop back to previous row
            selected_col = (selected_col > 0) ? selected_col - 1 : selected_col;
        }
        if (IsKeyPressed(KEY_RIGHT))
        {
            // @TODO: loop around to next row
            selected_col = (selected_col < 15) ? selected_col + 1 : selected_col;
        }
    }

    if (IsKeyPressed(KEY_HOME))
    {
        row_offset = 0;
    }

    if (IsKeyPressed(KEY_END))
    {
        s32 total_row_count = (byte_count - 1) / 16;
        row_offset = total_row_count;
    }

    if (IsKeyPressed(KEY_PAGE_UP))
    {
        Scroll(-max_rows);
    }

    if (IsKeyPressed(KEY_PAGE_DOWN))
    {
        Scroll(max_rows);
    }

    scroll_offset_buffer += -5 * GetMouseWheelMove();

    while (scroll_offset_buffer < -1.0f)
    {
        scroll_offset_buffer += 1.0f;
        Scroll(-1);
    }
    while (scroll_offset_buffer > 1.0f)
    {
        scroll_offset_buffer -= 1.0f;
        Scroll(1);
    }
}

void LoadFile(char *filepath)
{
    if (FileExists(filepath))
    {
        if (bytes)
        {
            // make sure to free any previously loaded file
            MemFree(bytes);
        }

        bytes = LoadFileData(filepath, &byte_count);
        row_offset = 0;
        selected_row = 0;
        selected_col = 0;
    }
    else
    {
        // @TODO: display error message
    }
}

void HandleDroppedFile()
{
    if (!IsFileDropped())
        return;

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

    InitWindow(1060, 900, "Hex");
    SetTargetFPS(60);
    SetFontSize(default_font_size);

    while (!WindowShouldClose())
    {
        HandleDroppedFile();
        HandleInput();

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (bytes)
        {
            s32 max = 16 * max_rows;
            s32 offset = row_offset * 16;
            s32 bytes_left_to_display = (byte_count - offset);
            s32 count = bytes_left_to_display > max ? max : bytes_left_to_display;

            for (s32 i = 0; i < count; i++)
            {
                char hex_chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                char hex[3] = {};
                hex[0] = hex_chars[(bytes[i + offset] & 0xF0) >> 4];
                hex[1] = hex_chars[(bytes[i + offset] & 0x0F) >> 0];

                s32 horizontal_separator = 8;
                s32 vertical_spacing = 4;
                f32 x = glyph_width + (i % 16) * (glyph_width * 2.5f);
                x += (i % 16 > 7) ? horizontal_separator : 0;
                f32 y = glyph_width + (i / 16) * (glyph_height + vertical_spacing);

                if (i % 16 == 0)
                {
                    char address[8] = {};
                    sprintf(address, "%07x", i + offset);
                    DrawTextEx(font, address, {x, y}, glyph_height, 0, GRAY);

                    char text[17] = {};
                    for (s32 c = 0; c < 16; c++)
                    {
                        s32 index = c + i + offset;
                        if (index < byte_count)
                        {
                            text[c] = (bytes[index] >= 32 && bytes[index] <= 126) ? bytes[index] : '.';
                        }
                    }
                    DrawTextEx(font, text, {810, y}, glyph_height, 0, GRAY);
                }

                Color byte_colour = (i % 16 == selected_col && i / 16 == selected_row) ? RED : BLACK;
                DrawTextEx(font, hex, {(f32)x + 121, (f32)y}, glyph_height, 0, byte_colour);
            }

            char text[50] = {};
            f32 x = glyph_width;
            f32 y = 750;
            void *data = &bytes[selected_row * 16 + offset + selected_col];

            auto Print = [&]() {
                DrawTextEx(font, text, {x, y}, glyph_height, 0, BLACK);
                y += glyph_height;
            };

            sprintf(text, " int8: %d", *((s8 *)data));
            Print();
            sprintf(text, "int16: %d", *((s16 *)data));
            Print();
            sprintf(text, "int32: %d", *((s32 *)data));
            Print();
            sprintf(text, "int64: %lld", *((s64 *)data));
            Print();

            x = 463;
            y = 750;
            sprintf(text, " uint8: %u", *((u8 *)data));
            Print();
            sprintf(text, "uint16: %u", *((u16 *)data));
            Print();
            sprintf(text, "uint32: %u", *((u32 *)data));
            Print();
            sprintf(text, "uint64: %llu", *((u64 *)data));
            Print();

            // @TODO: display address, floats, maybe string?
        }

        EndDrawing();
    }

    CloseWindow();
}