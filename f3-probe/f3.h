#ifndef F3_NELUA_H
#define F3_NELUA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define F3_ABI_VERSION 1u
#define F3_POSTSCRIPT_SIZE 32u
#define F3_FORMAT_MAJOR_VERSION 0u
#define F3_FORMAT_MINOR_VERSION 1u

typedef enum f3_status {
  F3_STATUS_OK = 0,
  F3_STATUS_NULL_POINTER = 1,
  F3_STATUS_SHORT_BUFFER = 2,
  F3_STATUS_FILE_TOO_SMALL = 3,
  F3_STATUS_BAD_MAGIC = 4,
  F3_STATUS_BAD_LAYOUT = 5,
  F3_STATUS_UNSUPPORTED_COMPRESSION = 6,
  F3_STATUS_UNSUPPORTED_CHECKSUM = 7
} f3_status;

typedef struct f3_postscript {
  uint64_t metadata_offset;
  uint64_t footer_offset;
  uint64_t data_checksum;
  uint64_t schema_checksum;
  uint32_t metadata_size;
  uint32_t footer_size;
  uint16_t major_version;
  uint16_t minor_version;
  uint8_t footer_compression;
  uint8_t checksum_type;
  uint8_t reserved[2];
} f3_postscript;

uint32_t f3_abi_version(void);
uint32_t f3_add_u32(uint32_t a, uint32_t b);
uint32_t f3_postscript_size(void);
int32_t f3_parse_postscript(const uint8_t *bytes, uint32_t length,
                            uint64_t file_size, f3_postscript *out);

#ifdef __cplusplus
}
#endif

#endif
