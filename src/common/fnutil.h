#ifndef HATRED_FNUTIL_H
#define HATRED_FNUTIL_H

#include <functional>

namespace hatred::fnutil {
    template<class CB>
    union cadata {
        cadata() {}
        std::decay_t<CB> callable;
    };

    // it was helpful getting this from stackoverflow, but i feel i can improve on it. ill need to see if i can set up decay to do the thing if used is true, i doubt it though, im pretty sure that would just cause infinite recursion
    // just a thing to make me think i guess
    template<unsigned int, class CB, class RT, class... AT>
    auto __decay(CB&& c, RT(*)(AT...)) {
        static bool used = 0;
        static cadata<CB> cadata;
        using catype = decltype(cadata.callable);

        if (used) cadata.callable.~catype();

        new(&cadata.callable) catype(std::forward<CB>(c));
        used = 1;

        return [](AT... args) -> RT {
            return RT(cadata.callable(std::forward<AT>(args)...));
        };
    }

    // <3 https://stackoverflow.com/questions/28746744/passing-capturing-lambda-as-function-pointer
    template<class F, unsigned int N, class CB>
    F* decay(CB&& c) {
        return __decay<N>(std::forward<CB>(c), (F*)0);
    }
};

#endif