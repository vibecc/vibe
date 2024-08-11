//
// Created by scythe on 5/07/23.
//

#ifndef MAIN_PROCESS_H
#define MAIN_PROCESS_H

#include <stdexcept>
#include <memory>
#include <array>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unordered_map>

#include "util/enums.h"
#include "util/parameter_proccess.h"

#include "routes.hpp"
#include "util/request.hpp"
#include "util/nterminal.h"


constexpr int BUFFER = enums::neo::eSize::BUFFER;
constexpr int SESSION = enums::neo::eSize::SESSION;
constexpr int INIT_MAX_EVENTS = 10;

namespace workers {
using RoutesMap = std::unordered_map<string, std::unique_ptr<listen_routes>>;

    template<class T>
    class pMain_t {
                       int epoll_fd;
        unsigned short int next_register;

        std::vector<epoll_event> events;
        std::shared_ptr<RoutesMap>  routes;
        std::shared_ptr<T> connection;

    public:

        explicit pMain_t(const std::shared_ptr<T> &conn, const shared_ptr<RoutesMap> &_routes) :

        epoll_fd(epoll_create1(0)),
        next_register(enums::neo::eSize::DEF_REG),

        events(std::vector<epoll_event>(INIT_MAX_EVENTS)),
        routes(_routes),
        connection(conn) {}

        auto getMainProcess(std::shared_ptr<HTTP_QUERY> &qProcess){
            return [&]()->void {
                // creacion del socket
                connection->on();
                const int file_descriptor = connection->getDescription();
                constexpr auto socket_len_error_value = static_cast<socklen_t>(-1);

                if(Server::setNonblocking(file_descriptor) == MG_ERROR){
                    close(file_descriptor);
                }

                if (epoll_fd == -1) {
                    throw std::range_error(VB_EPOLL_RANGE);
                }

                epoll_event event{};
                event.events = EPOLLIN;
                event.data.fd = file_descriptor;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, file_descriptor, &event) == VB_NVALUE) {
                    close(epoll_fd);
                    throw std::range_error(VB_EPOLL_CTL);
                }

                while (static_cast<bool>(enums::neo::eStatus::START)){
                    try {
                        qProcess = make_shared<HTTP_QUERY>();

                        int notice = epoll_wait(epoll_fd, events.data(), INIT_MAX_EVENTS, VB_NVALUE);
                        if (notice == -1) {
                            terminal(VB_EPOLL_CERR, strerror(errno));
                            break;
                        }

                        for( int i = 0; i < notice; i++) {
                            if(events[i].data.fd == file_descriptor) {

                                sockaddr_in client_addr{};
                                socklen_t  client_adrr_len = sizeof(client_addr);
                                int client_file_descriptor = accept(file_descriptor, reinterpret_cast<sockaddr*>(&client_addr), &client_adrr_len);
                                if(client_adrr_len == socket_len_error_value) {
                                    continue;
                                }
                                Server::setNonblocking(client_file_descriptor);

                                event.events = EPOLLIN | EPOLLET;
                                event.data.fd = client_file_descriptor;
                                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_file_descriptor, &event) == VB_NVALUE) {
                                    terminal(VB_EPOLL_CERR, strerror(errno));
                                    close(client_file_descriptor);
                                    continue;
                                }
                            } else {

                                // char buffer[DEF_BUFFER_SIZE] = {0};
                                std::array<char, DEF_BUFFER_SIZE> buffer{};

                                if (const int bytes = recv(events[i].data.fd, buffer.data(), buffer.size(), VB_OK); bytes == VB_NVALUE) {
                                    if (errno == EWOULDBLOCK) {
                                        continue;
                                    }
                                    terminal(VB_EPOLL_CERR, strerror(errno));
                                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                                    close(events[i].data.fd);
                                    continue;
                                } else if(bytes == VB_OK) {
                                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                                    close(events[i].data.fd);
                                } else {

                                    shared_ptr<T> base = std::make_shared<T>();

                                    base->setPort(connection->getPort());
                                    base->setSocketId(events[i].data.fd);
                                    base->setResponse(buffer);
                                    base->setEpollEvents(events);
                                    base->setEpollfd(epoll_fd);
                                    base->setNotices(notice);

                                    ExecuteRoute(base, qProcess, routes);

                                }
                            }
                        }
                    }

                    catch(const std::exception& e) {
                       terminal(e.what());
                        close(epoll_fd);
                        if(close(file_descriptor) < VB_OK)
                            throw std::range_error(VB_MAIN_THREAD);
                    }
                }
            };

        }

                 static void ExecuteRoute(const shared_ptr<T> &base,const shared_ptr<HTTP_QUERY>&qProcess, const shared_ptr<RoutesMap> &routes)  {

                                    string send_target = ERROR_GET;

                                     const string socket_response = base->getResponse();

                                        if (socket_response.empty()){
                                            throw std::range_error(VB_SOCKET_FAIL);
                                        }

                                    auto [type, route] = HTTP_QUERY::route_refactor(socket_response);

                                        if(const auto itr = routes->find(route + type); itr != routes->end()) {

                                          if(not timeGuard(itr)) {

                                              const string parameters = type == GET_TYPE ?
                                                    HTTP_QUERY::route_refactor_params_get(socket_response)
                                                   : HTTP_QUERY::route_refactor_params(socket_response);

                                              auto [data, time_key] = itr->second->middlewares.execute(parameters,
                                              HTTP_QUERY::headers_from(socket_response),
                                              itr->second->guardRouteMsg
                                              );

                                              if(time_key > VB_OK) {
                                                  itr->second->time_key = time_key;
                                                  itr->second->time_point = std::chrono::system_clock::now();
                                              }
                                              send_target = std::move(data);
                                          } else {
                                              send_target = itr->second->guardRouteMsg != nullptr ?
                                                             utility_t::guard_route(itr->second->time_key, *itr->second->guardRouteMsg)
                                                            : utility_t::guard_route(itr->second->time_key);
                                          }
                                        }

                                        base->sendResponse(send_target);

                                        if (close(base->getDescription()) < enums::neo::eReturn::OK) {
                                            throw std::range_error(VB_SOCKET_CLOSE);
                                        }
            }



            static bool timeGuard(const RoutesMap::const_iterator & itr) {

                if(itr->second->time_key <= 0)
                    return false;
                if(itr->second->time_point == std::chrono::time_point<std::chrono::system_clock>())
                    return false;

                const auto now = std::chrono::system_clock::now();

                const std::chrono::duration<double> distance = now - itr->second->time_point;
                return distance.count() < itr->second->time_key;
            }


    };
}


#endif //MAIN_PROCESS_H
