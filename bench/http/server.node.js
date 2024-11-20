const http = require('http')

http.createServer((req, res) => res.end('hello world')).listen(8080)
