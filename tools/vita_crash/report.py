"""
Output formatters for crash reports — text and HTML.
"""

import html
import os
from datetime import datetime
from typing import List, Optional

from .core_parser import CoreDump, Thread
from .symbolicate import StackTrace, StackFrame, SourceLocation


# ── Text Formatter ──────────────────────────────────────────────────────

def format_text(dump: CoreDump, traces: List[StackTrace],
                source_root: Optional[str] = None) -> str:
    """Generate a human-readable text crash report."""
    lines = []
    ct = dump.crashed_thread

    lines.append("═" * 60)
    lines.append("  CRASH REPORT: KeeperFX Vita")
    lines.append("═" * 60)
    lines.append(f"  Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

    if ct:
        lines.append(f"  Exception:  {ct.stop_reason_str}")
        lines.append(f"  Thread:     {ct.name} (TID 0x{ct.tid:x})")
        lines.append(f"  PC:         0x{ct.pc:08x}")
        if ct.regs:
            lines.append(f"  LR:         0x{ct.regs.lr:08x}")
    lines.append("")

    # Crashed thread trace first
    for trace in traces:
        if ct and trace.thread_id == ct.tid:
            lines.append(_format_trace_text(trace, source_root, is_crashed=True))
            break

    # Register dump for crashed thread
    if ct and ct.regs:
        lines.append("")
        lines.append("REGISTERS:")
        r = ct.regs
        for i in range(0, 13, 4):
            row = "  "
            for j in range(i, min(i + 4, 13)):
                row += f"R{j:<2}=0x{r.r[j]:08x}  "
            lines.append(row.rstrip())
        lines.append(f"  SP =0x{r.sp:08x}  LR =0x{r.lr:08x}  PC =0x{r.pc:08x}")

    # Diagnosis hints
    diag = _diagnose(dump, ct)
    if diag:
        lines.append("")
        lines.append("DIAGNOSIS:")
        for d in diag:
            lines.append(f"  {d}")

    # Other threads
    other_traces = [t for t in traces if not ct or t.thread_id != ct.tid]
    if other_traces:
        lines.append("")
        lines.append("─" * 60)
        lines.append("  OTHER THREADS")
        lines.append("─" * 60)
        for trace in other_traces:
            lines.append(_format_trace_text(trace, source_root, is_crashed=False))

    # All threads summary
    lines.append("")
    lines.append("─" * 60)
    lines.append("  THREAD SUMMARY")
    lines.append("─" * 60)
    for t in dump.threads:
        status = "** CRASHED **" if t.crashed else "Waiting" if t.status == 8 else f"Status 0x{t.status:x}"
        lines.append(f"  {t.name:<30} TID=0x{t.tid:08x}  {status}")
        lines.append(f"    PC=0x{t.pc:08x}  ({dump.resolve_address(t.pc) or 'unknown'})")

    # Module list
    if dump.modules:
        lines.append("")
        lines.append("─" * 60)
        lines.append("  LOADED MODULES")
        lines.append("─" * 60)
        for mod in dump.modules:
            lines.append(f"  {mod.name}")
            for seg in mod.segments:
                lines.append(
                    f"    @{seg.index + 1}: 0x{seg.base:08x} - 0x{seg.base + seg.size:08x}"
                    f"  ({seg.size:>10,} bytes)"
                )

    lines.append("")
    lines.append("═" * 60)
    return "\n".join(lines)


def _format_trace_text(trace: StackTrace, source_root: Optional[str],
                       is_crashed: bool) -> str:
    """Format a single thread's stack trace as text."""
    lines = []
    header = "CALL STACK" if is_crashed else f"Thread: {trace.thread_name}"
    lines.append(f"\n{header} (most recent call first):")

    for frame in trace.frames:
        addr_str = f"0x{frame.address:08x}"
        if frame.source:
            func = frame.source.function
            src = ""
            if frame.source.file and frame.source.file != "??":
                src = f"  {frame.source.file}:{frame.source.line}"
            lines.append(f"  #{frame.index:<3} {func}{src}")
            lines.append(f"       {addr_str}  ({frame.module_ref or 'unknown'})")

            # Show inlined chain
            loc = frame.source.inlined_from
            while loc:
                src = ""
                if loc.file and loc.file != "??":
                    src = f"  {loc.file}:{loc.line}"
                lines.append(f"       [inlined] {loc.function}{src}")
                loc = loc.inlined_from

            # Show source context if available
            if source_root and frame.source.file != "??" and frame.source.line > 0:
                context = _read_source_context(source_root, frame.source.file,
                                               frame.source.line)
                if context:
                    for ctx_line in context:
                        lines.append(f"       | {ctx_line}")
        else:
            lines.append(f"  #{frame.index:<3} {addr_str}  ({frame.module_ref or 'unknown'})")

    return "\n".join(lines)


def _read_source_context(source_root: str, filepath: str, line: int,
                         context: int = 2) -> List[str]:
    """Read source lines around the crash point."""
    # Handle various path formats from addr2line
    # Strip leading /src/ if present (Docker mount point)
    for prefix in ("/src/", "src/", "./"):
        if filepath.startswith(prefix):
            filepath = filepath[len(prefix):]
            break

    full_path = os.path.join(source_root, filepath)
    if not os.path.isfile(full_path):
        return []

    try:
        with open(full_path, "r", encoding="utf-8", errors="replace") as f:
            all_lines = f.readlines()

        start = max(0, line - context - 1)
        end = min(len(all_lines), line + context)
        result = []
        for i in range(start, end):
            marker = ">>>" if i == line - 1 else "   "
            result.append(f"{marker} {i + 1:>5}: {all_lines[i].rstrip()}")
        return result
    except OSError:
        return []


def _diagnose(dump: CoreDump, thread: Optional[Thread]) -> List[str]:
    """Generate automated diagnosis hints."""
    hints = []
    if not thread or not thread.regs:
        return hints

    r = thread.regs

    # NULL pointer detection
    null_regs = []
    for i in range(13):
        if r.r[i] == 0:
            null_regs.append(f"R{i}")
    if r.sp == 0:
        null_regs.append("SP")

    if thread.stop_reason == 0x30004:  # Data abort
        hints.append("Data abort — attempted to read/write an invalid memory address.")
        if null_regs:
            hints.append(f"NULL registers: {', '.join(null_regs)} — likely NULL pointer dereference.")

    elif thread.stop_reason == 0x20003:  # Prefetch abort
        hints.append("Prefetch abort — attempted to execute code at an invalid address.")
        if r.pc < 0x1000:
            hints.append(f"PC=0x{r.pc:08x} is near zero — likely call through NULL function pointer.")

    elif thread.stop_reason == 0x10002:  # Undefined instruction
        hints.append("Undefined instruction — CPU encountered an invalid opcode.")
        hints.append("This can indicate: corrupted code memory, wrong ARM/Thumb mode, or stack smashing.")

    # Stack overflow detection (Vita default thread stack is at ~0x836xxxxx range)
    if 0x83600000 <= r.sp <= 0x83600100:
        hints.append(f"SP=0x{r.sp:08x} is very close to the stack base — possible stack overflow.")

    # Check if crash is in malloc/memory allocation
    crash_ref = dump.resolve_address(r.pc)
    if crash_ref and any(kw in crash_ref.lower() for kw in ("malloc", "vglmalloc", "alloc")):
        hints.append("Crash in memory allocator — possible out-of-memory condition.")

    return hints


# ── HTML Formatter ──────────────────────────────────────────────────────

def format_html(dump: CoreDump, traces: List[StackTrace],
                source_root: Optional[str] = None) -> str:
    """Generate a self-contained HTML crash report."""
    ct = dump.crashed_thread
    h = html.escape

    parts = [
        "<!DOCTYPE html>",
        "<html><head><meta charset='utf-8'>",
        "<title>KeeperFX Vita Crash Report</title>",
        "<style>",
        _CSS,
        "</style></head><body>",
        "<div class='report'>",
        "<h1>KeeperFX Vita Crash Report</h1>",
        f"<p class='meta'>Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>",
    ]

    if ct:
        parts.append("<div class='crash-info'>")
        parts.append(f"<p><strong>Exception:</strong> {h(ct.stop_reason_str)}</p>")
        parts.append(f"<p><strong>Thread:</strong> {h(ct.name)} (TID 0x{ct.tid:x})</p>")
        parts.append(f"<p><strong>PC:</strong> <code>0x{ct.pc:08x}</code></p>")
        if ct.regs:
            parts.append(f"<p><strong>LR:</strong> <code>0x{ct.regs.lr:08x}</code></p>")
        parts.append("</div>")

    # Diagnosis
    diag = _diagnose(dump, ct)
    if diag:
        parts.append("<div class='diagnosis'>")
        parts.append("<h2>Diagnosis</h2>")
        for d in diag:
            parts.append(f"<p>{h(d)}</p>")
        parts.append("</div>")

    # Crashed thread stack trace
    for trace in traces:
        if ct and trace.thread_id == ct.tid:
            parts.append(_format_trace_html(trace, source_root, is_crashed=True))
            break

    # Registers
    if ct and ct.regs:
        parts.append("<h2>Registers</h2>")
        parts.append("<table class='regs'>")
        r = ct.regs
        for i in range(0, 13, 4):
            parts.append("<tr>")
            for j in range(i, min(i + 4, 13)):
                cls = "null" if r.r[j] == 0 else "code" if 0x81000000 <= r.r[j] < 0x82000000 else "stack" if 0x83000000 <= r.r[j] < 0x84000000 else ""
                parts.append(f"<td>R{j}</td><td class='{cls}'><code>0x{r.r[j]:08x}</code></td>")
            parts.append("</tr>")
        parts.append("<tr>")
        parts.append(f"<td>SP</td><td class='stack'><code>0x{r.sp:08x}</code></td>")
        parts.append(f"<td>LR</td><td class='code'><code>0x{r.lr:08x}</code></td>")
        parts.append(f"<td>PC</td><td class='code'><code>0x{r.pc:08x}</code></td>")
        parts.append("</tr></table>")

    # Other threads
    other_traces = [t for t in traces if not ct or t.thread_id != ct.tid]
    if other_traces:
        parts.append("<details><summary>Other Threads</summary>")
        for trace in other_traces:
            parts.append(_format_trace_html(trace, source_root, is_crashed=False))
        parts.append("</details>")

    # Thread summary
    parts.append("<details><summary>Thread Summary</summary>")
    parts.append("<table class='threads'><tr><th>Name</th><th>TID</th><th>Status</th><th>PC</th></tr>")
    for t in dump.threads:
        status = "CRASHED" if t.crashed else "Waiting" if t.status == 8 else f"0x{t.status:x}"
        cls = "crashed" if t.crashed else ""
        ref = dump.resolve_address(t.pc) or "unknown"
        parts.append(f"<tr class='{cls}'><td>{h(t.name)}</td><td><code>0x{t.tid:08x}</code></td>")
        parts.append(f"<td>{status}</td><td><code>0x{t.pc:08x}</code> ({h(ref)})</td></tr>")
    parts.append("</table></details>")

    # Modules
    if dump.modules:
        parts.append("<details><summary>Loaded Modules</summary>")
        parts.append("<table class='modules'><tr><th>Module</th><th>Segment</th><th>Base</th><th>Size</th></tr>")
        for mod in dump.modules:
            for seg in mod.segments:
                parts.append(
                    f"<tr><td>{h(mod.name)}</td><td>@{seg.index + 1}</td>"
                    f"<td><code>0x{seg.base:08x}</code></td>"
                    f"<td>{seg.size:,}</td></tr>"
                )
        parts.append("</table></details>")

    parts.append("</div></body></html>")
    return "\n".join(parts)


def _format_trace_html(trace: StackTrace, source_root: Optional[str],
                       is_crashed: bool) -> str:
    """Format a single thread's stack trace as HTML."""
    h = html.escape
    parts = []

    title = "Call Stack" if is_crashed else f"Thread: {trace.thread_name}"
    tag = "h2" if is_crashed else "h3"
    parts.append(f"<{tag}>{h(title)}</{tag}>")
    parts.append("<div class='stack-trace'>")

    for frame in trace.frames:
        cls = "frame crashed" if is_crashed and frame.index == 0 else "frame"
        parts.append(f"<div class='{cls}'>")
        parts.append(f"<span class='frame-idx'>#{frame.index}</span>")

        if frame.source:
            parts.append(f"<span class='func'>{h(frame.source.function)}</span>")
            if frame.source.file and frame.source.file != "??":
                parts.append(f"<span class='loc'>{h(frame.source.file)}:{frame.source.line}</span>")
            parts.append(f"<code class='addr'>0x{frame.address:08x}</code>")

            # Inline chain
            loc = frame.source.inlined_from
            while loc:
                parts.append(f"<div class='inline'>[inlined] {h(loc.function)}")
                if loc.file and loc.file != "??":
                    parts.append(f" <span class='loc'>{h(loc.file)}:{loc.line}</span>")
                parts.append("</div>")
                loc = loc.inlined_from

            # Source context
            if source_root and frame.source.file != "??" and frame.source.line > 0:
                context = _read_source_context(source_root, frame.source.file,
                                               frame.source.line)
                if context:
                    parts.append("<pre class='source'>")
                    for ctx_line in context:
                        escaped = h(ctx_line)
                        if ctx_line.startswith(">>>"):
                            escaped = f"<mark>{escaped}</mark>"
                        parts.append(escaped)
                    parts.append("</pre>")
        else:
            parts.append(f"<code class='addr'>0x{frame.address:08x}</code>")
            if frame.module_ref:
                parts.append(f"<span class='loc'>{h(frame.module_ref)}</span>")

        parts.append("</div>")

    parts.append("</div>")
    return "\n".join(parts)


_CSS = """
body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
       background: #1e1e2e; color: #cdd6f4; margin: 0; padding: 20px; }
.report { max-width: 1000px; margin: 0 auto; }
h1 { color: #f38ba8; border-bottom: 2px solid #45475a; padding-bottom: 8px; }
h2 { color: #89b4fa; }
h3 { color: #a6e3a1; }
.meta { color: #6c7086; }
.crash-info { background: #302030; border-left: 4px solid #f38ba8; padding: 12px; margin: 16px 0; border-radius: 4px; }
.diagnosis { background: #303020; border-left: 4px solid #f9e2af; padding: 12px; margin: 16px 0; border-radius: 4px; }
.diagnosis h2 { color: #f9e2af; margin-top: 0; }
code { background: #313244; padding: 2px 6px; border-radius: 3px; font-size: 0.9em; }
.stack-trace { margin: 12px 0; }
.frame { padding: 8px 12px; margin: 4px 0; background: #313244; border-radius: 4px; border-left: 3px solid #45475a; }
.frame.crashed { border-left-color: #f38ba8; background: #3a2030; }
.frame-idx { color: #6c7086; margin-right: 8px; font-weight: bold; }
.func { color: #89dceb; font-weight: bold; margin-right: 12px; }
.loc { color: #a6adc8; }
.addr { color: #6c7086; margin-left: 8px; }
.inline { color: #9399b2; margin-left: 32px; font-size: 0.9em; }
pre.source { background: #181825; padding: 8px; margin: 4px 0 0 32px; border-radius: 4px;
             font-size: 0.85em; overflow-x: auto; }
pre.source mark { background: #4a3040; color: #f38ba8; }
table { border-collapse: collapse; margin: 8px 0; width: 100%; }
td, th { padding: 4px 8px; border: 1px solid #45475a; }
th { background: #313244; text-align: left; }
.regs td { padding: 4px 12px; }
.regs .null code { color: #f38ba8; }
.regs .code code { color: #89b4fa; }
.regs .stack code { color: #a6e3a1; }
.threads .crashed { background: #3a2030; }
details { margin: 16px 0; }
summary { cursor: pointer; color: #89b4fa; font-weight: bold; padding: 8px; }
summary:hover { background: #313244; border-radius: 4px; }
"""
