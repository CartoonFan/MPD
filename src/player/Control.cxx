// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Control.hxx"
#include "Outputs.hxx"
#include "Listener.hxx"
#include "song/DetachedSong.hxx"

#include <algorithm>
#include <cassert>

PlayerControl::PlayerControl(PlayerListener &_listener,
			     PlayerOutputs &_outputs,
			     InputCacheManager *_input_cache,
			     const PlayerConfig &_config) noexcept
	:listener(_listener), outputs(_outputs),
	 input_cache(_input_cache),
	 config(_config),
	 thread(BIND_THIS_METHOD(RunThread))

{
}

PlayerControl::~PlayerControl() noexcept
{
	assert(!occupied);
}

bool
PlayerControl::WaitOutputConsumed(std::unique_lock<Mutex> &lock,
				  unsigned threshold) noexcept
{
	bool result = outputs.CheckPipe() < threshold;
	if (!result && command == PlayerCommand::NONE) {
		Wait(lock);
		result = outputs.CheckPipe() < threshold;
	}

	return result;
}

void
PlayerControl::Play(std::unique_ptr<DetachedSong> song)
{
	if (!thread.IsDefined())
		thread.Start();

	assert(song != nullptr);

	std::unique_lock lock{mutex};
	SeekLocked(lock, std::move(song), SongTime::zero());

	if (state == PlayerState::PAUSE)
		/* if the player was paused previously, we need to
		   unpause it */
		PauseLocked(lock);
}

void
PlayerControl::LockCancel() noexcept
{
	assert(thread.IsDefined());

	LockSynchronousCommand(PlayerCommand::CANCEL);
	assert(next_song == nullptr);
}

void
PlayerControl::LockStop() noexcept
{
	if (!thread.IsDefined())
		return;

	LockSynchronousCommand(PlayerCommand::CLOSE_AUDIO);
	assert(next_song == nullptr);

	listener.OnPlayerStateChanged();
}

void
PlayerControl::LockUpdateAudio() noexcept
{
	if (!thread.IsDefined())
		return;

	LockSynchronousCommand(PlayerCommand::UPDATE_AUDIO);
}

void
PlayerControl::Kill() noexcept
{
	if (!thread.IsDefined())
		return;

	LockSynchronousCommand(PlayerCommand::EXIT);
	thread.Join();

	listener.OnPlayerStateChanged();
}

inline void
PlayerControl::PauseLocked(std::unique_lock<Mutex> &lock) noexcept
{
	if (state != PlayerState::STOP) {
		SynchronousCommand(lock, PlayerCommand::PAUSE);
		listener.OnPlayerStateChanged();
	}
}

void
PlayerControl::LockPause() noexcept
{
	std::unique_lock lock{mutex};
	PauseLocked(lock);
}

void
PlayerControl::LockSetPause(bool pause_flag) noexcept
{
	if (!thread.IsDefined())
		return;

	std::unique_lock lock{mutex};

	switch (state) {
	case PlayerState::STOP:
		break;

	case PlayerState::PLAY:
		if (pause_flag)
			PauseLocked(lock);
		break;

	case PlayerState::PAUSE:
		if (!pause_flag)
			PauseLocked(lock);
		break;
	}
}

void
PlayerControl::LockSetBorderPause(bool _border_pause) noexcept
{
	const std::scoped_lock protect{mutex};
	border_pause = _border_pause;
}

PlayerStatus
PlayerControl::LockGetStatus() noexcept
{
	PlayerStatus status;

	std::unique_lock lock{mutex};
	if (!occupied && thread.IsDefined())
		SynchronousCommand(lock, PlayerCommand::REFRESH);

	status.state = state;

	if (state != PlayerState::STOP) {
		status.bit_rate = bit_rate;
		status.audio_format = audio_format;
		status.total_time = total_time;
		status.elapsed_time = elapsed_time;
	}

	return status;
}

void
PlayerControl::SetError(PlayerError type, std::exception_ptr &&_error) noexcept
{
	assert(type != PlayerError::NONE);
	assert(_error);

	error_type = type;
	error = std::move(_error);

	// TODO: is it ok to call this while holding mutex lock?
	listener.OnPlayerError();
}

void
PlayerControl::LockClearError() noexcept
{
	const std::scoped_lock protect{mutex};
	ClearError();
}

void
PlayerControl::LockSetTaggedSong(const DetachedSong &song) noexcept
{
	const std::scoped_lock protect{mutex};
	tagged_song.reset();
	tagged_song = std::make_unique<DetachedSong>(song);
}

void
PlayerControl::ClearTaggedSong() noexcept
{
	tagged_song.reset();
}

std::unique_ptr<DetachedSong>
PlayerControl::ReadTaggedSong() noexcept
{
	return std::exchange(tagged_song, nullptr);
}

std::unique_ptr<DetachedSong>
PlayerControl::LockReadTaggedSong() noexcept
{
	const std::scoped_lock protect{mutex};
	return ReadTaggedSong();
}

void
PlayerControl::LockEnqueueSong(std::unique_ptr<DetachedSong> song) noexcept
{
	assert(thread.IsDefined());
	assert(song != nullptr);

	std::unique_lock lock{mutex};
	EnqueueSongLocked(lock, std::move(song));
}

inline void
PlayerControl::EnqueueSongLocked(std::unique_lock<Mutex> &lock,
				 std::unique_ptr<DetachedSong> &&song) noexcept
{
	assert(song != nullptr);
	assert(next_song == nullptr);

	next_song = std::move(song);
	seek_time = SongTime::zero();
	SynchronousCommand(lock, PlayerCommand::QUEUE);
}

inline void
PlayerControl::SeekLocked(std::unique_lock<Mutex> &lock,
			  std::unique_ptr<DetachedSong> &&song, SongTime t)
{
	assert(song != nullptr);

	/* to issue the SEEK command below, we need to clear the
	   "next_song" attribute with the CANCEL command */
	/* optimization TODO: if the decoder happens to decode that
	   song already, don't cancel that */
	if (next_song != nullptr)
		SynchronousCommand(lock, PlayerCommand::CANCEL);

	assert(next_song == nullptr);

	ClearError();
	next_song = std::move(song);
	seek_time = t;
	SynchronousCommand(lock, PlayerCommand::SEEK);

	assert(next_song == nullptr);

	/* the SEEK command is asynchronous; until completion, the
	   "seeking" flag is set */
	while (seeking)
		ClientWait(lock);

	if (error_type != PlayerError::NONE) {
		assert(error);
		std::rethrow_exception(error);
	}

	assert(!error);
}

void
PlayerControl::LockSeek(std::unique_ptr<DetachedSong> song, SongTime t)
{
	if (!thread.IsDefined())
		thread.Start();

	assert(song != nullptr);

	std::unique_lock lock{mutex};
	SeekLocked(lock, std::move(song), t);
}

void
PlayerControl::SetCrossFade(FloatDuration duration) noexcept
{
	cross_fade.duration = std::max(duration, FloatDuration::zero());

	listener.OnPlayerOptionsChanged();
}

void
PlayerControl::SetMixRampDb(float _mixramp_db) noexcept
{
	cross_fade.mixramp_db = _mixramp_db;

	listener.OnPlayerOptionsChanged();
}

void
PlayerControl::SetMixRampDelay(FloatDuration _mixramp_delay) noexcept
{
	cross_fade.mixramp_delay = _mixramp_delay;

	listener.OnPlayerOptionsChanged();
}
