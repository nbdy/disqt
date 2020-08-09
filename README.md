### disqt
(dis cute)<br>
a header-only qml interface for redis<br>
#### features
- pub/sub
- set/get
#### todo
all other redis commands
#### dependencies
- [cpp_redis](https://github.com/cpp-redis/cpp_redis)
    - [tacopie](https://github.com/cylix/tacopie)
#### usage
##### cpp
```c++
#include <disqt/disqt.h> // you could also directly include it in your project

// register redisqt so it's usable from qml
RedisQT::registerType(); 
```
##### qml
```qml
import io.eberlein.disqt 1.0

Redis {
    host: "127.0.0.1"
    port: 6379

    onClientConnected: console.log("client connected")
    onSubscriberConnected: console.log("subscriber connected")
    onSubscribed: console.log(channel)
    onPsubscribed: console.log(channel)
    onUnsubscribed: console.log(channel)
    onPunsubscribed: console.log(channel)
    onMessage: console.log(channel, message)
    onGetReturned: console.log(key, value)
    onHostChanged: console.log(host)
    onPortChanged: console.log(port)
}
```