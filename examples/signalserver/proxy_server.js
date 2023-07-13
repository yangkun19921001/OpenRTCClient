//npm install http-proxy

const http = require('http');
const httpProxy = require('http-proxy');

const proxy = httpProxy.createProxyServer({
    secure: false // 关闭对目标服务器证书的验证
});

const server = http.createServer(function(req, res) {
  proxy.web(req, res, { target: 'https://localhost:443' });
});

server.on('upgrade', function (req, socket, head) {
  proxy.ws(req, socket, head, { target: 'https://localhost:443' });
});

console.log("Listening on port 8880")
server.listen(8880);
