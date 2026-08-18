# X683/H694 Transsion GC helper-cluster deep pass

Source: uploaded stock boot.img SHA-256 `a4908a19aacb463bd7028cb3a411a62a0486c458920c62cf89d42bed19c8f180`.

## 1. `0x4cbe38` — USB/charger wakelock scanner

The function begins from a vendor wakelock-list object, iterates active entries (`entry + 0xa8` bit 0), obtains a string/name pointer from `entry - 0x8`, and tests it against three literals:

- `"usb"`
- `"USB"`
- `"charger"`

When a match is found it logs the literal at image offset `0x10bc714`:

`usb hold the wakelock`

It increments a counter for scanned active entries and returns that counter. The function is therefore a wakelock-list inspection helper associated with USB/charger detection; it is not itself `detect_charger_type()`.

## 2. `0xb271c` — general wakelock scanner

This helper obtains the wakelock collection, iterates entries, reads `entry + 0xc8`, masks bit 0, and accumulates the number of entries with that bit set. It then logs:

`tran_show_wakelocks ret is %d`

and returns the count.

Therefore `0xb271c` is a general wakelock-state/count helper. It is distinct from the USB/charger-name scanner at `0x4cbe38`.

## 3. State-machine consequence at `0x3778d4`

Stock control flow:

```text
save detector baselines
    |
    +-> 0x4cbe38
    |      if nonzero -> proceed to exit/recheck path
    |
    +-> 0xb271c
           if nonzero -> proceed to exit/recheck path
           if zero    -> loop back to detector wait path
```

Thus the detector does not simply sleep unconditionally. It repeatedly inspects wakelock state and only advances through the normal detector loop when the relevant wake/abort conditions permit it.

## 4. `0x366cd4` — F2FS/GC policy gate

This function is called from `0x37742c` with the F2FS superblock pointer in `x0`, immediately after `tran_f2fs_gc()` returns. It reads `sbi + 0x534` (`gc_mode`) and performs multiple F2FS capacity/reservation checks, including:

- `mount_opt` state;
- free/reserved segment quantities through `sm_info`;
- block-count scaling using `0x51EB851F >> 37` (~2.5%);
- `reserved_blocks`-related comparisons;
- additional current-segment/time based thresholds.

It also contains explicit handling for `gc_mode == 3` and invokes the normal F2FS GC machinery at lower offsets. This strongly identifies it as a vendor/F2FS GC policy gate, but its proprietary source-level name is not proven from the current direct evidence. Do not label it `tran_urgent_gc()` yet.

## 5. Actual vendor debug/control names

The literals `detect_wakelock`, `need_switch_ssr`, `tran_urgent_gc`, and `detect_charger_type` occur in the vendor debug/control registration area around `0x37afxx..0x37b5xx`. Their presence is proven, but the string references are used to register/debug named controls; their occurrence alone does not establish that the corresponding string address is a callable function.

Directly resolved runtime helper usage therefore remains:

```text
USB/charger wakelock scanner  = 0x4cbe38
General wakelock scanner      = 0xb271c
F2FS/GC policy gate           = 0x366cd4 (name unresolved)
```

## 6. Correction to previous interpretation

Previous notes were too strong in suggesting that `0x4cbe38` or `0xb271c` directly implemented `need_switch_ssr()`, `detect_charger_type()`, or `tran_urgent_gc()`. They are now quarantined as wakelock helpers until a direct caller/name binding is proven.
