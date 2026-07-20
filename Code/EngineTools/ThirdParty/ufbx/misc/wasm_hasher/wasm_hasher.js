const fs = require("fs").promises
const { readFileSync, openSync, writeSync, closeSync } = require("fs")
const path = require("path")
const { parseArgs } = require("node:util")

const options = {
    wasm: {
        type: "string",
        default: "build/ufbx_hasher.wasm",
    },
    root: {
        type: "string",
        optional: true,
    },
    check: {
        type: "string",
        optional: true,
    },
    dump: {
        type: "string",
        optional: true,
    },
    "max-dump-errors": {
        type: "string",
        default: "3",
    },
    verbose: {
        type: "boolean",
    },
}

const { values: args } = parseArgs({ options })

async function main() {
    let totalCount = 0
    let failCount = 0
    let dumpCount = 0
    const maxDumpCount = parseInt(args["max-dump-errors"])

    let dumpStream = null

    const wasmData = await fs.readFile(args.wasm)

    const memory = new WebAssembly.Memory({
        initial: 40,
        maximum: 1000,
    })

    function mem() { return wasmModule.instance.exports.memory.buffer }

    function memString(ptr, length) {
        const data8 = new Uint8Array(mem())
        const strData = data8.slice(ptr, ptr + length)
        return new TextDecoder().decode(strData)
    }

    function allocString(str) {
        const utf8 = new TextEncoder().encode(str)
        const ptr = hashAlloc(utf8.length + 1)
        const arr = new Uint8Array(mem(), ptr, utf8.length + 1)
        arr.subarray(0, arr.length - 1).set(utf8)
        arr[arr.length - 1] = 0
        return ptr
    }

    const openFiles = []

    const wasmModule = await WebAssembly.instantiate(wasmData, {
        js: { mem: memory },
        host: {
            hostVerbose: (ptr, length) => {
                if (!args.verbose) return
                const str = memString(ptr, length)
                console.log(str.trimEnd())
            },
            hostError: (ptr, length) => {
                const str = memString(ptr, length)
                console.error(str.trimEnd())
            },
            hostExit: (code) => {
                process.exit(code)
            },
            hostOpenFile: (index, namePtr, nameLen) => {
                const name = memString(namePtr, nameLen)
                if (args.verbose) {
                    console.log(`Opening ${name}`)
                }
                try {
                    const data = readFileSync(name)
                    openFiles[index] = data
                    return data.length
                } catch (err) {
                    if (err.code !== "ENOENT") {
                        console.error(`Failed to open file: ${name}`)
                    }
                    return -1
                }
            },
            hostReadFile: (index, dst, offset, count) => {
                const dstSlice = new Uint8Array(mem(), dst, count)
                const srcSlice = openFiles[index].slice(offset, offset + count)
                dstSlice.set(srcSlice)
            },
            hostCloseFile: (index) => {
                delete openFiles[index]
            },
            hostDump: (data, length) => {
                const slice = new Uint8Array(mem(), data, length)
                writeSync(dumpStream, slice)
            },
        },
    })

    function hashAlloc(size) {
        return wasmModule.instance.exports.hashAlloc(size)
    }

    function hashFree(ptr) {
        wasmModule.instance.exports.hashFree(ptr)
    }

    async function hashScene(filename, frame, dumpFile) {
        const data = await fs.readFile(filename)

        const resultPtr = hashAlloc(2)
        const dataPtr = hashAlloc(data.length)

        const dataSlice = new Uint8Array(mem(), dataPtr, data.length)
        dataSlice.set(data)

        const filenamePtr = allocString(filename)

        new DataView(mem()).setBigUint64(resultPtr, 0n, true)

        const returnCode = wasmModule.instance.exports.hashScene(resultPtr, dataPtr, data.length, filenamePtr, frame, dumpFile ? 0xDF : 0)

        if (returnCode != 0) {
            console.error(`Failed to load: ${filename}`)
            return ""
        }

        const hash = new DataView(mem()).getBigUint64(resultPtr, true)

        hashFree(dataPtr)
        hashFree(resultPtr)
        hashFree(filenamePtr)

        return hash.toString(16).padStart(16, "0")
    }

    function parseHashList(data) {
        const lines = data.split(/\n/)
        const results = []
        for (const line of lines) {
            const m = line.match(/^\s*([0-9a-zA-Z]+)\s+(\d+)\s+(\S+)\s*$/)
            if (!m) continue

            const [, hash, frame, filename] = m
            results.push({ hash, frame, filename })
        }
        return results
    }

    async function hashList(specs) {
        for (const spec of specs) {
            const hash = await hashScene(spec.filename, spec.frame, false)
            totalCount += 1

            if (hash !== spec.hash) {
                console.log(`${spec.filename}: FAIL ${hash} (local) vs ${spec.hash} (reference)`)
                failCount += 1

                if (args.dump && dumpCount < maxDumpCount) {
                    dumpCount++
                    if (dumpStream === null) {
                        dumpStream = openSync(args.dump, "w")
                        await hashScene(spec.filename, spec.frame, true)
                    }
                }
            }
        }
    }

    if (args.check) {
        const data = await fs.readFile(args.check, { encoding: "utf-8" })
        const hashes = parseHashList(data)
        await hashList(hashes)
    }

    if (dumpStream !== null) {
        closeSync(dumpStream)
    }

    console.log(`${totalCount - failCount}/${totalCount} hashes match`)
    if (failCount > 0) {
        console.error(`Error: ${failCount} files failed to load.`)
        process.exit(1)
    }
}

main()
