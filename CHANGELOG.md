# Changelog - KeeperFX (Cerwym's Fork)

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/), and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### ✨ Features
- Semantic versioning system with Conventional Commits enforcement
- Fork identifier (-Cerwym) in all version strings and logs
- Automated changelog generation from commit messages
- Local prototype generation with testing notes

### 🔧 Infrastructure
- Layered deployment system with Windows junctions (97% disk savings)
- F5 debug workflow integrated with deployment
- Git commit-msg hook for message validation
- Version bumping and release automation scripts

## [1.3.1-Cerwym] - 2026-02-11

Initial fork version with enhanced development infrastructure.

### ✨ Features
- Casino room implementation with gambling mechanics
- Modular asset architecture using git submodules
- Clean master system for pristine game base

### 🔧 Infrastructure
- dev-environment branch as default (developer workflow)
- master branch for upstream sync only
- Layered deployment using Windows junctions and hard links
- Auto-initialization on F5 debug
- PowerShell automation for deployment and asset management

---

**Note**: This is a fork of [dkfans/keeperfx](https://github.com/dkfans/keeperfx) maintained by Cerwym. The `-Cerwym` identifier distinguishes this fork from upstream KeeperFX. For changes to the upstream project, see the [official KeeperFX wiki](https://github.com/dkfans/keeperfx/wiki/History-of-KeeperFX-releases).
