canvas = null
logElement = null
argsElement = null
args1Element = null
args2Element = null
ctx = null
socket = null
boardVisible = false
hasTurn = false
edit = false
showHighlight = false
showLog = true
game =
  w: 0
  h: 0
  size: 0
scale = 0
lastMove = null
blackId = 1
whiteId = 2
cursorStone = 0
vs = []
moves = []
at = 0
canvasWidth = 0
canvasHeight = 0
defaultTheme =
  name: 'default'
  className: null
  grid: '#888'
  lastMove: 'rgba(0,0,0,0.2)'
  highlightRadius: 0.48
  highlight: '#f00'
  stoneRadius: 0.4
  blackStone: '#000'
  whiteStone: '#fff'
  whiteStoneBorder: '#888'
  block: '#888'
  blockWidth: 0.4
darkTheme =
  name: 'dark'
  className: 'dark'
  grid: '#111'
  lastMove: '#444'
  whiteStone: '#ccc'
  whiteStoneBorder: ''
  block: '#111'
theme = null

class V
  constructor: (@i, @j, @index) ->
    @stone = 0
    @hl = false

  isEmpty: ->
    @stone is 0

  clear: ->
    @stone = 0
    @hl = false

  hasStone: ->
    @stone in [1, 2]

  hasHl: ->
    @hasStone() and @hl

  showHl: ->
    @hl = true if @hasStone()

  set: (stone) ->
    @stone = stone
    @showHl() if showHighlight

  getX: ->
    getX @j

  getY: ->
    getX @i

handleLogMessage = (msg) ->
  if /player 1 \(black\)/.test msg
    blackId = 1
    whiteId = 2
  else if /player 2 \(black\)/.test msg
    blackId = 2
    whiteId = 1
  print msg.replace (/ {2,}/g), '&nbsp;&nbsp;&nbsp;&nbsp;'

print = (msg) ->
  if msg.indexOf('[player ' + blackId) is 0
    printCustom msg, 'blackMessage'
  else if msg.indexOf('[player ' + whiteId) is 0
    printCustom msg, 'whiteMessage'
  else if /playback: /.test msg
    printCustom msg, 'playbackMessage'
  else
    printCustom msg, 'gameMessage'

printCustom = (msg, className) ->
  s = msg.toString().replace '\n', '<br />'
  logElement.append '<span class="' + className + '">' + s + '</span>'
  logElement.scrollTop 999999

printError = (msg) ->
  printCustom msg, 'errorMessage'

getX = (j) ->
  Math.round (j + 0.5) * scale

resize = ->
  maxW = $(window).width() * (100 - (if showLog then 37 else 0)) / 100
  maxH = $(window).height()
  scale = Math.min maxW / game.w, maxH / game.h
  canvasWidth = Math.round game.w * scale
  canvasHeight = Math.round game.h * scale
  canvas.attr 'width', canvasWidth
  canvas.attr 'height', canvasHeight
  draw()

write = (msg) ->
  socket.emit 'write', msg: msg

draw = ->
  ctx.clearRect 0, 0, canvasWidth, canvasHeight

  # draw grid
  ctx.beginPath()
  pad = 0.5 * scale
  for j in [0 .. game.w - 1]
    x = getX j
    ctx.moveTo x, pad
    ctx.lineTo x, canvasHeight - pad
  for i in [0 .. game.h - 1]
    y = getX i
    ctx.moveTo pad, y
    ctx.lineTo canvasWidth - pad, y
  ctx.strokeStyle = theme.grid
  ctx.stroke()

  # draw last move
  if lastMove and lastMove.hasStone()
    ctx.beginPath()
    ctx.arc lastMove.getX(), lastMove.getY(),
      theme.highlightRadius * scale, 0, Math.PI * 2
    ctx.fillStyle = theme.lastMove
    ctx.fill()

  # draw highlighted stones
  if showHighlight
    for v in vs when v.hasHl()
      ctx.beginPath()
      ctx.arc v.getX(), v.getY(),
        theme.highlightRadius * scale, 0, Math.PI * 2
      ctx.fillStyle = theme.highlight
      ctx.fill()

  # draw black stones
  ctx.fillStyle = theme.blackStone
  for v in vs when v.stone is 1
    ctx.beginPath()
    ctx.arc v.getX(), v.getY(), theme.stoneRadius * scale, 0, Math.PI * 2
    ctx.fill()

  # draw white stones
  ctx.fillStyle = theme.whiteStone
  ctx.strokeStyle = theme.whiteStoneBorder unless theme.whiteStoneBorder is ''
  for v in vs when v.stone is 2
    ctx.beginPath()
    ctx.arc v.getX(), v.getY(), theme.stoneRadius * scale, 0, Math.PI * 2
    ctx.fill()
    ctx.stroke() unless theme.whiteStoneBorder is ''

  # draw blocks
  ctx.fillStyle = theme.block
  for v in vs when v.stone is 3
    d = theme.blockWidth * scale
    ctx.fillRect v.getX() - d / 2, v.getY() - d / 2, d, d

makeNewGame = ->
  argsElement.show()
  args1Element.focus().select()

submitNewGame = ->
  localStorage.args1 = args1Element.val() if hasLocalStorage()
  localStorage.args2 = args2Element.val() if hasLocalStorage()
  s = args1Element.val() + ' ' + args2Element.val()
  socket.emit 'new game', args: s.split ' '
  argsElement.hide()

getBoardString = ->
  x = ['', '--black', '--white', '--block']
  for v in vs when not v.isEmpty()
    x[v.stone] += " #{v.index}"
  y = (s for s in x when s.indexOf(' ') != -1)
  y.join ' '

stopGame = ->
  socket.emit 'stop game', {}
  print 'game has stopped\n'
  hasTurn = false
  canvas.css 'cursor', 'auto'

connect = ->
  socket = io();

startEditor = ->
  edit = true
  canvas.css 'cursor', 'pointer'

stopEditor = ->
  edit = false
  canvas.css 'cursor', 'auto'

hasLocalStorage = ->
  try
    localStorage.setItem 'x', 'x'
    localStorage.removeItem 'x'
    true
  catch e
    false

getPar = (name) ->
  name = name.replace /[\[]/, "\\["
  name = name.replace /[\]]/, "\\]"
  regex = new RegExp "[\\?&]" + name + "=([^&#]*)"
  results = regex.exec location.search
  if results is null then '' else decodeURIComponent results[1].replace /\+/g, " "

setTheme = (t) ->
  $('body').removeClass theme.className unless theme is null or theme.className is null
  theme = {}
  for k, v of defaultTheme
    theme[k] = v
  for k, v of t
    theme[k] = v
  $('body').addClass theme.className unless theme.className is null

toggleTheme = (t) ->
  if theme.name is t.name
    setTheme defaultTheme
  else
    setTheme t
  draw()

undoMove = ->
  at--
  moves[at].v.stone = 0
  draw()

redoMove = ->
  moves[at].v.stone = moves[at].stone
  at++
  draw()

$(document).ready ->
  canvas = $ 'canvas'
  logElement = $ '#log'
  argsElement = $ '#args'
  args1Element = $ '#args1'
  args2Element = $ '#args2'
  ctx = canvas[0].getContext '2d'
  connect()
  document.oncontextmenu = -> false
  if getPar('dark') isnt ''
    setTheme darkTheme
  else
    setTheme defaultTheme
  if getPar('hideLog') isnt ''
    showLog = false
    $('body').addClass 'hideLog'
  if hasLocalStorage() and localStorage.args1?
    args1Element.val localStorage.args1
  if hasLocalStorage() and localStorage.args2?
    args2Element.val localStorage.args2
  socket.on 'new game', (data) ->
    [game.w, game.h] = data.pars
    game.size = game.w * game.h
    vs = new Array game.size
    k = 0
    for i in [0 .. game.h - 1]
      for j in [0 .. game.w - 1]
        vs[k] = new V(i, j, k)
        k++
    lastMove = null
    stopEditor() if edit
    boardVisible = true
    canvas.css 'display', 'block'
    resize()
  socket.on 'log', (data) ->
    handleLogMessage data.msg.toString().replace 'type "q"', 'press delete'
  socket.on 'log error', (data) ->
    printError data.msg
  socket.on 'clear board', (data) ->
    v.clear() for v in vs
    lastMove = null
  socket.on 'place stone', (data) ->
    v = vs[data.pos]
    v.set data.stone
    lastMove = v if data.stone isnt 3
    moves.push v: v, stone: data.stone
    at++
    hasTurn = false
    canvas.css 'cursor', 'auto' unless edit
    draw()
  socket.on 'your turn', (data) ->
    hasTurn = true
    canvas.css 'cursor', 'pointer'
  canvas.mousedown (e) ->
    elX = $(this).offset().left
    elY = $(this).offset().top
    x = e.pageX - elX
    y = e.pageY - elY
    i = Math.floor y / canvasHeight * game.h
    j = Math.floor x / canvasWidth * game.w
    v = vs[i * game.w + j]
    switch e.which
      when 1
        if edit
          if cursorStone is 4
            v.showHl()
            showHighlight = true
          else
            v.set cursorStone
          draw()
        else if v.isEmpty()
          if at < moves.length
            redoMove()
          else
            socket.emit 'submit move', pos: v.index if hasTurn
      when 3
        if edit
          if showHighlight and v.hasHl()
            v.hl = false
          else
            v.clear()
          draw()
  argsElement.keydown (e) ->
    switch e.keyCode
      when 13 # enter
        submitNewGame()
      when 27 # escape
        argsElement.hide()
  $(document).keydown (e) ->
    return true if args1Element.is(':focus')
    return true if args2Element.is(':focus')
    return true if e.ctrlKey or e.metaKey
    switch e.keyCode
      when 13 # enter
        if at < moves.length
          redoMove()
        else
          write '\n'
      when 46 # delete
        stopGame()
      when 49 # 1
        cursorStone = 1
        startEditor() unless edit
      when 50 # 2
        cursorStone = 2
        startEditor() unless edit
      when 51 # 3
        cursorStone = 3
        startEditor() unless edit
      when 52 # 4
        cursorStone = 4
        startEditor() unless edit
      when 66 # b
        $('body').toggleClass 'hideBoard'
      when 68 # d
        toggleTheme darkTheme
      when 69 # e
        if edit then stopEditor() else startEditor()
      when 70 # f
        write 'finish\n'
      when 72 # h
        write 'hs\n'
      when 76 # l
        showLog = not showLog
        $('body').toggleClass 'hideLog'
        resize()
      when 77 # m
        submitNewGame()
      when 78 # n
        makeNewGame()
      when 82 # r
        showHighlight = not showHighlight
        draw()
      when 83 # s
        print getBoardString() + '\n'
      when 85 # u
        undoMove() if at > 0
      else
        return true
    false
  $(window).resize ->
    if boardVisible then resize()
