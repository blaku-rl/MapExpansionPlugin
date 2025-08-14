#pragma once
#include "../BasePlugin.h"

namespace Utils {
	enum EventThreadState {
		STOPPED,
		RUNNING,
		RESTARTING,
		STOPPING,
	};

	template <typename T>
	struct is_chrono_duration
	{
		static constexpr bool value = false;
	};

	template <typename Rep, typename Period>
	struct is_chrono_duration<std::chrono::duration<Rep, Period>>
	{
		static constexpr bool value = true;
	};

	template <typename Duration = std::chrono::milliseconds, int duration_value = 50>
	class Debounce {
		static_assert(is_chrono_duration<Duration>::value, "duration must be a std::chrono::duration");

		BasePlugin* plugin;
		std::function<void()> OnCompletion;
		std::thread debounceThread;
		std::atomic<EventThreadState> threadState;
		size_t numLoops;

	public:
		Debounce(BasePlugin* plugin, std::function<void()> complete, const size_t& loops = 100);
		void StartOrRestartEventDebounce();
		void StopEventDebounce();
		void StartEventDebounce();
	private:
		void EndDebounceThread();
	};

	template<typename Duration, int duration_value>
	Debounce<Duration, duration_value>::Debounce(BasePlugin* plugin, std::function<void()> complete, const size_t& loops)
		: plugin(plugin)
	{
		OnCompletion = complete;
		numLoops = loops;
	}

	template<typename Duration, int duration_value>
	void Debounce<Duration, duration_value>::StartOrRestartEventDebounce()
	{
		if (threadState == EventThreadState::STOPPED) {
			StartEventDebounce();
		}
		else {
			threadState = EventThreadState::RESTARTING;
		}
	}

	template<typename Duration, int duration_value>
	void Debounce<Duration, duration_value>::StopEventDebounce()
	{
		if (threadState == EventThreadState::STOPPED) return;

		threadState = EventThreadState::STOPPING;
	}

	template<typename Duration, int duration_value>
	void Debounce<Duration, duration_value>::StartEventDebounce()
	{
		if (threadState != EventThreadState::STOPPED) return;

		debounceThread = std::thread([this]() {
			threadState = EventThreadState::RUNNING;
			size_t loops = 0;
			do {
				loops++;
				if (threadState == EventThreadState::STOPPING)
					break;

				if (threadState == EventThreadState::RESTARTING) {
					threadState = EventThreadState::RUNNING;
					loops = 0;
				}

				std::this_thread::sleep_for(Duration(duration_value));
			} while (loops < numLoops);

			threadState = EventThreadState::STOPPED;

			plugin->gameWrapper->Execute([this](GameWrapper*) {
				EndDebounceThread();
				});
			});
	}

	template<typename Duration, int duration_value>
	void Debounce<Duration, duration_value>::EndDebounceThread()
	{
		if (debounceThread.joinable())
			debounceThread.join();

		OnCompletion();
	}
 }