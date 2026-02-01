/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file config_modpack.h
 *     Header file for config_modpack.c.
 * @par Purpose:
 *     Binary mod package (.kfxmod) format support.
 * @par Comment:
 *     Defines structures and functions for loading, parsing, and managing
 *     binary mod packages with metadata, dependencies, and version control.
 * @author   KeeperFX Team
 * @date     01 Feb 2026
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#ifndef DK_CFG_MODPACK_H
#define DK_CFG_MODPACK_H

#include "globals.h"
#include "bflib_basics.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
#define KFXMOD_MAGIC "KFXMOD\0\0"
#define KFXMOD_MAGIC_SIZE 8
#define KFXMOD_HEADER_SIZE 64
#define KFXMOD_FORMAT_VERSION 1
#define KFXMOD_MAX_DEPENDENCIES 32
#define KFXMOD_MAX_CONFLICTS 16
#define KFXMOD_MAX_TAGS 16
#define KFXMOD_MAX_SCREENSHOTS 10
#define KFXMOD_MAX_CHANGELOG_ENTRIES 20
#define KFXMOD_MAX_PATH_LENGTH 512
#define KFXMOD_VERSION_STRING_LEN 32
#define KFXMOD_URL_MAX_LEN 512
#define KFXMOD_DESCRIPTION_MAX_LEN 2048
#define KFXMOD_TAG_MAX_LEN 32
#define KFXMOD_REASON_MAX_LEN 256

/******************************************************************************/

/** Compression types supported by the mod pack format */
enum ModPackCompression {
    MPComp_None = 0,
    MPComp_Zlib = 1,
    MPComp_LZ4 = 2,
};

/** Mod types for categorization */
enum ModPackType {
    MPType_Unknown = 0,
    MPType_Campaign,
    MPType_CreaturePack,
    MPType_TexturePack,
    MPType_AudioPack,
    MPType_ConfigMod,
    MPType_ContentPack,
    MPType_TotalConversion,
};

/** Load phase determines when mod is applied */
enum ModLoadPhase {
    MPPhase_AfterBase = 0,
    MPPhase_AfterCampaign = 1,
    MPPhase_AfterMap = 2,
};

/** Flags for file entries */
enum ModPackFileFlags {
    MPFile_None = 0x00,
    MPFile_Directory = 0x01,
    MPFile_Executable = 0x02,
};

/** Header flags (reserved for future use) */
enum ModPackHeaderFlags {
    MPHdr_None = 0x00,
    MPHdr_Encrypted = 0x01,
    MPHdr_Signed = 0x02,
};

/******************************************************************************/

/**
 * Binary mod pack header structure (64 bytes, matches disk layout).
 */
#pragma pack(push, 1)
struct ModPackHeader {
    char magic[KFXMOD_MAGIC_SIZE];  // "KFXMOD\0\0"
    uint16_t format_version;         // Format version (1)
    uint16_t compression_type;       // Compression type
    uint32_t metadata_offset;        // Offset to metadata from file start
    uint32_t metadata_size_compressed;
    uint32_t metadata_size_uncompressed;
    uint32_t file_table_offset;      // Offset to file table
    uint32_t file_table_count;       // Number of file entries
    uint32_t content_offset;         // Offset to file content
    uint32_t total_file_size;        // Total size of .kfxmod file
    uint32_t crc32_checksum;         // CRC32 of entire file
    uint32_t flags;                  // Header flags
    uint8_t reserved[16];            // Reserved, must be zero
};
#pragma pack(pop)

/**
 * File table entry structure (variable size).
 */
struct ModPackFileEntry {
    uint16_t path_length;
    char *path;                      // Relative path (UTF-8)
    uint64_t file_offset;            // Offset from content_offset
    uint64_t compressed_size;
    uint64_t uncompressed_size;
    uint32_t crc32;
    uint32_t flags;
    uint32_t timestamp;              // Unix epoch
};

/**
 * Dependency information.
 */
struct ModPackDependency {
    char mod_id[COMMAND_WORD_LEN];
    char version_constraint[KFXMOD_VERSION_STRING_LEN];  // e.g., ">=1.0.0"
    TbBool required;
    char update_url[KFXMOD_URL_MAX_LEN];
};

/**
 * Conflict information.
 */
struct ModPackConflict {
    char mod_id[COMMAND_WORD_LEN];
    char reason[KFXMOD_REASON_MAX_LEN];
};

/**
 * Changelog entry.
 */
struct ModPackChangelogEntry {
    char version[KFXMOD_VERSION_STRING_LEN];
    char date[16];  // YYYY-MM-DD
    char *changes;  // Newline-separated list
};

/**
 * Campaign-specific configuration.
 */
struct ModPackCampaignConfig {
    int levels_count;
    TbBool has_multiplayer;
    char difficulty[32];
    char estimated_playtime[64];
};

/**
 * Content manifest details what the mod provides.
 */
struct ModPackContentManifest {
    TbBool has_creatures;
    TbBool has_configs;
    TbBool has_levels;
    TbBool has_audio;
    TbBool has_graphics;
    char creatures_list[32][COMMAND_WORD_LEN];  // Up to 32 creatures
    int creatures_count;
    char new_objects[32][COMMAND_WORD_LEN];     // Up to 32 objects
    int new_objects_count;
    char modified_rules[32][COMMAND_WORD_LEN];  // Up to 32 rules
    int modified_rules_count;
};

/**
 * Load order configuration.
 */
struct ModPackLoadOrder {
    int priority;                    // Higher = later loading
    enum ModLoadPhase load_phase;
};

/**
 * Metadata structure (parsed from JSON).
 */
struct ModPackMetadata {
    char mod_id[COMMAND_WORD_LEN];
    char version[KFXMOD_VERSION_STRING_LEN];
    int format_version;
    char name[LINEMSG_SIZE];
    char display_name[LINEMSG_SIZE];
    char author[128];
    char description[KFXMOD_DESCRIPTION_MAX_LEN];
    enum ModPackType mod_type;
    char created_date[32];
    char updated_date[32];
    char homepage_url[KFXMOD_URL_MAX_LEN];
    char update_url[KFXMOD_URL_MAX_LEN];
    char min_keeperfx_version[KFXMOD_VERSION_STRING_LEN];
    char max_keeperfx_version[KFXMOD_VERSION_STRING_LEN];
    char tags[KFXMOD_MAX_TAGS][KFXMOD_TAG_MAX_LEN];
    int tags_count;
    struct ModPackDependency dependencies[KFXMOD_MAX_DEPENDENCIES];
    int dependencies_count;
    struct ModPackDependency optional_dependencies[KFXMOD_MAX_DEPENDENCIES];
    int optional_dependencies_count;
    struct ModPackConflict conflicts[KFXMOD_MAX_CONFLICTS];
    int conflicts_count;
    struct ModPackLoadOrder load_order;
    struct ModPackChangelogEntry changelog[KFXMOD_MAX_CHANGELOG_ENTRIES];
    int changelog_count;
    char screenshots[KFXMOD_MAX_SCREENSHOTS][DISKPATH_SIZE];
    int screenshots_count;
    char readme[DISKPATH_SIZE];
    char license[128];
    struct ModPackCampaignConfig campaign_config;
    struct ModPackContentManifest content_manifest;
};

/**
 * Complete mod pack structure.
 */
struct ModPack {
    char filepath[DISKPATH_SIZE];
    struct ModPackHeader header;
    struct ModPackMetadata metadata;
    struct ModPackFileEntry *file_table;
    char *metadata_json;             // Raw JSON string
    TbBool is_loaded;
    TbBool is_valid;
    FILE *file_handle;               // Keep file open for extraction
};

/**
 * Update information from CDN.
 */
struct ModPackUpdateInfo {
    char mod_id[COMMAND_WORD_LEN];
    char current_version[KFXMOD_VERSION_STRING_LEN];
    char min_keeperfx_version[KFXMOD_VERSION_STRING_LEN];
    char download_url[KFXMOD_URL_MAX_LEN];
    uint64_t file_size;
    char checksum_sha256[65];        // 64 hex chars + null
    char release_date[32];
    char release_notes_url[KFXMOD_URL_MAX_LEN];
    TbBool is_deprecated;
    char deprecation_reason[KFXMOD_DESCRIPTION_MAX_LEN];
    char alternative_mod_id[COMMAND_WORD_LEN];
};

/******************************************************************************/
// Function prototypes

// Core loading functions
TbBool modpack_load_header(const char *filepath, struct ModPackHeader *header);
TbBool modpack_load(const char *filepath, struct ModPack *modpack);
TbBool modpack_unload(struct ModPack *modpack);

// Metadata parsing
TbBool modpack_parse_metadata_json(const char *json, struct ModPackMetadata *metadata);
TbBool modpack_load_metadata(struct ModPack *modpack);

// File operations
TbBool modpack_extract_file(struct ModPack *modpack, const char *path, const char *output_path);
TbBool modpack_extract_all(struct ModPack *modpack, const char *output_dir);
const struct ModPackFileEntry *modpack_find_file(struct ModPack *modpack, const char *path);

// Version comparison
int modpack_compare_versions(const char *version1, const char *version2);
TbBool modpack_version_satisfies(const char *version, const char *constraint);

// Dependency resolution
TbBool modpack_check_dependencies(struct ModPack *modpack, char *error_msg, int error_msg_size);
TbBool modpack_resolve_load_order(struct ModPack *modpacks, int count, int *load_order);

// Update checking
TbBool modpack_check_update(struct ModPack *modpack, struct ModPackUpdateInfo *update_info);
TbBool modpack_download_update(const char *download_url, const char *output_path);

// Validation
TbBool modpack_validate(struct ModPack *modpack);
uint32_t modpack_calculate_crc32(const void *data, size_t size);

// Utility functions
const char *modpack_get_type_name(enum ModPackType type);
const char *modpack_get_phase_name(enum ModLoadPhase phase);
void modpack_print_info(struct ModPack *modpack);

/******************************************************************************/
#ifdef __cplusplus
}
#endif

#endif // DK_CFG_MODPACK_H
