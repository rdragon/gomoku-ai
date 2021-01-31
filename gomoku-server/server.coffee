express = require 'express'
server = express()
http = require('http').Server server
io = require('socket.io')(http)
spawn = require('child_process').spawn
fs = require 'fs'

server.use express.static __dirname + '/public'

exec = '../bin/release/gomoku-ai'

if not fs.existsSync exec
  exec1 = exec + '.exe'
  if not fs.existsSync exec1
    console.error "error: file '#{exec}[.exe]' not found"
    process.exit 1
  else
    exec = exec1

port = 7682
if process.argv.length > 2 and /^[1-9][0-9]*$/.test process.argv[2]
  port = parseInt process.argv[2]

http.listen port, ->
  console.log "listening on #{port}"

server.get '/', (req, res) ->
  res.sendFile 'index.html'

connectionIdCounter = 0

io.on 'connection', (socket) ->
  connectionId = connectionIdCounter++
  debug = (msg) ->
    console.log "#{new Date().toISOString()}  #{connectionId}  #{msg}"
  verbose = (msg) ->
    debug msg
  debug 'new connection'
  process = null
  processClosed = true
  restartProcess = false
  buffer = ''
  errorBuffer = ''
  defaultArgs = '--json-client'.split ' '
  args = defaultArgs.slice 0
  log = (msg) ->
    socket.emit 'log', msg: msg
  logError = (msg) ->
    socket.emit 'log error', msg: msg
  newGame = ->
    if processClosed
      startProcess()
    else
      restartProcess = true
      process.kill()
  startProcess = ->
    process = spawn exec, args
    verbose 'spawn process'
    processClosed = false
    process.stdout.on 'data', (data) ->
      verbose 'process data'
      buffer += data.toString()
      i = buffer.indexOf '\n'
      while i isnt -1
        line = buffer.substr 0, i
        buffer = buffer.substr i + 1
        i = buffer.indexOf '\n'
        if line.charAt(0) is '{'
          obj = JSON.parse line
          socket.emit obj.msg, obj
        else
          log line + '\n'
    process.stderr.on 'data', (data) ->
      verbose 'process error'
      errorBuffer += data.toString()
      i = errorBuffer.indexOf '\n'
      while i isnt -1
        line = errorBuffer.substr 0, i
        errorBuffer = errorBuffer.substr i + 1
        i = errorBuffer.indexOf '\n'
        logError line + '\n'
    process.on 'close', (exitCode) ->
      verbose 'process close'
      processClosed = true
      buffer = ''
      errorBuffer = ''
      if exitCode isnt 0 and exitCode isnt null
        logError "the process has exited with exit code #{exitCode}\n"
      if restartProcess
        restartProcess = false
        startProcess()
  socket.on 'disconnect', ->
    debug 'client disconnected'
    process.kill() unless processClosed
  socket.on 'submit move', (data) ->
    debug 'move submitted'
    process.stdin.write (data.pos + 1) + '\n' unless processClosed
  socket.on 'write', (data) ->
    debug 'write'
    process.stdin.write data.msg unless processClosed
  socket.on 'new game', (data) ->
    debug 'new game'
    args = defaultArgs.slice 0
    args.push x for x in data.args
    newGame()
  socket.on 'stop game', (data) ->
    debug 'stop game'
    process.kill() unless processClosed
  newGame()
