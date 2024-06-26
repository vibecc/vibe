//
// Created by scythe on 20/07/23.
//

#ifndef VIBE_CORE_WINSTANCES_H
#define VIBE_CORE_WINSTANCES_H

#include "parameter_proccess.h"

#include <vector>
#include <memory>
#include <array>
#include <condition_variable>

namespace  workers {
    template<class T>
    class wInstances {

    public:

        wInstances() = default;

        std::array<std::vector<std::shared_ptr<T>>, 0x3> workers;
        std::array<std::condition_variable, 0x3> conditions;

        std::vector<std::tuple<std::shared_ptr<T>, string>> worker_send;
        std::condition_variable condition_response;
        std::condition_variable condition_access;
    };
}

#endif //VIBE_CORE_WINSTANCES_H
