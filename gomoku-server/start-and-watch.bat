if not exist server.js call coffee --bare --compile server.coffee
start coffee --bare --compile --watch server.coffee public/client.coffee
supervisor --watch server.js --no-restart-on exit server.js