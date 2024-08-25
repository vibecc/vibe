//
// Created by owl on 19/08/24.
//

#ifndef IO_H
#define IO_H

#include <memory>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unordered_map>
#include <atomic>

#include "../util/enums.h"
#include "../util/parameter_proccess.h"

#include "../routes.hpp"
#include "request.hpp"
#include "../util/nterminal.h"
#include "../sockets.h"
#include "../threading/thread_pool.h"

using std::make_shared, std::vector, std::unique_ptr;

using RoutesMap = std::unordered_map<string, std::unique_ptr<listen_routes>>;
/*
 *  RequestIO for Server socket class, if you want implement other Server, you should create other RequestIO for the implementation
 */
class RequestIO {

    private:
    shared_ptr<std::vector<epoll_event>> events;
    shared_ptr<RoutesMap>  routes;
    unique_ptr<int> file_descriptor;
    unique_ptr<int> epoll_fd;
    shared_ptr<Server> connection;

    shared_ptr<threading::ThreadPool> thread_pool_;

    size_t threads_{4};

    void Process(int, epoll_event&) const;
    void ProcessFileDescriptor(int, int) const;

    public:

     RequestIO(const shared_ptr<vector<epoll_event>>&,
               const shared_ptr<RoutesMap> &,
               int &,
               int &,
               const shared_ptr<Server>&);


    void Dispatch(int id, epoll_event &event) const;
    void SetThreads(size_t size);

    static bool TimeGuard(const RoutesMap::const_iterator & itr);
    static void ExecuteRoute(const shared_ptr<Server> &instance, const shared_ptr<RoutesMap> &routes);
};

#endif //IO_H
