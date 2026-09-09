# Context keys

## What this is

The list of keys in use with the context registry, so that two modules do not invent different names for the same handle. The API itself is documented in [`include/bare.h`](../include/bare.h), with an overview in the [README](../README.md#context).

This file is the registry for the `bare.` namespace, which is Bare's own and is where the handles that Bare and its addons agree on live. If you own another namespace, keep your own list and link it from here.

## Rules

**A key is a contract between two parties who never meet.** The registry stores a pointer and never looks at it, so the embedder publishing a handle and the addon reading it have only the key to agree on. What that key means is written down here or it is not written down anywhere.

**A key never changes meaning.** Once it is listed, its type, what it points at and who owns it are fixed. Anything that needs a different contract is a different key, which is what the version suffix is for. Reusing a key with a new meaning breaks every addon built against the old one, silently, because a pointer is a pointer.

**A key is published once and never withdrawn.** An entry lasts for as long as the process, so an addon that has been handed a pointer can hold it without asking again. A handle that has to change over the life of the process does not fit here; publish something the addon can ask through instead, and keep the changing part behind it.

**Nothing is required.** A missing key is an ordinary outcome and consumers are expected to degrade rather than fail. An embedder that has no handle to publish publishes nothing.

**Publish before the first `bare_load()`.** Addons are loaded by `bare_load()`, so an addon only ever sees what was published before it was loaded.

## `bare.android.jvm.v1`

|              |                                         |
| ------------ | --------------------------------------- |
| Type         | `JavaVM *`                              |
| Published by | The embedder, on Android                |
| Destructor   | None; the JVM outlives the Bare process |

The Java virtual machine the process is running under, as returned by `JNI_OnLoad()` or by `GetJavaVM()` on a `JNIEnv *`. An addon needs it to attach its own threads and to reach anything in Java from one of them.

## `bare.android.context.v1`

|              |                                                                   |
| ------------ | ----------------------------------------------------------------- |
| Type         | `jobject`, a JNI global reference to an `android.content.Context` |
| Published by | The embedder, on Android                                          |
| Destructor   | Required; releases the global reference with `DeleteGlobalRef()`  |

The application context, which is what `getSystemService()` and the rest of the Android platform API hang off.

The `Context` is published rather than any particular service derived from it, so that each addon can ask for what it needs without every service needing a key of its own. An addon that wants a `ConnectivityManager`, for example, derives one from this and pays a JNI hop for it.

It must be a global reference, as the addon reading it runs on threads and at times the embedder does not control, and a local reference is only valid for the call it was made in. The destructor is where it is released, which ties its lifetime to the Bare process rather than to whichever call published it. It runs after the JavaScript environment has been destroyed, so it must do nothing but release the reference.

The application context is published rather than an `Activity`, which comes and goes over the life of the process and so cannot be a key.

## Adding a key

Add it here in the same pull request that first publishes or reads it, rather than after the fact. A key that is in use and not listed is a key someone else is about to pick a different name for.

Say what it points at precisely enough that an addon author can write the cast without asking; the C type, what owns the memory, and what the destructor is expected to do. Say who publishes it, since a key an embedder cannot produce is not a key.

Handles are powers, and publishing one grants it to every addon in the process rather than to the one you had in mind. See [`threat-model.md`](threat-model.md) before adding a key that hands out more than the one it replaces.
