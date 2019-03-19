#!/usr/bin/env node
const fs = require('fs')
const path = require('path')
const {spawnSync} = require('child_process')

const allTypesRaw = fs.readFileSync(__dirname + '/src/AllTypes.re', 'utf8')
const versions = allTypesRaw.split('\n').filter(line => line.match(/^module V/))
.map(line => {
  const [_, major, __, minor] = line.match(/^module V(\d+)(_(\d+))?/);
  return {major: parseInt(major), minor: parseInt(minor)}
})

const typesJson = __dirname + '/types.json'
const typesLock = __dirname + '/types.lock.json'

const config = {
  "//": "AUTOGENERATED by run_all.js",
  "version": 6,
  "$schemaVersion": 1,
  "output": "src/Serde.re",
  "engine": "Js.Json",
  "entries": [
    {
      "file": "src/Types.re",
      "type": "household"
    }
  ],
  "custom": []
}

const asFull = ({major, minor}) => major + (minor ? '_' + minor : '');

const runVersion = (version, allVersions) => {
  let full = asFull(version)
  console.log('For version', full)
  fs.writeFileSync(
    __dirname + '/src/Types.re',
    `include AllTypes.V${full}; let version = "${full}"; let all_versions = [${
      allVersions.map(version => `"${asFull(version)}"`).join(', ')
    }]`
  )
  fs.writeFileSync(typesJson, JSON.stringify({...config, version: version.major}, null, 2))
}

const run = (cmd, args) => {
  const result = spawnSync(cmd, args, {
    stdio: 'inherit',
    cwd: __dirname,
  })
  if (result.status !== 0) {
    console.log(`Error running ${cmd} ${args.join(' ')}. Aborting`)
    process.exit(result.status)
  }
};

run('rm', ['-f', 'types.lock.json'])
const old_data = fs.readdirSync(__dirname).filter(name => name.match(/^data\..*\.json$/))
if (old_data.length) {
  run('rm', ['-f'].concat(old_data))
}

versions.forEach((version, i) => {
  runVersion(version, versions.slice(0, i + 1));
  run('npm', ['start', '-s'])
  run('rm', ['-rf', 'lib'])
  run('npm', ['run', '-s', 'try'])
})