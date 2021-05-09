#include <stdio.h>
#include "raylib.h"

using byte = unsigned char;
using uint = unsigned int;

size_t max_rows = 20;

uint byte_count = 0;
byte *bytes = nullptr;
int row_offset = 0;
float scroll_offset_buffer = 0;

void Scroll(int amount)
{
    row_offset += amount;

    // clamp to end of bytes
    int total_row_count = (byte_count - 1) / 16;
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

void HandleInput()
{
    if (IsKeyPressed(KEY_DOWN))
    {
        Scroll(1);
    }

    if (IsKeyPressed(KEY_UP))
    {
        Scroll(-1);
    }

    if (IsKeyPressed(KEY_HOME))
    {
        row_offset = 0;
    }

    if (IsKeyPressed(KEY_END))
    {
        int total_row_count = (byte_count - 1) / 16;
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

void HandleDroppedFile()
{
    if (!IsFileDropped())
        return;

    int file_count;
    char **file_paths = GetDroppedFiles(&file_count);

    if (file_count > 1)
    {
        // @TODO: display error message
    }
    else
    {
        if (bytes)
        {
            // make sure to free any previously loaded file
            MemFree(bytes);
        }

        bytes = LoadFileData(file_paths[0], &byte_count);
        row_offset = 0;
    }

    ClearDroppedFiles();
}

void main()
{
    InitWindow(956, 494, "Hex");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        HandleDroppedFile();
        HandleInput();

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (bytes)
        {
            size_t max = 16 * max_rows;
            size_t offset = row_offset * 16;
            size_t bytes_left_to_display = (byte_count - offset);
            size_t count = bytes_left_to_display > max ? max : bytes_left_to_display;

            for (size_t i = 0; i < count; i++)
            {
                char hex_chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                char hex[3] = {};
                hex[0] = hex_chars[(bytes[i + offset] & 0xF0) >> 4];
                hex[1] = hex_chars[(bytes[i + offset] & 0x0F) >> 0];

                int font_size = 20;
                int horizontal_separator = 8;
                int vertical_spacing = 4;
                int left_padding = 10;
                int top_padding = 10;
                int x = left_padding + (i % 16) * (font_size * 2);
                x += (i % 16 > 7) ? horizontal_separator : 0;
                int y = top_padding + (i / 16) * (font_size + vertical_spacing);

                if (i % 16 == 0)
                {
                    char address[8] = {};
                    sprintf(address, "%07x", (i + offset) / 16);
                    DrawText(address, x, y, font_size, GRAY);

                    char text[17] = {};
                    for (size_t c = 0; c < 16; c++)
                    {
                        size_t index = c + i + offset;
                        if (index < byte_count)
                        {
                            text[c] = (bytes[index] >= 32 && bytes[index] <= 126) ? bytes[index] : '.';
                        }
                    }
                    DrawText(text, 758, y, font_size, GRAY);
                }

                DrawText(hex, x + 100, y, font_size, BLACK);
            }
        }

        EndDrawing();
    }

    CloseWindow();
}