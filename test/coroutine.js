const assert = require('assert');

async function runTestWithBindingPath (test, buildType, buildPathRoot = process.env.BUILD_PATH || '') {
  const path = require('path');
  const { spawnSync } = require('child_process');

  buildType = buildType || 'Release';
  const bindings = [
    path.join(buildPathRoot, `./build/${buildType}/binding_cpp20.node`),
    path.join(buildPathRoot, `./build/${buildType}/binding_cpp20_noexcept.node`),
    path.join(buildPathRoot, `./build/${buildType}/binding_cpp20_noexcept_maybe.node`),
    path.join(buildPathRoot, `./build/${buildType}/binding_cpp20_custom_namespace.node`)
  ].map(it => require.resolve(it));

  for (const item of bindings) {
    console.log(`> node ${__filename} ${item}`)
    const { status } = spawnSync('node', [__filename, item], { stdio: 'inherit' })
    assert.strictEqual(status, 0)
    console.log()
  }

  for (const item of bindings) {
    console.log(`> test("${item}")`)
    await test(item)
    console.log()
  }
}

if (process.argv[2]) {
  test(process.argv[2])
} else {
  module.exports = runTestWithBindingPath(test, undefined, __dirname);
}

async function test (path) {
  const binding = require(path);

  const array = await binding.testOrder();
  console.log(array)
  assert.deepStrictEqual(array, [1, 3, 2]);

  const promise = binding.coroutine(function () {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        resolve(42);
      }, 500);
    });
  });
  assert.ok(promise instanceof Promise);
  const result = await promise
  console.log(result)
  assert.strictEqual(result, 42 * 4);

  try {
    await binding.coroutine(function () {
      return new Promise((resolve, reject) => {
        setTimeout(() => {
          reject(new Error('42'));
        }, 500);
      });
    });
    throw new Error('should throw');
  } catch (err) {
    console.log(err.message)
    assert.strictEqual(err.message, '42');
  }

  try {
    await binding.coroutineThrow(function () {
      return new Promise((resolve, reject) => {
        setTimeout(() => {
          resolve(42);
        }, 500);
      });
    });
  } catch (err) {
    console.log(err.message)
    assert.strictEqual(err.message, 'test error');
  }

  const lastError = await binding.otherTypeCoroutine();
  console.log(lastError)
  assert.strictEqual(lastError, 'task error');
}
