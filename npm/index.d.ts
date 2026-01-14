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
  readonly simulator: boolean
  readonly argv: string[]
  readonly pid: number
  exitCode: number
  readonly version: string
  readonly versions: { readonly [package: string]: string }

  exit(code?: number): never
  suspend(linger?: number): void
  wakeup(deadline?: number): void
  idle(): void
  resume(): void

  Addon: typeof Addon
  Thread: typeof Thread
}

interface Addon {
  readonly url: URL
  readonly exports: unknown
}

declare class Addon {}

declare namespace Addon {
  export const cache: { readonly [href: string]: Addon }
  export const host: string

  export function load(url: URL): Addon
  export function resolve(specifier: string, parentURL?: URL): URL
}

interface ThreadOptions {
  data?: unknown
  transfer?: unknown[]
  source?: string | Buffer
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
  constructor(options?: ThreadOptions)
  constructor(filename: string, options?: ThreadOptions)
}

declare namespace Thread {
  interface ThreadProxy {
    readonly data: any
  }

  export const isMainThread: boolean
  export const self: ThreadProxy | null
}

declare const Bare: Bare

declare global {
  const Bare: Bare
}

export = Bare
