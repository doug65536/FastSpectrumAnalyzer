#include "taskqueue.h"
#include "asserts.h"

#include <stdexcept>

namespace Task {

template<bool enabled>
struct QueueI13n;

template<>
struct QueueI13n<false>
{
	template<typename ...A>
	static void out(A&& ...) {}
};

template<>
struct QueueI13n<true>
{
private:
	static void raw()
	{
	}

	template<typename T1>
	static void raw(T1&& first)
	{
		std::cout << std::forward<T1>(first);
	}

	template<typename T1, typename ...A>
	static void raw(T1&& first, A&& ...rest)
	{
		raw(std::forward<T1>(first));
		raw(std::forward<A>(rest)...);
	}

public:
	template<typename ...A>
	static void out(A&& ...args)
	{
		raw("QUEUE: ", std::forward<A>(args)...);
		std::cout << std::endl;
	}
};

typedef QueueI13n<false> Instr;

Queue::Queue(int workerCount)
	: done(false)
{
    start(workerCount);
}

Queue::~Queue()
{
	stop();
}

void Queue::start(int workerCount)
{
	// One worker thread per CPU
    if (workerCount < 0)
        workerCount = int(std::thread::hardware_concurrency());

    // Start thread(s)
    for (auto i = 0; i < workerCount; ++i)
		workers.emplace_back(&Queue::workLoop, this);
}

void Queue::stop()
{
	ScopedLock lock(queueLock);
	done = true;
	lock.unlock();
	notEmpty.notify_all();

	for (auto i = workers.begin(), e = workers.end(); i != e; ++i)
		i->join();

	workers.clear();
}

Queue &Queue::global()
{
	static Queue instance;
	return instance;
}

Handle Queue::run(Task::callback work, bool highPriority)
{
	return global().queueWork(std::move(work), highPriority);
}

Handle Queue::queueWork(Task::callback callback, bool highPriority)
{
	auto task = std::make_shared<Task>(std::move(callback));
	ScopedLock lock(queueLock);
	if (!highPriority)
		queue.emplace_back(task);
	else
		queue.emplace_front(task);
	lock.unlock();
	notEmpty.notify_one();
	return Handle(std::move(task));
}

When Queue::runMultiple(std::function<bool(Task::callback&)> generator, bool highPriority)
{
	typedef std::vector<TaskPointer> TaskPointerList;
	typedef std::vector<Handle> HandleList;
	TaskPointerList tasks;
	HandleList handles;
	Task::callback item;
	while (generator(item))
	{
		auto task = std::make_shared<Task>(std::move(item));
		tasks.emplace_back(task);
		handles.emplace_back(std::move(task));
	}

	ScopedLock lock(queueLock);
	if (!highPriority)
	{
		std::copy(std::move_iterator<TaskPointerList::iterator>(std::begin(tasks)),
				  std::move_iterator<TaskPointerList::iterator>(std::end(tasks)),
				  std::back_inserter(queue));
	}
	else
	{
		std::copy(std::move_iterator<TaskPointerList::reverse_iterator>(tasks.rbegin()),
				  std::move_iterator<TaskPointerList::reverse_iterator>(tasks.rend()),
				  std::front_inserter(queue));
	}
	lock.unlock();
	notEmpty.notify_all();

	return When(std::begin(handles), std::end(handles));
}

bool Queue::getMoreWork(TaskPointer &item)
{
	ScopedLock lock(queueLock);

	notEmpty.wait(lock, [this] { return !queue.empty() || done; });

	if (!done) {
		item = std::move(queue.front());
		queue.pop_front();

		Instr::out("QUEUE: took task");

		if (queue.empty())
			Instr::out("QUEUE: emptied");
	}

	return !done;
}

void Queue::workLoop()
{
	TaskPointer item;
	while (getMoreWork(item))
	{
		Instr::out("Started work");
		item->invoke();
		Instr::out("Finished work");

		// Release the object so we don't keep it alive while waiting
		item.reset();
	}
}

Task::Task(Task::callback func)
	: func(std::move(func))
	, done(false)
{
}

void Task::invoke()
{
	func();

	CallbackList thensSnapshot;
	ScopedLock lock(taskLock);
	done = true;
	thensSnapshot.swap(thens);
	lock.unlock();
	taskCond.notify_all();

	// Run all of the then callbacks
	run(thensSnapshot.begin(), thensSnapshot.end(), true);
}

void Task::wait()
{
	ScopedLock lock(taskLock);
	taskCond.wait(lock, [this]() { return done; });
}

bool Task::wait_for(int64_t milliseconds)
{
	ScopedLock lock(taskLock);

	if (milliseconds <= 0)
		return done;

	if (done)
		return true;

	return taskCond.wait_for(lock,
			std::chrono::milliseconds(milliseconds),
			[this]() { return done; });
}

void Task::then(std::function<void()> func)
{
	ScopedLock lock(taskLock);

	// If the task isn't done, register for callback
	// Otherwise, just immediately call the handler
	if (!done)
		thens.emplace_back(std::move(func));
	else
		func();
}

Handle::Handle()
	: task(nullptr)
{
}

Handle::Handle(std::shared_ptr<Task> task)
	: task(std::move(task))
{
}

Handle::Handle(const Handle& r)
	: task(r.task)
{
}

Handle::Handle(Handle&& r)
	: task(std::move(r.task))
{
}

Handle& Handle::operator =(Handle r)
{
	task = std::move(r.task);
	return *this;
}

void Handle::release()
{
	task = nullptr;
}

bool Handle::empty() const
{
	return task == nullptr;
}

Handle& Handle::then(Task::callback func)
{
	task->then(std::move(func));
	return *this;
}

Handle& Handle::wait()
{
	if (task)
	{
		task->wait();
		return *this;
	}
	throw std::logic_error("Attempt to wait for empty Task::Handle");
}

bool Handle::wait_for(int64_t milliseconds)
{
	if (task)
		return task->wait_for(milliseconds);
	throw std::logic_error("Attempt to wait for empty Task::Handle");
}

WhenImpl& WhenImpl::then(Task::callback func)
{
	if (got < expect)
		thens.emplace_back(std::move(func));
	else
		func();

	return *this;
}

void WhenImpl::wait()
{
	ScopedLock lock(*whenLock);

	if (got < expect)
		whenCond->wait(lock, [this] { return !(got < expect); });
}

bool WhenImpl::wait_for(int64_t milliseconds)
{
	ScopedLock lock(*whenLock);

	if (got < expect)
		return whenCond->wait_for(lock, std::chrono::milliseconds(milliseconds),
				[this] { return !(got < expect); });

	return true;
}

void WhenImpl::addref()
{
	++refcount;
}

void WhenImpl::release()
{
	if (--refcount == 0)
		delete this;
}

void WhenImpl::beginAdding()
{
	ScopedLock lock(*whenLock);
	++expect;
}

void WhenImpl::endAdding()
{
	// Signal releases a reference so balance that here
	addref();
	
	signal();
}

WhenImpl& WhenImpl::add()
{
	return *this;
}

void WhenImpl::signal()
{
	ScopedLock lock(*whenLock);

	AssertBreakGreater(expect, 0);
	auto newGot = ++got;
	AssertBreakLEqual(newGot, expect);
	if (newGot == expect)
	{
		Task::CallbackList thensSnapshot;
		thensSnapshot.swap(thens);
		lock.unlock();

		// If the condition variable has been created, notify it
		if (whenCond)
			whenCond->notify_all();

		// Call all then callbacks
		run(thensSnapshot.begin(), thensSnapshot.end(), true);
	}
	AssertBreakLEqual(got, expect);

	release();
}

When::When(When const& r)
	: ptr(r.ptr)
{
	if (r.ptr != nullptr)
		r.ptr->addref();
}

When::When(When&& r)
	: ptr(r.ptr)
{
	r.ptr = nullptr;
}

When::~When()
{
	if (ptr != nullptr)
		ptr->release();
}

When& When::operator=(When r)
{
	std::swap(ptr, r.ptr);
	return *this;
}

When& When::then(Task::callback func)
{
	ptr->then(std::move(func));
	return *this;
}

When& When::wait()
{
	ptr->wait();
	return *this;
}

bool When::wait_for(int64_t milliseconds)
{
	return ptr->wait_for(milliseconds);
}

}
