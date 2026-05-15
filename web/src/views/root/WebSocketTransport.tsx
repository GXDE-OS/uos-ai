// WebSocket传输层
class WebSocketTransport {
    constructor(url) {
        this.url = url;
        this.ws = null;
        this.onmessage = null;
        this.connected = false;
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
                    if (this.onmessage) {
                        this.onmessage(event);
                    }
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

    send(data) {
        if (this.connected && this.ws) {
            this.ws.send(data);
        } else {
            console.warn('WebSocket未连接，无法发送消息');
        }
    }

    close() {
        if (this.ws) {
            this.ws.close();
        }
    }
}

export default WebSocketTransport;



