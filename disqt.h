//
// Created by insane on 06.08.20.
//

#ifndef DISQT_DISQT_H
#define DISQT_DISQT_H

#include <future>

#include <cpp_redis/cpp_redis>

#include <QObject>
#include <QtQml>


// todo most other redis commands
class RedisQT : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool clientIsConnected READ getClientIsConnected NOTIFY clientIsConnectedChanged)
    Q_PROPERTY(bool subscriberIsConnected READ getSubscriberIsConnected NOTIFY subscriberIsConnectedChanged)

public:
    explicit RedisQT(QObject *parent = nullptr){
        connect_client();
        connect_subscriber();
    };

    ~RedisQT() override {};

    static void registerType(){
        qmlRegisterType<RedisQT>("io.eberlein.disqt", 1, 0, "Redis");
    }

    Q_INVOKABLE void connect(){
        connect_client();
        connect_subscriber();
    }

    Q_INVOKABLE void disconnect(){
        disconnect_client();
        disconnect_subscriber();
    }

    Q_INVOKABLE void connect_client(){
        client.connect(host.toStdString(), port);
        emit clientIsConnectedChanged(client.is_connected());
    }

    Q_INVOKABLE void disconnect_client(){
        client.disconnect();
        emit clientIsConnectedChanged(client.is_connected());
    }

    Q_INVOKABLE void disconnect_subscriber(){
        subscriber.disconnect();
        emit subscriberIsConnectedChanged(subscriber.is_connected());
    }

    Q_INVOKABLE void connect_subscriber(){
        subscriber.connect(host.toStdString(), port);
        emit subscriberIsConnectedChanged(subscriber.is_connected());
    }

    Q_INVOKABLE void set(const QString& key, const QString& value){
        client.set(key.toStdString(), value.toStdString());
        client.commit();
    }

    Q_INVOKABLE void getAsync(const QString& key){
        client.get(key.toStdString(), [&](cpp_redis::reply& r){
            getReturned(key, QString::fromStdString(r.as_string()));
        });
    }

    Q_INVOKABLE QString get(const QString& key){
        std::future<cpp_redis::reply> r = client.get(key.toStdString());
        r.wait();
        return QString::fromStdString(r.get().as_string());
    }

    Q_INVOKABLE void subscribe(const QString& channel){
        subscriber.subscribe(channel.toStdString(), [&](const std::string& channel, const std::string& msg){
            message(QString::fromStdString(channel), QString::fromStdString(msg));
        }); subscriber.commit();
        emit subscribed(channel);
    }

    Q_INVOKABLE void unsubscribe(const QString& channel){
        subscriber.unsubscribe(channel.toStdString()); subscriber.commit();
        emit unsubscribed(channel);
    }

    Q_INVOKABLE void psubscribe(const QString& channel){
        subscriber.psubscribe(channel.toStdString(), [&](const std::string& channel, const std::string& msg){
            message(QString::fromStdString(channel), QString::fromStdString(msg));
        }); subscriber.commit();
        psubscribed(channel);
    }

    Q_INVOKABLE void punsubscribe(const QString& channel){
        subscriber.punsubscribe(channel.toStdString()); subscriber.commit();
        punsubscribed(channel);
    }

    void setHost(const QString& host) {
        this->host = host;
        emit hostChanged();
    }

    QString getHost() const {return host;}

    void setPort(int port){
        this->port = port;
        emit portChanged();
    }

    int getPort() const {return port;}

    bool getClientIsConnected() const {return this->client.is_connected();}
    bool getSubscriberIsConnected() const {return this->subscriber.is_connected();}

signals:
    void hostChanged();
    void portChanged();
    void getReturned(const QString& key, const QString& reply);
    void message(const QString& channel, const QString& message);
    void subscribed(const QString& channel);
    void unsubscribed(const QString& channel);
    void psubscribed(const QString& channel);
    void punsubscribed(const QString& channel);
    void clientIsConnectedChanged(bool value);
    void subscriberIsConnectedChanged(bool value);

private:
    cpp_redis::client client;
    cpp_redis::subscriber subscriber;

    QString host = "127.0.0.1";
    int port = 6379;
};

#endif //DISQT_DISQT_H
