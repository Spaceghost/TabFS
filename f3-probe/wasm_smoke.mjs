import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

const bytes = await readFile(process.argv[2]);
const { instance } = await WebAssembly.instantiate(bytes, {});
const {
  memory,
  f3_abi_version,
  f3_add_u32,
  f3_postscript_size,
  f3_parse_postscript,
} = instance.exports;

assert.equal(typeof f3_postscript_size, 'function');
assert.equal(typeof f3_parse_postscript, 'function');
assert.equal(f3_abi_version(), 1);
assert.equal(f3_add_u32(19, 23), 42);
assert.equal(f3_postscript_size(), 32);
assert.ok(memory instanceof WebAssembly.Memory);

const inputPtr = 4096;
const outputPtr = 8192;
const resultSize = 48;
if (memory.buffer.byteLength < outputPtr + resultSize) {
  memory.grow(Math.ceil((outputPtr + resultSize - memory.buffer.byteLength) / 65536));
}

const u8 = () => new Uint8Array(memory.buffer);
const view = () => new DataView(memory.buffer);

function makeValidPostscript() {
  const fixture = new Uint8Array(32);
  const data = new DataView(fixture.buffer);
  data.setUint32(0, 100, true);
  data.setUint32(4, 40, true);
  fixture[8] = 0;
  fixture[9] = 0;
  data.setBigUint64(10, 0x0123456789abcdefn, true);
  data.setBigUint64(18, 0xfedcba9876543210n, true);
  data.setUint16(26, 0, true);
  data.setUint16(28, 1, true);
  fixture[30] = 70;
  fixture[31] = 51;
  return fixture;
}

function poisonOutput() {
  u8().fill(0xa5, outputPtr, outputPtr + resultSize);
}

function assertOutputZeroed() {
  assert.deepEqual(
    [...u8().slice(outputPtr, outputPtr + resultSize)],
    new Array(resultSize).fill(0),
  );
}

function submit(fixture, { length = fixture.length, fileSize = 1000n } = {}) {
  u8().set(fixture, inputPtr);
  poisonOutput();
  return f3_parse_postscript(inputPtr, length, fileSize, outputPtr);
}

{
  const fixture = makeValidPostscript();
  assert.equal(submit(fixture), 0);
  const out = view();
  assert.equal(out.getBigUint64(outputPtr + 0, true), 868n);
  assert.equal(out.getBigUint64(outputPtr + 8, true), 928n);
  assert.equal(out.getBigUint64(outputPtr + 16, true), 0x0123456789abcdefn);
  assert.equal(out.getBigUint64(outputPtr + 24, true), 0xfedcba9876543210n);
  assert.equal(out.getUint32(outputPtr + 32, true), 100);
  assert.equal(out.getUint32(outputPtr + 36, true), 40);
  assert.equal(out.getUint16(outputPtr + 40, true), 0);
  assert.equal(out.getUint16(outputPtr + 42, true), 1);
  assert.equal(out.getUint8(outputPtr + 44), 0);
  assert.equal(out.getUint8(outputPtr + 45), 0);
  assert.equal(out.getUint16(outputPtr + 46, true), 0);
}

{
  const fixture = new Uint8Array(40);
  fixture.fill(0xcc);
  fixture.set(makeValidPostscript(), 0);
  assert.equal(submit(fixture), 0);
  assert.equal(view().getBigUint64(outputPtr, true), 868n);
}

{
  const fixture = makeValidPostscript();
  const data = new DataView(fixture.buffer);
  data.setUint16(26, 9, true);
  data.setUint16(28, 42, true);
  assert.equal(submit(fixture), 0);
  assert.equal(view().getUint16(outputPtr + 40, true), 9);
  assert.equal(view().getUint16(outputPtr + 42, true), 42);
}

{
  poisonOutput();
  assert.equal(f3_parse_postscript(0, 32, 1000n, outputPtr), 1);
  assertOutputZeroed();
  assert.equal(f3_parse_postscript(inputPtr, 32, 1000n, 0), 1);
}

for (const [expected, mutate, options] of [
  [2, () => {}, { length: 31 }],
  [3, () => {}, { fileSize: 31n }],
  [4, fixture => { fixture[31] = 88; }, {}],
  [6, fixture => { fixture[8] = 3; }, {}],
  [7, fixture => { fixture[9] = 1; }, {}],
  [5, fixture => {
    const data = new DataView(fixture.buffer);
    data.setUint32(0, 39, true);
    data.setUint32(4, 40, true);
  }, {}],
  [5, fixture => {
    new DataView(fixture.buffer).setUint32(0, 969, true);
  }, {}],
]) {
  const fixture = makeValidPostscript();
  mutate(fixture);
  assert.equal(submit(fixture, options), expected);
  assertOutputZeroed();
}
