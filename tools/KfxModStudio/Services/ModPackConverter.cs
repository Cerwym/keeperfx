using System;
using System.Linq;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;

namespace KfxModStudio.Services;

/// <summary>
/// Service for converting folder-based mods to .kfxmod format
/// </summary>
public class ModPackConverter
{
    /// <summary>
    /// Converts a folder-based mod to .kfxmod format
    /// </summary>
    public static async Task<bool> ConvertFolderToKfxModAsync(
        string sourceFolder,
        string outputFile,
        Models.ModPackMetadata metadata,
        Models.ModPackCompression compressionType = Models.ModPackCompression.Zlib,
        IProgress<string>? progress = null)
    {
        try
        {
            progress?.Report("Starting conversion...");
            
            // Serialize metadata to JSON
            var metadataJson = JsonSerializer.Serialize(metadata, new JsonSerializerOptions
            {
                WriteIndented = true,
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase
            });
            
            progress?.Report("Serialized metadata");
            
            // Compress metadata
            var metadataBytes = Encoding.UTF8.GetBytes(metadataJson);
            var compressedMetadata = CompressData(metadataBytes, compressionType);
            
            progress?.Report($"Compressed metadata: {metadataBytes.Length} -> {compressedMetadata.Length} bytes");
            
            // Collect all files in the folder
            var files = Directory.GetFiles(sourceFolder, "*", SearchOption.AllDirectories)
                .Where(f => !f.EndsWith("metadata.json")) // Exclude metadata.json as it's embedded
                .ToList();
            
            progress?.Report($"Found {files.Count} files to pack");
            
            // Create header
            var header = new Models.ModPackHeader
            {
                Magic = Encoding.ASCII.GetBytes("KFXMOD\0\0"),
                FormatVersion = Models.ModPackHeader.CurrentVersion,
                CompressionType = (ushort)compressionType,
                MetadataOffset = Models.ModPackHeader.Size,
                MetadataSizeCompressed = (uint)compressedMetadata.Length,
                MetadataSizeUncompressed = (uint)metadataBytes.Length,
                FileTableOffset = (uint)(Models.ModPackHeader.Size + compressedMetadata.Length),
                FileTableCount = (uint)files.Count,
                Reserved = new byte[16]
            };
            
            // For now, we'll create a minimal file with just header and metadata
            // Full file table implementation can be added later
            header.ContentOffset = header.FileTableOffset;
            header.TotalFileSize = header.ContentOffset;
            
            // Write to file
            using (var fileStream = File.Create(outputFile))
            using (var writer = new BinaryWriter(fileStream))
            {
                // Write header
                WriteHeader(writer, header);
                
                // Write compressed metadata
                writer.Write(compressedMetadata);
                
                progress?.Report("Wrote header and metadata");
            }
            
            progress?.Report($"Successfully created {outputFile}");
            return true;
        }
        catch (Exception ex)
        {
            progress?.Report($"Error: {ex.Message}");
            return false;
        }
    }
    
    private static void WriteHeader(BinaryWriter writer, Models.ModPackHeader header)
    {
        writer.Write(header.Magic);
        writer.Write(header.FormatVersion);
        writer.Write(header.CompressionType);
        writer.Write(header.MetadataOffset);
        writer.Write(header.MetadataSizeCompressed);
        writer.Write(header.MetadataSizeUncompressed);
        writer.Write(header.FileTableOffset);
        writer.Write(header.FileTableCount);
        writer.Write(header.ContentOffset);
        writer.Write(header.TotalFileSize);
        writer.Write(header.Crc32Checksum);
        writer.Write(header.Flags);
        writer.Write(header.Reserved);
    }
    
    private static byte[] CompressData(byte[] data, Models.ModPackCompression compressionType)
    {
        switch (compressionType)
        {
            case Models.ModPackCompression.None:
                return data;
                
            case Models.ModPackCompression.Zlib:
                using (var outputStream = new MemoryStream())
                {
                    using (var zlibStream = new ZLibStream(outputStream, CompressionLevel.Optimal))
                    {
                        zlibStream.Write(data, 0, data.Length);
                    }
                    return outputStream.ToArray();
                }
                
            case Models.ModPackCompression.LZ4:
                throw new NotImplementedException("LZ4 compression not yet implemented");
                
            default:
                throw new NotSupportedException($"Unsupported compression type: {compressionType}");
        }
    }
    
    /// <summary>
    /// Scans a folder and creates a default metadata object
    /// </summary>
    public static Models.ModPackMetadata CreateDefaultMetadata(string folderPath, string modId)
    {
        var folderName = Path.GetFileName(folderPath);
        
        return new Models.ModPackMetadata
        {
            ModId = modId,
            Version = "1.0.0",
            FormatVersion = 1,
            Name = folderName,
            DisplayName = folderName,
            Author = "Unknown",
            Description = $"Converted from folder: {folderName}",
            ModType = Models.ModPackType.Unknown,
            CreatedDate = DateTime.UtcNow.ToString("o"),
            UpdatedDate = DateTime.UtcNow.ToString("o"),
            Tags = new List<string> { "converted" },
            LoadOrder = new Models.ModPackLoadOrder
            {
                Priority = 100,
                LoadPhase = Models.ModLoadPhase.AfterCampaign
            },
            ContentManifest = new Models.ModPackContentManifest()
        };
    }
}
