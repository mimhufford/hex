#include "raylib.h"

using byte = unsigned char;
using uint = unsigned int;

uint byte_count = 0;
byte *bytes = nullptr;
uint row_offset = 0;

void main()
{
    InitWindow(800, 600, "Hex");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        if (IsFileDropped())
        {
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

        if (IsKeyPressed(KEY_DOWN))
        {
            // @TODO: limit to end of bytes
            row_offset += 1;
        }

        if (IsKeyPressed(KEY_UP))
        {
            row_offset = (row_offset > 0) ? row_offset - 1 : row_offset;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (bytes)
        {
            size_t max_rows = 20;
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
                int horizontal_spacing = 0;
                int horizontal_separator = 20;
                int vertical_spacing = 4;
                int x = (i % 16) * (font_size * 2) + ((i % 16) > 7 ? horizontal_separator : 0);
                int y = i / 16 * (font_size + vertical_spacing);

                DrawText(hex, x, y, font_size, BLACK);
            }
        }

        EndDrawing();
    }

    CloseWindow();
}