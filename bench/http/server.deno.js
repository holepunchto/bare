/* global Deno */

Deno.serve({ port: 8080 }, () => new Response('hello world'))
