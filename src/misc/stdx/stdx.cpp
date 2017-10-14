#include "thread""

namespace stdx {

size_t thread::default_stack_size = 4 * 1024 * 1024;

static thread_local std::string thread_name = "main";

thread_local std::shared_ptr<std::atomic_bool> thread_is_cancelled_sp__;
thread_local std::atomic_bool* thread_is_cancelled__;



thread::thread() noexcept
	: std::thread()
{}

thread::thread(thread&& other) noexcept
	: std::thread(std::move(static_cast<std::thread&>(other)))
	, thread_is_cancelled(std::move(other.thread_is_cancelled))
{}

void thread::join()
{
	if (joinable()) {
		// best effort.. since join is not virtual this can't always override whenever join is called
		set_cancelled(true);
	}

	std::thread::join();
}
void swap( thread &lhs, thread &rhs ) noexcept
{
	std::swap(static_cast<std::thread&>(lhs), static_cast<std::thread&>(rhs));
	std::swap(lhs.thread_is_cancelled, rhs.thread_is_cancelled);
}


namespace this_thread {
	std::string get_thread_name() { return thread_name; }
	void set_thread_name(const std::string& name) { thread_name = name; }

	void set_is_cancelled(bool cancelled) { *thread_is_cancelled__ = cancelled; }
}


}
