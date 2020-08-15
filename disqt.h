//
// Created by insane on 06.08.20.
//

#ifndef DISQT_DISQT_H
#define DISQT_DISQT_H

#include <future>
#include <chrono>

#include "cpp_redis/cpp_redis"

#include <QObject>
#include <QtQml>

/**
 * RedisQT class
 * Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
 * Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
 * Q_PROPERTY(bool clientIsConnected READ getClientIsConnected NOTIFY clientIsConnectedChanged)
 * Q_PROPERTY(bool subscriberIsConnected READ getSubscriberIsConnected NOTIFY subscriberIsConnectedChanged)
 * Q_PROPERTY(bool isReady READ getIsReady NOTIFY isReadyChanged)
 */
class RedisQT : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool clientIsConnected READ getClientIsConnected NOTIFY clientIsConnectedChanged)
    Q_PROPERTY(bool subscriberIsConnected READ getSubscriberIsConnected NOTIFY subscriberIsConnectedChanged)
    Q_PROPERTY(bool isReady READ getIsReady NOTIFY isReadyChanged)

public:
    /**
     * create RedisQT instance
     * @param parent QObject
     */
    explicit RedisQT(QObject *parent = nullptr): QObject(parent){
        setIsReady(true);
    };

    /**
     * destructor
     */
    ~RedisQT() override {};

    /**
     * register the qml type 'Redis' with url 'io.eberlein.disqt' version 1.0
     */
    static void registerType(){
        qmlRegisterType<RedisQT>("io.eberlein.disqt", 1, 0, "Redis");
    }

    /**
     * calls connect() in a detached thread
     */
    Q_INVOKABLE void connectAsync(){
        std::thread t([&](){connect();});
        t.detach();
    }

    /**
     * connects the client and the subscriber
     * basically calls connect_client() and connect_subscriber()
     */
    Q_INVOKABLE void connect(){
        connect_client();
        connect_subscriber();
    }

    /**
     * calls disconnect() in a detached thread
     */
    Q_INVOKABLE void disconnectAsync(){
        std::thread t([&](){disconnect();});
        t.detach();
    }

    /**
     * disconnects the client and the subscriber
     * basically calls disconnect_client() and disconnect_subscriber()
     */
    Q_INVOKABLE void disconnect(){
        disconnect_client();
        disconnect_subscriber();
    }

    /**
     * connects the client to host and port
     */
    Q_INVOKABLE void connect_client(){
        client.connect(host.toStdString(), port);
        qDebug() << "client should now be connected to" << host << ":" << port;
        emit clientIsConnectedChanged(client.is_connected());
    }

    /**
     * disconnects the client and emits clientIsConnectedChanged
     */
    Q_INVOKABLE void disconnect_client(){
        client.disconnect();
        emit clientIsConnectedChanged(client.is_connected());
    }

    /**
     * connects the subscriber to host and port
     */
    Q_INVOKABLE void connect_subscriber(){
        subscriber.connect(host.toStdString(), port);
        qDebug() << "subscriber should now be connected to" << host << ":" << port;
        emit subscriberIsConnectedChanged(subscriber.is_connected());
    }

    /**
     * disconnects the subscriber and emits subscriberIsConnectedChanged
     */
    Q_INVOKABLE void disconnect_subscriber(){
        subscriber.disconnect();
        emit subscriberIsConnectedChanged(subscriber.is_connected());
    }

    /**
     * set a value for a key
     * @tparam T Anything that fits into a QJsonValue
     * @param key QString
     * @param value T
     */
    Q_INVOKABLE template<typename T> void set(const std::string& key, const QJsonObject& o){
        QJsonDocument t(o);
        auto json = t.toJson().toStdString();
        qDebug() << "setting" << QString::fromStdString(key) << ":" << t.toJson();
        client.set(key, json);
        client.publish(key, json);
        client.sync_commit();
    }

    /**
     * basically calls set() in a detached thread
     * @tparam T Anything that fits into a QJsonValue
     * @param key QString
     * @param value T
     */
    Q_INVOKABLE template<typename T> void setAsync(const QString& key, T value) {
        std::thread t([&](){set(key, value);});
        t.detach();
    }

    /**
     * synchronously gets a value by its key
     * @param key QString
     * @return QJsonValue
     */
    Q_INVOKABLE QJsonValue get(const std::string& key) {
        std::future<cpp_redis::reply> r = client.get(key);
        client.sync_commit();
        return str2doc(r.get().as_string())[QString::fromStdString(key)];
    }

    /**
     * gets a value by a key asynchronously
     * emits getReturned()
     * @param key QString
     */
    Q_INVOKABLE void getAsync(const QString& key){
        client.get(key.toStdString(), [&](cpp_redis::reply& r){
            emit getReturned(key, str2doc(r.as_string())[key]);
        });
    }

    /**
     * checks if a key exists
     * @param key QString
     * @return bool
     */
    Q_INVOKABLE bool exists(const QString& key){
        auto f = client.exists({key.toStdString()});
        client.sync_commit();
        return f.get().as_integer() == 1;
    }

    /**
     * subscribes to channel
     * emits message() when receiving a message
     * @param channel QString
     */
    Q_INVOKABLE void subscribe(const QString& channel){
        subscriber.subscribe(channel.toStdString(), [&](const std::string& channel, const std::string& msg){
            emit message(QString::fromStdString(channel), QString::fromStdString(msg));
        }); subscriber.commit();
        emit subscribed(channel);
    }

    /**
     * unsubscribes from a channel
     * @param channel QString
     */
    Q_INVOKABLE void unsubscribe(const QString& channel){
        subscriber.unsubscribe(channel.toStdString()); subscriber.commit();
        emit unsubscribed(channel);
    }

    /**
     * subscribe with a wildcard
     * emits message() when receiving a message
     * @param channel QString
     */
    Q_INVOKABLE void psubscribe(const QString& channel){
        subscriber.psubscribe(channel.toStdString(), [&](const std::string& channel, const std::string& msg){
            emit message(QString::fromStdString(channel), QString::fromStdString(msg));
        }); subscriber.commit();
        psubscribed(channel);
    }

    /**
     * unsubscribes from a wildcard
     * @param channel QString
     */
    Q_INVOKABLE void punsubscribe(const QString& channel){
        subscriber.punsubscribe(channel.toStdString()); subscriber.commit();
        punsubscribed(channel);
    }

    Q_INVOKABLE void publish(const QString& channel, QString message){
        client.publish(channel.toStdString(), message.toStdString());
    }

    /**
     * sets the current hosts
     * emits hostChanged()
     * @param host
     */
    void setHost(const QString& host) {
        this->host = host;
        emit hostChanged();
    }

    /**
     * gets the current host
     * @return QString
     */
    QString getHost() const {return host;}

    /**
     * sets the current port
     * @param port int
     */
    void setPort(int port){
        this->port = port;
        emit portChanged();
    }

    /**
     * gets the current port
     * @return int
     */
    int getPort() const {return port;}

    /**
     * returns if the client is connected
     * @return bool
     */
    bool getClientIsConnected() const {
        return this->client.is_connected();
    }

    /**
     * returns if the subscriber is connected
     * @return bool
     */
    bool getSubscriberIsConnected() const {
        return this->subscriber.is_connected();
    }

    /**
     * returns if RedisQT is ready
     * @return bool
     */
    bool getIsReady() const {
        return this->isReady;
    }

    /**
     * creates a QJsonDocument from a std::string
     * @param data std::string
     * @return QJsonDocument
     */
    QJsonDocument str2doc(const std::string& data){
        return QJsonDocument::fromBinaryData(QString::fromStdString(data).toUtf8());
    }

    template<typename T> QJsonObject makeQJsonObject(const QString& key, T value){
        QJsonObject o;
        o.insert(key, QJsonValue(value));
        return o;
    }

    template<typename T> QJsonObject mjo(const QString& key, T value){
        return makeQJsonObject(key, value);
    }

signals:
    void hostChanged();
    void portChanged();
    void getReturned(const QString& key, const QJsonValue& value);
    void message(const QString& channel, const QString& message);
    void subscribed(const QString& channel);
    void unsubscribed(const QString& channel);
    void psubscribed(const QString& channel);
    void punsubscribed(const QString& channel);
    void clientIsConnectedChanged(bool value);
    void subscriberIsConnectedChanged(bool value);
    void isReadyChanged();

private:
    template<typename R> std::future_status get_future_status(std::future<R> const& f, int wt=0){
        return f.wait_for(std::chrono::seconds(wt));
    }

    template<typename R> bool is_future_ready(std::future<R> const& f){
        return get_future_status(f) == std::future_status::ready;
    }

    /**
     * set the isReady value
     * @param value bool
     */
    void setIsReady(bool value){
        qDebug() << "DisQT is" << (!value ? "not" : "") << "ready.";
        this->isReady = value;
        emit isReadyChanged();
    }

    bool isReady = false;
    cpp_redis::client client;
    cpp_redis::subscriber subscriber;

    QString host = "127.0.0.1";
    int port = 6379;
};

#endif //DISQT_DISQT_H
