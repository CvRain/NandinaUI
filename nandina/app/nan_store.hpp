//
// app/nan_store — application-level signal store base.
//
// A store is the long-lived, developer-defined state container for an app. It
// usually contains reactive::Signal<T> members and is owned by NanApplication.
// Pages receive it through PageContext, which keeps route params (downward data)
// separate from shared app state (upward / lateral sync).
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_STORE_HPP
#define NANDINA_EXPERIMENT_APP_NAN_STORE_HPP

namespace nandina::app
{

    class NanStore {
    public:
        NanStore() = default;
        virtual ~NanStore() = default;

        NanStore(const NanStore&) = delete;
        auto operator=(const NanStore&) -> NanStore& = delete;
        NanStore(NanStore&&) = delete;
        auto operator=(NanStore&&) -> NanStore& = delete;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_STORE_HPP
