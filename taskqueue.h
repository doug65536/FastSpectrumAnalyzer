#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <functional>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <atomic>

namespace Task {

template<typename T>
class Lazy
{
	std::atomic<T*> item;
	
public:
	typedef T value_type;
	typedef T* pointer;
	typedef T const* const_pointer;
	typedef T& reference;
	typedef T const& const_reference;
	
	Lazy();
	~Lazy();
	
	Lazy(const_reference) = delete;
	reference operator=(const_reference) = delete;
	
	operator bool() const;
	bool empty() const;
	void clear();
	pointer detach();

	const_pointer get() const;
	pointer get(bool create = true);

	const_reference operator*() const;
	reference operator*();
	
	const_pointer operator->() const;
	pointer operator->();
	
	// Execute lambda or other callable object only if the instance exists
	template<typename F>
	void ifExists(F const& f);
};

template<typename T>
Lazy<T>::Lazy()
	: item(nullptr)
{
}

template<typename T>
Lazy<T>::~Lazy()
{
	clear();
}

template<typename T>
Lazy<T>::operator bool() const
{
	return item != nullptr;
}

template<typename T>
bool Lazy<T>::empty() const
{
	return item != nullptr;
}

template<typename T>
void Lazy<T>::clear()
{
	delete item;
	item = nullptr;
}

template<typename T>
typename Lazy<T>::pointer Lazy<T>::detach()
{
	pointer p = item;
	item = nullptr;
	return p;
}

template<typename T>
typename Lazy<T>::const_pointer Lazy<T>::get() const
{
	return item;
}

template<typename T>
typename Lazy<T>::pointer Lazy<T>::get(bool create)
{
	pointer p = item, newItem;
	if (p)
		return p;

	if (create)
	{
		newItem = new value_type;

		// Race to set the pointer to the new object
		if (item.compare_exchange_strong(p, newItem))
			return newItem;

		// Another thread won the race, delete the one we made
		// Return the one the winner created
		delete newItem;

		return p;
	}

	return nullptr;
}

template<typename T>
typename Lazy<T>::const_reference Lazy<T>::operator*() const
{
	return *get();
}

template<typename T>
typename Lazy<T>::reference Lazy<T>::operator*()
{
	return *get();
}

template<typename T>
typename Lazy<T>::const_pointer Lazy<T>::operator->() const
{
	return get();
}

template<typename T>
typename Lazy<T>::pointer Lazy<T>::operator->()
{
	return get();
}

template<typename T>
template<typename F>
void Lazy<T>::ifExists(F const& f)
{
	if (item)
		f(item);
}

class Queue;

class Task
{
public:
	typedef std::function<void()> callback;
	typedef std::vector<callback> CallbackList;

	explicit Task(callback func);
	void invoke();

	void wait();
	bool wait_for(int64_t milliseconds);

	// Run callback when task is done
	void then(callback func);

private:
	typedef std::unique_lock<std::mutex> ScopedLock;

	callback func;
	CallbackList thens;
	bool done;

	std::mutex taskLock;
	std::condition_variable taskCond;
};

// This class needs its own lifetime management with refcount because it
// needs to keep itself alive while it is expecting "then" callbacks
class WhenImpl
{
public:
	WhenImpl& then(Task::callback func);

	void wait();
	bool wait_for(int64_t milliseconds);

	template<typename Rep, typename Period>
	bool wait_for(std::chrono::duration<Rep, Period> duration);

	template<typename Clock, typename Duration>
	bool wait_until(std::chrono::time_point<Clock, Duration> point);
	
	// Increase expect count by one, to prevent it becoming completed
	// too soon when adding multiple workitems
	void beginAdding();
	void endAdding();
	
private:
	typedef std::mutex Lock;
	typedef std::condition_variable Cond;
	typedef std::unique_lock<Lock> ScopedLock;

	template<typename ...T>
	explicit WhenImpl(T& ...handles);

	template<typename InputIt, typename = typename std::iterator_traits<InputIt>::iterator_category *>
	WhenImpl(InputIt st, InputIt en);

	friend class When;
	void addref();
	void release();

	WhenImpl& add();

	template<typename T1>
	WhenImpl& add(T1& handle);

	template<typename T1, typename ...T>
	WhenImpl& add(T1& handle, T& ...rest);

	void signal();

	Lazy<Lock> whenLock;
	Lazy<Cond> whenCond;
	size_t got, expect;
	std::atomic_uint refcount;
	Task::CallbackList thens;
};

template<typename ...T>
WhenImpl::WhenImpl(T& ...handles)
	: got(0)
	, expect(sizeof...(handles))
	, refcount(1)
{
	add(handles...);
}

template<typename InputIt, typename>
WhenImpl::WhenImpl(InputIt st, InputIt en)
	: got(0)
	, expect(std::distance(st, en))
	, refcount(1)
{
	for ( ; st != en; ++st)
		add(*st);
}

template<typename T1>
WhenImpl& WhenImpl::add(T1& handle)
{
	// Keep self alive until then callback
	addref();
	
	// Request a callback when this item is completed
	handle.then(std::bind(&WhenImpl::signal, this));
	
	return *this;
}

template<typename T1, typename ...T>
WhenImpl& WhenImpl::add(T1& handle, T& ...rest)
{
	add(handle);
	return add(rest...);
}

class When
{
public:
	template<typename ...T>
	When(T&& ...handles);

	template<typename InputIt, typename = typename std::iterator_traits<InputIt>::iterator_category *>
	When(InputIt st, InputIt en);

	When(const When& r);
	When(When&& r);
	~When();
	When& operator=(When r);

	When& then(Task::callback func);

	When& wait();
	bool wait_for(int64_t milliseconds);

	template<typename Rep, typename Period>
	bool wait_for(std::chrono::duration<Rep, Period> duration);

	template<typename Clock, typename Duration>
	bool wait_until(std::chrono::time_point<Clock, Duration> point);

private:
	WhenImpl* ptr;
};

template<typename ...T>
When::When(T&& ...handles)
	: ptr(new WhenImpl(std::forward<T>(handles)...))
{
}

template<typename InputIt, typename>
When::When(InputIt st, InputIt en)
	: ptr(new WhenImpl(st, en))
{
}

template<typename Rep, typename Period>
bool When::wait_for(std::chrono::duration<Rep, Period> duration)
{
	return ptr->wait_for(duration);
}

template<typename Clock, typename Duration>
bool When::wait_until(std::chrono::time_point<Clock, Duration> point)
{
	return ptr->wait_until(point);
}

// RAII object that tracks whether the caller cares about a task anymore,
// and provides a way to test-for or wait-for completion
class Handle
{
public:
	Handle();
	explicit Handle(std::shared_ptr<Task> task);
	Handle(const Handle& r);
	Handle(Handle&& r);
	Handle& operator=(Handle r);

	void release();
	bool empty() const;

	Handle& then(Task::callback func);

	Handle& wait();
	bool wait_for(int64_t milliseconds);

	template<typename Rep, typename Period>
	bool wait_for(std::chrono::duration<Rep, Period> duration);

	template<typename Clock, typename Duration>
	bool wait_until(std::chrono::time_point<Clock, Duration> point);

private:
	std::shared_ptr<Task> task;
};

class Queue
{
public:
    Queue(int workerCount = -1);
	~Queue();

    Handle queueWork(Task::callback callback, bool highPriority = true);

	static Queue &global();
    static Handle run(Task::callback work, bool highPriority = true);
	
	template<typename InputIt, typename V = typename std::iterator_traits<InputIt>::value_type>
	When run(InputIt st, InputIt en, bool highPriority);
    void finish();

private:
	typedef std::shared_ptr<Task> TaskPointer;
	typedef std::deque<TaskPointer> WorkQueue;

	When runMultiple(std::function<bool(Task::callback&)> generator,
			bool highPriority = false);

    void start(int workerCount = -1);
	void stop();

	bool done;
    
    enum FinishState {
        NOTFINISHED = 0,
        
        // finish() has been called but the queue isn't drained
        FINISHING,
        
        // queue has been drained and worker threads have exited
        FINISHED
    };
    
    FinishState finished;

	typedef std::mutex Lock;
	typedef std::unique_lock<Lock> ScopedLock;

	mutable Lock queueLock;
	std::vector<std::thread> workers;
	std::condition_variable notEmpty;
    std::condition_variable empty;
	WorkQueue queue;

	bool getMoreWork(TaskPointer &item);
	void workLoop();
};

template<typename Rep, typename Period>
bool Handle::wait_for(std::chrono::duration<Rep, Period> duration)
{
	return wait_for(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

template<typename Clock, typename Duration>
bool Handle::wait_until(std::chrono::time_point<Clock, Duration> point)
{
	return wait_for(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - point).count());
}

static inline Handle run(Task::callback work)
{
	return Queue::run(std::move(work));
}

template<typename InputIt, typename V>
When Queue::run(InputIt st, InputIt en, bool highPriority)
{
	return Queue::runMultiple([&](Task::callback& work) mutable -> bool {
		if (st != en)
		{
			work = *st;
			++st;
			return true;
		}
		return false;
	}, highPriority);
}

template<typename InputIt>
static inline When run(InputIt st, InputIt en, bool highPriority = false)
{
	return Queue::global().run(st, en, highPriority);
}

}
#endif // TASKQUEUE_H
