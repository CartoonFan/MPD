// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Control.hxx"
#include "Filtered.hxx"
#include "Client.hxx"
#include "Domain.hxx"
#include "lib/fmt/ExceptionFormatter.hxx"
#include "mixer/Mixer.hxx"
#include "config/Block.hxx"
#include "Log.hxx"

#include <cassert>

/** after a failure, wait this duration before
    automatically reopening the device */
static constexpr PeriodClock::Duration REOPEN_AFTER = std::chrono::seconds(10);

AudioOutputControl::AudioOutputControl(std::unique_ptr<FilteredAudioOutput> _output,
				       AudioOutputClient &_client,
				       const ConfigBlock &block)
	:output(std::move(_output)),
	 name(output->GetName()),
	 client(_client),
	 thread(BIND_THIS_METHOD(Task)),
	 tags(block.GetBlockValue("tags", true)),
	 always_on(block.GetBlockValue("always_on", false)),
	 always_off(block.GetBlockValue("always_off", false)),
	 enabled(block.GetBlockValue("enabled", true))
{
}

AudioOutputControl::AudioOutputControl(AudioOutputControl &&src,
				       AudioOutputClient &_client) noexcept
	:output(src.Steal()),
	 name(output->GetName()),
	 client(_client),
	 thread(BIND_THIS_METHOD(Task)),
	 tags(src.tags),
	 always_on(src.always_on),
	 always_off(src.always_off)
{
}

AudioOutputControl::~AudioOutputControl() noexcept
{
	StopThread();
}

std::unique_ptr<FilteredAudioOutput>
AudioOutputControl::Steal() noexcept
{
	assert(!IsDummy());

	/* close and disable the output */
	{
		std::unique_lock lock{mutex};
		if (really_enabled && output->SupportsEnableDisable())
			CommandWait(lock, Command::DISABLE);

		enabled = really_enabled = false;
	}

	/* stop the thread */
	StopThread();

	/* now we can finally remove it */
	const std::scoped_lock protect{mutex};
	return std::exchange(output, nullptr);
}

void
AudioOutputControl::ReplaceDummy(std::unique_ptr<FilteredAudioOutput> new_output,
				 bool _enabled) noexcept
{
	assert(IsDummy());
	assert(new_output);

	{
		const std::scoped_lock protect{mutex};
		output = std::move(new_output);
		enabled = _enabled;
	}

	client.ApplyEnabled();
}

const char *
AudioOutputControl::GetPluginName() const noexcept
{
	return output ? output->GetPluginName() : "dummy";
}

const char *
AudioOutputControl::GetLogName() const noexcept
{
	assert(!IsDummy());

	return output ? output->GetLogName() : name.c_str();
}

Mixer *
AudioOutputControl::GetMixer() const noexcept
{
	return output ? output->mixer : nullptr;
}

std::map<std::string, std::string, std::less<>>
AudioOutputControl::GetAttributes() const noexcept
{
	return output
		? output->GetAttributes()
		: std::map<std::string, std::string, std::less<>>{};
}

void
AudioOutputControl::SetAttribute(std::string &&attribute_name,
				 std::string &&value)
{
	if (!output)
		throw std::runtime_error("Cannot set attribute on dummy output");

	output->SetAttribute(std::move(attribute_name), std::move(value));
}

bool
AudioOutputControl::LockSetEnabled(bool new_value) noexcept
{
	const std::scoped_lock protect{mutex};

	if (new_value == enabled)
		return false;

	enabled = new_value;
	return true;
}

bool
AudioOutputControl::LockToggleEnabled() noexcept
{
	const std::scoped_lock protect{mutex};
	return enabled = !enabled;
}

void
AudioOutputControl::WaitForCommand(std::unique_lock<Mutex> &lock) noexcept
{
	client_cond.wait(lock, [this]{ return IsCommandFinished(); });
}

void
AudioOutputControl::CommandAsync(Command cmd) noexcept
{
	assert(IsCommandFinished());

	command = cmd;
	wake_cond.notify_one();
}

void
AudioOutputControl::CommandWait(std::unique_lock<Mutex> &lock,
				Command cmd) noexcept
{
	CommandAsync(cmd);
	WaitForCommand(lock);
}

void
AudioOutputControl::LockCommandWait(Command cmd) noexcept
{
	std::unique_lock lock{mutex};
	CommandWait(lock, cmd);
}

void
AudioOutputControl::EnableAsync()
{
	if (!output)
		return;

	if (always_off)
		return;

	if (!thread.IsDefined()) {
		if (!output->SupportsEnableDisable()) {
			/* don't bother to start the thread now if the
			   device doesn't even have a enable() method;
			   just assign the variable and we're done */
			really_enabled = true;
			return;
		}

		StartThread();
	}

	CommandAsync(Command::ENABLE);
}

void
AudioOutputControl::DisableAsync() noexcept
{
	if (!output)
		return;

	if (!thread.IsDefined()) {
		if (!output->SupportsEnableDisable())
			really_enabled = false;
		else
			/* if there's no thread yet, the device cannot
			   be enabled */
			assert(!really_enabled);

		return;
	}

	CommandAsync(Command::DISABLE);
}

void
AudioOutputControl::EnableDisableAsync()
{
	if (enabled == really_enabled)
		return;

	if (enabled)
		EnableAsync();
	else
		DisableAsync();
}

inline bool
AudioOutputControl::Open(std::unique_lock<Mutex> &&lock,
			 const AudioFormat audio_format,
			 const MusicPipe &mp) noexcept
{
	assert(allow_play);
	assert(audio_format.IsValid());

	fail_timer.Reset();

	if (open && audio_format == request.audio_format) {
		assert(request.pipe == &mp || (always_on && pause));

		if (!pause && !should_reopen)
			/* already open, already the right parameters
			   - nothing needs to be done */
			return true;
	}

	request.audio_format = audio_format;
	request.pipe = &mp;

	if (!thread.IsDefined()) {
		try {
			StartThread();
		} catch (...) {
			LogError(std::current_exception());
			return false;
		}
	}

	CommandWait(lock, Command::OPEN);
	const bool open2 = open;

	if (open2 && output->mixer != nullptr) {
		lock.unlock();

		try {
			output->mixer->LockOpen();
		} catch (...) {
			FmtError(output_domain,
				 "Failed to open mixer for {:?}: {}",
				 GetName(), std::current_exception());
		}
	}

	return open2;
}

void
AudioOutputControl::CloseWait(std::unique_lock<Mutex> &lock) noexcept
{
	assert(allow_play);

	if (IsDummy())
		return;

	if (output->mixer != nullptr)
		output->mixer->LockAutoClose();

	assert(!open || !fail_timer.IsDefined());

	if (open)
		CommandWait(lock, Command::CLOSE);
	else
		fail_timer.Reset();
}

bool
AudioOutputControl::LockUpdate(const AudioFormat audio_format,
			       const MusicPipe &mp,
			       bool force) noexcept
{
	std::unique_lock lock{mutex};

	if (enabled && really_enabled) {
		if (force || !fail_timer.IsDefined() ||
		    fail_timer.Check(REOPEN_AFTER * 1000)) {
			return Open(std::move(lock), audio_format, mp);
		}
	} else if (IsOpen())
		CloseWait(lock);

	return false;
}

bool
AudioOutputControl::IsChunkConsumed(const MusicChunk &chunk) const noexcept
{
	if (!open)
		return true;

	return source.IsChunkConsumed(chunk);
}

bool
AudioOutputControl::LockIsChunkConsumed(const MusicChunk &chunk) const noexcept
{
	const std::scoped_lock protect{mutex};
	return IsChunkConsumed(chunk);
}

void
AudioOutputControl::LockPlay() noexcept
{
	const std::scoped_lock protect{mutex};

	assert(allow_play);

	if (IsOpen() && !in_playback_loop && !woken_for_play) {
		woken_for_play = true;
		wake_cond.notify_one();
	}
}

void
AudioOutputControl::LockPauseAsync() noexcept
{
	if (output && output->mixer != nullptr && !output->SupportsPause())
		/* the device has no pause mode: close the mixer,
		   unless its "global" flag is set (checked by
		   Mixer::LockAutoClose()) */
		output->mixer->LockAutoClose();

	if (output)
		output->Interrupt();

	const std::scoped_lock protect{mutex};

	assert(allow_play);
	if (IsOpen())
		CommandAsync(Command::PAUSE);
}

void
AudioOutputControl::LockDrainAsync() noexcept
{
	const std::scoped_lock protect{mutex};

	assert(allow_play);
	if (IsOpen())
		CommandAsync(Command::DRAIN);
}

void
AudioOutputControl::LockCancelAsync() noexcept
{
	if (output)
		output->Interrupt();

	const std::scoped_lock protect{mutex};

	if (IsOpen()) {
		allow_play = false;
		CommandAsync(Command::CANCEL);
	}
}

void
AudioOutputControl::LockAllowPlay() noexcept
{
	const std::scoped_lock protect{mutex};

	allow_play = true;
	if (IsOpen())
		wake_cond.notify_one();
}

void
AudioOutputControl::LockRelease() noexcept
{
	if (!output)
		return;

	output->Interrupt();

	if (output->mixer != nullptr &&
	    (!always_on || !output->SupportsPause()))
		/* the device has no pause mode: close the mixer,
		   unless its "global" flag is set (checked by
		   Mixer::LockAutoClose()) */
		output->mixer->LockAutoClose();

	std::unique_lock lock{mutex};

	assert(!open || !fail_timer.IsDefined());
	assert(allow_play);

	if (IsOpen())
		CommandWait(lock, Command::RELEASE);
	else
		fail_timer.Reset();
}

void
AudioOutputControl::LockCloseWait() noexcept
{
	assert(!open || !fail_timer.IsDefined());

	if (output)
		output->Interrupt();

	std::unique_lock lock{mutex};
	CloseWait(lock);
}

void
AudioOutputControl::BeginDestroy() noexcept
{
	if (thread.IsDefined()) {
		if (output)
			output->Interrupt();

		const std::scoped_lock protect{mutex};
		if (!killed) {
			killed = true;
			CommandAsync(Command::KILL);
		}
	}
}

void
AudioOutputControl::StopThread() noexcept
{
	BeginDestroy();

	if (thread.IsDefined())
		thread.Join();

	assert(IsCommandFinished());
}
