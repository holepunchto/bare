# Builtins

This directory contains the builtin core modules, which is all the JavaScript APIs that Bare provides on top of those already provided by the underlying JavaScript engine. Bare aims to have a lean and minimal core runtime, providing only what is absolutely necessary to support a wider ecosystem of modules. As such, only APIs that fall into any one of the following categories should be considered for inclusion as a core builtin:

1. The API can't reasonably be implemented elsewhere. This may be the case if the API needs access to internal implementation details of Bare or must otherwise tightly integrate with the event loop lifecycle, such as the threads API.

2. The API is needed by another API which itself falls into the first category. The native addon API, for example, needs to manipulate paths and so `path` is available as a core builtin.

3. The API is relatively small and affords Bare large ecosystem compatibility. As an example, the global timers API such as `setTimeout()` is extensively used in both Web and Node.js and so Bare also makes this API available in the global scope.
