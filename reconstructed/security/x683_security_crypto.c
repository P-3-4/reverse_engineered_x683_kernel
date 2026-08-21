/* X683 security/crypto surface selected from the actual configuration. */
#include <stdint.h>

struct x683_security_feature { const char *feature; const char *config; uint8_t enabled; };
static const struct x683_security_feature x683_security[] __attribute__((used)) = {
    { "SELinux", "CONFIG_SECURITY_SELINUX", 1 },
    { "F2FS encryption", "CONFIG_F2FS_FS_ENCRYPTION", 1 },
    { "FS encryption", "CONFIG_FS_ENCRYPTION", 1 },
    { "dm-verity", "CONFIG_DM_VERITY", 1 },
    { "dm-verity FEC", "CONFIG_DM_VERITY_FEC", 1 },
    { "AES", "CONFIG_CRYPTO_AES", 1 },
    { "XTS", "CONFIG_CRYPTO_XTS", 1 },
    { "GCM", "CONFIG_CRYPTO_GCM", 1 },
    { "SHA-256", "CONFIG_CRYPTO_SHA256", 1 },
    { "SHA-512", "CONFIG_CRYPTO_SHA512", 1 },
    { "Microtrust TEE", "CONFIG_MICROTRUST_TEE_SUPPORT", 1 },
    { "generic TEE API", "CONFIG_TEE", 0 },
};

/* Configuration proves the feature surface. It does not by itself prove that
 * every enabled algorithm is exercised by X683 userspace; kallsyms, source
 * fingerprints and call/data-reference evidence are required for that claim. */
