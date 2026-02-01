using System;
using System.Threading.Tasks;
using System.IO;
using System.IO.Compression;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;

namespace KfxModStudio.Services;

/// <summary>
/// Service for reading and writing .kfxmod binary format files
/// </summary>
public class ModPackReader
{
    /// <summary>
    /// Loads a mod pack from a file
    /// </summary>
    public static async Task<Models.ModPack?> LoadAsync(string filePath)
    {
        if (!File.Exists(filePath))
        {
            throw new FileNotFoundException($"Mod pack file not found: {filePath}");
        }

        var modPack = new Models.ModPack
        {
            FilePath = filePath
        };

        try
        {
            using var fileStream = File.OpenRead(filePath);
            using var reader = new BinaryReader(fileStream);

            // Read header
            modPack.Header = ReadHeader(reader);
            
            if (!modPack.Header.IsValid())
            {
                throw new InvalidDataException("Invalid mod pack header");
            }

            // Read and decompress metadata
            fileStream.Seek(modPack.Header.MetadataOffset, SeekOrigin.Begin);
            var compressedMetadata = reader.ReadBytes((int)modPack.Header.MetadataSizeCompressed);
            
            var metadataJson = DecompressMetadata(
                compressedMetadata,
                (int)modPack.Header.MetadataSizeUncompressed,
                (Models.ModPackCompression)modPack.Header.CompressionType
            );
            
            modPack.MetadataJson = metadataJson;
            
            // Parse metadata JSON
            modPack.Metadata = JsonSerializer.Deserialize<Models.ModPackMetadata>(metadataJson,
                new JsonSerializerOptions
                {
                    PropertyNameCaseInsensitive = true,
                    PropertyNamingPolicy = JsonNamingPolicy.SnakeCaseLower
                }) ?? new Models.ModPackMetadata();

            modPack.IsLoaded = true;
            modPack.IsValid = true;

            return modPack;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error loading mod pack: {ex.Message}");
            return null;
        }
    }

    private static Models.ModPackHeader ReadHeader(BinaryReader reader)
    {
        var headerBytes = reader.ReadBytes(Models.ModPackHeader.Size);
        
        var handle = GCHandle.Alloc(headerBytes, GCHandleType.Pinned);
        try
        {
            var header = Marshal.PtrToStructure<Models.ModPackHeader>(handle.AddrOfPinnedObject());
            return header;
        }
        finally
        {
            handle.Free();
        }
    }

    private static string DecompressMetadata(byte[] compressedData, int uncompressedSize, Models.ModPackCompression compressionType)
    {
        switch (compressionType)
        {
            case Models.ModPackCompression.None:
                return Encoding.UTF8.GetString(compressedData);
                
            case Models.ModPackCompression.Zlib:
                using (var compressedStream = new MemoryStream(compressedData))
                using (var zlibStream = new ZLibStream(compressedStream, CompressionMode.Decompress))
                using (var decompressedStream = new MemoryStream(uncompressedSize))
                {
                    zlibStream.CopyTo(decompressedStream);
                    return Encoding.UTF8.GetString(decompressedStream.ToArray());
                }
                
            case Models.ModPackCompression.LZ4:
                throw new NotImplementedException("LZ4 compression not yet implemented");
                
            default:
                throw new NotSupportedException($"Unsupported compression type: {compressionType}");
        }
    }

    /// <summary>
    /// Calculates CRC32 checksum
    /// </summary>
    public static uint CalculateCrc32(byte[] data)
    {
        uint crc = 0xFFFFFFFF;
        
        foreach (byte b in data)
        {
            crc ^= b;
            for (int i = 0; i < 8; i++)
            {
                if ((crc & 1) != 0)
                    crc = (crc >> 1) ^ 0xEDB88320;
                else
                    crc >>= 1;
            }
        }
        
        return ~crc;
    }
}
