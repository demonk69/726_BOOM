#!/usr/bin/env python3
import csv
import sys

OUT = sys.argv[1]
PATTERN = 0x80FF7F0102030481


def store(address, data=PATTERN, mask=0xFF, resolved=True, rob_valid=True,
          sq_valid=True, address_valid=True, data_valid=True, rob_idx=30,
          sq_idx=0, allocation=100, rob_generation=1, sq_generation=None,
          sq_allocation=None):
    if sq_generation is None:
        sq_generation = rob_generation
    if sq_allocation is None:
        sq_allocation = allocation
    rob = (((1 if rob_valid else 0) << 63) | (allocation << 32) |
           (rob_generation << 16) | (sq_idx << 8) | rob_idx)
    sq = ((sq_allocation << 32) | (sq_generation << 16) |
          ((1 if resolved else 0) << 3) |
          ((1 if data_valid else 0) << 2) |
          ((1 if address_valid else 0) << 1) | (1 if sq_valid else 0))
    return {"rob": rob, "sq": sq, "address": address, "data": data,
            "mask": mask, "resolved": resolved, "rob_idx": rob_idx,
            "sq_idx": sq_idx, "allocation": allocation,
            "rob_valid": rob_valid,
            "eligible": rob_valid and resolved and sq_valid and address_valid and
                        data_valid and rob_generation == sq_generation and
                        allocation == sq_allocation}


def extend(value, size, signed):
    bits = (1 << size) * 8
    mask = (1 << bits) - 1 if bits < 64 else (1 << 64) - 1
    value &= mask
    if signed and bits < 64 and value & (1 << (bits - 1)):
        value |= ((1 << 64) - 1) ^ mask
    return value & ((1 << 64) - 1)


def expected(stores, head, load_rob, address, size, signed):
    ordered = []
    idx = head
    for _ in range(32):
        if idx == load_rob:
            break
        ordered.extend(s for s in stores if s["rob_valid"] and
                       s["rob_idx"] == idx)
        idx = (idx + 1) % 32
    else:
        return 0, 0, 0, 0
    selected = None
    load_bytes = 1 << size
    for s in ordered:
        if not s["eligible"]:
            return 0, 0, 0, 0
        overlap = any((s["mask"] & (1 << sb)) and
                      s["address"] + sb == address + lb
                      for lb in range(load_bytes) for sb in range(8))
        if overlap:
            selected = s
    if selected is None:
        return 1, 0, 0, 0
    raw = 0
    coverage = 0
    for lb in range(load_bytes):
        matches = [sb for sb in range(8)
                   if selected["mask"] & (1 << sb) and
                   selected["address"] + sb == address + lb]
        if matches:
            sb = matches[-1]
            coverage |= 1 << lb
            raw |= ((selected["data"] >> (8 * sb)) & 0xFF) << (8 * lb)
    requested = (1 << load_bytes) - 1
    if coverage != requested:
        return 0, selected["sq_idx"], 0, 0
    return 2, selected["sq_idx"], extend(raw, size, signed), coverage


cases = []


def add(name, stores, address=0x1000, size=3, signed=False, head=30,
        load_rob=1):
    while len(stores) < 3:
        stores.append(store(0, rob_valid=False, rob_idx=0, sq_idx=len(stores)))
    decision, selected, value, coverage = expected(
        stores, head, load_rob, address, size, signed)
    row = {"case_id": len(cases), "case_name": name,
           "head": head, "load_rob": load_rob, "load_allocation": 900,
           "load_address": address, "load_size": size,
           "load_signed": int(signed), "expected_decision": decision,
           "expected_selected_sq": selected, "expected_value": value,
           "expected_coverage": coverage}
    for i, s in enumerate(stores):
        for key in ("rob", "sq", "address", "data", "mask"):
            row[f"store{i}_{key}"] = s[key]
    cases.append(row)


add("no_older_store", [])
for i, address in enumerate((0x2000, 0x0FF0, 0x1010, 0x3004, 0x0F00)):
    add(f"known_nonalias_{i}", [store(address)])
for size, mnemonic in enumerate(("byte", "half", "word", "double")):
    width = 1 << size
    add(f"exact_{mnemonic}_signed", [store(0x1000, mask=(1 << width) - 1)],
        size=size, signed=True)
    add(f"exact_{mnemonic}_unsigned", [store(0x1000, mask=(1 << width) - 1)],
        size=size, signed=False)
    add(f"wide_subload_{mnemonic}", [store(0x1000)],
        address=0x1000 + (0 if size == 3 else 1), size=size)
add("youngest_of_two_full", [store(0x1000, data=0x1111111111111111,
     rob_idx=30, sq_idx=0), store(0x1000, data=0x2222222222222222,
     rob_idx=31, sq_idx=1, allocation=101)])
add("older_full_younger_partial", [store(0x1000, rob_idx=30, sq_idx=0),
     store(0x1000, mask=0x01, rob_idx=31, sq_idx=1, allocation=101)])
add("two_partial_union_full", [store(0x1000, mask=0x0F, rob_idx=30, sq_idx=0),
     store(0x1000, mask=0xF0, rob_idx=31, sq_idx=1, allocation=101)])
add("same_address_partial", [store(0x1000, mask=0x03)], size=2)
add("younger_store_ignored", [store(0x1000, rob_idx=2, sq_idx=0)], load_rob=1)
add("unresolved_older", [store(0x1000, resolved=False)])
add("stale_sq_generation", [store(0x1000, rob_generation=2, sq_generation=1)])
add("stale_rob_allocation", [store(0x1000, allocation=100, sq_allocation=99)])
add("invalid_sq_owner", [store(0x1000, sq_valid=False)])
add("unknown_store_address", [store(0x1000, address_valid=False)])
add("unknown_store_data", [store(0x1000, data_valid=False)])
add("rob_wrap_full", [store(0x1000, rob_idx=31, sq_idx=2)], head=31,
    load_rob=0, size=2)
add("rob_wrap_nonalias", [store(0x2000, rob_idx=31, sq_idx=2)], head=31,
    load_rob=0, size=2)
add("same_base_disjoint_masks", [store(0x1000, mask=0x0F)],
    address=0x1004, size=2)
add("offset_extract", [store(0x1000)], address=0x1003, size=2)
add("negative_lb", [store(0x1000, data=0x80)], size=0, signed=True)
add("negative_lh", [store(0x1000, data=0x8001)], size=1, signed=True)
add("negative_lw", [store(0x1000, data=0x80000001)], size=2, signed=True)
add("zero_lbu", [store(0x1000, data=0xFF)], size=0)
add("zero_lhu", [store(0x1000, data=0xFFFF)], size=1)
add("zero_lwu", [store(0x1000, data=0xFFFFFFFF)], size=2)
add("wide_byte_at_offset7", [store(0x1000)], address=0x1007, size=0)
add("word_store_half_subload", [store(0x1000, mask=0x0F)],
    address=0x1002, size=1)
add("multiple_known_nonalias", [store(0x2000, rob_idx=30, sq_idx=0),
    store(0x3000, rob_idx=31, sq_idx=1, allocation=101)])
add("three_store_youngest_full", [store(0x1000, data=0x11, rob_idx=30, sq_idx=0),
    store(0x2000, rob_idx=31, sq_idx=1, allocation=101),
    store(0x1000, data=0x33, rob_idx=0, sq_idx=2, allocation=102)])
add("youngest_partial_over_full", [store(0x1000, rob_idx=30, sq_idx=0),
    store(0x2000, rob_idx=31, sq_idx=1, allocation=101),
    store(0x1000, mask=0x80, rob_idx=0, sq_idx=2, allocation=102)])
add("store_mask_hole", [store(0x1000, mask=0xFB)])
add("narrow_store_blocks_dword", [store(0x1000, mask=0x0F)])
add("load_before_all_stores", [store(0x1000, rob_idx=2, sq_idx=0),
    store(0x1000, rob_idx=3, sq_idx=1, allocation=101)], load_rob=1)
add("stale_older_blocks_fresh_candidate", [store(0x2000, rob_generation=2,
    sq_generation=1, rob_idx=30, sq_idx=0), store(0x1000, rob_idx=31,
    sq_idx=1, allocation=101)])

assert len(cases) == 48
fields = list(cases[0])
with open(OUT, "w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(cases)
