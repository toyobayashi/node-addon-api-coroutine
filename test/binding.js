const { coroutineFunction } = require('./build/Release/binding.node')

coroutineFunction(function () {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve(42)
    }, 1000)
  })
}).then(result => {
  console.log(result) // 42
})
