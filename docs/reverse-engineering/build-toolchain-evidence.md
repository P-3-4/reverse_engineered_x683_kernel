# X683/H694 kernel build-toolchain evidence

The canonical decompressed stock kernel image contains the complete Linux version/build string. Direct string extraction from the supplied compressed kernel yields:

```text
Linux version 4.14.141+ (nobody@android-build)
(Android (5484270 based on r353983c) clang version 9.0.3
(https://android.googlesource.com/toolchain/clang
745b335211bb9eadfa6aa6301f84715cee4b37c5)
(https://android.googlesource.com/toolchain/llvm
60cf23e54e46c807513f7a36d0a7b777920b5881)
(based on LLVM 9.0.3svn)) #1 SMP PREEMPT
Fri Nov 5 15:56:25 CST 2021
```

## Build constraints now proven

The first replacement-kernel build should target:

```text
ARCH=arm64
Linux 4.14.141+
Android clang 9.0.3
clang revision 745b335211bb9eadfa6aa6301f84715cee4b37c5
LLVM revision 60cf23e54e46c807513f7a36d0a7b777920b5881
Android build base r353983c
```

The stock image was built SMP and PREEMPT.

This is stronger than merely selecting a generic Android Clang 9 toolchain: the compiler/LLVM source revisions are embedded in the kernel itself.

## Not yet proven

The image string does not by itself prove the exact build-system wrapper, compiler flags, linker invocation, or Android build manifest used by Transsion. Those must still be recovered from source/config/build artifacts or inferred only after binary comparison.

Do not substitute a newer Clang 9 build and assume byte equivalence. It may be suitable for an initial functional build, but exact reproduction requires the revisions above and matching kernel-source/toolchain integration.
