#ifndef VIBE_H
#define VIBE_H

#include "sockets.h"
#include "routes.hpp"
#include "util/request.hpp"
#include "util/enums.h"
#include "util/worker_core.h"
#include "util/wInstances.h"

#include <thread>
#include <memory>
#include <string>

using std::make_shared;
using std::make_unique;
using std::string;

using workers::eWorkers;
using workers::wInstances;

template <class T>
class Vibe {

    shared_ptr<::eWorkers<T>> eWorkers = nullptr;
    shared_ptr<::wInstances<T>> wInstances = nullptr;

    std::unordered_map<string, std::unique_ptr<listen_routes>> routes;
    std::shared_ptr<T> tcpControl;
    std::shared_ptr<HTTP_QUERY> qProcess = nullptr;

    uint16_t PORT{enums::neo::eSize::DEF_PORT};

    std::mutex lock_process;

    void tcpInt();

public:
    [[maybe_unused]] explicit Vibe(uint16_t port);
    explicit Vibe();

    int http_response(const string&, MiddlewareList, const string&);

    int get(const string& route,    const MiddlewareList& middlewares);
    int post(const string& route,   const MiddlewareList& middlewares);
    int put(const string& route,    const MiddlewareList& middlewares);
    int deleteX(const string& route,const MiddlewareList& middlewares);
    int patch(const string& route,  const MiddlewareList& middlewares);
    int head(const string& route,   const MiddlewareList& middlewares);
    int options(const string& route,const MiddlewareList& middlewares);
    int link(const string& route,   const MiddlewareList& middlewares);
    int unlink(const string& route, const MiddlewareList& middlewares);
    int purge(const string& route,  const MiddlewareList& middlewares);

    int use(const Route_t&);

    int setPort(uint16_t) noexcept;
    [[nodiscard]] [[maybe_unused]] inline uint16_t getPort() const noexcept{return PORT;};
    void listen();

};

template <class T>
[[maybe_unused]] Vibe<T>::Vibe(const uint16_t port) {
    if (port >= enums::neo::eSize::MIN_PORT) { PORT = port; }
    tcpInt();
}

template <class T>
Vibe<T>::Vibe() {
    tcpInt();
}

template <class T>

int Vibe<T>::http_response(const string &endpoint, MiddlewareList middlewareList, const string& type) {
    try {
        routes[ endpoint + type ] = make_unique<listen_routes>( endpoint, std::move(middlewareList), type);
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return enums::neo::eReturn::ERROR;
    }
    return enums::neo::eReturn::OK;
}

template <class T>
[[maybe_unused]] int Vibe<T>::get(const string& route,const MiddlewareList &_funcs){
    return http_response(route, _funcs, GET_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::post(const string& route,const MiddlewareList &_funcs) {
    return http_response(route, _funcs, POST_TYPE );
}
template <class T>
[[maybe_unused]] int Vibe<T>::put(const string& route,const MiddlewareList &_funcs) {
    return http_response(route, _funcs, PUT_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::deleteX(const string& route,const MiddlewareList &_funcs) {
    return http_response(route, _funcs, DELETE_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::patch(const string& route,const MiddlewareList &_funcs) {
    return http_response(route, _funcs, PATCH_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::head(const string& route,const MiddlewareList &_funcs) {
    return http_response(route, _funcs, HEAD_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::options(const string& route,const MiddlewareList &_funcs) {
    return http_response(route, _funcs, OPTIONS_TYPE);
}
template <class T>
[[maybe_unused]]  int Vibe<T>::link(const string& route,const MiddlewareList &_funcs) {
    return http_response(route, _funcs, LINK_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::unlink(const string& route,const MiddlewareList &_funcs) {
    return http_response(route, _funcs, UNLINK_TYPE);
}
template <class T>
[[maybe_unused]] int Vibe<T>::purge(const string& route,const MiddlewareList &_funcs) {
    return http_response(route, _funcs, PURGE_TYPE);
}

template <class T>
[[maybe_unused]] int Vibe<T>::use(const Route_t & route) {
    return http_response( route.route, route.middlewares, route.type);
}


template <class T>
void Vibe<T>::listen() {
    std::thread _main(eWorkers->Main->getMainProcess(qProcess));
    _main.join();
}

template <class T>
int Vibe<T>::setPort(const uint16_t _port) noexcept {
    if (_port >= enums::neo::eSize::MIN_PORT) {
        PORT = _port;
        if(tcpControl != nullptr) {
            tcpControl->setPort(PORT);
            return enums::neo::eReturn::OK;
        }
    }
    return enums::neo::eReturn::ERROR;
}

template<class T>
void Vibe<T>::tcpInt() {

    eWorkers = make_shared<::eWorkers<T>>();
    wInstances = make_shared<::wInstances<T>>();
    eWorkers->Main =make_unique<workers::pMain_t<T>>(wInstances->workers, wInstances->conditions, tcpControl,  lock_process, routes);

    tcpControl = make_shared<T>();
    tcpControl->setBuffer(BUFFER);
    tcpControl->setPort(PORT);
    tcpControl->setSessions(SESSION);
}
typedef Vibe<Server> Router;
typedef utility_t Convert;


#endif // VIBE_H