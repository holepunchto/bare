import EventEmitter, { EventMap } from 'bare-events'
import Buffer, { BufferEncoding } from 'bare-buffer'
import URL from 'bare-url'
import { Duplex } from 'bare-stream'

import 'bare-queue-microtask/global'
import 'bare-buffer/global'
import 'bare-timers/global'
import 'bare-structured-clone/global'
import 'bare-url/global'
import 'bare-console/global'

interface BareEvents extends EventMap {
  /**
   * Emitted when a JavaScript exception is thrown within an execution context
   * without being caught by any exception handlers within that execution
   * context. By default, uncaught exceptions are printed to `stderr` and the
   * process aborted. Adding an event listener for the `uncaughtException` event
   * overrides the default behavior.
   */
  uncaughtException: [err: unknown]

  /**
   * Emitted when a JavaScript promise is rejected within an execution context
   * without that rejection being handled within that execution context. By
   * default, unhandled rejections are printed to `stderr` and the process
   * aborted. Adding an event listener for the `unhandledRejection` event
   * overrides the default behavior.
   */
  unhandledRejection: [reason: unknown, promise: Promise<unknown>]

  /**
   * Emitted when the loop runs out of work and before the process or current
   * thread exits. This provides a chance to schedule additional work and keep
   * the process from exiting. If additional work is scheduled, `beforeExit`
   * will be emitted again once the loop runs out of work.
   *
   * If the process is exited explicitly, such as by calling `Bare.exit()` or
   * as the result of an uncaught exception, the `beforeExit` event will not be
   * emitted.
   */
  beforeExit: [code: number]

  /**
   * Emitted when the process or current thread exits. If the process is
   * forcefully terminated from an `exit` event listener, the remaining
   * listeners will not run.
   *
   * Additional work **MUST NOT** be scheduled from an `exit` event listener.
   */
  exit: [code: number]

  /**
   * Emitted when the process or current thread is suspended. Any in-progress
   * or outstanding work, such as network activity or file system access,
   * should be deferred, cancelled, or paused when the `suspend` event is
   * emitted and no additional work should be scheduled. A `suspend` event
   * listener may call `Bare.resume()` to cancel the suspension.
   */
  suspend: [linger: number]

  /**
   * Emitted when the process or current thread wakes up during suspension.
   * Once the process becomes idle, or if the process is not idle by the time
   * `deadline` has passed, the process will suspend itself again and an `idle`
   * event be emitted. A `wakeup` event listener may call `Bare.resume()` to
   * resume the process.
   */
  wakeup: [deadline: number]

  /**
   * Emitted when the process or current thread becomes idle after suspension.
   * After all handlers have run, the event loop will block and no additional
   * work be performed until the process is resumed. An `idle` event listener
   * may call `Bare.resume()` to cancel the suspension.
   */
  idle: []

  /**
   * Emitted when the process or current thread resumes after suspension.
   * Deferred and paused work should be continued when the `resume` event is
   * emitted and new work may again be scheduled.
   */
  resume: []
}

/**
 * The core JavaScript API of Bare, available through the global `Bare`
 * namespace.
 */
interface Bare extends EventEmitter<BareEvents> {
  /**
   * The identifier of the operating system for which Bare was compiled.
   */
  readonly platform: 'android' | 'darwin' | 'ios' | 'linux' | 'win32'

  /**
   * The identifier of the processor architecture for which Bare was compiled.
   */
  readonly arch: 'arm' | 'arm64' | 'ia32' | 'x64' | 'mips' | 'mipsel' | 'riscv64'

  /** @deprecated */
  readonly simulator: boolean

  /**
   * The command line arguments passed to the process when launched.
   */
  readonly argv: string[]

  /**
   * The ID of the current process.
   */
  readonly pid: number

  /**
   * The code that will be returned once the process exits. If the process is
   * exited using `Bare.exit()` without specifying a code, `Bare.exitCode` is
   * used.
   */
  exitCode: number

  /**
   * The Bare version string.
   */
  readonly version: string

  /**
   * An object containing the version strings of Bare and its dependencies.
   */
  readonly versions: { readonly [library: string]: string }

  /**
   * Support for optional streaming communication between an embedder and
   * JavaScript code. By default, its value is `null` indicating that streaming
   * communication is not supported. If set by embedders, `Bare.IPC` is
   * expected to be an instance of a `Duplex` stream.
   *
   * This is an advanced API that users should never have to interact with
   * directly.
   */
  IPC: Duplex | null

  /**
   * Immediately terminate the process or current thread with an exit status of
   * `code` which defaults to `Bare.exitCode`.
   */
  exit(code?: number): never

  /**
   * Suspend the process and all threads. This will emit a `suspend` event
   * signalling that all work should stop immediately. When all work has
   * stopped and the process would otherwise exit, an `idle` event will be
   * emitted. If the process is not resumed from an `idle` event listener, the
   * loop will block until the process is resumed.
   */
  suspend(linger?: number): void

  /**
   * Wake the process and all threads during suspension. This will emit a
   * `wakeup` event signalling that work may be performed until `deadline` is
   * reached.
   */
  wakeup(deadline?: number): void

  /**
   * Immediately suspend the event loop and trigger the `idle` event.
   */
  idle(): void

  /**
   * Resume the process and all threads after suspension. This can be used to
   * cancel suspension after the `suspend` event has been emitted and up until
   * all `idle` event listeners have run.
   */
  resume(): void
}

declare namespace Bare {
  export { Addon, Thread }
}

interface Addon {
  /**
   * The WHATWG `URL` identifier of the addon.
   */
  readonly url: URL

  /**
   * The exports of the addon.
   */
  readonly exports: unknown
}

/**
 * Support for loading native addons, which are typically written in C/C++ and
 * distributed as shared libraries.
 *
 * This is an advanced API that users should never have to interact with
 * directly.
 */
declare class Addon {
  /**
   * Load a static or dynamic native addon identified by `url`. If `url` is not
   * a static native addon, Bare will instead look for a matching dynamic object
   * library.
   */
  constructor(url: URL)
}

declare namespace Addon {
  /**
   * The target triplet identifying the current addon host.
   */
  export const host: string

  /**
   * Whether addon loading has been sealed with `Addon.seal()`.
   */
  export const sealed: boolean

  /**
   * Seal addon loading. Once sealed, no further dynamic addons can be loaded by
   * the current thread or any other thread of the process, now or in the
   * future; attempting to do so throws. Statically linked addons are compiled
   * in and remain available.
   *
   * This is a one-way operation that cannot be undone for the lifetime of the
   * process. It is intended for embedders that wish to load a fixed set of
   * trusted addons up front and then prevent any further native code from
   * being introduced, such as when establishing a sandbox.
   *
   * The seal applies to the process alone. Embedders running several Bare
   * processes within the same operating system process may seal each of them
   * independently, and sealing one has no effect on the addons the others may
   * load. Addons are likewise owned by the process that loaded them and are
   * unloaded when it is torn down.
   *
   * Sealing also freezes the context registry that embedders publish handles
   * to, after which nothing further may be published to it.
   *
   * For what sealing guarantees, what it deliberately leaves alone, and what
   * embedders are expected to do on top of it, see
   * {@link https://github.com/holepunchto/bare/blob/main/docs/threat-model.md}.
   */
  export function seal(): void

  /**
   * Whether the current process has already loaded the dynamic addon
   * identified by `url`. Such an addon can still be loaded after sealing.
   */
  export function loaded(url: URL): boolean

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
  /**
   * Data to pass to the thread, available as `Thread.self.data`.
   */
  data?: unknown

  /**
   * The transfer list for `data`.
   */
  transfer?: unknown[]

  /** @deprecated Pass the source after `filename` instead. */
  source?: ThreadSource

  /**
   * The encoding of `source` if it is a string.
   *
   * @default 'utf8'
   */
  encoding?: BufferEncoding

  /**
   * The stack size of the thread in bytes. Pass 0 for the default.
   *
   * @default 0
   */
  stackSize?: number

  /**
   * The URL to mount the thread's bundle at, defaulting to its `filename`
   * followed by a `/`. The bundle serves only the modules under it, so
   * `file:///` lets it serve any file.
   */
  mount?: URL | string
}

interface Thread {
  /**
   * Whether or not the thread has been joined with the current thread.
   */
  readonly joined: boolean

  /**
   * Block and wait for the thread to exit.
   */
  join(): void

  /**
   * Suspend the thread. Equivalent to calling `Bare.suspend()` from within the
   * thread.
   */
  suspend(linger?: number): void

  /**
   * Wake the thread. Equivalent to calling `Bare.wakeup()` from within the
   * thread.
   */
  wakeup(deadline?: number): void

  /**
   * Resume the thread. Equivalent to calling `Bare.resume()` from within the
   * thread.
   */
  resume(): void

  /**
   * Terminate the thread. Equivalent to calling `Bare.exit()` from within the
   * thread.
   */
  terminate(): void
}

/**
 * Support for lightweight threads. Threads are similar to workers in Node.js,
 * but provide only minimal API surface for creating and joining threads.
 *
 * Start a new thread that will run `source`, which is a string or a `Buffer`.
 * If `callback` is provided, its function body will be used as the source
 * instead and invoked on the new thread with `Thread.self.data` passed as an
 * argument.
 *
 * A thread is loaded through a protocol that reaches nothing, so it runs the
 * source it was given and no more; `filename` names that source rather than
 * locating it. Anything else the thread needs, including the modules it
 * imports, must travel with it as `source` or `data`. To run a module graph on
 * a thread, gather it into a {@link https://github.com/holepunchto/bare-bundle}
 * first and pass the bundle as `source`, which is what
 * {@link https://github.com/holepunchto/bare-thread} does.
 *
 * A thread does not inherit the module protocol of whoever spawned it. Reading
 * a graph off disk and handing it over is the spawner's job, so that a thread
 * never reaches further than the code that started it.
 *
 * This is an advanced API that users should never have to interact with
 * directly.
 */
declare class Thread {
  constructor(callback: ThreadCallback)
  constructor(options?: ThreadOptions, callback?: ThreadCallback)
  constructor(filename: string, callback: ThreadCallback)
  constructor(filename: string, source: ThreadSource, options?: ThreadOptions)
  constructor(filename: string, options?: ThreadOptions, callback?: ThreadCallback)
}

declare namespace Thread {
  interface ThreadProxy {
    /**
     * The data that was passed to the current thread on creation. Will be
     * `null` if no data was passed.
     */
    readonly data: unknown
  }

  /**
   * `true` if the current thread is the main thread.
   */
  export const isMainThread: boolean

  /**
   * A reference to the current thread. Will be `null` on the main thread.
   */
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
