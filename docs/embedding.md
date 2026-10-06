# Embedding

Complements the C embedding API documented in the [runtime API reference](https://docs.pears.com/bare/reference/bare/runtime/#embedding), for embedders whose thread belongs to a host loop and for embedders that call into the JavaScript environment themselves.

## Polling

An embedder whose thread belongs to a host loop, such as the run loop of a user interface, drives the loop with `bare_poll()` rather than `bare_run()`. It runs the loop without blocking and reports how long the host may sleep before calling again:

```c
int timeout;
bare_poll(bare, &timeout);
```

A timeout of `-1` means that the host may sleep until the backend descriptor of the loop, as given by `uv_backend_fd()`, becomes readable. A host that sleeps on the timeout alone rather than on the descriptor will miss work that arrives from another thread.

## Attaching

`bare_run()` attaches the process to the thread while it runs, and so does loading an addon, so native code reached through either is already attached. A call the embedder makes itself is not, so attach the process around it:

```c
bare_t *previous;
bare_attach(bare, &previous);

js_call_function(env, receiver, fn, argc, argv, &result);

bare_detach(bare, previous);

bare_run(bare, UV_RUN_NOWAIT);
```

Attachments nest. `bare_attach()` hands back the process that was attached before, which `bare_detach()` attaches again, so detach on the same thread and in the reverse order of attaching. Detaching out of order returns `-1` and restores nothing.

Attaching does not run the loop. The call may leave work behind that only the loop will run, which is why it is run above. A process that has terminated or exited cannot be attached.
