using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace KfxModStudio.Models;

/// <summary>
/// Compression types supported by the mod pack format
/// </summary>
public enum ModPackCompression : ushort
{
    None = 0,
    Zlib = 1,
    LZ4 = 2
}

/// <summary>
/// Mod types for categorization
/// </summary>
public enum ModPackType
{
    Unknown = 0,
    Campaign,
    CreaturePack,
    TexturePack,
    AudioPack,
    ConfigMod,
    ContentPack,
    TotalConversion
}

/// <summary>
/// Load phase determines when mod is applied
/// </summary>
public enum ModLoadPhase
{
    AfterBase = 0,
    AfterCampaign = 1,
    AfterMap = 2
}

/// <summary>
/// Binary mod pack header structure (64 bytes, matches C structure)
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct ModPackHeader
{
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
    public byte[] Magic;  // "KFXMOD\0\0"
    
    public ushort FormatVersion;
    public ushort CompressionType;
    public uint MetadataOffset;
    public uint MetadataSizeCompressed;
    public uint MetadataSizeUncompressed;
    public uint FileTableOffset;
    public uint FileTableCount;
    public uint ContentOffset;
    public uint TotalFileSize;
    public uint Crc32Checksum;
    public uint Flags;
    
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
    public byte[] Reserved;

    public const int Size = 64;
    public const string ExpectedMagic = "KFXMOD\0\0";
    public const ushort CurrentVersion = 1;
    
    public bool IsValid()
    {
        if (Magic == null || Magic.Length != 8)
            return false;
            
        var expectedBytes = System.Text.Encoding.ASCII.GetBytes(ExpectedMagic);
        for (int i = 0; i < 8; i++)
        {
            if (Magic[i] != expectedBytes[i])
                return false;
        }
        
        return FormatVersion == CurrentVersion;
    }
}

/// <summary>
/// Dependency information
/// </summary>
public class ModPackDependency
{
    public string ModId { get; set; } = string.Empty;
    public string VersionConstraint { get; set; } = string.Empty;
    public bool Required { get; set; }
    public string UpdateUrl { get; set; } = string.Empty;
}

/// <summary>
/// Conflict information
/// </summary>
public class ModPackConflict
{
    public string ModId { get; set; } = string.Empty;
    public string Reason { get; set; } = string.Empty;
}

/// <summary>
/// Changelog entry
/// </summary>
public class ModPackChangelogEntry
{
    public string Version { get; set; } = string.Empty;
    public string Date { get; set; } = string.Empty;
    public List<string> Changes { get; set; } = new();
}

/// <summary>
/// Campaign-specific configuration
/// </summary>
public class ModPackCampaignConfig
{
    public int LevelsCount { get; set; }
    public bool HasMultiplayer { get; set; }
    public string Difficulty { get; set; } = string.Empty;
    public string EstimatedPlaytime { get; set; } = string.Empty;
}

/// <summary>
/// Content manifest details what the mod provides
/// </summary>
public class ModPackContentManifest
{
    public bool HasCreatures { get; set; }
    public bool HasConfigs { get; set; }
    public bool HasLevels { get; set; }
    public bool HasAudio { get; set; }
    public bool HasGraphics { get; set; }
    public List<string> CreaturesList { get; set; } = new();
    public List<string> NewObjects { get; set; } = new();
    public List<string> ModifiedRules { get; set; } = new();
}

/// <summary>
/// Load order configuration
/// </summary>
public class ModPackLoadOrder
{
    public int Priority { get; set; }
    public ModLoadPhase LoadPhase { get; set; }
}

/// <summary>
/// Complete metadata structure (parsed from JSON)
/// </summary>
public class ModPackMetadata
{
    public string ModId { get; set; } = string.Empty;
    public string Version { get; set; } = string.Empty;
    public int FormatVersion { get; set; }
    public string Name { get; set; } = string.Empty;
    public string DisplayName { get; set; } = string.Empty;
    public string Author { get; set; } = string.Empty;
    public string Description { get; set; } = string.Empty;
    public ModPackType ModType { get; set; }
    public string CreatedDate { get; set; } = string.Empty;
    public string UpdatedDate { get; set; } = string.Empty;
    public string HomepageUrl { get; set; } = string.Empty;
    public string UpdateUrl { get; set; } = string.Empty;
    public string MinKeeperFxVersion { get; set; } = string.Empty;
    public string MaxKeeperFxVersion { get; set; } = string.Empty;
    public List<string> Tags { get; set; } = new();
    public List<ModPackDependency> Dependencies { get; set; } = new();
    public List<ModPackDependency> OptionalDependencies { get; set; } = new();
    public List<ModPackConflict> Conflicts { get; set; } = new();
    public ModPackLoadOrder LoadOrder { get; set; } = new();
    public List<ModPackChangelogEntry> Changelog { get; set; } = new();
    public List<string> Screenshots { get; set; } = new();
    public string Readme { get; set; } = string.Empty;
    public string License { get; set; } = string.Empty;
    public ModPackCampaignConfig? CampaignConfig { get; set; }
    public ModPackContentManifest ContentManifest { get; set; } = new();
}

/// <summary>
/// File entry in the mod pack
/// </summary>
public class ModPackFileEntry
{
    public string Path { get; set; } = string.Empty;
    public ulong FileOffset { get; set; }
    public ulong CompressedSize { get; set; }
    public ulong UncompressedSize { get; set; }
    public uint Crc32 { get; set; }
    public uint Flags { get; set; }
    public uint Timestamp { get; set; }
}

/// <summary>
/// Complete mod pack structure
/// </summary>
public class ModPack
{
    public string FilePath { get; set; } = string.Empty;
    public ModPackHeader Header { get; set; }
    public ModPackMetadata Metadata { get; set; } = new();
    public List<ModPackFileEntry> FileTable { get; set; } = new();
    public string MetadataJson { get; set; } = string.Empty;
    public bool IsLoaded { get; set; }
    public bool IsValid { get; set; }
}
