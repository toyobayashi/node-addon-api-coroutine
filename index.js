'use strict'
Object.defineProperty(exports, '__esModule', { value: true })

const path = require('path')

const includeDir = path.relative('.', __dirname)

exports.include_dir = includeDir
exports.targets = path.join(includeDir, 'naaco.gyp')
