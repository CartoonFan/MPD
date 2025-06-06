// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "PlaylistCommands.hxx"
#include "PositionArg.hxx"
#include "Request.hxx"
#include "Instance.hxx"
#include "db/Features.hxx" // for ENABLE_DATABASE
#include "db/Interface.hxx"
#include "db/Selection.hxx"
#include "db/DatabasePlaylist.hxx"
#include "db/DatabaseSong.hxx"
#include "PlaylistSave.hxx"
#include "PlaylistFile.hxx"
#include "PlaylistError.hxx"
#include "db/PlaylistVector.hxx"
#include "SongLoader.hxx"
#include "song/Filter.hxx"
#include "song/DetachedSong.hxx"
#include "BulkEdit.hxx"
#include "playlist/Length.hxx"
#include "playlist/PlaylistQueue.hxx"
#include "playlist/Print.hxx"
#include "TimePrint.hxx"
#include "client/Client.hxx"
#include "client/Response.hxx"
#include "Mapper.hxx"
#include "fs/AllocatedPath.hxx"
#include "time/ChronoUtil.hxx"
#include "util/Exception.hxx"
#include "util/UriExtract.hxx"
#include "LocateUri.hxx"

#include <fmt/format.h>

bool
playlist_commands_available() noexcept
{
	return !map_spl_path().IsNull();
}

static void
print_spl_list(Response &r, const PlaylistVector &list)
{
	for (const auto &i : list) {
		r.Fmt("playlist: {}\n", i.name);

		if (!IsNegative(i.mtime))
			time_print(r, "Last-Modified", i.mtime);
	}
}

CommandResult
handle_save(Client &client, Request args, [[maybe_unused]] Response &r)
{
	PlaylistSaveMode mode = PlaylistSaveMode::CREATE;

	const char *mode_arg = args.GetOptional(1);
	if (mode_arg != nullptr) {
		if (StringIsEqual(mode_arg, "create"))
			mode = PlaylistSaveMode::CREATE;
		else if (StringIsEqual(mode_arg, "append"))
			mode = PlaylistSaveMode::APPEND;
		else if (StringIsEqual(mode_arg, "replace"))
			mode = PlaylistSaveMode::REPLACE;
		else
			throw std::invalid_argument("Unrecognized save mode, expected one of 'create', 'append', 'replace'");
	}

	spl_save_playlist(args.front(), mode, client.GetPlaylist());

	return CommandResult::OK;
}

CommandResult
handle_load(Client &client, Request args, [[maybe_unused]] Response &r)
{
	const auto uri = LocateUri(UriPluginKind::PLAYLIST, args.front(),
				   &client
#ifdef ENABLE_DATABASE
					   , nullptr
#endif
					   );
	RangeArg range = args.ParseOptional(1, RangeArg::All());

	auto &partition = client.GetPartition();
	const ScopeBulkEdit bulk_edit(partition);

	auto &playlist = client.GetPlaylist();
	const unsigned old_size = playlist.GetLength();

	const unsigned position = args.size() > 2
		? ParseInsertPosition(args[2], partition.playlist)
		: old_size;

	const SongLoader loader(client);
	playlist_open_into_queue(uri,
				 range.start, range.end,
				 playlist,
				 client.GetPlayerControl(), loader);

	/* invoke the RemoteTagScanner on all newly added songs */
	auto &instance = client.GetInstance();
	const unsigned new_size = playlist.GetLength();
	for (unsigned i = old_size; i < new_size; ++i)
		instance.LookupRemoteTag(playlist.queue.Get(i).GetRealURI());

	if (position < old_size) {
		const RangeArg move_range{old_size, new_size};

		try {
			partition.MoveRange(move_range, position);
		} catch (...) {
			/* ignore - shall we handle it? */
		}
	}

	return CommandResult::OK;
}

CommandResult
handle_listplaylist(Client &client, Request args, Response &r)
{
	const auto name = LocateUri(UriPluginKind::PLAYLIST, args.front(),
				    &client
#ifdef ENABLE_DATABASE
					   , nullptr
#endif
					   );

	RangeArg range = args.ParseOptional(1, RangeArg::All());

	playlist_file_print(r, client.GetPartition(), SongLoader(client),
			    name, range.start, range.end, false, nullptr);
	return CommandResult::OK;
}

CommandResult
handle_listplaylistinfo(Client &client, Request args, Response &r)
{
	const auto name = LocateUri(UriPluginKind::PLAYLIST, args.front(),
				    &client
#ifdef ENABLE_DATABASE
					   , nullptr
#endif
					   );

	RangeArg range = args.ParseOptional(1, RangeArg::All());

	playlist_file_print(r, client.GetPartition(), SongLoader(client),
			    name, range.start, range.end, true, nullptr);
	return CommandResult::OK;
}

CommandResult
handle_searchplaylist(Client &client, Request args, Response &r)
{
	const auto name = LocateUri(UriPluginKind::PLAYLIST, args.front(),
				    &client
#ifdef ENABLE_DATABASE
					   , nullptr
#endif
					   );
	args.shift();

	RangeArg window = RangeArg::All();
	if (args.size() == 3 && StringIsEqual(args[args.size() - 2], "window")) {
		window = args.ParseRange(args.size() - 1);

		args.pop_back();
		args.pop_back();
	}

	SongFilter filter;
	try {
		filter.Parse(args, true);
	} catch (...) {
		r.Error(ACK_ERROR_ARG,
			GetFullMessage(std::current_exception()).c_str());
		return CommandResult::ERROR;
	}
	filter.Optimize();

	playlist_file_print(r, client.GetPartition(), SongLoader(client),
				   name, window.start, window.end, true, &filter);
	return CommandResult::OK;
}

CommandResult
handle_playlistlength(Client &client, Request args, Response &r)
{
	const auto name = LocateUri(UriPluginKind::PLAYLIST, args.front(),
				    &client
#ifdef ENABLE_DATABASE
					   , nullptr
#endif
					   );

	playlist_file_length(r, client.GetPartition(), SongLoader(client), name);
	return CommandResult::OK;
}

CommandResult
handle_rm([[maybe_unused]] Client &client, Request args, [[maybe_unused]] Response &r)
{
	const char *const name = args.front();

	spl_delete(name);

	client.GetInstance().OnPlaylistDeleted(name);

	return CommandResult::OK;
}

CommandResult
handle_rename([[maybe_unused]] Client &client, Request args, [[maybe_unused]] Response &r)
{
	const char *const old_name = args[0];
	const char *const new_name = args[1];

	spl_rename(old_name, new_name);
	return CommandResult::OK;
}

CommandResult
handle_playlistdelete([[maybe_unused]] Client &client,
		      Request args, [[maybe_unused]] Response &r)
{
	const char *const name = args[0];
	const auto range = args.ParseRange(1);

	PlaylistFileEditor editor(name, PlaylistFileEditor::LoadMode::YES);
	editor.RemoveRange(range);
	editor.Save();
	return CommandResult::OK;
}

CommandResult
handle_playlistmove([[maybe_unused]] Client &client,
		    Request args, [[maybe_unused]] Response &r)
{
	const char *const name = args.front();

	RangeArg from = args.ParseRange(1);
	if (from.IsOpenEnded()) {
		r.Error(ACK_ERROR_ARG, "Open-ended range not supported");
		return CommandResult::ERROR;
	}

	unsigned to = args.ParseUnsigned(2);

	if (from.IsEmpty() || from.start == to)
		/* this doesn't check whether the playlist exists, but
		   what the hell.. */
		return CommandResult::OK;

	PlaylistFileEditor editor(name, PlaylistFileEditor::LoadMode::YES);
	editor.MoveIndex(from, to);
	editor.Save();
	return CommandResult::OK;
}

CommandResult
handle_playlistclear([[maybe_unused]] Client &client,
		     Request args, [[maybe_unused]] Response &r)
{
	const char *const name = args.front();

	spl_clear(name);
	return CommandResult::OK;
}

static CommandResult
handle_playlistadd_position(Client &client, const char *playlist_name,
			    const char *uri, unsigned position,
			    Response &r)
{
	PlaylistFileEditor editor{
		playlist_name,
		PlaylistFileEditor::LoadMode::TRY,
	};

	if (position > editor.size()) {
		r.Error(ACK_ERROR_ARG, "Bad position");
		return CommandResult::ERROR;
	}

	if (uri_has_scheme(uri)) {
		editor.Insert(position, uri);
	} else {
#ifdef ENABLE_DATABASE
		const DatabaseSelection selection(uri, true, nullptr);

		if (SearchInsertIntoPlaylist(client.GetDatabaseOrThrow(),
					     client.GetStorage(),
					     selection,
					     editor, position) == 0)
			/* no song was found, don't need to save */
			return CommandResult::OK;
#else
		(void)client;
		r.Error(ACK_ERROR_NO_EXIST, "No database");
		return CommandResult::ERROR;
#endif
	}

	editor.Save();

	return CommandResult::OK;
}

CommandResult
handle_playlistadd(Client &client, Request args, [[maybe_unused]] Response &r)
{
	const char *const playlist = args[0];
	const char *const uri = args[1];

	if (args.size() >= 3)
		return handle_playlistadd_position(client, playlist, uri,
						   args.ParseUnsigned(2), r);

	if (uri_has_scheme(uri)) {
		const SongLoader loader(client);
		spl_append_uri(playlist, loader, uri);
	} else {
#ifdef ENABLE_DATABASE
		const Database &db = client.GetDatabaseOrThrow();
		const DatabaseSelection selection(uri, true, nullptr);

		search_add_to_playlist(db, client.GetStorage(),
				       playlist, selection);
#else
		r.Error(ACK_ERROR_NO_EXIST, "directory or file not found");
		return CommandResult::ERROR;
#endif
	}

	return CommandResult::OK;
}

CommandResult
handle_listplaylists([[maybe_unused]] Client &client, [[maybe_unused]] Request args,
		     Response &r)
{
	print_spl_list(r, ListPlaylistFiles());
	return CommandResult::OK;
}
