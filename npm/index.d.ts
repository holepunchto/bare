import EventEmitter, { EventMap } from 'bare-events'
import Buffer, { BufferEncoding } from 'bare-buffer'
import URL from 'bare-url'

import 'bare-queue-microtask/global'
import 'bare-buffer/global'
import 'bare-timers/global'
import 'bare-structured-clone/global'
import 'bare-url/global'
import 'bare-console/global'

interface BareEvents extends EventMap {
  uncaughtException: [err: unknown]
  unhandledRejection: [reason: unknown, promise: Promise<unknown>]
  beforeExit: [code: number]
  exit: [code: number]
  suspend: [linger: number]
  wakeup: [deadline: number]
  idle: []
  resume: []
}

interface Bare extends EventEmitter<BareEvents> {
  readonly platform: 'android' | 'darwin' | 'ios' | 'linux' | 'win32'
  readonly arch: 'arm' | 'arm64' | 'ia32' | 'x64' | 'mips' | 'mipsel'
  /** @deprecated */
  readonly simulator: boolean
  readonly argv: string[]
  readonly pid: number
  exitCode: number
  readonly version: string
  readonly versions: { readonly [library: string]: string }

  exit(code?: number): never
  suspend(linger?: number): void
  wakeup(deadline?: number): void
  idle(): void
  resume(): void
}

declare namespace Bare {
  export { Addon, Thread }
}

interface Addon {
  readonly url: URL
  readonly exports: unknown
}

declare class Addon {
  constructor(url: URL)
}

declare namespace Addon {
  export const host: string
  /** @deprecated */
  export const cache: { readonly [href: string]: Addon }

  /** @deprecated */
  export function load(url: URL): Addon
  /** @deprecated */
  export function resolve(specifier: string, parentURL?: URL): URL
}

type ThreadSource = string | Buffer

type ThreadCallback = (data: unknown) => unknown

interface ThreadOptions {
  data?: unknown
  transfer?: unknown[]
  /** @deprecated Pass the source after `filename` instead. */
  source?: ThreadSource
  encoding?: BufferEncoding
  stackSize?: number
}

interface Thread {
  readonly joined: boolean

  join(): void
  suspend(linger?: number): void
  wakeup(deadline?: number): void
  resume(): void
  terminate(): void
}

declare class Thread {
  constructor(callback: ThreadCallback)
  constructor(options?: ThreadOptions, callback?: ThreadCallback)
  constructor(filename: string, callback: ThreadCallback)
  constructor(filename: string, source: ThreadSource, options?: ThreadOptions)
  constructor(filename: string, options?: ThreadOptions, callback?: ThreadCallback)
}

declare namespace Thread {
  interface ThreadProxy {
    readonly data: unknown
  }

  export const isMainThread: boolean
  export const self: ThreadProxy | null

  /** @deprecated */
  export function create(callback: ThreadCallback): Thread
  /** @deprecated */
  export function create(options?: ThreadOptions, callback?: ThreadCallback): Thread
  /** @deprecated */
  export function create(filename: string, callback: ThreadCallback): Thread
  /** @deprecated */
  export function create(filename: string, source: ThreadSource, options?: ThreadOptions): Thread
  /** @deprecated */
  export function create(
    filename: string,
    options?: ThreadOptions,
    callback?: ThreadCallback
  ): Thread
}

declare const Bare: Bare

declare global {
  const Bare: Bare
}

export = Bare
