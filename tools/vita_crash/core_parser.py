"""
Binary parser for PS Vita .psp2dmp crash dump files.

.psp2dmp format: gzip-compressed ELF with:
  - PT_NOTE segments containing MODULE_INFO, THREAD_INFO, THREAD_REG_INFO
  - PT_LOAD segments containing raw memory pages (stack, heap)
"""

import gzip
import struct
import io
from dataclasses import dataclass, field
from typing import List, Dict, Optional, Tuple


# ── ELF constants ──────────────────────────────────────────────────────

ELF_MAGIC = b"\x7fELF"
PT_NOTE = 4
PT_LOAD = 1

# Vita core NOTE types (from SCE kernel)
NT_PSP2_MODULE_INFO = 1
NT_PSP2_THREAD_INFO = 3
NT_PSP2_THREAD_REG_INFO = 6

# Known stop reasons
STOP_REASONS = {
    0x00000: "No reason",
    0x10002: "Undefined instruction exception",
    0x20003: "Prefetch abort exception",
    0x30004: "Data abort exception",
}


@dataclass
class ModuleSegment:
    """A single segment (text or data) of a loaded module."""
    index: int
    base: int
    size: int
    perms: int  # rwx flags


@dataclass
class Module:
    """A loaded module in the crash dump."""
    name: str
    segments: List[ModuleSegment] = field(default_factory=list)

    def contains(self, addr: int) -> Optional[Tuple[int, int]]:
        """Return (segment_index, offset) if addr falls within this module."""
        for seg in self.segments:
            if seg.base <= addr < seg.base + seg.size:
                return seg.index, addr - seg.base
        return None


@dataclass
class ThreadRegisters:
    """ARM register set for a thread."""
    r: List[int] = field(default_factory=lambda: [0] * 13)  # R0-R12
    sp: int = 0
    lr: int = 0
    pc: int = 0


@dataclass
class Thread:
    """A thread captured in the crash dump."""
    name: str
    tid: int
    stop_reason: int
    status: int
    pc: int
    regs: Optional[ThreadRegisters] = None

    @property
    def stop_reason_str(self) -> str:
        return STOP_REASONS.get(self.stop_reason, f"Unknown (0x{self.stop_reason:x})")

    @property
    def crashed(self) -> bool:
        return self.stop_reason != 0


@dataclass
class MemoryRegion:
    """A raw memory region from PT_LOAD."""
    vaddr: int
    data: bytes

    def contains(self, addr: int) -> bool:
        return self.vaddr <= addr < self.vaddr + len(self.data)

    def read_u32(self, addr: int) -> Optional[int]:
        if not self.contains(addr) or not self.contains(addr + 3):
            return None
        off = addr - self.vaddr
        return struct.unpack_from("<I", self.data, off)[0]


@dataclass
class CoreDump:
    """Parsed PS Vita core dump."""
    modules: List[Module] = field(default_factory=list)
    threads: List[Thread] = field(default_factory=list)
    memory: List[MemoryRegion] = field(default_factory=list)

    @property
    def crashed_thread(self) -> Optional[Thread]:
        for t in self.threads:
            if t.crashed:
                return t
        return None

    def resolve_address(self, addr: int) -> Optional[str]:
        """Resolve an address to module@segment + offset."""
        for mod in self.modules:
            result = mod.contains(addr)
            if result:
                seg_idx, offset = result
                return f"{mod.name}@{seg_idx + 1} + 0x{offset:x}"
        return None

    def read_u32(self, addr: int) -> Optional[int]:
        """Read a 32-bit LE value from memory."""
        for region in self.memory:
            val = region.read_u32(addr)
            if val is not None:
                return val
        return None


def _align4(n: int) -> int:
    return (n + 3) & ~3


def _read_cstr(data: bytes, offset: int) -> str:
    """Read a null-terminated string from bytes."""
    end = data.index(b"\x00", offset) if b"\x00" in data[offset:] else len(data)
    return data[offset:end].decode("utf-8", errors="replace")


def _parse_elf_header(data: bytes) -> dict:
    """Parse minimal ELF32 header fields."""
    if data[:4] != ELF_MAGIC:
        raise ValueError("Not an ELF file")
    # ELF32 header: e_phoff at offset 28, e_phentsize at 42, e_phnum at 44
    e_phoff = struct.unpack_from("<I", data, 28)[0]
    e_phentsize = struct.unpack_from("<H", data, 42)[0]
    e_phnum = struct.unpack_from("<H", data, 44)[0]
    return {"phoff": e_phoff, "phentsize": e_phentsize, "phnum": e_phnum}


def _parse_phdr(data: bytes, offset: int) -> dict:
    """Parse an ELF32 program header."""
    fields = struct.unpack_from("<IIIIIIII", data, offset)
    return {
        "type": fields[0],
        "offset": fields[1],
        "vaddr": fields[2],
        "paddr": fields[3],
        "filesz": fields[4],
        "memsz": fields[5],
        "flags": fields[6],
        "align": fields[7],
    }


def _parse_notes(note_data: bytes) -> list:
    """Parse ELF NOTE entries from a segment."""
    notes = []
    off = 0
    while off < len(note_data):
        if off + 12 > len(note_data):
            break
        namesz, descsz, ntype = struct.unpack_from("<III", note_data, off)
        off += 12
        name = note_data[off : off + namesz].rstrip(b"\x00").decode("ascii", errors="replace")
        off += _align4(namesz)
        desc = note_data[off : off + descsz]
        off += _align4(descsz)
        notes.append({"name": name, "type": ntype, "desc": desc})
    return notes


def _parse_module_info(desc: bytes, modules: List[Module]):
    """Parse MODULE_INFO note descriptor into module list."""
    off = 0
    while off + 4 <= len(desc):
        # Module entry: name (variable), then segments
        # Format from vita-parse-core: each module has a name string,
        # followed by segment count, then segment entries.
        # Actual format reverse-engineered from vita-parse-core core.py:
        # The entire note is a flat list: for each module:
        #   uint32 name_offset (into a separate string table? no — inline)
        #   Actually the format is more complex. Let's parse based on
        #   what vita-parse-core does.
        break  # Handled by the comprehensive parser below


def _parse_module_info_v2(desc: bytes) -> List[Module]:
    """
    Parse MODULE_INFO from the core dump.

    Based on vita-parse-core's core.py CoreParser._parse_modules():
    The descriptor is a flat binary blob with module entries.
    Each entry: 4 bytes flags, 32-byte name, then per-segment entries.
    """
    modules = []
    off = 0

    # Header: uint32 count
    if len(desc) < 4:
        return modules
    count = struct.unpack_from("<I", desc, off)[0]
    off += 4

    for _ in range(count):
        if off + 36 > len(desc):
            break
        # Module entry header varies by Vita FW version.
        # Common layout: 4 bytes (flags+num_segments), 32 bytes name
        flags_and_segs = struct.unpack_from("<I", desc, off)[0]
        off += 4
        num_segments = flags_and_segs & 0xFF

        # Name: 32 bytes, null-terminated
        name_bytes = desc[off : off + 32]
        off += 32
        name = name_bytes.split(b"\x00", 1)[0].decode("ascii", errors="replace")

        mod = Module(name=name)
        for seg_idx in range(num_segments):
            if off + 16 > len(desc):
                break
            seg_base, seg_size, seg_perms, _pad = struct.unpack_from("<IIII", desc, off)
            off += 16
            mod.segments.append(ModuleSegment(
                index=seg_idx,
                base=seg_base,
                size=seg_size,
                perms=seg_perms,
            ))
        modules.append(mod)

    return modules


def _parse_thread_info(desc: bytes) -> List[Thread]:
    """
    Parse THREAD_INFO note descriptor.

    Based on vita-parse-core core.py:
    Header: uint32 count
    Each thread: uint32 tid, uint32 stop_reason, uint32 status,
                 uint32 pc, 32-byte name
    """
    threads = []
    off = 0

    if len(desc) < 4:
        return threads
    count = struct.unpack_from("<I", desc, off)[0]
    off += 4

    for _ in range(count):
        if off + 48 > len(desc):
            break
        tid, stop_reason, status, pc = struct.unpack_from("<IIII", desc, off)
        off += 16
        name_bytes = desc[off : off + 32]
        off += 32
        name = name_bytes.split(b"\x00", 1)[0].decode("ascii", errors="replace")
        threads.append(Thread(
            name=name,
            tid=tid,
            stop_reason=stop_reason,
            status=status,
            pc=pc,
        ))
    return threads


def _parse_thread_reg_info(desc: bytes, threads: List[Thread]):
    """
    Parse THREAD_REG_INFO and attach register sets to matching threads.

    Each entry: uint32 tid, then 16 x uint32 registers (R0-R12, SP, LR, PC).
    """
    off = 0
    while off + 68 <= len(desc):  # 4 (tid) + 16*4 (regs) = 68
        tid = struct.unpack_from("<I", desc, off)[0]
        off += 4
        regs_raw = struct.unpack_from("<16I", desc, off)
        off += 64

        reg = ThreadRegisters(
            r=list(regs_raw[:13]),
            sp=regs_raw[13],
            lr=regs_raw[14],
            pc=regs_raw[15],
        )

        for t in threads:
            if t.tid == tid:
                t.regs = reg
                break


def parse_core_dump(path: str) -> CoreDump:
    """
    Parse a .psp2dmp file and return a CoreDump.

    The file may be gzip-compressed or raw ELF.
    """
    with open(path, "rb") as f:
        raw = f.read()

    # Try gzip decompression
    try:
        data = gzip.decompress(raw)
    except (gzip.BadGzipFile, OSError):
        data = raw

    if data[:4] != ELF_MAGIC:
        raise ValueError(f"Not a valid ELF core dump: {path}")

    hdr = _parse_elf_header(data)
    dump = CoreDump()

    # Collect all program headers
    phdrs = []
    for i in range(hdr["phnum"]):
        offset = hdr["phoff"] + i * hdr["phentsize"]
        phdrs.append(_parse_phdr(data, offset))

    # Parse NOTE segments for metadata
    for ph in phdrs:
        if ph["type"] == PT_NOTE:
            note_data = data[ph["offset"] : ph["offset"] + ph["filesz"]]
            for note in _parse_notes(note_data):
                if note["type"] == NT_PSP2_MODULE_INFO:
                    dump.modules = _parse_module_info_v2(note["desc"])
                elif note["type"] == NT_PSP2_THREAD_INFO:
                    dump.threads = _parse_thread_info(note["desc"])
                elif note["type"] == NT_PSP2_THREAD_REG_INFO:
                    _parse_thread_reg_info(note["desc"], dump.threads)

    # Parse LOAD segments for memory
    for ph in phdrs:
        if ph["type"] == PT_LOAD and ph["filesz"] > 0:
            dump.memory.append(MemoryRegion(
                vaddr=ph["vaddr"],
                data=data[ph["offset"] : ph["offset"] + ph["filesz"]],
            ))

    return dump
