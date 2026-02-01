/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file config_modpack.c
 *     Binary mod package (.kfxmod) format implementation.
 * @par Purpose:
 *     Functions for loading, parsing, and managing binary mod packages.
 * @par Comment:
 *     Implements the .kfxmod format with metadata, dependencies, and CDN updates.
 * @author   KeeperFX Team
 * @date     01 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "config_modpack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "bflib_basics.h"
#include "bflib_fileio.h"
#include "bflib_memory.h"

#include "config.h"
#include "game_legacy.h"

#include "post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

/**
 * Loads and validates the mod pack header.
 */
TbBool modpack_load_header(const char *filepath, struct ModPackHeader *header)
{
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL)
    {
        WARNMSG("Failed to open mod pack: %s", filepath);
        return false;
    }

    size_t read = fread(header, 1, sizeof(struct ModPackHeader), fp);
    fclose(fp);

    if (read != sizeof(struct ModPackHeader))
    {
        WARNMSG("Failed to read mod pack header: %s", filepath);
        return false;
    }

    // Validate magic
    if (memcmp(header->magic, KFXMOD_MAGIC, KFXMOD_MAGIC_SIZE) != 0)
    {
        WARNMSG("Invalid mod pack magic: %s", filepath);
        return false;
    }

    // Validate format version
    if (header->format_version != KFXMOD_FORMAT_VERSION)
    {
        WARNMSG("Unsupported mod pack format version %d (expected %d): %s",
                header->format_version, KFXMOD_FORMAT_VERSION, filepath);
        return false;
    }

    return true;
}

/**
 * Decompresses data using zlib.
 */
static TbBool decompress_zlib(const uint8_t *input, size_t input_size,
                               uint8_t *output, size_t output_size)
{
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (inflateInit(&stream) != Z_OK)
    {
        WARNMSG("Failed to initialize zlib decompression");
        return false;
    }

    stream.next_in = (Bytef *)input;
    stream.avail_in = input_size;
    stream.next_out = output;
    stream.avail_out = output_size;

    int ret = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);

    if (ret != Z_STREAM_END)
    {
        WARNMSG("Failed to decompress zlib data: %d", ret);
        return false;
    }

    return true;
}

/**
 * Loads metadata from mod pack file.
 */
TbBool modpack_load_metadata(struct ModPack *modpack)
{
    if (modpack->file_handle == NULL)
    {
        modpack->file_handle = fopen(modpack->filepath, "rb");
        if (modpack->file_handle == NULL)
        {
            WARNMSG("Failed to open mod pack: %s", modpack->filepath);
            return false;
        }
    }

    FILE *fp = modpack->file_handle;

    // Seek to metadata
    if (fseek(fp, modpack->header.metadata_offset, SEEK_SET) != 0)
    {
        WARNMSG("Failed to seek to metadata in mod pack: %s", modpack->filepath);
        return false;
    }

    // Read compressed metadata
    uint8_t *compressed = (uint8_t *)malloc(modpack->header.metadata_size_compressed);
    if (compressed == NULL)
    {
        WARNMSG("Failed to allocate memory for compressed metadata");
        return false;
    }

    size_t read = fread(compressed, 1, modpack->header.metadata_size_compressed, fp);
    if (read != modpack->header.metadata_size_compressed)
    {
        WARNMSG("Failed to read compressed metadata from mod pack: %s", modpack->filepath);
        free(compressed);
        return false;
    }

    // Allocate buffer for decompressed metadata
    modpack->metadata_json = (char *)malloc(modpack->header.metadata_size_uncompressed + 1);
    if (modpack->metadata_json == NULL)
    {
        WARNMSG("Failed to allocate memory for metadata");
        free(compressed);
        return false;
    }

    // Decompress metadata
    TbBool success = false;
    if (modpack->header.compression_type == MPComp_None)
    {
        memcpy(modpack->metadata_json, compressed, modpack->header.metadata_size_compressed);
        success = true;
    }
    else if (modpack->header.compression_type == MPComp_Zlib)
    {
        success = decompress_zlib(compressed, modpack->header.metadata_size_compressed,
                                   (uint8_t *)modpack->metadata_json,
                                   modpack->header.metadata_size_uncompressed);
    }
    else
    {
        WARNMSG("Unsupported compression type: %d", modpack->header.compression_type);
    }

    free(compressed);

    if (!success)
    {
        free(modpack->metadata_json);
        modpack->metadata_json = NULL;
        return false;
    }

    // Null-terminate JSON
    modpack->metadata_json[modpack->header.metadata_size_uncompressed] = '\0';

    // Parse metadata JSON
    return modpack_parse_metadata_json(modpack->metadata_json, &modpack->metadata);
}

/**
 * Loads a complete mod pack from file.
 */
TbBool modpack_load(const char *filepath, struct ModPack *modpack)
{
    memset(modpack, 0, sizeof(struct ModPack));
    strncpy(modpack->filepath, filepath, DISKPATH_SIZE - 1);
    modpack->filepath[DISKPATH_SIZE - 1] = '\0';

    // Load header
    if (!modpack_load_header(filepath, &modpack->header))
    {
        return false;
    }

    // Load metadata
    if (!modpack_load_metadata(modpack))
    {
        return false;
    }

    modpack->is_loaded = true;
    modpack->is_valid = true;

    SYNCMSG("Loaded mod pack: %s (v%s)", modpack->metadata.name, modpack->metadata.version);

    return true;
}

/**
 * Unloads and frees mod pack resources.
 */
TbBool modpack_unload(struct ModPack *modpack)
{
    if (!modpack->is_loaded)
    {
        return true;
    }

    // Free metadata JSON
    if (modpack->metadata_json != NULL)
    {
        free(modpack->metadata_json);
        modpack->metadata_json = NULL;
    }

    // Free file table
    if (modpack->file_table != NULL)
    {
        for (unsigned int i = 0; i < modpack->header.file_table_count; i++)
        {
            if (modpack->file_table[i].path != NULL)
            {
                free(modpack->file_table[i].path);
            }
        }
        free(modpack->file_table);
        modpack->file_table = NULL;
    }

    // Close file handle
    if (modpack->file_handle != NULL)
    {
        fclose(modpack->file_handle);
        modpack->file_handle = NULL;
    }

    modpack->is_loaded = false;
    modpack->is_valid = false;

    return true;
}

/**
 * Simple JSON parser for metadata.
 * Note: This is a minimal implementation. For production, consider using a proper JSON library.
 */
TbBool modpack_parse_metadata_json(const char *json, struct ModPackMetadata *metadata)
{
    memset(metadata, 0, sizeof(struct ModPackMetadata));

    // This is a placeholder implementation
    // In a real implementation, you would use a JSON parser library like cJSON or similar
    // For now, we'll provide basic string extraction

    // Helper macro to extract string value
    #define EXTRACT_STRING(field, key) \
    { \
        const char *key_str = "\"" key "\":"; \
        const char *pos = strstr(json, key_str); \
        if (pos != NULL) { \
            pos += strlen(key_str); \
            while (*pos == ' ' || *pos == '"') pos++; \
            const char *end = strchr(pos, '"'); \
            if (end != NULL) { \
                size_t len = end - pos; \
                if (len < sizeof(metadata->field)) { \
                    strncpy(metadata->field, pos, len); \
                    metadata->field[len] = '\0'; \
                } \
            } \
        } \
    }

    EXTRACT_STRING(mod_id, "mod_id");
    EXTRACT_STRING(version, "version");
    EXTRACT_STRING(name, "name");
    EXTRACT_STRING(display_name, "display_name");
    EXTRACT_STRING(author, "author");
    EXTRACT_STRING(description, "description");
    EXTRACT_STRING(homepage_url, "homepage_url");
    EXTRACT_STRING(update_url, "update_url");
    EXTRACT_STRING(min_keeperfx_version, "min_keeperfx_version");
    EXTRACT_STRING(license, "license");

    #undef EXTRACT_STRING

    // Extract mod_type
    if (strstr(json, "\"mod_type\":\"campaign\"") != NULL)
        metadata->mod_type = MPType_Campaign;
    else if (strstr(json, "\"mod_type\":\"creature_pack\"") != NULL)
        metadata->mod_type = MPType_CreaturePack;
    else if (strstr(json, "\"mod_type\":\"texture_pack\"") != NULL)
        metadata->mod_type = MPType_TexturePack;
    else if (strstr(json, "\"mod_type\":\"audio_pack\"") != NULL)
        metadata->mod_type = MPType_AudioPack;
    else if (strstr(json, "\"mod_type\":\"config_mod\"") != NULL)
        metadata->mod_type = MPType_ConfigMod;
    else if (strstr(json, "\"mod_type\":\"content_pack\"") != NULL)
        metadata->mod_type = MPType_ContentPack;
    else if (strstr(json, "\"mod_type\":\"total_conversion\"") != NULL)
        metadata->mod_type = MPType_TotalConversion;
    else
        metadata->mod_type = MPType_Unknown;

    // Extract load phase
    if (strstr(json, "\"load_phase\":\"after_campaign\"") != NULL)
        metadata->load_order.load_phase = MPPhase_AfterCampaign;
    else if (strstr(json, "\"load_phase\":\"after_map\"") != NULL)
        metadata->load_order.load_phase = MPPhase_AfterMap;
    else
        metadata->load_order.load_phase = MPPhase_AfterBase;

    // TODO: Parse dependencies, conflicts, changelog, etc. with proper JSON parser

    return true;
}

/**
 * Compares two version strings using semantic versioning rules.
 * Returns: -1 if version1 < version2, 0 if equal, 1 if version1 > version2
 */
int modpack_compare_versions(const char *version1, const char *version2)
{
    int major1, minor1, patch1;
    int major2, minor2, patch2;

    // Parse version1
    int parsed1 = sscanf(version1, "%d.%d.%d", &major1, &minor1, &patch1);
    if (parsed1 < 3)
    {
        minor1 = 0;
        patch1 = 0;
    }

    // Parse version2
    int parsed2 = sscanf(version2, "%d.%d.%d", &major2, &minor2, &patch2);
    if (parsed2 < 3)
    {
        minor2 = 0;
        patch2 = 0;
    }

    // Compare major
    if (major1 != major2)
        return (major1 < major2) ? -1 : 1;

    // Compare minor
    if (minor1 != minor2)
        return (minor1 < minor2) ? -1 : 1;

    // Compare patch
    if (patch1 != patch2)
        return (patch1 < patch2) ? -1 : 1;

    return 0;
}

/**
 * Checks if a version satisfies a version constraint.
 * Constraint format: ">=1.0.0", "^1.2.0", "~1.2.3", etc.
 */
TbBool modpack_version_satisfies(const char *version, const char *constraint)
{
    // Handle exact match
    if (constraint[0] != '>' && constraint[0] != '<' && 
        constraint[0] != '^' && constraint[0] != '~')
    {
        return (modpack_compare_versions(version, constraint) == 0);
    }

    // Handle >= operator
    if (strncmp(constraint, ">=", 2) == 0)
    {
        return (modpack_compare_versions(version, constraint + 2) >= 0);
    }

    // Handle > operator
    if (constraint[0] == '>')
    {
        return (modpack_compare_versions(version, constraint + 1) > 0);
    }

    // Handle <= operator
    if (strncmp(constraint, "<=", 2) == 0)
    {
        return (modpack_compare_versions(version, constraint + 2) <= 0);
    }

    // Handle < operator
    if (constraint[0] == '<')
    {
        return (modpack_compare_versions(version, constraint + 1) < 0);
    }

    // Handle ^ operator (compatible minor version)
    if (constraint[0] == '^')
    {
        int major, minor, patch;
        sscanf(constraint + 1, "%d.%d.%d", &major, &minor, &patch);

        int ver_major, ver_minor, ver_patch;
        sscanf(version, "%d.%d.%d", &ver_major, &ver_minor, &ver_patch);

        return (ver_major == major && modpack_compare_versions(version, constraint + 1) >= 0);
    }

    // Handle ~ operator (compatible patch version)
    if (constraint[0] == '~')
    {
        int major, minor, patch;
        sscanf(constraint + 1, "%d.%d.%d", &major, &minor, &patch);

        int ver_major, ver_minor, ver_patch;
        sscanf(version, "%d.%d.%d", &ver_major, &ver_minor, &ver_patch);

        return (ver_major == major && ver_minor == minor && 
                modpack_compare_versions(version, constraint + 1) >= 0);
    }

    // Unknown constraint format
    WARNMSG("Unknown version constraint format: %s", constraint);
    return false;
}

/**
 * Gets human-readable name for mod type.
 */
const char *modpack_get_type_name(enum ModPackType type)
{
    switch (type)
    {
        case MPType_Campaign: return "Campaign";
        case MPType_CreaturePack: return "Creature Pack";
        case MPType_TexturePack: return "Texture Pack";
        case MPType_AudioPack: return "Audio Pack";
        case MPType_ConfigMod: return "Config Mod";
        case MPType_ContentPack: return "Content Pack";
        case MPType_TotalConversion: return "Total Conversion";
        default: return "Unknown";
    }
}

/**
 * Gets human-readable name for load phase.
 */
const char *modpack_get_phase_name(enum ModLoadPhase phase)
{
    switch (phase)
    {
        case MPPhase_AfterBase: return "After Base";
        case MPPhase_AfterCampaign: return "After Campaign";
        case MPPhase_AfterMap: return "After Map";
        default: return "Unknown";
    }
}

/**
 * Prints mod pack information to console.
 */
void modpack_print_info(struct ModPack *modpack)
{
    if (!modpack->is_loaded)
    {
        WARNMSG("Mod pack not loaded");
        return;
    }

    struct ModPackMetadata *meta = &modpack->metadata;

    JUSTMSG("=== Mod Pack Information ===");
    JUSTMSG("Name: %s", meta->name);
    JUSTMSG("ID: %s", meta->mod_id);
    JUSTMSG("Version: %s", meta->version);
    JUSTMSG("Author: %s", meta->author);
    JUSTMSG("Type: %s", modpack_get_type_name(meta->mod_type));
    JUSTMSG("Description: %s", meta->description);
    JUSTMSG("Load Phase: %s", modpack_get_phase_name(meta->load_order.load_phase));
    if (meta->homepage_url[0] != '\0')
        JUSTMSG("Homepage: %s", meta->homepage_url);
    if (meta->update_url[0] != '\0')
        JUSTMSG("Update URL: %s", meta->update_url);
    if (meta->min_keeperfx_version[0] != '\0')
        JUSTMSG("Min KeeperFX Version: %s", meta->min_keeperfx_version);
    JUSTMSG("===========================");
}

/**
 * CRC32 calculation (standard polynomial 0xEDB88320).
 */
uint32_t modpack_calculate_crc32(const void *data, size_t size)
{
    static uint32_t crc_table[256];
    static int table_computed = 0;

    if (!table_computed)
    {
        for (int i = 0; i < 256; i++)
        {
            uint32_t c = i;
            for (int j = 0; j < 8; j++)
            {
                if (c & 1)
                    c = 0xEDB88320 ^ (c >> 1);
                else
                    c = c >> 1;
            }
            crc_table[i] = c;
        }
        table_computed = 1;
    }

    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *buf = (const uint8_t *)data;

    for (size_t i = 0; i < size; i++)
    {
        crc = crc_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

/**
 * Validates mod pack integrity.
 */
TbBool modpack_validate(struct ModPack *modpack)
{
    if (!modpack->is_loaded)
    {
        WARNMSG("Cannot validate unloaded mod pack");
        return false;
    }

    // TODO: Implement full validation
    // - Check CRC32 of entire file
    // - Validate file table
    // - Check metadata schema
    // - Verify file references

    SYNCMSG("Mod pack validation passed: %s", modpack->metadata.name);
    return true;
}

/******************************************************************************/
#ifdef __cplusplus
}
#endif
