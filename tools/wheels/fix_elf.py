#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
In-place ELF patcher for DT_NEEDED and DT_RPATH/DT_RUNPATH entries.

Replaces strings by overwriting bytes in the dynamic string table,
zero-padding to the original length. Because no sections or segments
are moved, this avoids the page-alignment assertion that patchelf hits
on aarch64 (64KB pages) when rewriting executables.

Locates the dynamic string table via program headers (PT_DYNAMIC →
DT_STRTAB), so it works on stripped binaries where section headers
have been removed.

Only valid when the replacement string is <= the original length —
always true when replacing absolute build paths with basenames or
with a shorter $ORIGIN-relative RPATH.

Usage:
  fix_elf.py replace-needed <old> <new> <elf>
  fix_elf.py set-rpath <rpath> <elf>
  fix_elf.py print-needed <elf>
  fix_elf.py print-rpath <elf>
"""

import struct
import sys

# Dynamic section tag constants
DT_NULL = 0
DT_NEEDED = 1
DT_STRTAB = 5
DT_STRSZ = 10
DT_RPATH = 15
DT_RUNPATH = 29

# Program header type constants
PT_LOAD = 1
PT_DYNAMIC = 2


def _read(path):
    with open(path, "rb") as f:
        return bytearray(f.read())


def _write(path, data):
    with open(path, "r+b") as f:
        f.seek(0)
        f.write(data)


def _parse_header(data):
    assert data[:4] == b"\x7fELF", "not an ELF file"
    bits = 64 if data[4] == 2 else 32
    endian = "<" if data[5] == 1 else ">"
    # e_ident is 16 bytes; the typed fields start at offset 16.
    # e_type(H) e_machine(H) e_version(I) e_entry(Q) e_phoff(Q)
    # e_shoff(Q) e_flags(I) e_ehsize(H) e_phentsize(H) e_phnum(H)
    # e_shentsize(H) e_shnum(H) e_shstrndx(H)
    if bits == 64:
        f = struct.unpack_from(endian + "HHIQQQIHHHHHH", data, 16)
        return dict(bits=64, endian=endian, phoff=f[4], phentsize=f[8], phnum=f[9])
    else:
        f = struct.unpack_from(endian + "HHIIIIIHHHHHH", data, 16)
        return dict(bits=32, endian=endian, phoff=f[4], phentsize=f[8], phnum=f[9])


def _parse_phdrs(data, hdr):
    """Parse ELF program headers."""
    off, entsize, n = hdr["phoff"], hdr["phentsize"], hdr["phnum"]
    fmt, bits = hdr["endian"], hdr["bits"]
    phdrs = []
    for i in range(n):
        base = off + i * entsize
        if bits == 64:
            # p_type(I) p_flags(I) p_offset(Q) p_vaddr(Q) p_paddr(Q)
            # p_filesz(Q) p_memsz(Q) p_align(Q)
            f = struct.unpack_from(fmt + "IIQQQQQQ", data, base)
            phdrs.append(dict(type=f[0], offset=f[2], vaddr=f[3], filesz=f[5]))
        else:
            # p_type(I) p_offset(I) p_vaddr(I) p_paddr(I)
            # p_filesz(I) p_memsz(I) p_flags(I) p_align(I)
            f = struct.unpack_from(fmt + "IIIIIIII", data, base)
            phdrs.append(dict(type=f[0], offset=f[1], vaddr=f[2], filesz=f[4]))
    return phdrs


def _vaddr_to_offset(phdrs, vaddr):
    """Convert virtual address to file offset via PT_LOAD segments."""
    for ph in phdrs:
        if ph["type"] == PT_LOAD:
            if ph["vaddr"] <= vaddr < ph["vaddr"] + ph["filesz"]:
                return ph["offset"] + (vaddr - ph["vaddr"])
    return None


def _parse_dynamic(data, dyn_offset, dyn_filesz, hdr):
    """Parse dynamic section entries, return list of (tag, val, entry_offset)."""
    fmt, bits = hdr["endian"], hdr["bits"]
    tag_fmt = fmt + ("qQ" if bits == 64 else "iI")
    entry_size = 16 if bits == 64 else 8
    entries = []
    for i in range(dyn_filesz // entry_size):
        entry_off = dyn_offset + i * entry_size
        tag, val = struct.unpack_from(tag_fmt, data, entry_off)
        entries.append((tag, val, entry_off))
        if tag == DT_NULL:
            break
    return entries


def _load(elf_path):
    """
    Load ELF and locate the dynamic string table and dynamic section.
    Uses program headers (works on stripped binaries — no section headers needed).
    Returns (data, hdr, dynstr_info, dynamic_entries).
    dynstr_info = {'offset': file_offset, 'size': byte_count}
    """
    data = _read(elf_path)
    hdr = _parse_header(data)
    phdrs = _parse_phdrs(data, hdr)

    # Locate PT_DYNAMIC
    dyn_ph = next((ph for ph in phdrs if ph["type"] == PT_DYNAMIC), None)
    if not dyn_ph:
        sys.exit(f"error: no PT_DYNAMIC segment in {elf_path}")

    dyn_entries = _parse_dynamic(data, dyn_ph["offset"], dyn_ph["filesz"], hdr)

    # Find DT_STRTAB (virtual address) and DT_STRSZ
    strtab_vaddr = None
    strtab_size = 65536  # fallback if DT_STRSZ absent
    for tag, val, _ in dyn_entries:
        if tag == DT_STRTAB:
            strtab_vaddr = val
        elif tag == DT_STRSZ:
            strtab_size = val

    if strtab_vaddr is None:
        sys.exit(f"error: no DT_STRTAB in {elf_path}")

    strtab_offset = _vaddr_to_offset(phdrs, strtab_vaddr)
    if strtab_offset is None:
        sys.exit(f"error: DT_STRTAB vaddr 0x{strtab_vaddr:x} not mapped in {elf_path}")

    dynstr = {"offset": strtab_offset, "size": strtab_size}
    return data, hdr, dynstr, dyn_entries


def _str_at(data, dynstr, str_offset):
    start = dynstr["offset"] + str_offset
    end = data.index(b"\x00", start)
    return data[start:end].decode()


def _patch_str(data, dynstr, old, new):
    """Overwrite 'old' with 'new' (null-padded) in the string table. Returns True on success."""
    old_b = old.encode() + b"\x00"
    new_b = new.encode() + b"\x00"
    if len(new_b) > len(old_b):
        raise ValueError(
            f"replacement '{new}' ({len(new_b)}B) longer than original '{old}' ({len(old_b)}B)"
        )
    start = dynstr["offset"]
    window = data[start : start + dynstr["size"]]
    idx = window.find(old_b)
    if idx < 0:
        return False
    padded = new_b + b"\x00" * (len(old_b) - len(new_b))
    data[start + idx : start + idx + len(old_b)] = padded
    return True


def cmd_replace_needed(old, new, elf_path):
    data, hdr, dynstr, _ = _load(elf_path)
    if _patch_str(data, dynstr, old, new):
        _write(elf_path, data)
        print(f"  replaced DT_NEEDED '{old}' → '{new}'")
    else:
        print(f"  '{old}' not found in string table (already replaced?)")


def cmd_set_rpath(new_rpath, elf_path):
    data, hdr, dynstr, dyn_entries = _load(elf_path)
    for tag, val, _ in dyn_entries:
        if tag in (DT_RPATH, DT_RUNPATH):
            old_rpath = _str_at(data, dynstr, val)
            if _patch_str(data, dynstr, old_rpath, new_rpath):
                _write(elf_path, data)
                print(f"  set rpath '{old_rpath}' → '{new_rpath}'")
                return
    print(f"  warning: no DT_RPATH/DT_RUNPATH in {elf_path}; rpath not set")


def cmd_print_needed(elf_path):
    data, hdr, dynstr, dyn_entries = _load(elf_path)
    for tag, val, _ in dyn_entries:
        if tag == DT_NEEDED:
            print(_str_at(data, dynstr, val))


def cmd_print_rpath(elf_path):
    data, hdr, dynstr, dyn_entries = _load(elf_path)
    for tag, val, _ in dyn_entries:
        if tag in (DT_RPATH, DT_RUNPATH):
            print(_str_at(data, dynstr, val))


if __name__ == "__main__":
    cmds = {
        "replace-needed": (cmd_replace_needed, 3, "<old> <new> <elf>"),
        "set-rpath": (cmd_set_rpath, 2, "<rpath> <elf>"),
        "print-needed": (cmd_print_needed, 1, "<elf>"),
        "print-rpath": (cmd_print_rpath, 1, "<elf>"),
    }
    if len(sys.argv) < 2 or sys.argv[1] not in cmds:
        print(__doc__)
        sys.exit(1)
    fn, nargs, usage = cmds[sys.argv[1]]
    if len(sys.argv) != 2 + nargs:
        sys.exit(f"usage: {sys.argv[0]} {sys.argv[1]} {usage}")
    fn(*sys.argv[2:])
