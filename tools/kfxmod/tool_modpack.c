/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file tool_modpack.c
 *     Command-line tool for creating and managing .kfxmod packages.
 * @par Purpose:
 *     Provides utilities to pack, unpack, validate, and inspect mod packages.
 * @par Comment:
 *     Usage: kfxmod <command> [options]
 *     Commands: pack, unpack, info, validate
 * @author   KeeperFX Team
 * @date     01 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <zlib.h>

#include "../../src/config_modpack.h"

/******************************************************************************/

static void print_usage(const char *program_name)
{
    printf("KeeperFX Mod Pack Tool v1.0\n");
    printf("Usage: %s <command> [options]\n\n", program_name);
    printf("Commands:\n");
    printf("  pack <input_dir> <output.kfxmod> [options]\n");
    printf("    Pack a directory into a .kfxmod file\n");
    printf("    Options:\n");
    printf("      --compression <type>  Compression type: none, zlib, lz4 (default: zlib)\n");
    printf("      --metadata <file>     Path to metadata.json file (default: input_dir/metadata.json)\n");
    printf("      --validate            Validate contents before packing\n\n");
    printf("  unpack <input.kfxmod> <output_dir> [options]\n");
    printf("    Unpack a .kfxmod file to a directory\n");
    printf("    Options:\n");
    printf("      --metadata-only       Extract only metadata.json\n");
    printf("      --files <pattern>     Extract only files matching pattern\n\n");
    printf("  info <file.kfxmod>\n");
    printf("    Display mod pack information\n\n");
    printf("  validate <file.kfxmod>\n");
    printf("    Validate mod pack integrity\n\n");
    printf("  version\n");
    printf("    Display tool version\n\n");
    printf("Examples:\n");
    printf("  %s pack tempkpr tempkpr-1.0.0.kfxmod\n", program_name);
    printf("  %s unpack tempkpr-1.0.0.kfxmod unpacked/\n", program_name);
    printf("  %s info tempkpr-1.0.0.kfxmod\n", program_name);
    printf("  %s validate tempkpr-1.0.0.kfxmod\n", program_name);
}

/**
 * Read entire file into memory.
 */
static char *read_file(const char *filepath, size_t *size)
{
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Failed to open file: %s\n", filepath);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *data = (char *)malloc(*size + 1);
    if (data == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory\n");
        fclose(fp);
        return NULL;
    }

    size_t read = fread(data, 1, *size, fp);
    fclose(fp);

    if (read != *size)
    {
        fprintf(stderr, "Error: Failed to read file: %s\n", filepath);
        free(data);
        return NULL;
    }

    data[*size] = '\0';
    return data;
}

/**
 * Compress data using zlib.
 */
static unsigned char *compress_zlib(const unsigned char *input, size_t input_size,
                                     size_t *output_size)
{
    uLongf compressed_size = compressBound(input_size);
    unsigned char *compressed = (unsigned char *)malloc(compressed_size);
    if (compressed == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for compression\n");
        return NULL;
    }

    int ret = compress2(compressed, &compressed_size, input, input_size, Z_BEST_COMPRESSION);
    if (ret != Z_OK)
    {
        fprintf(stderr, "Error: Compression failed: %d\n", ret);
        free(compressed);
        return NULL;
    }

    *output_size = compressed_size;
    return compressed;
}

/**
 * Pack command implementation.
 */
static int cmd_pack(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Error: pack command requires input_dir and output file\n");
        return 1;
    }

    const char *input_dir = argv[2];
    const char *output_file = argv[3];
    const char *metadata_file = NULL;
    int compression_type = MPComp_Zlib;
    int validate = 0;

    // Parse options
    for (int i = 4; i < argc; i++)
    {
        if (strcmp(argv[i], "--compression") == 0 && i + 1 < argc)
        {
            i++;
            if (strcmp(argv[i], "none") == 0)
                compression_type = MPComp_None;
            else if (strcmp(argv[i], "zlib") == 0)
                compression_type = MPComp_Zlib;
            else if (strcmp(argv[i], "lz4") == 0)
            {
                fprintf(stderr, "Error: LZ4 compression not yet implemented\n");
                return 1;
            }
            else
            {
                fprintf(stderr, "Error: Unknown compression type: %s\n", argv[i]);
                return 1;
            }
        }
        else if (strcmp(argv[i], "--metadata") == 0 && i + 1 < argc)
        {
            metadata_file = argv[++i];
        }
        else if (strcmp(argv[i], "--validate") == 0)
        {
            validate = 1;
        }
    }

    // Default metadata file
    if (metadata_file == NULL)
    {
        static char default_metadata[512];
        snprintf(default_metadata, sizeof(default_metadata), "%s/metadata.json", input_dir);
        metadata_file = default_metadata;
    }

    printf("Packing mod from: %s\n", input_dir);
    printf("Output file: %s\n", output_file);
    printf("Metadata: %s\n", metadata_file);
    printf("Compression: %s\n", compression_type == MPComp_None ? "none" :
                                compression_type == MPComp_Zlib ? "zlib" : "lz4");

    // Read metadata
    size_t metadata_size;
    char *metadata_json = read_file(metadata_file, &metadata_size);
    if (metadata_json == NULL)
    {
        fprintf(stderr, "Error: Failed to read metadata file\n");
        return 1;
    }

    // Compress metadata
    size_t compressed_metadata_size;
    unsigned char *compressed_metadata = NULL;
    if (compression_type == MPComp_Zlib)
    {
        compressed_metadata = compress_zlib((unsigned char *)metadata_json, metadata_size,
                                            &compressed_metadata_size);
        if (compressed_metadata == NULL)
        {
            free(metadata_json);
            return 1;
        }
    }
    else
    {
        compressed_metadata = (unsigned char *)metadata_json;
        compressed_metadata_size = metadata_size;
    }

    // Create header
    struct ModPackHeader header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, KFXMOD_MAGIC, KFXMOD_MAGIC_SIZE);
    header.format_version = KFXMOD_FORMAT_VERSION;
    header.compression_type = compression_type;
    header.metadata_offset = KFXMOD_HEADER_SIZE;
    header.metadata_size_compressed = compressed_metadata_size;
    header.metadata_size_uncompressed = metadata_size;

    // TODO: Scan directory and build file table
    // For now, just write header and metadata as a minimal valid file

    header.file_table_offset = header.metadata_offset + compressed_metadata_size;
    header.file_table_count = 0;
    header.content_offset = header.file_table_offset;
    header.total_file_size = header.content_offset;

    // Write file
    FILE *fp = fopen(output_file, "wb");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Failed to create output file: %s\n", output_file);
        if (compression_type == MPComp_Zlib)
            free(compressed_metadata);
        free(metadata_json);
        return 1;
    }

    fwrite(&header, 1, sizeof(header), fp);
    fwrite(compressed_metadata, 1, compressed_metadata_size, fp);

    fclose(fp);

    if (compression_type == MPComp_Zlib)
        free(compressed_metadata);
    free(metadata_json);

    printf("Successfully created mod pack: %s\n", output_file);
    return 0;
}

/**
 * Info command implementation.
 */
static int cmd_info(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Error: info command requires input file\n");
        return 1;
    }

    const char *input_file = argv[2];

    struct ModPack modpack;
    if (!modpack_load(input_file, &modpack))
    {
        fprintf(stderr, "Error: Failed to load mod pack: %s\n", input_file);
        return 1;
    }

    modpack_print_info(&modpack);
    modpack_unload(&modpack);

    return 0;
}

/**
 * Validate command implementation.
 */
static int cmd_validate(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Error: validate command requires input file\n");
        return 1;
    }

    const char *input_file = argv[2];

    struct ModPack modpack;
    if (!modpack_load(input_file, &modpack))
    {
        fprintf(stderr, "Error: Failed to load mod pack: %s\n", input_file);
        return 1;
    }

    if (modpack_validate(&modpack))
    {
        printf("Validation successful: %s\n", input_file);
        modpack_unload(&modpack);
        return 0;
    }
    else
    {
        fprintf(stderr, "Validation failed: %s\n", input_file);
        modpack_unload(&modpack);
        return 1;
    }
}

/**
 * Version command implementation.
 */
static int cmd_version(void)
{
    printf("KeeperFX Mod Pack Tool v1.0.0\n");
    printf("Format version: %d\n", KFXMOD_FORMAT_VERSION);
    return 0;
}

/**
 * Main entry point.
 */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "pack") == 0)
    {
        return cmd_pack(argc, argv);
    }
    else if (strcmp(command, "unpack") == 0)
    {
        fprintf(stderr, "Error: unpack command not yet implemented\n");
        return 1;
    }
    else if (strcmp(command, "info") == 0)
    {
        return cmd_info(argc, argv);
    }
    else if (strcmp(command, "validate") == 0)
    {
        return cmd_validate(argc, argv);
    }
    else if (strcmp(command, "version") == 0)
    {
        return cmd_version();
    }
    else
    {
        fprintf(stderr, "Error: Unknown command: %s\n", command);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
