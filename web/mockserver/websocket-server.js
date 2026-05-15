import { WebSocketServer } from 'ws';
import { createServer } from 'http';
import { randomUUID } from 'crypto';

const server = createServer();
const wss = new WebSocketServer({ server });

// 存储连接的客户端
const clients = new Map();

wss.on('connection', (ws, req) => {
    const clientId = randomUUID();
    const clientIp = req.socket.remoteAddress;
    
    console.log(`客户端 ${clientId} 已连接 (IP: ${clientIp})`);
    
    // 存储客户端信息
    clients.set(clientId, {
        ws,
        id: clientId,
        ip: clientIp,
        connectedAt: new Date()
    });
    
    // 处理客户端消息
    ws.on('message', (data) => {
        try {
            console.log(`收到来自 ${clientId} 的消息，正在转发给其他客户端`, data.toString(), typeof data);
            
            // 直接转发原始数据给其他客户端（排除发送者自己）
            clients.forEach((client, id) => {
                if (id !== clientId && client.ws.readyState === client.ws.OPEN) {
                    client.ws.send(data.toString());
                }
            });
            
        } catch (error) {
            console.error('转发消息时出错:', error);
        }
    });
    
    // 处理连接关闭
    ws.on('close', () => {
        console.log(`客户端 ${clientId} 已断开连接`);
        clients.delete(clientId);
    });
    
    // 处理错误
    ws.on('error', (error) => {
        console.error(`客户端 ${clientId} 连接错误:`, error);
        clients.delete(clientId);
    });
});

// 启动服务器
const PORT = process.env.PORT || 8081;
server.listen(PORT, () => {
    console.log(`WebSocket转发服务器运行在端口 ${PORT}`);
    console.log(`可以通过 ws://localhost:${PORT} 连接`);
});

// 优雅关闭
process.on('SIGINT', () => {
    console.log('正在关闭服务器...');
    
    // 关闭所有连接
    clients.forEach((client) => {
        client.ws.close();
    });
    
    wss.close(() => {
        server.close(() => {
            console.log('服务器已关闭');
            process.exit(0);
        });
    });
});