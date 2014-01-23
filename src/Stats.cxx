/*
 * Copyright (C) 2003-2014 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "Stats.hxx"
#include "PlayerControl.hxx"
#include "client/Client.hxx"
#include "DatabaseSelection.hxx"
#include "DatabaseGlue.hxx"
#include "DatabasePlugin.hxx"
#include "DatabaseSimple.hxx"
#include "util/Error.hxx"
#include "system/Clock.hxx"
#include "Log.hxx"

#ifndef WIN32
/**
 * The monotonic time stamp when MPD was started.  It is used to
 * calculate the uptime.
 */
static unsigned start_time;
#endif

static DatabaseStats stats;

enum class StatsValidity : uint8_t {
	INVALID, VALID, FAILED,
};

static StatsValidity stats_validity = StatsValidity::INVALID;

void stats_global_init(void)
{
#ifndef WIN32
	start_time = MonotonicClockS();
#endif
}

void
stats_invalidate()
{
	assert(GetDatabase() != nullptr);

	stats_validity = StatsValidity::INVALID;
}

static bool
stats_update()
{
	switch (stats_validity) {
	case StatsValidity::INVALID:
		break;

	case StatsValidity::VALID:
		return true;

	case StatsValidity::FAILED:
		return false;
	}

	Error error;

	const DatabaseSelection selection("", true);
	if (GetDatabase()->GetStats(selection, stats, error)) {
		stats_validity = StatsValidity::VALID;
		return true;
	} else {
		LogError(error);

		stats_validity = StatsValidity::FAILED;
		return true;
	}
}

static void
db_stats_print(Client &client)
{
	assert(GetDatabase() != nullptr);

	if (!db_is_simple())
		/* reload statistics if we're using the "proxy"
		   database plugin */
		/* TODO: move this into the "proxy" database plugin as
		   an "idle" handler */
		stats_invalidate();

	if (!stats_update())
		return;

	client_printf(client,
		      "artists: %u\n"
		      "albums: %u\n"
		      "songs: %u\n"
		      "db_playtime: %lu\n",
		      stats.artist_count,
		      stats.album_count,
		      stats.song_count,
		      stats.total_duration);

	const time_t update_stamp = GetDatabase()->GetUpdateStamp();
	if (update_stamp > 0)
		client_printf(client,
			      "db_update: %lu\n",
			      (unsigned long)update_stamp);
}

void
stats_print(Client &client)
{
	client_printf(client,
		      "uptime: %u\n"
		      "playtime: %lu\n",
#ifdef WIN32
		      GetProcessUptimeS(),
#else
		      MonotonicClockS() - start_time,
#endif
		      (unsigned long)(client.player_control.GetTotalPlayTime() + 0.5));

	if (GetDatabase() != nullptr)
		db_stats_print(client);
}
