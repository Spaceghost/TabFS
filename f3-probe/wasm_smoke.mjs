import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
const bytes = await readFile(process.argv[2]);
const { instance } = await WebAssembly.instantiate(bytes, {});
assert.equal(instance.exports.f3_abi_version(), 1);
assert.equal(instance.exports.f3_add_u32(19, 23), 42);
assert.ok(instance.exports.memory instanceof WebAssembly.Memory);
