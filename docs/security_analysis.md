# Security Analysis for Binary Mod Format Implementation

## Overview

This document provides a security analysis of the .kfxmod binary mod format implementation added to KeeperFX.

## Files Analyzed

- `src/config_modpack.h` - Header file with structure definitions
- `src/config_modpack.c` - Core implementation (16KB)
- `tools/kfxmod/tool_modpack.c` - CLI tool (10.5KB)

## Security Considerations Implemented

### 1. File Format Validation

**Header Validation** (`modpack_load_header`):
```c
// Validate magic
if (memcmp(header->magic, KFXMOD_MAGIC, KFXMOD_MAGIC_SIZE) != 0)
{
    WARNMSG("Invalid mod pack magic: %s", filepath);
    return false;
}

// Validate format version
if (header->format_version != KFXMOD_FORMAT_VERSION)
{
    WARNMSG("Unsupported mod pack format version");
    return false;
}
```

✅ **Status**: Properly validates file magic and version before processing

### 2. Buffer Overflow Protection

**String Length Validation**:
```c
strncpy(modpack->filepath, filepath, DISKPATH_SIZE - 1);
modpack->filepath[DISKPATH_SIZE - 1] = '\0';  // Null-terminate
```

✅ **Status**: Uses `strncpy` with explicit null-termination

**Memory Allocation Checks**:
```c
uint8_t *compressed = (uint8_t *)malloc(modpack->header.metadata_size_compressed);
if (compressed == NULL)
{
    WARNMSG("Failed to allocate memory for compressed metadata");
    return false;
}
```

✅ **Status**: All malloc calls checked for NULL before use

### 3. Compression/Decompression Safety

**zlib Decompression** (`decompress_zlib`):
```c
stream.next_in = (Bytef *)input;
stream.avail_in = input_size;
stream.next_out = output;
stream.avail_out = output_size;

int ret = inflate(&stream, Z_FINISH);
if (ret != Z_STREAM_END)
{
    WARNMSG("Failed to decompress zlib data: %d", ret);
    return false;
}
```

✅ **Status**: Checks decompression return value and bounds

⚠️ **Note**: Pre-allocates output buffer based on header size - header values should be validated against reasonable limits

### 4. Integer Overflow Protection

**File Size Checks Needed**:
```c
// Current code reads size from header without validation
size_t read = fread(compressed, 1, modpack->header.metadata_size_compressed, fp);
```

⚠️ **Recommendation**: Add maximum size validation:
```c
#define MAX_METADATA_SIZE (10 * 1024 * 1024)  // 10 MB max

if (modpack->header.metadata_size_compressed > MAX_METADATA_SIZE ||
    modpack->header.metadata_size_uncompressed > MAX_METADATA_SIZE)
{
    WARNMSG("Metadata size too large");
    return false;
}
```

### 5. Path Traversal Protection

**File Extraction** (not yet implemented):

⚠️ **TODO**: When implementing `modpack_extract_file`, must validate paths:
```c
TbBool is_safe_path(const char *path)
{
    // Reject absolute paths
    if (path[0] == '/' || path[0] == '\\')
        return false;
    
    // Reject parent directory references
    if (strstr(path, "..") != NULL)
        return false;
    
    // Reject drive letters (Windows)
    if (path[1] == ':')
        return false;
    
    return true;
}
```

### 6. CRC32 Integrity Checking

**Checksum Calculation** (`modpack_calculate_crc32`):
```c
uint32_t modpack_calculate_crc32(const void *data, size_t size)
{
    // Standard CRC32 implementation with 0xEDB88320 polynomial
    // ...
}
```

✅ **Status**: Provides CRC32 for integrity checking

⚠️ **TODO**: Actually verify checksums in `modpack_validate`:
```c
TbBool modpack_validate(struct ModPack *modpack)
{
    // TODO: Read entire file and verify CRC32
    // TODO: Verify individual file checksums from file table
}
```

### 7. Resource Cleanup

**Memory Management**:
```c
TbBool modpack_unload(struct ModPack *modpack)
{
    if (modpack->metadata_json != NULL)
    {
        free(modpack->metadata_json);
        modpack->metadata_json = NULL;
    }
    
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
    
    if (modpack->file_handle != NULL)
    {
        fclose(modpack->file_handle);
        modpack->file_handle = NULL;
    }
}
```

✅ **Status**: Properly frees all allocated resources and nulls pointers

### 8. File Handle Management

**Open File Handles**:
```c
if (modpack->file_handle == NULL)
{
    modpack->file_handle = fopen(modpack->filepath, "rb");
}
```

⚠️ **Concern**: File handle kept open during mod pack lifetime

**Recommendation**: Consider closing after each operation or implement reference counting

### 9. JSON Parsing Security

**Current Implementation**:
```c
TbBool modpack_parse_metadata_json(const char *json, struct ModPackMetadata *metadata)
{
    // Minimal string-based parser
    // Uses strstr, strchr for extraction
}
```

⚠️ **Limitations**: 
- No validation of JSON structure
- No handling of escaped characters
- No protection against malformed JSON

**Recommendation**: Integrate proper JSON parser library (e.g., cJSON, jansson)

### 10. Version String Parsing

**Version Comparison**:
```c
int parsed1 = sscanf(version1, "%d.%d.%d", &major1, &minor1, &patch1);
if (parsed1 < 3)
{
    minor1 = 0;
    patch1 = 0;
}
```

✅ **Status**: Safely handles incomplete version strings

⚠️ **Note**: Doesn't validate version string format before parsing

### 11. Command-Line Tool Security

**File Path Handling** (`tool_modpack.c`):
```c
const char *input_dir = argv[2];
const char *output_file = argv[3];
// Direct use without validation
```

⚠️ **Recommendation**: Validate file paths and check for directory traversal

**Buffer Sizes**:
```c
static char default_metadata[512];
snprintf(default_metadata, sizeof(default_metadata), "%s/metadata.json", input_dir);
```

✅ **Status**: Uses `snprintf` with size limits

## Security Risks Summary

### High Priority

None identified in current implementation

### Medium Priority

1. **Size Validation**: Add maximum size limits for metadata and file table
2. **Path Traversal**: Implement safe path validation for file extraction
3. **JSON Parsing**: Replace minimal parser with proper library
4. **Checksum Verification**: Complete implementation of CRC32 validation

### Low Priority

1. **File Handle Management**: Consider resource optimization
2. **Version String Validation**: Add format checks before parsing
3. **Command-Line Input**: Validate paths in CLI tool

## Recommendations

### Immediate Actions

1. **Add Size Limits**:
```c
#define MAX_METADATA_SIZE (10 * 1024 * 1024)
#define MAX_FILE_TABLE_ENTRIES 100000
#define MAX_SINGLE_FILE_SIZE (1024 * 1024 * 1024)  // 1 GB
```

2. **Implement Path Validation**:
```c
TbBool validate_file_path(const char *path)
{
    if (path[0] == '/' || path[0] == '\\') return false;
    if (strstr(path, "..") != NULL) return false;
    if (path[1] == ':') return false;
    return true;
}
```

3. **Complete CRC32 Verification**:
```c
TbBool modpack_validate(struct ModPack *modpack)
{
    // Read file and verify header CRC32
    // Verify each file entry CRC32
    return true;
}
```

### Future Enhancements

1. **Digital Signatures**: Add RSA signature verification
2. **Sandboxing**: Extract files to temporary sandbox before validation
3. **Rate Limiting**: Limit CDN update checks to prevent DoS
4. **Compression Bomb Protection**: Validate compression ratios

## Conclusion

The current implementation demonstrates good security practices with proper validation, memory management, and error handling. The identified issues are mostly missing validations that should be added before the feature is considered production-ready.

**Overall Security Rating**: ⭐⭐⭐⭐☆ (4/5)

- Strong foundation with validation and error handling
- Good memory management
- Needs additional size limits and path validation
- JSON parsing should use proper library

**Recommendation**: Address medium-priority items before merging to main branch. Implementation is safe for development and testing.

---

**Reviewed by**: Security Analysis Bot  
**Date**: February 1, 2026  
**Version**: 1.0
