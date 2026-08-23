#include "f3.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(f3_postscript) == 48, "f3_postscript ABI size changed");
_Static_assert(offsetof(f3_postscript, metadata_offset) == 0, "metadata_offset ABI changed");
_Static_assert(offsetof(f3_postscript, footer_offset) == 8, "footer_offset ABI changed");
_Static_assert(offsetof(f3_postscript, data_checksum) == 16, "data_checksum ABI changed");
_Static_assert(offsetof(f3_postscript, schema_checksum) == 24, "schema_checksum ABI changed");
_Static_assert(offsetof(f3_postscript, metadata_size) == 32, "metadata_size ABI changed");
_Static_assert(offsetof(f3_postscript, footer_size) == 36, "footer_size ABI changed");
_Static_assert(offsetof(f3_postscript, major_version) == 40, "major_version ABI changed");
_Static_assert(offsetof(f3_postscript, minor_version) == 42, "minor_version ABI changed");
_Static_assert(offsetof(f3_postscript, footer_compression) == 44, "footer_compression ABI changed");
_Static_assert(offsetof(f3_postscript, checksum_type) == 45, "checksum_type ABI changed");

static void put_u16le(uint8_t *out, uint16_t value) {
  out[0] = (uint8_t)value;
  out[1] = (uint8_t)(value >> 8);
}

static void put_u32le(uint8_t *out, uint32_t value) {
  out[0] = (uint8_t)value;
  out[1] = (uint8_t)(value >> 8);
  out[2] = (uint8_t)(value >> 16);
  out[3] = (uint8_t)(value >> 24);
}

static void put_u64le(uint8_t *out, uint64_t value) {
  for (unsigned i = 0; i < 8; ++i) out[i] = (uint8_t)(value >> (i * 8));
}

static void make_valid_postscript(uint8_t out[F3_POSTSCRIPT_SIZE]) {
  memset(out, 0, F3_POSTSCRIPT_SIZE);
  put_u32le(out + 0, UINT32_C(100));
  put_u32le(out + 4, UINT32_C(40));
  out[8] = UINT8_C(0);
  out[9] = UINT8_C(0);
  put_u64le(out + 10, UINT64_C(0x0123456789abcdef));
  put_u64le(out + 18, UINT64_C(0xfedcba9876543210));
  put_u16le(out + 26, F3_FORMAT_MAJOR_VERSION);
  put_u16le(out + 28, F3_FORMAT_MINOR_VERSION);
  out[30] = (uint8_t)'F';
  out[31] = (uint8_t)'3';
}

static void poison(f3_postscript *out) { memset(out, 0xa5, sizeof(*out)); }

static void assert_zeroed(const f3_postscript *out) {
  const uint8_t *bytes = (const uint8_t *)out;
  for (size_t i = 0; i < sizeof(*out); ++i) assert(bytes[i] == 0);
}

static void test_valid_postscript(void) {
  uint8_t bytes[F3_POSTSCRIPT_SIZE];
  f3_postscript out;
  make_valid_postscript(bytes);
  poison(&out);
  assert(f3_postscript_size() == F3_POSTSCRIPT_SIZE);
  assert(f3_parse_postscript(bytes, F3_POSTSCRIPT_SIZE, UINT64_C(1000), &out) == F3_STATUS_OK);
  assert(out.metadata_offset == UINT64_C(868));
  assert(out.footer_offset == UINT64_C(928));
  assert(out.data_checksum == UINT64_C(0x0123456789abcdef));
  assert(out.schema_checksum == UINT64_C(0xfedcba9876543210));
  assert(out.metadata_size == UINT32_C(100));
  assert(out.footer_size == UINT32_C(40));
  assert(out.major_version == F3_FORMAT_MAJOR_VERSION);
  assert(out.minor_version == F3_FORMAT_MINOR_VERSION);
  assert(out.footer_compression == UINT8_C(0));
  assert(out.checksum_type == UINT8_C(0));
  assert(out.reserved[0] == 0 && out.reserved[1] == 0);
}

static void test_extra_input_is_ignored(void) {
  uint8_t bytes[F3_POSTSCRIPT_SIZE + 8];
  f3_postscript out;
  memset(bytes, 0xcc, sizeof(bytes));
  make_valid_postscript(bytes);
  assert(f3_parse_postscript(bytes, sizeof(bytes), UINT64_C(1000), &out) == F3_STATUS_OK);
  assert(out.metadata_offset == UINT64_C(868));
}

static void test_unknown_version_is_reported(void) {
  uint8_t bytes[F3_POSTSCRIPT_SIZE];
  f3_postscript out;
  make_valid_postscript(bytes);
  put_u16le(bytes + 26, UINT16_C(9));
  put_u16le(bytes + 28, UINT16_C(42));
  assert(f3_parse_postscript(bytes, sizeof(bytes), UINT64_C(1000), &out) == F3_STATUS_OK);
  assert(out.major_version == UINT16_C(9));
  assert(out.minor_version == UINT16_C(42));
}

static void test_failures_zero_output(void) {
  uint8_t bytes[F3_POSTSCRIPT_SIZE];
  f3_postscript out;

  make_valid_postscript(bytes);
  poison(&out);
  assert(f3_parse_postscript(NULL, sizeof(bytes), UINT64_C(1000), &out) == F3_STATUS_NULL_POINTER);
  assert_zeroed(&out);
  assert(f3_parse_postscript(bytes, sizeof(bytes), UINT64_C(1000), NULL) == F3_STATUS_NULL_POINTER);

  make_valid_postscript(bytes);
  poison(&out);
  assert(f3_parse_postscript(bytes, F3_POSTSCRIPT_SIZE - 1, UINT64_C(1000), &out) == F3_STATUS_SHORT_BUFFER);
  assert_zeroed(&out);

  make_valid_postscript(bytes);
  poison(&out);
  assert(f3_parse_postscript(bytes, sizeof(bytes), F3_POSTSCRIPT_SIZE - 1, &out) == F3_STATUS_FILE_TOO_SMALL);
  assert_zeroed(&out);

  make_valid_postscript(bytes);
  bytes[31] = (uint8_t)'X';
  poison(&out);
  assert(f3_parse_postscript(bytes, sizeof(bytes), UINT64_C(1000), &out) == F3_STATUS_BAD_MAGIC);
  assert_zeroed(&out);

  make_valid_postscript(bytes);
  bytes[8] = UINT8_C(3);
  poison(&out);
  assert(f3_parse_postscript(bytes, sizeof(bytes), UINT64_C(1000), &out) == F3_STATUS_UNSUPPORTED_COMPRESSION);
  assert_zeroed(&out);

  make_valid_postscript(bytes);
  bytes[9] = UINT8_C(1);
  poison(&out);
  assert(f3_parse_postscript(bytes, sizeof(bytes), UINT64_C(1000), &out) == F3_STATUS_UNSUPPORTED_CHECKSUM);
  assert_zeroed(&out);

  make_valid_postscript(bytes);
  put_u32le(bytes + 0, UINT32_C(39));
  put_u32le(bytes + 4, UINT32_C(40));
  poison(&out);
  assert(f3_parse_postscript(bytes, sizeof(bytes), UINT64_C(1000), &out) == F3_STATUS_BAD_LAYOUT);
  assert_zeroed(&out);

  make_valid_postscript(bytes);
  put_u32le(bytes + 0, UINT32_C(969));
  poison(&out);
  assert(f3_parse_postscript(bytes, sizeof(bytes), UINT64_C(1000), &out) == F3_STATUS_BAD_LAYOUT);
  assert_zeroed(&out);
}

int main(void) {
  assert(f3_abi_version() == F3_ABI_VERSION);
  assert(f3_add_u32(UINT32_C(19), UINT32_C(23)) == UINT32_C(42));
  test_valid_postscript();
  test_extra_input_is_ignored();
  test_unknown_version_is_reported();
  test_failures_zero_output();
  return 0;
}
