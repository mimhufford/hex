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
    char addresses[20][8]; // NOTE: first dimension must match view.max_rows
    s32 window_padding;
    s32 width = 1060;
    s32 height = 900;
    s32 details_panel_top;
} canvas;

void Scroll(s32 amount)
{
    view.row_offset += amount;

    // clamp to end of bytes
    s32 total_row_count = (loaded_file.byte_count - 1) / 16;
    if (view.row_offset > total_row_count) view.row_offset = total_row_count;

    // clamp to start of bytes
    if (view.row_offset < 0) view.row_offset = 0;

    // generate address text
    for (int i = 0; i < view.max_rows; i++)
    {
        sprintf(canvas.addresses[i], "%07x", i * 16 + view.row_offset * 16);
    }
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
    }

    if (IsKeyUp(KEY_LEFT_CONTROL))
    {
        if (IsKeyPressed(KEY_DOWN))
        {
            // @TODO: check col, might be moving to last column which could be shorter
            // @TODO: this does scroll down but it gets a bit messy at the end of the file
            s32 total_row_count = (loaded_file.byte_count - 1) / 16;
            if (cursor.row == view.max_rows - 1 && view.row_offset < total_row_count)
            {
                Scroll(1);
            }
            cursor.row = (cursor.row < view.max_rows - 1) ? cursor.row + 1 : cursor.row;
        }
        if (IsKeyPressed(KEY_UP))
        {
            if (cursor.row == 0 && view.row_offset > 0)
            {
                Scroll(-1);
            }
            cursor.row = (cursor.row > 0) ? cursor.row - 1 : cursor.row;
        }
        if (IsKeyPressed(KEY_LEFT))
        {
            // @TODO: loop back to previous row
            cursor.col = (cursor.col > 0) ? cursor.col - 1 : cursor.col;
        }
        if (IsKeyPressed(KEY_RIGHT))
        {
            // @TODO: loop around to next row
            cursor.col = (cursor.col < 15) ? cursor.col + 1 : cursor.col;
        }
    }

    if (IsKeyPressed(KEY_HOME))
    {
        view.row_offset = 0;
    }

    if (IsKeyPressed(KEY_END))
    {
        s32 total_row_count = (loaded_file.byte_count - 1) / 16;
        view.row_offset = total_row_count;
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
                    f32 x = canvas.window_padding;
                    DrawTextEx(font.font, canvas.addresses[i/16], {x, y}, font.height, 0, GRAY);
                }

                {
                    char hex_chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                    char hex[3] = {};
                    hex[0] = hex_chars[(loaded_file.bytes[i + offset] & 0xF0) >> 4];
                    hex[1] = hex_chars[(loaded_file.bytes[i + offset] & 0x0F) >> 0];
                    Color byte_colour = (i % 16 == cursor.col && i / 16 == cursor.row) ? RED : BLACK;
                    f32 x = canvas.window_padding + font.width * 8;
                    x += (i % 16) * (font.width * 2.5f) + ((i % 16 > 7) ? font.width : 0);
                    DrawTextEx(font.font, hex, {x, y}, font.height, 0, byte_colour);
                }

                if (i % 16 == 0)
                {
                    char text[17] = {};
                    for (s32 c = 0; c < 16; c++)
                    {
                        s32 index = c + i + offset;
                        if (index < loaded_file.byte_count)
                        {
                            text[c] = (loaded_file.bytes[index] >= 32 && loaded_file.bytes[index] <= 126) ? loaded_file.bytes[index] : '.';
                        }
                    }
                    f32 x = canvas.window_padding + font.width * 50;
                    DrawTextEx(font.font, text, {x, y}, font.height, 0, GRAY);
                }
            }

            char text[50] = {};
            f32 x = canvas.window_padding;
            f32 y = canvas.details_panel_top;
            void *data = &loaded_file.bytes[cursor.row * 16 + offset + cursor.col];

            auto Print = [&]() {
                DrawTextEx(font.font, text, {x, y}, font.height, 0, BLACK);
                y += font.height;
            };

            sprintf(text, "Address: %07x", offset + cursor.row * 16 + cursor.col);
            Print();

            sprintf(text, " int8: %d", *((s8 *)data));
            Print();
            sprintf(text, "int16: %d", *((s16 *)data));
            Print();
            sprintf(text, "int32: %d", *((s32 *)data));
            Print();
            sprintf(text, "int64: %lld", *((s64 *)data));
            Print();

            x = canvas.window_padding + font.width * 30;
            y = canvas.details_panel_top + font.height;
            sprintf(text, " uint8: %u", *((u8 *)data));
            Print();
            sprintf(text, "uint16: %u", *((u16 *)data));
            Print();
            sprintf(text, "uint32: %u", *((u32 *)data));
            Print();
            sprintf(text, "uint64: %llu", *((u64 *)data));
            Print();

            // @TODO: display floats, maybe string?
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