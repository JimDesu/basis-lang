#ifndef ROLLBACKGUARD_H
#define ROLLBACKGUARD_H

namespace basis {
    template<typename T>
    class RollbackGuard {
    public:
        explicit RollbackGuard(T* pActive)
            : pActive(pActive), saved(*pActive), shouldRestore(true) {}

        ~RollbackGuard() {
            if (shouldRestore) {
                *pActive = saved;
            }
        }

        void commit() {
            shouldRestore = false;
        }

        void restore() {
            shouldRestore = true;
        }

    private:
        T* pActive;
        T saved;
        bool shouldRestore;
    };

}

#endif // ROLLBACKGUARD_H

