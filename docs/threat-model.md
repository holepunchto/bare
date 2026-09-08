# Threat model

## What this is

Bare does almost nothing on its own, and addons do the real work. That keeps this document short, but it also means the few promises Bare makes have to be exact, because there is nothing else holding things up.

What follows says what Bare promises, what it does not promise, and which of the two your bug report falls under.

## What counts

Bare is a library, and embedders are the ones using it. The `bare` CLI is a toy for developers; it never seals and it promises nothing, so you can ignore it here.

- **Counts:** `src/`, the public headers, the `Bare` namespace, and the addons listed in `src/builtins.json`.
- **Does not count:** `bin/`, `bare-*` modules that are not compiled in, and addon code.

## The one power

Bare gives out a single power, which is the power to load a native addon.

That one power is special because it hands you every other power. An addon can read files, open sockets, and start programs, so whoever can load addons can do anything at all.

Everything else in `src/` is harmless on its own. Threads and the lifecycle events never reach the outside world by themselves, and the module system reaches only as far as the protocol it was handed. The addons you compile in are another matter. So this whole document is about one power and about how to take it away.

## The module protocol

Modules have to be read from somewhere, and only Bare knows how. `bare_load()` hands that protocol to the module it loads, which is the embedder's own entry point, so the filesystem goes to the embedder and to nobody else.

Nothing else hands it out. A protocol is the only way to reach a store, so code loaded without one reads nothing at all.

A protocol travels with the module graph it was given to. Load untrusted code with one of your own and it reaches only as far as that one:

```js
Module.load(url, source, { protocol: restricted })
```

Pass a referrer and it gets yours instead, so pass a protocol too when the code is untrusted.

Threads inherit nothing. A thread runs the source it was handed and no more, so spawning one is no way around the protocol you were given. To give a thread a module graph, gather it into a bundle first and hand that over.

The reach of the module system is the embedder's to pick. The CLI hands on its own, which is why `bare` can read the disk.

## Before and after

Bare has no walls inside it. What it has instead is a moment in time.

**Before the seal.** The power is on, and every piece of code in the process is fully trusted and can do whatever native code can do. Bare promises nothing here, so nothing that happens in this phase is a bug.

**The seal.** The embedder calls `bare_seal()`, or JavaScript calls `Addon.seal()`, and the power turns off for good. It cannot be turned back on, it applies to every thread including ones made later, and it only lifts when the process is torn down.

The seal covers one Bare process. A sibling in the same OS process is not sealed by it, and an unsealed sibling can still pull native code into the memory you share. So seal every process you set up, or the unsealed ones speak for all of them.

Seal early, before any untrusted code has run. If untrusted code goes first it may already have loaded whatever it wanted, and the seal will have come too late to matter.

Sealing waits for any load that is already running, even on another thread. Loads are lined up one at a time across the whole OS process, so a seal can end up waiting on a load in a sibling Bare process, and that wait has no time limit.

**After the seal.** The set of powers is frozen, and this is where the promise lives.

## The promise

After the seal returns, Bare will not bring any new native code into the process. Only what was already there stays. The two exceptions below are the only exceptions.

Said another way, sealed JavaScript cannot get more native code than it was given through anything Bare offers.

There are two things this does **not** mean:

- New **JavaScript** can still show up. Modules keep loading and `Thread` still takes source and callbacks, which is fine, because the promise is only about native code.
- `new Addon(url)` does not always throw after the seal. If the process already owns that addon it hands it back, so anything you loaded before sealing is there for the asking, by any code in the process that knows the path. Loading an addon **is** granting what it can do, and the seal freezes that set rather than keeping it to yourself.

## The two exceptions

### Addons built into the binary

Some addons are compiled in, which means they are always available to every process whether it has sealed or not. `src/builtins.json` is the list for a default build.

So whatever you compile in **is** your security policy, and you should pick it deliberately.

### Registering late

`bare_module_register()` looks at what the calling thread happens to be doing. If that thread is loading an addon then the registration belongs to that load, and if it is not, the registration is treated as built-in instead. Built-in means it is visible to every process, sealed ones included, for as long as the OS process lives.

So an addon that was loaded before the seal can register late and become built-in, either from a thread of its own or by waiting until its load has finished.

**We accept this.** Native code is trusted anyway, and anyone who can call that function is already running native code, so they never needed the trick in the first place.

Still, note the wording carefully. It is native code that can do this rather than just the embedder, and native code is a much bigger group, so choose your addons with that in mind.

There is one more wrinkle. Seals lift when a process is torn down, but built-in registrations never do, so the list only ever grows. A sealed process that starts late in a long-lived app inherits everything that was registered before it. The list is also keyed by name and the newest registration wins, so a late one under a name that is already taken takes it over for every process that looks it up afterwards. Names change hands, in other words, rather than the list simply getting longer.

## What is a wall and what is not

A **No** row cannot be the basis of a bug report.

| Thing                     | Is it a wall? | Why                                                                              |
| ------------------------- | ------------- | -------------------------------------------------------------------------------- |
| OS process                | Yes           | The OS does this, and Bare leans on it                                           |
| Before seal -> After seal | Yes           | The only wall Bare builds itself                                                 |
| Bare process (`bare_t`)   | No            | Addons and the seal are tracked per process, but memory is shared                |
| Thread                    | No            | Memory is shared, and `SharedArrayBuffer` is always on                           |
| Realm                     | No            | Same heap, so objects cross freely                                               |
| Module graph              | No            | Every module can see the whole `Bare` namespace, and a graph shares one protocol |
| Addon ABI                 | No            | Addons are native code and already have everything                               |
| Embedder <-> JavaScript   | Yes           | `Bare.IPC`, plus the options and `argv` you pass in                              |
| V8 sandbox                | No            | Nice to have, but we do not rely on it                                           |

Two Bare processes inside one OS process share memory and are not walled off from each other, so if you need two separate sets of powers you need two OS processes.

The `Bare` namespace is frozen. It cannot be deleted, replaced or added to, and neither can `Bare.Addon` or `Bare.Thread`, so code cannot swap out what the rest of the realm reaches for. `Bare.IPC` stays writable because embedders set it.

## What we trust

- The JavaScript engine, through `libjs`
- `libuv`
- Bare's own C in `src/`
- Every addon compiled into the binary, and every addon loaded before the seal
- The embedder, including its options, its `argv`, and whatever it hooks up to `Bare.IPC`
- The OS and its loader

## Who we are defending against

The attacker we care about is untrusted JavaScript running inside a sealed process. We assume it writes whatever it likes, runs on as many threads as it wants, and calls anything it can reach in any order and all at once.

We are **not** defending against:

- Native code already running in the OS process. It shares memory, so it wins, and the seal cannot stop it.
- A bad addon. Addons are trusted, and picking them is the embedder's job.
- Anything before the seal, since we promise nothing there.

## What is actually left to worry about

After the seal there are only four ways native code can get in, and this is the whole list.

**1. A bug in the seal itself.** Races between sealing and loading, thread startup order, the constructor and `bare_module_register()` paths, and cache reuse across processes.

**2. The two exceptions above**, if they turn out to be reachable without native code or wider than we described.

**3. A sibling Bare process that never sealed.** It shares the memory, so what it loads lands next to you. Sealing every process is the embedder's job.

**4. Memory bugs in our own C.** This is the big one, and it is also the one the seal cannot help with, because the attacker never asks for a power here. They just write one straight into memory.

The risky spots are wherever we read data an attacker controls:

- Structured clone
- Thread transfer lists
- Resolving, parsing and linking, which the module system does on code it may not trust
- Data races on `SharedArrayBuffer`
- `WebAssembly`, which hands the engine bytes to compile
- The engine and `libuv` themselves

Turning powers off does nothing about memory bugs, and only an OS sandbox does, which is why the next section makes one a requirement.

## What embedders must do

Bare promises that the set of addons is **frozen**, but it does not promise that the set is **safe**. That part is yours to work out.

**1. Check what your powers add up to.** Two safe powers can combine into an unsafe one. A bundle reader is fine on its own and a peer connection is fine on its own, but together they leak data, and neither addon author did anything wrong. So look at the whole set and assume someone is trying to abuse it. Every addon you load before sealing belongs to that set for all the code in the process, not just for the part of it that wanted the addon. `Bare.IPC` counts as part of that set, and on mobile it talks straight to your app, so write the app side carefully.

**2. Mind what you hand on.** The protocol you were handed reaches the whole filesystem. Load untrusted code with one of your own, and pass it even when you pass a referrer.

**3. Use an OS sandbox.** The seal does nothing about memory bugs, so if you are running untrusted JavaScript a sandbox is required rather than a bonus.

**4. Seal early, and seal every process.**

## Things that still work after the seal

All of these are there without loading a single addon, and none of them are bugs.

- Reading whatever the protocol the code was loaded with reaches
- `Bare.exit()`, which kills the process or the thread
- `Bare.suspend()` and `Bare.idle()`, which jam the loop
- Writing to `stdout`, `stderr` and the system log, which `console` does for you through the loggers a default build compiles in
- `WebAssembly`, which turns bytes into code the engine runs, though it is handed no imports and so reaches nothing by itself
- Making threads and using lots of memory
- `SharedArrayBuffer` plus fast timers, which gives side channels against anything sharing the address space, including your own app

If you need to stop any of this, the seal will not do it for you and you will have to use the OS.

## What to report

**Please report:**

- Any way to get native code into a sealed process without already having native code
- Any way to get more power than was granted, using a Bare interface
- Memory bugs that JavaScript can reach, in `src/` or in an addon we compile in
- Anything that breaks the promise or the wall table

**Not a bug:**

- Anything before the seal
- Anything that needs native code or a bad addon to begin with
- Anything crossing a **No** row
- Anything in the "still works after the seal" list
- Anything in `bin/`
- Harm from powers the embedder chose to grant

Engine and `libuv` bugs go upstream, and our job is to ship the fixes.

## Common pitfalls

These came out of a pass over the addons Bare compiles in. Nothing here is a new rule. It is one old rule turning up in the same few shapes, which is that a binding is reachable from JavaScript and the JavaScript in front of it is not a wall.

Most of it is the fourth thing on the list of what is left to worry about, memory bugs in our own C. The rest is power handed out by accident. Anyone writing or reviewing an addon we compile in should read it, because a builtin's bugs are Bare's bugs.

### At the boundary

**Trusting the JavaScript wrapper.** The binding is an object like any other, and whoever holds it calls it directly, in any order, with anything. The checks on the JavaScript side are there for callers who meant well. A binding validates its own arguments or it is not validated.

**Asserting on what JavaScript controls.** An assertion says this cannot happen. Input can always happen. Assert on it and a bad argument aborts the process, and in a release build the assertion is gone and the failure is ignored instead, so the code runs on a value it never got. Assert on engine calls that cannot fail once the input is checked. Raise for everything else.

**No type tag on a native-backed object.** Without a tag, any object can be passed as the receiver, and the binding unwraps a pointer out of something that never had one. Tag every native-backed class. Check the tag at every entry point, and check it before the unwrap.

**Turning three answers into two.** A check that can fail has three answers: yes, no, and could not tell. Fold the third into either of the others and you have answered without the evidence. You have also dropped an exception, and the next call trips over it. This is how an object that was never tagged passes for one that was.

**Assuming a JavaScript hook is still a function.** Handlers hang off objects that JavaScript can write to. Check that what you are about to call is callable, and know where to go when it is not.

### In the C

**Checking bounds after the pointer is made.** `&buf[offset]` is already out of bounds by the time anything looks at `offset`. Slice first and hand back a checked pointer, so there is no unchecked one to reach for.

**Checking bounds in a width that wraps.** Add in a width the sum cannot escape, and compare a start as a signed 64 bit value, so a position past the end cannot wrap back into range where `size_t` is narrower.

**Two call paths for one function.** A typed callback and an untyped one are two C functions behind one JavaScript function, and the caller picks which runs. Check different things in each and the weaker one is the real one. Both need the same checks in the same order, so the same call fails the same way either way. A typed callback also runs without a handle scope and cannot raise at all, so it reports back and lets the untyped path raise.

**Running JavaScript while holding a raw pointer.** A pointer into a string or a backing store is good until the next thing that can run JavaScript. A getter, a proxy trap, a `toString()`, an array read, an allocation that sets off a collection: any of them can move what you hold, free it, or detach it. Read everything first and make the values afterwards, or take a copy.

**Reaching an attacker's object with plain property access.** `object.constructor`, `object[key]`, `Symbol.toStringTag` and `for...in` all run code the object chose. That is worst on a diagnostic path, where looking at a value is meant to cost nothing and `console` points it at anything at all. Read own property descriptors, and expect whatever you do call to throw.

**Allocating to a size that came from JavaScript.** A length from JavaScript is a request rather than a size. Put a bound on it. Check it against the platform's own limits before you allocate, not after the allocation fails. Handle the failure, because an attacker can get there on purpose. Use `calloc()` for arrays of handles, so a half filled one does not read as garbage.

**Leaking on the error path.** Nobody exercises the failure branch. A string view still held when an allocation fails, a handle never released, half a structure left behind. Give back what you took, in reverse.

**Truncating into a fixed buffer.** Measure first and turn down what does not fit. Truncating turns one name into another name, and the decision after it is made about something the caller never passed.

### In the design

**Falling back to a default.** A default module loader stood in when a dynamic import had no referrer, which handed code a protocol nobody gave it. A fallback is a grant. When the lookup fails, fail.

**Caches that carry power.** A module cache entry holds the loader that read it, protocol and builtins and all. Share one cache between loaders that do not reach as far as each other and the short one gets the long one's reach. Anything shared and keyed by name is a channel, so check that what comes out was put there by someone who reaches as far as you do.

**Handing out raw addresses.** Backing stores and externals used to cross as pointers. That gives the address away, and it can be forged too, because a number is a number and a serialized value passes through JavaScript on its way between threads. Mint an opaque, random, single use token instead. Keep the mapping in the process, and check the type when the token comes back. Inspection follows the same rule: label an external, never print it.

**A second one of something meant to be single.** An `init()` that could be called twice. A second module context in one environment. An internal object that got out into JavaScript. Each one is a second handle on state that was written believing it was alone. Take the slot once and turn down the next, and keep internals away from JavaScript to begin with.

**Splicing a value into structured text.** Building a URL by putting a value into the serialized string let a value carrying a delimiter land in a component it was never meant to reach. Encode or turn down whatever would let it escape its component, then reparse and let the parser have the last word.

**Keeping a capability nobody uses.** One addon carried a tagging capability with no callers. Unused surface is still surface, and it is the cheapest kind to remove.
