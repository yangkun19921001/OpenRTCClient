const express = require('express');
const app = express();
const fs = require('fs');
const http = process.env.HTTPS === 'true' ? require('https') : require('http');
const server = process.env.HTTPS === 'true' ? http.createServer({
    key : fs.readFileSync('./cert/rtcmedia.top.key'),
    cert: fs.readFileSync('./cert/rtcmedia.top_bundle.pem')
  }, app): http.createServer(app);
//const http = require('http').Server(app);
const io = require('socket.io')(server);
const PORT = process.env.HTTPS === 'true' ? 443:8880;

app.use(express.static(__dirname + '/client'));

io.on('connection', (socket) => {
  console.log('A user connected:', socket.id);

// 加入房间
socket.on('join', (room) => {
    socket.join(room);
    console.log(`Socket ID ${socket.id} joined room ${room}`);
  
    const clientsInRoom = io.sockets.adapter.rooms[room];
    const clientsCount = Object.keys(clientsInRoom.sockets).length;
  
    // 获取房间中的所有客户端 ID，除了当前加入的客户端
    const otherClientIds = Object.keys(clientsInRoom.sockets).filter(id => id !== socket.id);
  
    // 发送包含所有其他客户端 ID 的 join 消息给新加入的客户端
    socket.emit('joined',  room, socket.id, otherClientIds );
  
    // 如果房间中有多个客户端，将 join 消息发送给房间中的其他客户端
    if (clientsCount > 1) {
      socket.to(room).emit('joined',  room,  socket.id );
    }
  });
  

  // 发送消息到指定的房间和 socket.id
  socket.on('message', ( room, id, msg ) => {
    const sender = socket.id;
    const clientsInRoom = io.sockets.adapter.rooms[room];
    if (clientsInRoom && clientsInRoom.sockets.hasOwnProperty(id)) {
      socket.to(id).emit('message', sender, id, msg);
    } else {
      console.log(`Socket ID ${id} is not in room ${room}`);
    }
  });

  // 离开房间
  socket.on('leave', (room) => {
    console.log(`User ${socket.id} left room ${room}`);

    // 发送 leave 消息给自己和房间中的其他客户端
    io.to(room).emit('leaved',  room, socket.id );

    socket.leave(room);
    console.log(`Socket ID ${socket.id} left room ${room}`);
  });

  // 断开连接
  socket.on('disconnect', () => {
    console.log('A user disconnected:', socket.id);
  });
});

server.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}`);
});