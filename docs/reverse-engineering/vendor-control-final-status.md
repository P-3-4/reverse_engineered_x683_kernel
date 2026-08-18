# X683/H694 vendor GC controls — final status for stages #4/#5

## `need_switch_ssr`

- Registered at `0x37af88`.
- Descriptor/backing object: `Image + 0x173b9d0`.
- Passed through common registration `0x274ea0 -> 0x274dac`.
- No direct branch/call from the Stop-4 basic block to a function named `need_switch_ssr`.
- Stop 4 directly writes controller `+0x998 = 2` when `+0x9c0` permits it.

Status: **control/data binding proven; implementation callback not proven**.

## `tran_urgent_gc`

- Registered at `0x37b068`.
- Descriptor/backing object: `Image + 0x173bbb0`.
- Same generic registration/operation framework.
- Stop 4 does not directly call a function named `tran_urgent_gc`.
- The proven urgent action is the controller transition consumed later by `tran_f2fs_gc`, which temporarily sets `sbi->gc_mode = 3`.

Status: **control/data binding proven; direct callback not proven**.

## `detect_charger_type`

- Registered at `0x37b184`.
- Descriptor/backing object: `Image + 0x173bf70`.
- Registration is direct, but its consumers are outside the currently resolved GC detector path.

Status: **control/data binding proven; runtime consumer unresolved**.

## `tran_gc_usb_wakelock`

String exists at `Image + 0x10a5ee7` but is not in the contiguous registration sequence containing the three controls above.

The separate registration/use path is therefore not safely assigned.

Status: **unresolved**.

## Architectural conclusion

The evidence supports:

```text
named control
  → common registry node
  → per-control backing descriptor/state
  → generic attribute operation
```

not:

```text
named control
  → unique standalone function
```

Accordingly, the reconstructed `tran_gc_thread_func()` must use the directly proven controller/descriptor values rather than invented calls to these names.
