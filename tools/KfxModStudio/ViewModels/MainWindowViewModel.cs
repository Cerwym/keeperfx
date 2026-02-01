using System;
using System.Threading.Tasks;
using System.Windows.Input;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using KfxModStudio.Services;
using Avalonia.Platform.Storage;
using System.Collections.Generic;
using System.Linq;

namespace KfxModStudio.ViewModels;

public partial class MainWindowViewModel : ViewModelBase
{
    [ObservableProperty]
    private Models.ModPack _modPack = new();

    [ObservableProperty]
    private string _statusText = "Ready";

    [ObservableProperty]
    private bool _isEditMode = false;

    [ObservableProperty]
    private bool _hasDependencies = false;

    public ICommand NewModCommand => new RelayCommand(OnNewMod);
    public ICommand OpenModCommand => new AsyncRelayCommand(OnOpenModAsync);
    public ICommand ConvertFolderCommand => new AsyncRelayCommand(OnConvertFolderAsync);
    public ICommand SaveCommand => new RelayCommand(OnSave);
    public ICommand SaveAsCommand => new RelayCommand(OnSaveAs);
    public ICommand ExitCommand => new RelayCommand(OnExit);
    public ICommand ValidateCommand => new RelayCommand(OnValidate);
    public ICommand ViewMetadataCommand => new RelayCommand(OnViewMetadata);
    public ICommand AboutCommand => new RelayCommand(OnAbout);
    public ICommand DocumentationCommand => new RelayCommand(OnDocumentation);

    private void OnNewMod()
    {
        ModPack = new Models.ModPack
        {
            Metadata = new Models.ModPackMetadata
            {
                ModId = "new_mod",
                Name = "New Mod",
                Version = "1.0.0",
                Author = Environment.UserName,
                Description = "A new KeeperFX mod",
                ModType = Models.ModPackType.Unknown,
                CreatedDate = DateTime.UtcNow.ToString("o"),
                LoadOrder = new Models.ModPackLoadOrder
                {
                    Priority = 100,
                    LoadPhase = Models.ModLoadPhase.AfterCampaign
                },
                ContentManifest = new Models.ModPackContentManifest()
            }
        };
        IsEditMode = true;
        StatusText = "Created new mod";
    }

    private async Task OnOpenModAsync()
    {
        try
        {
            // For now, simulate file picker
            // In a real app, we'd use Avalonia's file picker
            StatusText = "Opening mod file...";
            
            // TODO: Implement file picker and load
            StatusText = "Ready to open mod (file picker not yet implemented)";
        }
        catch (Exception ex)
        {
            StatusText = $"Error: {ex.Message}";
        }
    }

    private async Task OnConvertFolderAsync()
    {
        try
        {
            StatusText = "Starting conversion wizard...";
            
            // TODO: Implement folder picker and conversion wizard
            StatusText = "Ready to convert folder (wizard not yet implemented)";
        }
        catch (Exception ex)
        {
            StatusText = $"Error: {ex.Message}";
        }
    }

    private void OnSave()
    {
        StatusText = "Saving mod...";
        // TODO: Implement save
    }

    private void OnSaveAs()
    {
        StatusText = "Save As...";
        // TODO: Implement save as
    }

    private void OnExit()
    {
        Environment.Exit(0);
    }

    private void OnValidate()
    {
        if (ModPack.IsLoaded)
        {
            StatusText = ModPack.IsValid ? "Mod is valid" : "Mod has errors";
        }
        else
        {
            StatusText = "No mod loaded";
        }
    }

    private void OnViewMetadata()
    {
        StatusText = "Viewing metadata JSON";
    }

    private void OnAbout()
    {
        StatusText = "KeeperFX Mod Studio v1.0 - A tool for creating and editing .kfxmod files";
    }

    private void OnDocumentation()
    {
        StatusText = "Opening documentation...";
        // TODO: Open documentation in browser
    }
}
