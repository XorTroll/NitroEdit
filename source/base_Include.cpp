#include <base_Include.hpp>

extern "C" {

    void *__real_malloc(size_t size);

    void *__wrap_malloc(size_t size) {
        auto ptr = __real_malloc(size);
        if(ptr == nullptr) {
            OnCriticalError("Out of memory trying\nto allocate " + std::to_string(size) + " bytes");
        }
        return ptr;
    }

    void __real_free(void *ptr);

    void __wrap_free(void *ptr) {
        if(ptr != nullptr) {
            __real_free(ptr);
        }
    }

    void __wrap_abort() {
        OnCriticalError("An abort() ocurred");
    }

}