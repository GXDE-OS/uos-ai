// WebSocket通道
class WebSocketChannel {
    constructor(url, transport){
        this.url = url;
        this.ws = null;
        this.transport = transport;
        this.connected = false;

        var channel = this;
        this.transport.onmessage = function (message) {
            console.log('收到QtWebChannel的消息:', typeof message, message);
            
            var data = message.data;
            data = typeof data === 'string' ? data : JSON.stringify(data);
            channel.ws.send(data);
        }
    }

    connect() {
        return new Promise((resolve, reject) => {
            try {
                this.ws = new WebSocket(this.url);
                
                this.ws.onopen = () => {
                    console.log('WebSocket连接成功');
                    this.connected = true;
                    resolve();
                };

                this.ws.onmessage = (event) => {
                    var data = event.data;
                    console.log('收到客户端消息:', typeof data, data);
                    const message = typeof data === 'string' ? data : JSON.stringify(data);
                    this.transport.send(message);
                };

                this.ws.onclose = () => {
                    console.log('WebSocket连接关闭');
                    this.connected = false;
                };

                this.ws.onerror = (error) => {
                    console.error('WebSocket错误:', error);
                    this.connected = false;
                    reject(error);
                };

            } catch (error) {
                reject(error);
            }
        });
    }

    close() {
        if (this.ws) {
            this.ws.close();
        }
    }
}

export default WebSocketChannel;



