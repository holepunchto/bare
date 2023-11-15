const http = require('bare-http1')

http
  .createServer((req, res) => res.end('hello world'))
  .listen(8080)
