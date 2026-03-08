"""
FTP client for downloading crash dumps from PS Vita.

Connects to vitacompanion/vdbtcp FTP server (default port 1337)
and lists/downloads .psp2dmp files from ux0:/data/.

Note: The Vita FTP server does not support path arguments in LIST/RETR
commands. All navigation must use CWD first, then operate on the current
directory.
"""

import ftplib
import os
import re
from dataclasses import dataclass
from typing import List, Optional

DUMP_DIR = "ux0:/data"


@dataclass
class DumpEntry:
    """A crash dump file on the Vita."""
    filename: str
    size: int

    @property
    def is_eboot(self) -> bool:
        return "eboot.bin" in self.filename


def connect(host: str, port: int = 1337, timeout: int = 10) -> ftplib.FTP:
    """Connect to Vita FTP server."""
    ftp = ftplib.FTP()
    ftp.connect(host, port, timeout=timeout)
    ftp.login()  # anonymous
    return ftp


def _cwd_vita(ftp: ftplib.FTP, path: str) -> None:
    """Navigate to a Vita path using sequential CWD calls.

    The Vita FTP server requires navigating one component at a time
    (e.g. CWD ux0: then CWD data) rather than CWD ux0:/data.
    """
    # Split "ux0:/data/subdir" into ["ux0:", "data", "subdir"]
    parts = path.replace(":/", ":/ ").split("/")
    parts = [p.strip() for p in parts if p.strip()]
    ftp.cwd("/")
    for part in parts:
        ftp.cwd(part)


def list_dumps(ftp: ftplib.FTP, include_all: bool = False) -> List[DumpEntry]:
    """List .psp2dmp files on the Vita."""
    entries = []
    lines = []
    _cwd_vita(ftp, DUMP_DIR)
    ftp.retrlines("LIST", lines.append)

    for line in lines:
        # FTP LIST format: permissions ... size month day time filename
        parts = line.split()
        if len(parts) < 5:
            continue
        filename = parts[-1]
        if not filename.endswith(".psp2dmp"):
            continue
        if not include_all and "eboot.bin" not in filename:
            continue
        try:
            size = int(parts[4])
        except (ValueError, IndexError):
            size = 0
        entries.append(DumpEntry(filename=filename, size=size))

    return entries


def download_dump(ftp: ftplib.FTP, filename: str, output_dir: str) -> str:
    """Download a dump file from the Vita. Returns local path."""
    os.makedirs(output_dir, exist_ok=True)
    local_path = os.path.join(output_dir, filename)

    _cwd_vita(ftp, DUMP_DIR)
    with open(local_path, "wb") as f:
        ftp.retrbinary(f"RETR {filename}", f.write)

    return local_path


def download_crash_log(ftp: ftplib.FTP, output_dir: str) -> Optional[str]:
    """Try to download the on-device crash.log. Returns path or None."""
    os.makedirs(output_dir, exist_ok=True)
    local_path = os.path.join(output_dir, "crash.log")

    try:
        _cwd_vita(ftp, "ux0:/data/keeperfx")
        with open(local_path, "wb") as f:
            ftp.retrbinary("RETR crash.log", f.write)
        return local_path
    except ftplib.error_perm:
        return None
