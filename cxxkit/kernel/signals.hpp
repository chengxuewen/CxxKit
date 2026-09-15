/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/tools/checks.hpp>
#include <cxxkit/memory/memory.hpp>
#include <cxxkit/tools/type_list.hpp>
#include <cxxkit/tools/type_traits.hpp>
#include <cxxkit/tools/optional.hpp>

#include <mutex>
#include <memory>
#include <thread>
#include <atomic>
#include <vector>
#include <utility>

#if CXXKIT_RTTI_ENABLED
#    include <typeinfo>
#endif

CXXKIT_BEGIN_NAMESPACE
#if 1
/**
 * @addtogroup core
 * @{
 * @addtogroup UniqueFunction
 * @brief Signal & slot: thread-safe and single-threaded signal types with observer-based lifetime tracking.
 * @details
 *
 * Emission-time contract (applies to every signal variant, @c SignalR and
 * @c SignalUnsafeR included):
 *
 * - @b Connect @b during @b emission: the new slot is @em not invoked by the
 *   emission in progress. It becomes visible to the next emission.
 * - @b Disconnect @b during @b emission: a slot that is already executing
 *   runs to completion; a slot disconnected before its turn is reached is
 *   skipped (invoke-time connectivity check); the disconnect takes effect
 *   for the remaining and subsequent emissions.
 * - @b Mutual-exclusion snapshotting: the thread-safe variants (@c Signal,
 *   @c SignalR) snapshot via copy-on-write under the signal mutex; the
 *   single-threaded variants (@c SignalUnsafe, @c SignalUnsafeR) snapshot
 *   once at emission start under their no-op lock.
 *
 * @b Recursion: emitting the same signal from inside a slot is supported in
 * @em all variants (including @c SignalUnsafeR) — emission never invokes
 * slots while holding the signal lock, so re-entrant emissions see the
 * current slot list and cannot deadlock.
 *
 * For emissions that produce a value, see @c SignalR / @c SignalUnsafeR and
 * their combiner documentation (@c optional_last_value, @c maximum, custom
 * combiners).
 * @{
 */

namespace signals
{

template <typename, typename...>
class SignalBase;
template <typename, typename, typename, typename...>
class SignalBaseR;

/**
 * A group_id is used to identify a group of slots
 */
using GroupId = std::int32_t;

namespace detail
{
// Used to detect an object of observer type
struct ObserverType
{
};
} // namespace detail

namespace trait
{
namespace detail
{
template <typename... Args>
struct IsCallableImpl;
// F, typelist<Args...>
template <typename F, typename... Args>
struct IsCallableImpl<F, TypeList<Args...>> : traits::is_invocable<F, Args...>
{
};
// F, P, typelist<Args...>
template <typename F, typename P, typename... Args>
struct IsCallableImpl<F, P, TypeList<Args...>> : traits::is_invocable<F, P, Args...>
{
};
template <typename... Args>
using is_callable = IsCallableImpl<Args...>;
#    if CXXKIT_CC_CPP14_OR_GREATER
template <typename... Args>
constexpr bool is_callable_v = is_callable<Args...>::value;
#    endif
} // namespace detail

template <typename... Args>
using TypeList = TypeList<Args...>;

static constexpr bool with_rtti =
#    if CXXKIT_RTTI_ENABLED
    true;
#    else
    false;
#    endif

template <typename T>
struct is_pointer : traits::is_pointer<T>
{
};
template <typename T>
struct is_function : traits::is_function<T>
{
};
template <typename T>
struct is_weak_ptr : traits::is_weak_ptr<T>
{
};
template <typename P>
struct is_weak_ptr_compatible : traits::is_weak_ptr_compatible<typename std::decay<P>::type>
{
};
template <typename T>
struct has_call_operator : traits::has_call_operator<T>
{
};
template <typename L, typename... Args>
using is_callable = detail::is_callable<Args..., L>;
template <typename T>
struct is_member_function_pointer : traits::is_member_function_pointer<T>
{
};
#    if CXXKIT_CC_CPP14_OR_GREATER
template <typename T>
constexpr bool is_pointer_v = traits::is_pointer<T>::value;
template <typename T>
constexpr bool is_function_v = traits::is_function<T>::value;
template <typename T>
constexpr bool is_weak_ptr_v = traits::is_weak_ptr<T>::value;
template <typename P>
constexpr bool is_weak_ptr_compatible_v = traits::is_weak_ptr_compatible<typename std::decay<P>::type>::value;
template <typename T>
constexpr bool has_call_operator_v = traits::has_call_operator<T>::value;
template <typename L, typename... Args>
constexpr bool is_callable_v = detail::is_callable<Args..., L>::value;
template <typename T>
constexpr bool is_member_function_pointer_v = traits::is_member_function_pointer<T>::value;
#    endif


template <typename...>
struct is_signal : std::false_type
{
};
template <typename L, typename... T>
struct is_signal<SignalBase<L, T...>> : std::true_type
{
};
template <typename T>
struct is_observer : std::is_base_of<::cxxkit::signals::detail::ObserverType,
                                     typename std::remove_pointer<typename std::remove_reference<T>::type>::type>
{
};
#    if CXXKIT_CC_CPP14_OR_GREATER
template <typename S>
constexpr bool is_signal_v = is_signal<S>::value;
template <typename T>
constexpr bool is_observer_v =
    std::is_base_of<::cxxkit::signals::detail::ObserverType,
                    typename std::remove_pointer<typename std::remove_reference<T>::type>::type>::value;
#    endif

} // namespace trait


namespace detail
{

/**
 * The following function_traits and object_pointer series of templates are
 * used to circumvent the type-erasing that takes place in the slot_base
 * implementations. They are used to compare the stored functions and objects
 * with another one for disconnection purpose.
 */

/*
 * Function pointers and member function pointers size differ from compiler to
 * compiler, and for virtual members compared to non virtual members. On some
 * compilers, multiple inheritance has an impact too. Hence, we form an union
 * big enough to store any kind of function pointer.
 */
namespace mock
{

struct a
{
    virtual ~a() = default;
    void f();
    virtual void g();
    static void h();
};
struct b
{
    virtual ~b() = default;
    void f();
    virtual void g();
};
struct c : a, b
{
    void f();
    void g() override;
};
struct d : virtual a
{
    void g() override;
};

union fun_types
{
    decltype(&d::g) dm;
    decltype(&c::g) mm;
    decltype(&c::g) mvm;
    decltype(&a::f) m;
    decltype(&a::g) vm;
    decltype(&a::h) s;
    void (*f)();
    void *o;
};

} // namespace mock

/*
 * This struct is used to store function pointers.
 * This is needed for slot disconnection by function pointer.
 * It assumes the underlying implementation to be trivially copiable.
 */
struct func_ptr
{
    func_ptr()
        : sz{0}
    {
        std::uninitialized_fill(std::begin(data), std::end(data), '\0');
    }

    template <typename T>
    void store(const T &t)
    {
        const auto *b = reinterpret_cast<const char *>(&t);
        sz = sizeof(T);
        std::memcpy(data, b, sz);
    }

    template <typename T>
    const T *as() const
    {
        if (sizeof(T) != sz)
        {
            return nullptr;
        }
        return reinterpret_cast<const T *>(data);
    }

private:
    alignas(sizeof(mock::fun_types)) char data[sizeof(mock::fun_types)];
    size_t sz;
};


template <typename T, typename = void>
struct function_traits
{
    static void ptr(const T & /*t*/, func_ptr & /*d*/) { }

    static bool eq(const T & /*t*/, const func_ptr & /*d*/) { return false; }

    static constexpr bool is_disconnectable = false;
    static constexpr bool must_check_object = true;
};

template <typename T>
struct function_traits<T, typename std::enable_if<trait::is_function<T>::value>::type>
{
    static void ptr(T &t, func_ptr &d) { d.store(&t); }

    static bool eq(T &t, const func_ptr &d)
    {
        const auto *r = d.as<const T *>();
        return r && *r == &t;
    }

    static constexpr bool is_disconnectable = true;
    static constexpr bool must_check_object = false;
};

template <typename T>
struct function_traits<T *, typename std::enable_if<trait::is_function<T>::value>::type>
{
    static void ptr(T *t, func_ptr &d) { function_traits<T>::ptr(*t, d); }

    static bool eq(T *t, const func_ptr &d) { return function_traits<T>::eq(*t, d); }

    static constexpr bool is_disconnectable = true;
    static constexpr bool must_check_object = false;
};

template <typename T>
struct function_traits<T, typename std::enable_if<trait::is_member_function_pointer<T>::value>::type>
{
    static void ptr(T t, func_ptr &d) { d.store(t); }

    static bool eq(T t, const func_ptr &d)
    {
        const auto *r = d.as<const T>();
        return r && *r == t;
    }

    static constexpr bool is_disconnectable = trait::with_rtti;
    static constexpr bool must_check_object = true;
};

// for function objects, the assumption is that we are looking for the call operator
template <typename T>
struct function_traits<T, typename std::enable_if<trait::has_call_operator<T>::value>::type>
{
    using call_type = decltype(&std::remove_reference<T>::type::operator());

    static void ptr(const T & /*t*/, func_ptr &d) { function_traits<call_type>::ptr(&T::operator(), d); }

    static bool eq(const T & /*t*/, const func_ptr &d) { return function_traits<call_type>::eq(&T::operator(), d); }

    static constexpr bool is_disconnectable = function_traits<call_type>::is_disconnectable;
    static constexpr bool must_check_object = function_traits<call_type>::must_check_object;
};

template <typename T>
func_ptr get_function_ptr(const T &t)
{
    func_ptr d;
    function_traits<typename std::decay<T>::type>::ptr(t, d);
    return d;
}

template <typename T>
bool eq_function_ptr(const T &t, const func_ptr &d)
{
    return function_traits<typename std::decay<T>::type>::eq(t, d);
}

/*
 * obj_ptr is used to store a pointer to an object.
 * The object_pointer traits are needed to handle trackable objects correctly,
 * as they are likely to not be pointers.
 */
using obj_ptr = const void *;

template <typename T>
obj_ptr get_object_ptr(const T &t);

template <typename T, typename = void>
struct object_pointer
{
    static obj_ptr get(const T &) { return nullptr; }
};

template <typename T>
struct object_pointer<T *, typename std::enable_if<trait::is_pointer<T *>::value>::type>
{
    static obj_ptr get(const T *t) { return reinterpret_cast<obj_ptr>(t); }
};

template <typename T>
struct object_pointer<T, typename std::enable_if<trait::is_weak_ptr<T>::value>::type>
{
    static obj_ptr get(const T &t)
    {
        auto p = t.lock();
        return get_object_ptr(p);
    }
};

template <typename T>
struct object_pointer<T,
                      typename std::enable_if<!trait::is_pointer<T>::value && !trait::is_weak_ptr<T>::value &&
                                              trait::is_weak_ptr_compatible<T>::value>::type>
{
    static obj_ptr get(const T &t) { return t ? reinterpret_cast<obj_ptr>(t.get()) : nullptr; }
};

template <typename T>
obj_ptr get_object_ptr(const T &t)
{
    return object_pointer<T>::get(t);
}


// noop mutex for thread-unsafe use
struct NullMutex
{
    NullMutex() noexcept = default;
    ~NullMutex() noexcept = default;
    NullMutex(const NullMutex &) = delete;
    NullMutex &operator=(const NullMutex &) = delete;
    NullMutex(NullMutex &&) = delete;
    NullMutex &operator=(NullMutex &&) = delete;

    inline bool try_lock() noexcept { return true; }
    inline void lock() noexcept { }
    inline void unlock() noexcept { }
};

/**
 * A spin mutex that yields, mostly for use in benchmarks and scenarii that invoke
 * slots at a very high pace.
 * One should almost always prefer a standard mutex over this.
 */
struct SpinMutex
{
    SpinMutex() noexcept = default;
    ~SpinMutex() noexcept = default;
    SpinMutex(SpinMutex const &) = delete;
    SpinMutex &operator=(const SpinMutex &) = delete;
    SpinMutex(SpinMutex &&) = delete;
    SpinMutex &operator=(SpinMutex &&) = delete;

    void lock() noexcept
    {
        while (true)
        {
            while (!mState.load(std::memory_order_relaxed))
            {
                std::this_thread::yield();
            }

            if (try_lock())
            {
                break;
            }
        }
    }

    bool try_lock() noexcept { return mState.exchange(false, std::memory_order_acquire); }

    void unlock() noexcept { mState.store(true, std::memory_order_release); }

private:
    std::atomic<bool> mState{true};
};

/**
 * A simple copy on write container that will be used to improve slot lists
 * access efficiency in a multithreaded context.
 */
template <typename T>
class copy_on_write
{
    struct payload
    {
        payload() = default;

        template <typename... Args>
        explicit payload(Args &&...args)
            : value(std::forward<Args>(args)...)
        {
        }

        std::atomic<std::size_t> count{1};
        T value;
    };

public:
    using element_type = T;

    copy_on_write()
        : mData(new payload)
    {
    }

    template <typename U>
    explicit copy_on_write(
        U &&x,
        typename std::enable_if<!std::is_same<typename std::decay<U>::type, copy_on_write>::value>::type * = nullptr)
        : mData(new payload(std::forward<U>(x)))
    {
    }

    copy_on_write(const copy_on_write &x) noexcept
        : mData(x.mData)
    {
        ++mData->count;
    }

    copy_on_write(copy_on_write &&x) noexcept
        : mData(x.mData)
    {
        x.mData = nullptr;
    }

    ~copy_on_write()
    {
        if (mData && (--mData->count == 0))
        {
            delete mData;
        }
    }

    copy_on_write &operator=(const copy_on_write &x) noexcept
    {
        if (&x != this)
        {
            *this = copy_on_write(x);
        }
        return *this;
    }

    copy_on_write &operator=(copy_on_write &&x) noexcept
    {
        auto tmp = std::move(x);
        swap(*this, tmp);
        return *this;
    }

    element_type &write()
    {
        if (!unique())
        {
            *this = copy_on_write(read());
        }
        return mData->value;
    }

    const element_type &read() const noexcept { return mData->value; }

    friend inline void swap(copy_on_write &x, copy_on_write &y) noexcept
    {
        using std::swap;
        swap(x.mData, y.mData);
    }

private:
    bool unique() const noexcept { return mData->count == 1; }

private:
    payload *mData;
};

/**
 * Specializations for thread-safe code path
 */
template <typename T>
const T &cow_read(const T &v)
{
    return v;
}

template <typename T>
const T &cow_read(copy_on_write<T> &v)
{
    return v.read();
}

template <typename T>
T &cow_write(T &v)
{
    return v;
}

template <typename T>
T &cow_write(copy_on_write<T> &v)
{
    return v.write();
}

/**
 * std::make_shared instantiates a lot a templates, and makes both compilation time
 * and executable size far bigger than they need to be. We offer a make_shared
 * equivalent that will avoid most instantiations with the following tradeoffs:
 * - Not exception safe,
 * - Allocates a separate control block, and will thus make the code slower.
 */
#    ifdef SIGSLOT_REDUCE_COMPILE_TIME
template <typename B, typename D, typename... Arg>
inline std::shared_ptr<B> make_shared(Arg &&...arg)
{
    return std::shared_ptr<B>(static_cast<B *>(new D(std::forward<Arg>(arg)...)));
}
#    else
template <typename B, typename D, typename... Arg>
inline std::shared_ptr<B> make_shared(Arg &&...arg)
{
    return std::static_pointer_cast<B>(std::make_shared<D>(std::forward<Arg>(arg)...));
}
#    endif


// Adapt a signal into a cheap function object, for easy signal chaining
template <typename SigT>
struct signal_wrapper
{
    template <typename... U>
    void operator()(U &&...u)
    {
        (*m_sig)(std::forward<U>(u)...);
    }

    SigT *m_sig{};
};


/* slot_state holds slot type independent state, to be used to interact with
 * slots indirectly through connection and scoped_connection objects.
 */
class SlotState
{
public:
    constexpr SlotState(GroupId gid) noexcept
        : mIndex(0)
        , mGroup(gid)
        , mBlocked(false)
        , mConnected(true)
    {
    }

    virtual ~SlotState() = default;

    virtual bool connected() const noexcept { return mConnected; }

    bool disconnect() noexcept
    {
        bool ret = mConnected.exchange(false);
        if (ret)
        {
            do_disconnect();
        }
        return ret;
    }

    bool blocked() const noexcept { return mBlocked.load(); }
    void block() noexcept { mBlocked.store(true); }
    void unblock() noexcept { mBlocked.store(false); }

protected:
    virtual void do_disconnect() { }

    std::size_t index() const { return mIndex; }

    std::size_t &index() { return mIndex; }

    GroupId group() const { return mGroup; }

private:
    template <typename, typename...>
    friend class ::cxxkit::signals::SignalBase;

    template <typename, typename, typename, typename...>
    friend class ::cxxkit::signals::SignalBaseR;

    std::size_t mIndex;   // index into the array of slot pointers inside the signal
    const GroupId mGroup; // slot group this slot belongs to
    std::atomic<bool> mBlocked;
    std::atomic<bool> mConnected;
};

} // namespace detail

/**
 * connection_blocker is a RAII object that blocks a connection until destruction
 */
class ConnectionBlocker
{
public:
    ConnectionBlocker() = default;
    ~ConnectionBlocker() noexcept { release(); }

    ConnectionBlocker(const ConnectionBlocker &) = delete;
    ConnectionBlocker &operator=(const ConnectionBlocker &) = delete;

    ConnectionBlocker(ConnectionBlocker &&o) noexcept
        : mState{std::move(o.mState)}
    {
    }

    ConnectionBlocker &operator=(ConnectionBlocker &&o) noexcept
    {
        release();
        mState.swap(o.mState);
        return *this;
    }

private:
    friend class Connection;
    explicit ConnectionBlocker(std::weak_ptr<detail::SlotState> s) noexcept
        : mState{std::move(s)}
    {
        if (auto d = mState.lock())
        {
            d->block();
        }
    }

    void release() noexcept
    {
        if (auto d = mState.lock())
        {
            d->unblock();
        }
    }

private:
    std::weak_ptr<detail::SlotState> mState;
};


/**
 * A connection object allows interaction with an ongoing slot connection
 *
 * It allows common actions such as connection blocking and disconnection.
 * Note that connection is not a RAII object, one does not need to hold one
 * such object to keep the signal-slot connection alive.
 */
class Connection
{
public:
    Connection() = default;
    virtual ~Connection() = default;

    Connection(const Connection &) noexcept = default;
    Connection &operator=(const Connection &) noexcept = default;
    Connection(Connection &&) noexcept = default;
    Connection &operator=(Connection &&) noexcept = default;

    bool valid() const noexcept { return !mState.expired(); }

    bool connected() const noexcept
    {
        const auto d = mState.lock();
        return d && d->connected();
    }

    bool disconnect() noexcept
    {
        auto d = mState.lock();
        return d && d->disconnect();
    }

    bool blocked() const noexcept
    {
        const auto d = mState.lock();
        return d && d->blocked();
    }

    void block() noexcept
    {
        if (auto d = mState.lock())
        {
            d->block();
        }
    }

    void unblock() noexcept
    {
        if (auto d = mState.lock())
        {
            d->unblock();
        }
    }

    ConnectionBlocker blocker() const noexcept { return ConnectionBlocker{mState}; }

protected:
    template <typename, typename...>
    friend class SignalBase;
    template <typename, typename, typename, typename...>
    friend class SignalBaseR;
    explicit Connection(std::weak_ptr<detail::SlotState> s) noexcept
        : mState{std::move(s)}
    {
    }

protected:
    std::weak_ptr<detail::SlotState> mState;
};

/**
 * scoped_connection is a RAII version of connection
 * It disconnects the slot from the signal upon destruction.
 */
class ScopedConnection final : public Connection
{
public:
    ScopedConnection() = default;
    ~ScopedConnection() override { disconnect(); }

    /*implicit*/ ScopedConnection(const Connection &c) noexcept
        : Connection(c)
    {
    }
    /*implicit*/ ScopedConnection(Connection &&c) noexcept
        : Connection(std::move(c))
    {
    }

    ScopedConnection(const ScopedConnection &) noexcept = delete;
    ScopedConnection &operator=(const ScopedConnection &) noexcept = delete;

    ScopedConnection(ScopedConnection &&o) noexcept
        : Connection{std::move(o.mState)}
    {
    }

    ScopedConnection &operator=(ScopedConnection &&o) noexcept
    {
        disconnect();
        mState.swap(o.mState);
        return *this;
    }


    explicit ScopedConnection(std::weak_ptr<detail::SlotState> s) noexcept
        : Connection{std::move(s)}
    {
    }
};

/**
 * Observer is a base class for intrusive lifetime tracking of objects.
 *
 * This is an alternative to trackable pointers, such as std::shared_ptr,
 * and manual connection management by keeping connection objects in scope.
 * Deriving from this class allows automatic disconnection of all the slots
 * connected to any signal when an instance is destroyed.
 */
template <typename Lockable>
struct ObserverBase : private detail::ObserverType
{
    virtual ~ObserverBase() = default;

protected:
    /**
     * Disconnect all signals connected to this object.
     *
     * To avoid invocation of slots on a semi-destructed instance, which may happen
     * in multi-threaded contexts, derived classes should call this method in their
     * destructor. This will ensure proper disconnection prior to the destruction.
     */
    void disconnect_all()
    {
        std::unique_lock<Lockable> _{m_mutex};
        m_connections.clear();
    }

private:
    template <typename, typename...>
    friend class SignalBase;

    void add_connection(Connection conn)
    {
        std::unique_lock<Lockable> _{m_mutex};
        m_connections.emplace_back(std::move(conn));
    }

    Lockable m_mutex;
    std::vector<ScopedConnection> m_connections;
};

/**
 * Specialization of observer_base to be used in single threaded contexts.
 */
using observer_st = ObserverBase<detail::NullMutex>;

/**
 * Specialization of observer_base to be used in multi-threaded contexts.
 */
using observer = ObserverBase<std::mutex>;


namespace detail
{

// interface for cleanable objects, used to cleanup disconnected slots
struct Cleanable
{
    virtual ~Cleanable() = default;
    virtual void clean(SlotState *) = 0;
};

template <typename...>
class SlotBase;

template <typename... T>
using SlotSharedPtr = std::shared_ptr<SlotBase<T...>>;


/* A base class for slot objects. This base type only depends on slot argument
 * types, it will be used as an element in an intrusive singly-linked list of
 * slots, hence the public next member.
 */
template <typename... Args>
class SlotBase : public SlotState
{
public:
    using ArgTypes = trait::TypeList<Args...>;

    explicit SlotBase(Cleanable &c, GroupId gid)
        : SlotState(gid)
        , mCleaner(&c)
    {
    }
    ~SlotBase() override = default;

    void set_cleaner(Cleanable &c) { mCleaner = &c; }
    // method effectively responsible for calling the "slot" function with
    // supplied arguments whenever emission happens.
    virtual void call_slot(Args...) = 0;

    template <typename... U>
    void operator()(U &&...u)
    {
        if (SlotState::connected() && !SlotState::blocked())
        {
            call_slot(std::forward<U>(u)...);
        }
    }

    // check if we are storing callable c
    template <typename C>
    bool has_callable(const C &c) const
    {
        auto p = get_callable();
        return eq_function_ptr(c, p);
    }

    template <typename C>
    typename std::enable_if<function_traits<C>::must_check_object, bool>::type has_full_callable(const C &c) const
    {
        return has_callable(c) && check_class_type<typename std::decay<C>::type>();
    }

    template <typename C>
    typename std::enable_if<!function_traits<C>::must_check_object, bool>::type has_full_callable(const C &c) const
    {
        return has_callable(c);
    }

    // check if we are storing object o
    template <typename O>
    bool has_object(const O &o) const
    {
        return get_object() == get_object_ptr(o);
    }

protected:
    void do_disconnect() final { mCleaner->clean(this); }

    // retieve a pointer to the object embedded in the slot
    virtual obj_ptr get_object() const noexcept { return nullptr; }

    // retieve a pointer to the callable embedded in the slot
    virtual func_ptr get_callable() const noexcept { return get_function_ptr(nullptr); }

#    if CXXKIT_RTTI_ENABLED
    // retieve a pointer to the callable embedded in the slot
    virtual const std::type_info &get_callable_type() const noexcept { return typeid(nullptr); }

private:
    template <typename U>
    bool check_class_type() const
    {
        return typeid(U) == get_callable_type();
    }

#    else
    template <typename U>
    bool check_class_type() const
    {
        return false;
    }
#    endif

private:
    Cleanable *mCleaner;
};

/*
 * A slot object holds state information, and a callable to to be called
 * whenever the function call operator of its slot_base base class is called.
 */
template <typename Func, typename... Args>
class Slot final : public SlotBase<Args...>
{
public:
    template <typename F, typename Gid>
    constexpr Slot(Cleanable &c, F &&f, Gid gid)
        : SlotBase<Args...>(c, gid)
        , func{std::forward<F>(f)}
    {
    }

protected:
    void call_slot(Args... args) override { func(args...); }

    func_ptr get_callable() const noexcept override { return get_function_ptr(func); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(func); }
#    endif

private:
    typename std::decay<Func>::type func;
};

/*
 * Variation of slot that prepends a connection object to the callable
 */
template <typename Func, typename... Args>
class SlotExtended final : public SlotBase<Args...>
{
public:
    template <typename F>
    constexpr SlotExtended(Cleanable &c, F &&f, GroupId gid)
        : SlotBase<Args...>(c, gid)
        , func{std::forward<F>(f)}
    {
    }

    Connection conn;

protected:
    void call_slot(Args... args) override { func(conn, args...); }

    func_ptr get_callable() const noexcept override { return get_function_ptr(func); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(func); }
#    endif

private:
    typename std::decay<Func>::type func;
};

/*
 * Result-returning slot family for SignalBaseR (Task 8, S2-style combiners).
 *
 * SlotBaseR reuses the whole SlotBase/SlotState state machine (connected/
 * blocked/cleaner/group bookkeeping) but adds call_slot_r, a pure virtual that
 * RETURNS the slot result instead of discarding it. The void call_slot inherited
 * from SlotBase simply forwards to call_slot_r and drops the value, so the
 * existing void-invocation path stays usable if a SignalBaseR slot is ever
 * invoked through the base interface.
 */
template <typename R, typename... Args>
class SlotBaseR : public SlotBase<Args...>
{
public:
    explicit SlotBaseR(Cleanable &c, GroupId gid)
        : SlotBase<Args...>(c, gid)
    {
    }
    ~SlotBaseR() override = default;

    // invoke the slot and RETURN its result (the combiner's value source)
    virtual R call_slot_r(Args... args) = 0;

protected:
    void call_slot(Args... args) override { call_slot_r(args...); }
};

/*
 * A SlotR holds a callable whose return value is convertible to R.
 */
template <typename Func, typename R, typename... Args>
class SlotR final : public SlotBaseR<R, Args...>
{
public:
    template <typename F, typename Gid>
    constexpr SlotR(Cleanable &c, F &&f, Gid gid)
        : SlotBaseR<R, Args...>(c, gid)
        , func{std::forward<F>(f)}
    {
    }

protected:
    R call_slot_r(Args... args) override { return func(args...); }

    func_ptr get_callable() const noexcept override { return get_function_ptr(func); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(func); }
#    endif

private:
    typename std::decay<Func>::type func;
};

/*
 * Variation of SlotR that prepends a connection object to the callable.
 */
template <typename Func, typename R, typename... Args>
class SlotRExtended final : public SlotBaseR<R, Args...>
{
public:
    template <typename F>
    constexpr SlotRExtended(Cleanable &c, F &&f, GroupId gid)
        : SlotBaseR<R, Args...>(c, gid)
        , func{std::forward<F>(f)}
    {
    }

    Connection conn;

protected:
    R call_slot_r(Args... args) override { return func(conn, args...); }

    func_ptr get_callable() const noexcept override { return get_function_ptr(func); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(func); }
#    endif

private:
    typename std::decay<Func>::type func;
};


/*
 * A slot object holds state information, an object and a pointer over member
 * function to be called whenever the function call operator of its slot_base
 * base class is called.
 */
template <typename Pmf, typename Ptr, typename... Args>
class slot_pmf final : public SlotBase<Args...>
{
public:
    template <typename F, typename P>
    constexpr slot_pmf(Cleanable &c, F &&f, P &&p, GroupId gid)
        : SlotBase<Args...>(c, gid)
        , pmf{std::forward<F>(f)}
        , ptr{std::forward<P>(p)}
    {
    }

protected:
    void call_slot(Args... args) override { ((*ptr).*pmf)(args...); }

    func_ptr get_callable() const noexcept override { return get_function_ptr(pmf); }

    obj_ptr get_object() const noexcept override { return get_object_ptr(ptr); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(pmf); }
#    endif

private:
    typename std::decay<Pmf>::type pmf;
    typename std::decay<Ptr>::type ptr;
};

/*
 * Variation of slot that prepends a connection object to the callable
 */
template <typename Pmf, typename Ptr, typename... Args>
class slot_pmf_extended final : public SlotBase<Args...>
{
public:
    template <typename F, typename P>
    constexpr slot_pmf_extended(Cleanable &c, F &&f, P &&p, GroupId gid)
        : SlotBase<Args...>(c, gid)
        , pmf{std::forward<F>(f)}
        , ptr{std::forward<P>(p)}
    {
    }

    Connection conn;

protected:
    void call_slot(Args... args) override { ((*ptr).*pmf)(conn, args...); }

    func_ptr get_callable() const noexcept override { return get_function_ptr(pmf); }
    obj_ptr get_object() const noexcept override { return get_object_ptr(ptr); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(pmf); }
#    endif

private:
    typename std::decay<Pmf>::type pmf;
    typename std::decay<Ptr>::type ptr;
};

/*
 * An implementation of a slot that tracks the life of a supplied object
 * through a weak pointer in order to automatically disconnect the slot
 * on said object destruction.
 */
template <typename Func, typename WeakPtr, typename... Args>
class slot_tracked final : public SlotBase<Args...>
{
public:
    template <typename F, typename P>
    constexpr slot_tracked(Cleanable &c, F &&f, P &&p, GroupId gid)
        : SlotBase<Args...>(c, gid)
        , func{std::forward<F>(f)}
        , ptr{std::forward<P>(p)}
    {
    }

    bool connected() const noexcept override { return !ptr.expired() && SlotState::connected(); }

protected:
    void call_slot(Args... args) override
    {
        auto sp = ptr.lock();
        if (!sp)
        {
            SlotState::disconnect();
            return;
        }
        if (SlotState::connected())
        {
            func(args...);
        }
    }

    func_ptr get_callable() const noexcept override { return get_function_ptr(func); }

    obj_ptr get_object() const noexcept override { return get_object_ptr(ptr); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(func); }
#    endif

private:
    typename std::decay<Func>::type func;
    typename std::decay<WeakPtr>::type ptr;
};

// Same as above with extended signature
template <typename Func, typename WeakPtr, typename... Args>
class slot_tracked_extended final : public SlotBase<Args...>
{
public:
    template <typename F, typename P>
    constexpr slot_tracked_extended(Cleanable &c, F &&f, P &&p, GroupId gid)
        : SlotBase<Args...>(c, gid)
        , func{std::forward<F>(f)}
        , ptr{std::forward<P>(p)}
    {
    }

    Connection conn;

    bool connected() const noexcept override { return !ptr.expired() && SlotState::connected(); }

protected:
    void call_slot(Args... args) override
    {
        auto sp = ptr.lock();
        if (!sp)
        {
            SlotState::disconnect();
            return;
        }
        if (SlotState::connected())
        {
            func(conn, args...);
        }
    }

    func_ptr get_callable() const noexcept override { return get_function_ptr(func); }

    obj_ptr get_object() const noexcept override { return get_object_ptr(ptr); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(func); }
#    endif

private:
    typename std::decay<Func>::type func;
    typename std::decay<WeakPtr>::type ptr;
};

/*
 * An implementation of a slot as a pointer over member function, that tracks
 * the life of a supplied object through a weak pointer in order to automatically
 * disconnect the slot on said object destruction.
 */
template <typename Pmf, typename WeakPtr, typename... Args>
class slot_pmf_tracked final : public SlotBase<Args...>
{
public:
    template <typename F, typename P>
    constexpr slot_pmf_tracked(Cleanable &c, F &&f, P &&p, GroupId gid)
        : SlotBase<Args...>(c, gid)
        , pmf{std::forward<F>(f)}
        , ptr{std::forward<P>(p)}
    {
    }

    bool connected() const noexcept override { return !ptr.expired() && SlotState::connected(); }

protected:
    void call_slot(Args... args) override
    {
        auto sp = ptr.lock();
        if (!sp)
        {
            SlotState::disconnect();
            return;
        }
        if (SlotState::connected())
        {
            ((*sp).*pmf)(args...);
        }
    }

    func_ptr get_callable() const noexcept override { return get_function_ptr(pmf); }

    obj_ptr get_object() const noexcept override { return get_object_ptr(ptr); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(pmf); }
#    endif

private:
    typename std::decay<Pmf>::type pmf;
    typename std::decay<WeakPtr>::type ptr;
};

// same as above with extended signature
template <typename Pmf, typename WeakPtr, typename... Args>
class slot_pmf_tracked_extended final : public SlotBase<Args...>
{
public:
    template <typename F, typename P>
    constexpr slot_pmf_tracked_extended(Cleanable &c, F &&f, P &&p, GroupId gid)
        : SlotBase<Args...>(c, gid)
        , pmf{std::forward<F>(f)}
        , ptr{std::forward<P>(p)}
    {
    }

    Connection conn;

    bool connected() const noexcept override { return !ptr.expired() && SlotState::connected(); }

protected:
    void call_slot(Args... args) override
    {
        auto sp = ptr.lock();
        if (!sp)
        {
            SlotState::disconnect();
            return;
        }
        if (SlotState::connected())
        {
            ((*sp).*pmf)(conn, args...);
        }
    }

    func_ptr get_callable() const noexcept override { return get_function_ptr(pmf); }

    obj_ptr get_object() const noexcept override { return get_object_ptr(ptr); }

#    if CXXKIT_RTTI_ENABLED
    const std::type_info &get_callable_type() const noexcept override { return typeid(pmf); }
#    endif

private:
    typename std::decay<Pmf>::type pmf;
    typename std::decay<WeakPtr>::type ptr;
};

/*
 * Combiners for SignalBaseR (S2 semantics, C++11, snake_case per D26).
 *
 * A Combiner is a callable invoked as comb(first, last) where first/last is a
 * [first, last) range of slot_call_iterators. Dereferencing an iterator
 * lazily invokes the corresponding live slot and returns its R result;
 * incrementing skips disconnected/blocked slots. Combiners may stop early
 * (short-circuit) simply by not reaching last.
 */


/*
 * slot_call_iterator caches the result of the slot it currently points to.
 * Repeated dereference of the same iterator must not re-invoke the slot. All
 * copies share the result cache; advancing any copy invalidates it for all
 * (a stale copy dereferenced after a sibling advanced re-invokes the slot).
 * Single-pass combiner loops never fork iterators, so this is safe.
 */
template <typename R>
struct slot_result_cache
{
    const void *slot_id = nullptr; // identity of the slot the value belongs to
    Optional<R> value;             // engaged only when slot_id is set
};


/*
 * slot_call_iterator: a forward iterator over a SNAPSHOT of result-returning
 * slot pointers. Dereferencing lazily invokes the pointed-to slot (skipping
 * disconnected/blocked ones) and buffers the result; incrementing advances.
 * The S2 boost.signals2 slot_call_iterator adapted to C++11: no auto return
 * types, hand-written typedefs, position-based equality. The value is
 * returned BY VALUE (reference = R) because each deref may compute a fresh
 * result for a newly skipped-to slot.
 *
 * @tparam R the slot result type
 * @tparam Iter the snapshot vector's const_iterator
 * @tparam Invoker std::function<R(const slot_r_ptr&)>-compatible invoker
 */
template <typename R, typename Iter, typename Invoker>
class slot_call_iterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = R;
    using difference_type = std::ptrdiff_t;
    using pointer = R *;
    using reference = R;

    slot_call_iterator(Iter it, Iter last, const Invoker &invoker, std::shared_ptr<slot_result_cache<R>> cache)
        : mIter(it)
        , mLast(last)
        , mInvoker(invoker)
        , mCache(std::move(cache))
    {
        normalize();
    }

    R operator*() const
    {
        CXXKIT_CHECK(mIter != mLast) << "slot_call_iterator: dereferencing the end iterator";
        if (!cached())
        {
            mCache->slot_id = static_cast<const void *>(mIter->get());
            mCache->value = mInvoker(*mIter);
        }
        return mCache->value.value();
    }

    slot_call_iterator &operator++()
    {
        CXXKIT_CHECK(mIter != mLast) << "slot_call_iterator: incrementing past the end iterator";
        ++mIter;
        mCache->slot_id = nullptr;
        mCache->value.reset();
        normalize();
        return *this;
    }

    slot_call_iterator operator++(int)
    {
        slot_call_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    bool operator==(const slot_call_iterator &o) const { return mIter == o.mIter; }
    bool operator!=(const slot_call_iterator &o) const { return mIter != o.mIter; }

private:
    // skip dead slots so *this always points at an invocable slot (or end)
    void normalize()
    {
        while (mIter != mLast && !(mIter->get()->connected() && !mIter->get()->blocked()))
        {
            ++mIter;
        }
    }

    // true when the shared cache already holds this slot's result
    bool cached() const
    {
        return mCache->slot_id == static_cast<const void *>(mIter->get()) && mCache->value.has_value();
    }

    Iter mIter;
    Iter mLast;
    Invoker mInvoker;
    std::shared_ptr<slot_result_cache<R>> mCache;
};

} // namespace detail

/**
 * S2 optional_last_value: returns an Optional<R> holding the result of the
 * LAST invoked slot; empty when no slot is invoked (empty range or all
 * slots dead/blocked). The signature R(InputIterator, InputIterator) matches
 * the S2 combiner convention so user combiners compose interchangeably.
 */
template <typename R>
struct optional_last_value
{
    using result_type = Optional<R>;

    template <typename InputIterator>
    result_type operator()(InputIterator first, InputIterator last) const
    {
        Optional<R> value;
        while (first != last)
        {
            value = *first;
            ++first;
        }
        return value;
    }
};

/**
 * S2 maximum: returns the largest result (operator<), or an empty Optional
 * when no slot is invoked.
 */
template <typename R>
struct maximum
{
    using result_type = Optional<R>;

    template <typename InputIterator>
    result_type operator()(InputIterator first, InputIterator last) const
    {
        Optional<R> max_value;
        while (first != last)
        {
            const R value = *first;
            if (!max_value || max_value < value)
            {
                max_value = value;
            }
            ++first;
        }
        return max_value;
    }
};

/**
 * signal_base is an implementation of the observer pattern, through the use
 * of an emitting object and slots that are connected to the signal and called
 * with supplied arguments when a signal is emitted.
 *
 * signal_base is the general implementation, whose locking policy must be
 * set in order to decide thread safety guarantees. signal and signal_st
 * are partial specializations for multi-threaded and single-threaded use.
 *
 * It does not allow slots to return a value.
 *
 * Slot execution order can be constrained by assigning group ids to the slots.
 * The execution order of slots in a same group is unspecified and should not be
 * relied upon, however groups are executed in ascending group ids order. When
 * the group id of a slot is not set, it is assigned to the group 0. Group ids
 * can have any value in the range of signed 32 bit integers.
 *
 * @tparam Lockable a lock type to decide the lock policy
 * @tparam T... the argument types of the emitting and slots functions.
 */
template <typename Lockable, typename... T>
class SignalBase final : public detail::Cleanable
{
    template <typename L>
    using is_thread_safe = std::integral_constant<bool, !std::is_same<L, detail::NullMutex>::value>;

    template <typename U, typename L>
    using cow_type = typename std::conditional<is_thread_safe<L>::value, detail::copy_on_write<U>, U>::type;

    template <typename U, typename L>
    using cow_copy_type = typename std::conditional<is_thread_safe<L>::value, detail::copy_on_write<U>, U>::type;

    using lock_type = std::unique_lock<Lockable>;
    using slot_base = detail::SlotBase<T...>;
    using slot_ptr = detail::SlotSharedPtr<T...>;
    using slots_type = std::vector<slot_ptr>;
    struct group_type
    {
        slots_type slts;
        GroupId gid;
    };
    using list_type = std::vector<group_type>; // kept ordered by ascending gid

public:
    using arg_list = trait::TypeList<T...>;
    using ext_arg_list = trait::TypeList<Connection &, T...>;

    SignalBase() noexcept
        : m_block(false)
    {
    }
    ~SignalBase() override { disconnect_all(); }

    SignalBase(const SignalBase &) = delete;
    SignalBase &operator=(const SignalBase &) = delete;

    SignalBase(SignalBase &&o) /* not noexcept */
        : m_block{o.m_block.load()}
    {
        lock_type lock(o.m_mutex);
        using std::swap;
        swap(m_slots, o.m_slots);
        reroute_slot_cleaners();
    }

    SignalBase &operator=(SignalBase &&o) /* not noexcept */
    {
        lock_type lock1(m_mutex, std::defer_lock);
        lock_type lock2(o.m_mutex, std::defer_lock);
        std::lock(lock1, lock2);

        using std::swap;
        swap(m_slots, o.m_slots);
        m_block.store(o.m_block.exchange(m_block.load()));
        reroute_slot_cleaners();
        return *this;
    }

    /**
     * Emit a signal
     *
     * Effect: All non blocked and connected slot functions will be called
     *         with supplied arguments.
     * Safety: With proper locking (see pal::signal), emission can happen from
     *         multiple threads simultaneously. The guarantees only apply to the
     *         signal object, it does not cover thread safety of potentially
     *         shared state used in slot functions.
     *
     * @param a arguments to emit
     */
    template <typename... U>
    void operator()(U &&...a) const
    {
        if (m_block)
        {
            return;
        }

        // Reference to the slots to execute them out of the lock
        // a copy may occur if another thread writes to it.
        cow_copy_type<list_type, Lockable> ref = slots_reference();

        for (const auto &group : detail::cow_read(ref))
        {
            for (const auto &s : group.slts)
            {
                s->operator()(a...);
            }
        }
    }

    /**
     * Connect a callable of compatible arguments.
     *
     * Effect: Creates and stores a new slot responsible for executing the
     *         supplied callable for every subsequent signal emission.
     * Safety: Thread-safety depends on locking policy.
     *
     * @param c a callable
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    typename std::enable_if<trait::detail::is_callable<Callable, arg_list>::value, Connection>::type connect(
        Callable &&c,
        GroupId gid = 0)
    {
        using slot_t = detail::Slot<Callable, T...>;
        auto s = make_slot<slot_t>(std::forward<Callable>(c), gid);
        Connection conn(s);
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Connect a callable with an additional connection argument.
     *
     * The callable's first argument must be of type connection. The callable
     * can manage its own connection through this argument.
     *
     * @param c a callable
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    typename std::enable_if<trait::detail::is_callable<Callable, ext_arg_list>::value, Connection>::type
    connect_extended(Callable &&c, GroupId gid = 0)
    {
        using slot_t = detail::SlotExtended<Callable, T...>;
        auto s = make_slot<slot_t>(std::forward<Callable>(c), gid);
        Connection conn(s);
        std::static_pointer_cast<slot_t>(s)->conn = conn;
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Overload of connect for pointers over member functions derived from
     * observer.
     *
     * @param pmf a pointer over member function
     * @param ptr an object pointer derived from observer
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr>
    typename std::enable_if<trait::detail::is_callable<Pmf, Ptr, arg_list>::value && trait::is_observer<Ptr>::value,
                            Connection>::type
    connect(Pmf &&pmf, Ptr &&ptr, GroupId gid = 0)
    {
        using slot_t = detail::slot_pmf<Pmf, Ptr, T...>;
        auto s = make_slot<slot_t>(std::forward<Pmf>(pmf), std::forward<Ptr>(ptr), gid);
        Connection conn(s);
        add_slot(std::move(s));
        ptr->add_connection(conn);
        return conn;
    }

    /**
     * Overload of connect for pointers over member functions.
     *
     * @param pmf a pointer over member function
     * @param ptr an object pointer
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr>
    typename std::enable_if<trait::detail::is_callable<Pmf, Ptr, arg_list>::value && !trait::is_observer<Ptr>::value &&
                                !trait::is_weak_ptr_compatible<Ptr>::value,
                            Connection>::type
    connect(Pmf &&pmf, Ptr &&ptr, GroupId gid = 0)
    {
        using slot_t = detail::slot_pmf<Pmf, Ptr, T...>;
        auto s = make_slot<slot_t>(std::forward<Pmf>(pmf), std::forward<Ptr>(ptr), gid);
        Connection conn(s);
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Overload of connect for pointer over member functions and additional
     * connection argument.
     *
     * The callable's first argument must be of type connection. The callable
     * can manage its own connection through this argument.
     *
     * @param pmf a pointer over member function
     * @param ptr an object pointer
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr>
    typename std::enable_if<trait::detail::is_callable<Pmf, Ptr, ext_arg_list>::value &&
                                !trait::is_weak_ptr_compatible<Ptr>::value,
                            Connection>::type
    connect_extended(Pmf &&pmf, Ptr &&ptr, GroupId gid = 0)
    {
        using slot_t = detail::slot_pmf_extended<Pmf, Ptr, T...>;
        auto s = make_slot<slot_t>(std::forward<Pmf>(pmf), std::forward<Ptr>(ptr), gid);
        Connection conn(s);
        std::static_pointer_cast<slot_t>(s)->conn = conn;
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Overload of connect for lifetime object tracking and automatic disconnection.
     *
     * Ptr must be convertible to an object following a loose form of weak pointer
     * concept, by implementing the ADL-detected conversion function to_weak().
     *
     * This overload covers the case of a pointer over member function and a
     * trackable pointer of that class.
     *
     * Note: only weak references are stored, a slot does not extend the lifetime
     * of a supplied object.
     *
     * @param pmf a pointer over member function
     * @param ptr a trackable object pointer
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr>
    typename std::enable_if<!trait::detail::is_callable<Pmf, arg_list>::value &&
                                trait::is_weak_ptr_compatible<Ptr>::value,
                            Connection>::type
    connect(Pmf &&pmf, Ptr &&ptr, GroupId gid = 0)
    {
        auto w = utils::to_weak_ptr(std::forward<Ptr>(ptr));
        using slot_t = detail::slot_pmf_tracked<Pmf, decltype(w), T...>;
        auto s = make_slot<slot_t>(std::forward<Pmf>(pmf), w, gid);
        Connection conn(s);
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Overload of connect for lifetime object tracking and automatic disconnection
     * with additional connection management.
     *
     * The callable's first argument must be of type connection. The callable
     * can manage its own connection through this argument.
     *
     * Ptr must be convertible to an object following a loose form of weak pointer
     * concept, by implementing the ADL-detected conversion function to_weak().
     *
     * This overload covers the case of a pointer over member function and a
     * trackable pointer of that class.
     *
     * Note: only weak references are stored, a slot does not extend the lifetime
     * of a supplied object.
     *
     * @param pmf a pointer over member function
     * @param ptr a trackable object pointer
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Pmf, typename Ptr>
    typename std::enable_if<!trait::detail::is_callable<Pmf, ext_arg_list>::value &&
                                trait::is_weak_ptr_compatible<Ptr>::value,
                            Connection>::type
    connect_extended(Pmf &&pmf, Ptr &&ptr, GroupId gid = 0)
    {
        auto w = utils::to_weak_ptr(std::forward<Ptr>(ptr));
        using slot_t = detail::slot_pmf_tracked_extended<Pmf, decltype(w), T...>;
        auto s = make_slot<slot_t>(std::forward<Pmf>(pmf), w, gid);
        Connection conn(s);
        std::static_pointer_cast<slot_t>(s)->conn = conn;
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Overload of connect for lifetime object tracking and automatic disconnection.
     *
     * Trackable must be convertible to an object following a loose form of weak
     * pointer concept, by implementing the ADL-detected conversion function to_weak().
     *
     * This overload covers the case of a standalone callable and unrelated trackable
     * object.
     *
     * Note: only weak references are stored, a slot does not extend the lifetime
     * of a supplied object.
     *
     * @param c a callable
     * @param ptr a trackable object pointer
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable, typename Trackable>
    typename std::enable_if<trait::detail::is_callable<Callable, arg_list>::value &&
                                trait::is_weak_ptr_compatible<Trackable>::value,
                            Connection>::type
    connect(Callable &&c, Trackable &&ptr, GroupId gid = 0)
    {
        auto w = utils::to_weak_ptr(std::forward<Trackable>(ptr));
        using slot_t = detail::slot_tracked<Callable, decltype(w), T...>;
        auto s = make_slot<slot_t>(std::forward<Callable>(c), w, gid);
        Connection conn(s);
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Overload of connect for lifetime object tracking and automatic disconnection
     * with additional connection management.
     *
     * The callable's first argument must be of type connection. The callable
     * can manage its own connection through this argument.
     *
     * Trackable must be convertible to an object following a loose form of weak
     * pointer concept, by implementing the ADL-detected conversion function to_weak().
     *
     * This overload covers the case of a standalone callable and unrelated trackable
     * object.
     *
     * Note: only weak references are stored, a slot does not extend the lifetime
     * of a suppied object.
     *
     * @param c a callable
     * @param ptr a trackable object pointer
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable, typename Trackable>
    typename std::enable_if<trait::detail::is_callable<Callable, ext_arg_list>::value &&
                                trait::is_weak_ptr_compatible<Trackable>::value,
                            Connection>::type
    connect_extended(Callable &&c, Trackable &&ptr, GroupId gid = 0)
    {
        auto w = utils::to_weak_ptr(std::forward<Trackable>(ptr));
        using slot_t = detail::slot_tracked_extended<Callable, decltype(w), T...>;
        auto s = make_slot<slot_t>(std::forward<Callable>(c), w, gid);
        Connection conn(s);
        std::static_pointer_cast<slot_t>(s)->conn = conn;
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Creates a connection whose duration is tied to the return object.
     * Uses the same semantics as connect
     */
    template <typename... CallArgs>
    ScopedConnection connect_scoped(CallArgs &&...args)
    {
        return connect(std::forward<CallArgs>(args)...);
    }

    /**
     * Connect a callable to be fired exactly once.
     *
     * The slot disconnects itself BEFORE the callable runs, so a re-entrant
     * emission from inside the callable cannot fire it a second time.
     *
     * @param c a callable
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    Connection connect_once(Callable &&c, GroupId gid = 0)
    {
        Callable wrap(std::forward<Callable>(c));
        return connect_extended(
            [wrap](Connection &self, T... args) mutable
            {
                self.disconnect();
                wrap(std::forward<T>(args)...);
            },
            gid);
    }

    /**
     * Disconnect slots bound to a callable
     *
     * Effect: Disconnects all the slots bound to the callable in argument.
     * Safety: Thread-safety depends on locking policy.
     *
     * If the callable is a free or static member function, this overload is always
     * available. However, RTTI is needed for it to work for pointer to member
     * functions, function objects or and (references to) lambdas, because the
     * C++ spec does not mandate the pointers to member functions to be unique.
     *
     * @param c a callable
     * @return the number of disconnected slots
     */
    template <typename Callable>
    typename std::enable_if<(trait::detail::is_callable<Callable, arg_list>::value ||
                             trait::detail::is_callable<Callable, ext_arg_list>::value ||
                             trait::is_member_function_pointer<Callable>::value) &&
                                detail::function_traits<Callable>::is_disconnectable,
                            size_t>::type
    disconnect(const Callable &c)
    {
        return disconnect_if([&](const slot_ptr &s) { return s->has_full_callable(c); });
    }

    /**
     * Disconnect slots bound to this object
     *
     * Effect: Disconnects all the slots bound to the object or tracked object
     *         in argument.
     * Safety: Thread-safety depends on locking policy.
     *
     * The object may be a pointer or trackable object.
     *
     * @param obj an object
     * @return the number of disconnected slots
     */
    template <typename Obj>
    typename std::enable_if<!trait::detail::is_callable<Obj, arg_list>::value &&
                                !trait::detail::is_callable<Obj, ext_arg_list>::value &&
                                !trait::is_member_function_pointer<Obj>::value,
                            size_t>::type
    disconnect(const Obj &obj)
    {
        return disconnect_if([&](const slot_ptr &s) { return s->has_object(obj); });
    }

    /**
     * Disconnect slots bound both to a callable and object
     *
     * Effect: Disconnects all the slots bound to the callable and object in argument.
     * Safety: Thread-safety depends on locking policy.
     *
     * For naked pointers, the Callable is expected to be a pointer over member
     * function. If obj is trackable, any kind of Callable can be used.
     *
     * @param c a callable
     * @param obj an object
     * @return the number of disconnected slots
     */
    template <typename Callable, typename Obj>
    size_t disconnect(const Callable &c, const Obj &obj)
    {
        return disconnect_if([&](const slot_ptr &s) { return s->has_object(obj) && s->has_callable(c); });
    }

    /**
     * Disconnect slots in a particular group
     *
     * Effect: Disconnects all the slots in the group id in argument.
     * Safety: Thread-safety depends on locking policy.
     *
     * @param gid a group id
     * @return the number of disconnected slots
     */
    size_t disconnect(GroupId gid)
    {
        lock_type lock(m_mutex);
        for (auto &group : detail::cow_write(m_slots))
        {
            if (group.gid == gid)
            {
                size_t count = group.slts.size();
                group.slts.clear();
                return count;
            }
        }
        return 0;
    }

    /**
     * Disconnects all the slots
     * Safety: Thread safety depends on locking policy
     */
    void disconnect_all()
    {
        lock_type lock(m_mutex);
        clear();
    }

    /**
     * Blocks signal emission.
     *
     * The @c m_block member is a plain @c std::atomic<bool> with
     * @c memory_order_seq_cst default ordering, so @c block() / @c unblock()
     * / @c blocked() participate in a single total order.
     *
     * If @c block() is called while an emission is already in progress,
     * the current emission completes against the snapshot it already
     * captured; only subsequent @c operator() calls observe the block.
     */
    void block() noexcept { m_block.store(true); }

    /**
     * Unblocks signal emission.
     *
     * Releases the block established by @c block().  Uses
     * @c memory_order_seq_cst (the @c std::atomic default).
     */
    void unblock() noexcept { m_block.store(false); }

    /**
     * Returns @c true when emission is blocked.
     *
     * Reads with @c memory_order_seq_cst (the @c std::atomic default),
     * which is in the same total order as @c block() and @c unblock().
     */
    bool blocked() const noexcept { return m_block.load(); }

    /**
     * get number of connected slots
     * Safety: thread safe
     *
     * Deprecated alias: see @c num_slots() (the const superset).
     */
    size_t slot_count() noexcept { return num_slots(); }

    /**
     * Returns the number of connected (non disconnected) slots.
     *
     * Complexity is linear in the number of groups because disconnected slots
     * are removed lazily (they can be resurrected by @c connected() racing with
     * emission), so a dead-entry scan is required.
     *
     * Safety: thread safe
     */
    size_t num_slots() const
    {
        cow_copy_type<list_type, Lockable> ref = slots_reference();
        size_t count = 0;
        for (const auto &g : detail::cow_read(ref))
        {
            count += g.slts.size();
        }
        return count;
    }

    /**
     * Returns @c true when no slots are connected.
     *
     * Safety: thread safe
     */
    bool empty() const { return num_slots() == 0; }


protected:
    /**
     * remove disconnected slots
     */
    void clean(detail::SlotState *state) override
    {
        lock_type lock(m_mutex);
        const auto idx = state->index();
        const auto gid = state->group();

        // find the group
        for (auto &group : detail::cow_write(m_slots))
        {
            if (group.gid == gid)
            {
                auto &slts = group.slts;

                // ensure we have the right slot, in case of concurrent cleaning
                if (idx < slts.size() && slts[idx] && slts[idx].get() == state)
                {
                    std::swap(slts[idx], slts.back());
                    slts[idx]->index() = idx;
                    slts.pop_back();
                }

                return;
            }
        }
    }

private:
    // used to get a reference to the slots for reading
    inline cow_copy_type<list_type, Lockable> slots_reference() const
    {
        lock_type lock(m_mutex);
        return m_slots;
    }

    // create a new slot
    template <typename Slot, typename... A>
    inline slot_ptr make_slot(A &&...a)
    {
        return detail::make_shared<slot_base, Slot>(*this, std::forward<A>(a)...);
    }

    // add the slot to the list of slots of the right group
    void add_slot(slot_ptr &&s)
    {
        const GroupId gid = s->group();

        lock_type lock(m_mutex);
        auto &groups = detail::cow_write(m_slots);

        // find the group
        auto it = groups.begin();
        while (it != groups.end() && it->gid < gid)
        {
            it++;
        }

        // create a new group if necessary
        if (it == groups.end() || it->gid != gid)
        {
            it = groups.insert(it, {{}, gid});
        }

        // add the slot
        s->index() = it->slts.size();
        it->slts.push_back(std::move(s));
    }

    // disconnect a slot if a condition occurs
    template <typename Cond>
    size_t disconnect_if(Cond &&cond)
    {
        lock_type lock(m_mutex);
        auto &groups = detail::cow_write(m_slots);

        size_t count = 0;

        for (auto &group : groups)
        {
            auto &slts = group.slts;
            size_t i = 0;
            while (i < slts.size())
            {
                if (cond(slts[i]))
                {
                    std::swap(slts[i], slts.back());
                    slts[i]->index() = i;
                    slts.pop_back();
                    ++count;
                }
                else
                {
                    ++i;
                }
            }
        }

        return count;
    }

    // to be called under lock: remove all the slots
    void clear() { detail::cow_write(m_slots).clear(); }

private:
    // re-point every slot's cleaner to this; must be called under lock
    void reroute_slot_cleaners()
    {
        auto &groups = detail::cow_write(m_slots);
        for (auto &group : groups)
        {
            for (auto &s : group.slts)
            {
                s->set_cleaner(*this);
            }
        }
    }

    mutable Lockable m_mutex;
    cow_type<list_type, Lockable> m_slots;
    std::atomic<bool> m_block;
};


/**
 * SignalBaseR is the result-returning sibling of SignalBase (S2-style, Task 8).
 *
 * It runs the same group-ordered slot machinery (via the detail::SlotBaseR
 * family reusing SlotState/SlotBase) but emission feeds a COMBINER with a lazy
 * [first, last) range of slot_call_iterators whose dereference invokes one slot
 * and returns its result. This enables return-value aggregation (last value,
 * maximum) and short-circuit evaluation (stop before reaching last).
 *
 * Differences vs SignalBase, by design (oracle F8 - SignalBase stays untouched):
 * - slots return a value convertible to R; void slots are rejected (R != void)
 * - connect()/connect_extended() accept callables only (no pmf/tracked
 *   overloads - YAGNI, connect_once precedent); connect_extended prepends
 *   the Connection& as usual
 * - the combiner is default-constructed per emission (set_combiner deferred)
 * - a blocked signal still runs the combiner over an EMPTY range (S2 semantics),
 *   e.g. optional_last_value yields an empty Optional
 * - this class is intentionally not movable (copy deleted, no move declared);
 *   move support for the R-family is deferred, while the void SignalBase
 *   family supports moves with cleaner re-routing
 *
 * @tparam R the slot result type (must not be void)
 * @tparam Lockable a lock type to decide the lock policy
 * @tparam Combiner a default-constructible callable R(It, It) over slot results
 * @tparam T... the argument types of the emitting and slots functions
 */
template <typename R, typename Lockable, typename Combiner, typename... T>
class SignalBaseR final : public detail::Cleanable
{
    static_assert(!std::is_void<R>::value, "SignalBaseR: R must not be void - use SignalBase for void slots");

    template <typename L>
    using is_thread_safe = std::integral_constant<bool, !std::is_same<L, detail::NullMutex>::value>;

    template <typename U, typename L>
    using cow_type = typename std::conditional<is_thread_safe<L>::value, detail::copy_on_write<U>, U>::type;

    template <typename U, typename L>
    using cow_copy_type = typename std::conditional<is_thread_safe<L>::value, detail::copy_on_write<U>, U>::type;

    using lock_type = std::unique_lock<Lockable>;
    using slot_r_base = detail::SlotBaseR<R, T...>;
    using slot_r_ptr = std::shared_ptr<slot_r_base>;
    using slots_type = std::vector<slot_r_ptr>;
    struct group_type
    {
        slots_type slts;
        GroupId gid;
    };
    using list_type = std::vector<group_type>; // kept ordered by ascending gid

public:
    using arg_list = trait::TypeList<T...>;
    using ext_arg_list = trait::TypeList<Connection &, T...>;
    using result_type = typename Combiner::result_type;

    SignalBaseR() noexcept
        : m_block(false)
    {
    }
    ~SignalBaseR() override { disconnect_all(); }

    SignalBaseR(const SignalBaseR &) = delete;
    SignalBaseR &operator=(const SignalBaseR &) = delete;

    /**
     * Emit a signal and collect slot results through the combiner.
     *
     * Effect: all non blocked and connected slots are invoked lazily as the
     *         combiner dereferences the slot iterator range; the combiner's
     *         return value is returned. Slots run OUTSIDE the signal lock, on a
     *         snapshot of the slot list (same MT contract as SignalBase).
     * Safety: with proper locking (see SignalR), emission can happen from
     *         multiple threads simultaneously.
     *
     * @param a arguments to emit
     * @return the combiner's result over the invoked slots
     */
    template <typename... U>
    result_type operator()(U &&...a) const
    {
        if (m_block)
        {
            // S2 semantics: a blocked signal still runs the combiner over an
            // empty range (e.g. optional_last_value returns an empty Optional).
            const slots_type empty_slots;
            return combiner_range(empty_slots, a...);
        }

        cow_copy_type<list_type, Lockable> ref = slots_reference();

        // flatten the group-ordered snapshot into one invocation order
        slots_type slot_ptrs;
        for (const auto &group : detail::cow_read(ref))
        {
            slot_ptrs.insert(slot_ptrs.end(), group.slts.begin(), group.slts.end());
        }
        return combiner_range(slot_ptrs, a...);
    }

    /**
     * Connect a callable whose return value is convertible to R.
     *
     * @param c a callable
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    typename std::enable_if<trait::detail::is_callable<Callable, arg_list>::value, Connection>::type connect(
        Callable &&c,
        GroupId gid = 0)
    {
        using slot_t = detail::SlotR<Callable, R, T...>;
        auto s = std::static_pointer_cast<slot_r_base>(
            detail::make_shared<slot_r_base, slot_t>(*this, std::forward<Callable>(c), gid));
        Connection conn(s);
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Connect a callable with an additional connection argument.
     *
     * The callable's first argument must be of type connection and return a
     * value convertible to R.
     *
     * @param c a callable
     * @param gid an identifier that can be used to order slot execution
     * @return a connection object that can be used to interact with the slot
     */
    template <typename Callable>
    typename std::enable_if<trait::detail::is_callable<Callable, ext_arg_list>::value, Connection>::type
    connect_extended(Callable &&c, GroupId gid = 0)
    {
        using slot_t = detail::SlotRExtended<Callable, R, T...>;
        auto s = std::static_pointer_cast<slot_r_base>(
            detail::make_shared<slot_r_base, slot_t>(*this, std::forward<Callable>(c), gid));
        Connection conn(s);
        std::static_pointer_cast<slot_t>(s)->conn = conn;
        add_slot(std::move(s));
        return conn;
    }

    /**
     * Disconnect slots bound to a callable (free functions only - function
     * objects/lambdas need RTTI, same limitation as SignalBase).
     *
     * @param c a callable
     * @return the number of disconnected slots
     */
    template <typename Callable>
    typename std::enable_if<(trait::detail::is_callable<Callable, arg_list>::value ||
                             trait::detail::is_callable<Callable, ext_arg_list>::value ||
                             trait::is_member_function_pointer<Callable>::value) &&
                                detail::function_traits<Callable>::is_disconnectable,
                            size_t>::type
    disconnect(const Callable &c)
    {
        return disconnect_if([&](const slot_r_ptr &s) { return s->has_full_callable(c); });
    }

    /**
     * Disconnects all the slots.
     * Safety: thread safety depends on locking policy
     */
    void disconnect_all()
    {
        lock_type lock(m_mutex);
        clear();
    }

    /**
     * Blocks signal emission (see SignalBase::block for memory-order notes).
     */
    void block() noexcept { m_block.store(true); }

    /**
     * Unblocks signal emission.
     */
    void unblock() noexcept { m_block.store(false); }

    /**
     * Returns @c true when emission is blocked.
     */
    bool blocked() const noexcept { return m_block.load(); }

    /**
     * Returns the number of connected (non disconnected) slots.
     *
     * Complexity is linear in the number of groups (a size sum over the group
     * list). A slot disconnected via its Connection is removed from the list
     * synchronously by the cleaner; between the connection-state exchange and
     * the list removal a concurrent num_slots() may transiently count it.
     */
    size_t num_slots() const
    {
        cow_copy_type<list_type, Lockable> ref = slots_reference();
        size_t count = 0;
        for (const auto &g : detail::cow_read(ref))
        {
            count += g.slts.size();
        }
        return count;
    }

    /**
     * Returns @c true when no slots are connected.
     */
    bool empty() const { return num_slots() == 0; }

protected:
    /**
     * remove disconnected slots (Cleanable override, same lazy-swap scheme as
     * SignalBase::clean)
     */
    void clean(detail::SlotState *state) override
    {
        lock_type lock(m_mutex);
        const auto idx = state->index();
        const auto gid = state->group();

        for (auto &group : detail::cow_write(m_slots))
        {
            if (group.gid == gid)
            {
                auto &slts = group.slts;

                if (idx < slts.size() && slts[idx] && slts[idx].get() == state)
                {
                    std::swap(slts[idx], slts.back());
                    slts[idx]->index() = idx;
                    slts.pop_back();
                }

                return;
            }
        }
    }

private:
    // used to get a reference to the slots for reading
    inline cow_copy_type<list_type, Lockable> slots_reference() const
    {
        lock_type lock(m_mutex);
        return m_slots;
    }

    // run the combiner over a snapshot of slot pointers
    template <typename... A>
    result_type combiner_range(const slots_type &slot_ptrs, A &&...a) const
    {
        using invoker_type = std::function<R(const slot_r_ptr &)>;
        invoker_type invoker = [&a...](const slot_r_ptr &s) -> R { return s->call_slot_r(a...); };
        using iter_type = detail::slot_call_iterator<R, typename slots_type::const_iterator, invoker_type>;
        auto cache = std::make_shared<detail::slot_result_cache<R>>();
        Combiner comb;
        return comb(iter_type(slot_ptrs.begin(), slot_ptrs.end(), invoker, cache),
                    iter_type(slot_ptrs.end(), slot_ptrs.end(), invoker, cache));
    }

    // add the slot to the list of slots of the right group
    void add_slot(slot_r_ptr &&s)
    {
        const GroupId gid = s->group();

        lock_type lock(m_mutex);
        auto &groups = detail::cow_write(m_slots);

        auto it = groups.begin();
        while (it != groups.end() && it->gid < gid)
        {
            it++;
        }

        if (it == groups.end() || it->gid != gid)
        {
            it = groups.insert(it, {{}, gid});
        }

        s->index() = it->slts.size();
        it->slts.push_back(std::move(s));
    }

    // disconnect a slot if a condition occurs
    template <typename Cond>
    size_t disconnect_if(Cond &&cond)
    {
        lock_type lock(m_mutex);
        auto &groups = detail::cow_write(m_slots);

        size_t count = 0;

        for (auto &group : groups)
        {
            auto &slts = group.slts;
            size_t i = 0;
            while (i < slts.size())
            {
                if (cond(slts[i]))
                {
                    std::swap(slts[i], slts.back());
                    slts[i]->index() = i;
                    slts.pop_back();
                    ++count;
                }
                else
                {
                    ++i;
                }
            }
        }

        return count;
    }

    // to be called under lock: remove all the slots
    void clear() { detail::cow_write(m_slots).clear(); }

private:
    mutable Lockable m_mutex;
    cow_type<list_type, Lockable> m_slots;
    std::atomic<bool> m_block;
};


/**
 * Freestanding connect function that defers to the `signal_base::connect` member.
 */
template <typename Lockable, typename Arg, typename... T, typename... Args>
typename std::enable_if<!trait::is_signal<typename std::decay<Arg>::type>::value, Connection>::type
connect(SignalBase<Lockable, T...> &sig, Arg &&arg, Args &&...args)
{
    return sig.connect(std::forward<Arg>(arg), std::forward<Args>(args)...);
}

/**
 * Freestanding connect function that chains one signal to another.
 */
template <typename Lockable1, typename Lockable2, typename... T1, typename... T2, typename... Args>
Connection connect(SignalBase<Lockable1, T1...> &sig1, SignalBase<Lockable2, T2...> &sig2, Args &&...args)
{
    return sig1.connect(detail::signal_wrapper<SignalBase<Lockable2, T2...>>{std::addressof(sig2)},
                        std::forward<Args>(args)...);
}


/**
 * Specialization of signal_base to be used in multi-threaded contexts.
 * Slot connection, disconnection and signal emission are thread-safe.
 *
 * Recursive signal emission and emission cycles are supported too.
 */
template <typename... T>
using Signal = SignalBase<std::mutex, T...>;

/**
 * Specialization of signal_base to be used in single threaded contexts.
 * Slot connection, disconnection and signal emission are not thread-safe.
 * The performance improvement over the thread-safe variant is not impressive,
 * so this is not very useful.
 */
template <typename... T>
using SignalUnsafe = SignalBase<detail::NullMutex, T...>;

/**
 * Result-collecting signal for multi-threaded contexts: slots return a value
 * convertible to R, aggregated per emission by Combiner (S2 semantics).
 * Combiner defaults to optional_last_value<R> (last slot's result or empty).
 */
template <typename R, typename Combiner = optional_last_value<R>, typename... T>
using SignalR = SignalBaseR<R, std::mutex, Combiner, T...>;

/**
 * Result-collecting signal for single-threaded contexts.
 */
template <typename R, typename Combiner = optional_last_value<R>, typename... T>
using SignalUnsafeR = SignalBaseR<R, detail::NullMutex, Combiner, T...>;
/**
 * RAII helper that blocks a signal for the lifetime of the ScopedBlock object.
 *
 * The Qt QSignalBlocker analog: the constructor blocks emission on the signal and
 * the destructor restores unblocked state. Nesting two ScopedBlock objects on the
 * same signal in one thread is not supported - the block flag is boolean (not
 * counted), so the first destructor unblocks.
 *
 * @tparam SignalType a SignalBase specialization
 */
template <typename SignalType>
class ScopedBlock final
{
public:
    explicit ScopedBlock(SignalType &sig, bool initially_blocked = true) noexcept
        : mSignal(sig)
    {
        if (initially_blocked)
        {
            mSignal.block();
        }
        else
        {
            mSignal.unblock();
        }
    }

    ~ScopedBlock() { mSignal.unblock(); }

    ScopedBlock(const ScopedBlock &) = delete;
    ScopedBlock &operator=(const ScopedBlock &) = delete;

private:
    SignalType &mSignal;
};

} // namespace signals

template <typename... Args>
using Signal = signals::Signal<Args...>;
template <typename... Args>
using SignalUnsafe = signals::SignalUnsafe<Args...>;
// Flat R-signal aliases: R/Combiner must be named explicitly (C++11 alias
// templates cannot pack-expand a trailing Args... into SignalR's leading
// non-pack R/Combiner parameters), e.g. SignalR<int, MyCombiner, int, double>.
template <typename R, typename Combiner = signals::optional_last_value<R>, typename... Args>
using SignalR = signals::SignalR<R, Combiner, Args...>;
template <typename R, typename Combiner = signals::optional_last_value<R>, typename... Args>
using SignalUnsafeR = signals::SignalUnsafeR<R, Combiner, Args...>;

/**
 * @}
 * @}
 */
#endif
CXXKIT_END_NAMESPACE
