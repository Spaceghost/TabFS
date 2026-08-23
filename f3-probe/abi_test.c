#include "f3.h"
#include <assert.h>
#include <stdint.h>
int main(void) {
  assert(f3_abi_version() == F3_ABI_VERSION);
  assert(f3_add_u32(UINT32_C(19), UINT32_C(23)) == UINT32_C(42));
  return 0;
}
