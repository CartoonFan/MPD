gtest_dep = dependency('gtest', main: true,
                       include_type: 'system',
                       fallback: ['gtest', 'gtest_main_dep'])

subdir('util')
subdir('net')
subdir('time')
subdir('tag')
subdir('playlist')

executable(
  'read_conf',
  'read_conf.cxx',
  include_directories: inc,
  dependencies: [
    config_dep,
    fmt_dep,
  ],
)

test(
  'TestRewindInputStream',
  executable(
    'TestRewindInputStream',
    'TestRewindInputStream.cxx',
    include_directories: inc,
    dependencies: [
      input_glue_dep,
      gtest_dep,
    ],
  ),
  protocol: 'gtest',
)

test(
  'test_protocol',
  executable(
    'test_protocol',
    'test_protocol.cxx',
    '../src/protocol/ArgParser.cxx',
    include_directories: inc,
    dependencies: [
      gtest_dep,
      fmt_dep,
    ],
  ),
  protocol: 'gtest',
)

test(
  'test_queue_priority',
  executable(
    'test_queue_priority',
    'test_queue_priority.cxx',
    '../src/queue/Queue.cxx',
    include_directories: inc,
    dependencies: [
      util_dep,
      gtest_dep,
    ],
  ),
  protocol: 'gtest',
)

test(
  'TestIcu',
  executable(
    'TestIcu',
    'TestIcu.cxx',
    include_directories: inc,
    dependencies: [
      util_dep,
      icu_dep,
      gtest_dep,
    ],
  ),
  protocol: 'gtest',
)

if zeroconf_dep.found()
  executable(
    'RunZeroconf',
    'RunZeroconf.cxx',
    'ShutdownHandler.cxx',
    include_directories: inc,
    dependencies: [
      zeroconf_dep,
      event_dep,
      util_dep,
    ],
  )
endif

if enable_inotify
  executable(
    'run_inotify',
    'run_inotify.cxx',
    'ShutdownHandler.cxx',
    include_directories: inc,
    dependencies: [
      log_dep,
      event_dep,
      util_dep,
    ],
  )
endif

executable(
  'run_resolver',
  'run_resolver.cxx',
  include_directories: inc,
  dependencies: [
    fmt_dep,
    net_dep,
    util_dep,
  ],
)

#
# I/O
#

executable(
  'WriteFile',
  'WriteFile.cxx',
  include_directories: inc,
  dependencies: [
    fs_dep,
    io_fs_dep,
  ],
)

executable(
  'dump_text_file',
  'dump_text_file.cxx',
  include_directories: inc,
  dependencies: [
    input_glue_dep,
    archive_glue_dep,
  ],
)

if zlib_dep.found()
  executable(
    'run_gzip',
    'run_gzip.cxx',
    include_directories: inc,
    dependencies: [
      zlib_dep,
      util_dep,
    ],
  )

  executable(
    'run_gunzip',
    'run_gunzip.cxx',
    include_directories: inc,
    dependencies: [
      fs_dep,
      io_fs_dep,
      zlib_dep,
    ],
  )
endif

#
# Filter
#

executable(
  'ParseSongFilter',
  'ParseSongFilter.cxx',
  include_directories: inc,
  dependencies: [
    song_dep,
    pcm_dep,
  ],
)

test(
  'TestSongFilter',
  executable(
    'TestSongFilter',
    'TestStringFilter.cxx',
    'TestTagSongFilter.cxx',
    include_directories: inc,
    dependencies: [
      song_dep,
      gtest_dep,
    ],
  ),
  protocol: 'gtest',
)

#
# Neighbor
#

if neighbor_glue_dep.found()
  executable(
    'run_neighbor_explorer',
    'run_neighbor_explorer.cxx',
    'ShutdownHandler.cxx',
    include_directories: inc,
    dependencies: [
      neighbor_glue_dep,
      event_dep,
    ],
  )
endif

#
# Database
#

if enable_database
  executable(
    'run_storage',
    'run_storage.cxx',
    '../src/TagSave.cxx',
    include_directories: inc,
    dependencies: [
      event_dep,
      storage_glue_dep,
      cmdline_dep,
    ],
  )

  executable(
    'DumpDatabase',
    'DumpDatabase.cxx',
    '../src/db/Configured.cxx',
    '../src/db/DatabaseGlue.cxx',
    '../src/db/Registry.cxx',
    '../src/db/Selection.cxx',
    '../src/db/PlaylistVector.cxx',
    '../src/db/DatabaseLock.cxx',
    '../src/SongSave.cxx',
    '../src/TagSave.cxx',
    include_directories: inc,
    dependencies: [
      fmt_dep,
      pcm_basic_dep,
      song_dep,
      fs_dep,
      event_dep,
      db_plugins_dep,
    ],
  )

  executable(
    'LoadDatabase',
    'LoadDatabase.cxx',
    '../src/db/PlaylistVector.cxx',
    '../src/SongSave.cxx',
    '../src/TagSave.cxx',
    include_directories: inc,
    dependencies: [
      fmt_dep,
      pcm_basic_dep,
      song_dep,
      db_plugins_dep,
      zlib_dep,
    ],
  )

  test(
    'test_translate_song',
    executable(
      'test_translate_song',
      'test_translate_song.cxx',
      '../src/playlist/PlaylistSong.cxx',
      '../src/SongLoader.cxx',
      '../src/LocateUri.cxx',
      include_directories: inc,
      dependencies: [
        event_dep,
        log_dep,
        tag_dep,
        fs_dep,
        storage_plugins_dep,
        gtest_dep,
      ],
    ),
    protocol: 'gtest',
  )
endif

#
# Input
#

run_input = executable(
  'run_input',
  'run_input.cxx',
  '../src/TagSave.cxx',
  include_directories: inc,
  dependencies: [
    log_dep,
    input_glue_dep,
    archive_glue_dep,
    cmdline_dep,
  ],
)

if curl_dep.found()
  executable(
    'RunCurl',
    'RunCurl.cxx',
    'ShutdownHandler.cxx',
    include_directories: inc,
    dependencies: [
      curl_dep,
      event_dep,
    ],
  )

  test(
    'test_icy_parser',
    executable(
      'test_icy_parser',
      'test_icy_parser.cxx',
      include_directories: inc,
      dependencies: [
        icu_dep,
        tag_dep,
        gtest_dep,
      ],
    ),
    protocol: 'gtest',
  )
endif

#
# Archive
#

if archive_glue_dep.found()
  executable(
    'visit_archive',
    'visit_archive.cxx',
    include_directories: inc,
    dependencies: [
      archive_glue_dep,
    ],
  )

  if libiso9660_dep.found()
    if find_program('mkisofs', required: false).found()
      test(
        'test_archive_iso9660',
        find_program('test_archive_iso9660.sh'),
        depends: run_input,
      )
    endif
  endif

  if libbz2_dep.found()
    if find_program('bzip2', required: false).found()
      test(
        'test_archive_bzip2',
        find_program('test_archive_bzip2.sh'),
        depends: run_input,
      )
    endif
  endif

  if libzzip_dep.found()
    if find_program('zip', required: false).found()
      test(
        'test_archive_zzip',
        find_program('test_archive_zzip.sh'),
        depends: run_input,
      )
    endif
  endif
endif

#
# Playlist
#

executable(
  'dump_playlist',
  'dump_playlist.cxx',
  '../src/TagSave.cxx',
  '../src/TagFile.cxx',
  include_directories: inc,
  dependencies: [
    fmt_dep,
    playlist_glue_dep,
    input_glue_dep,
    archive_glue_dep,
    decoder_glue_dep,
  ],
)

#
# Tag
#

if chromaprint_dep.found()
  executable(
    'RunChromaprint',
    'RunChromaprint.cxx',
    '../src/lib/chromaprint/DecoderClient.cxx',
    include_directories: inc,
    dependencies: [
      decoder_glue_dep,
      input_glue_dep,
      archive_glue_dep,
      chromaprint_dep,
      cmdline_dep,
    ],
  )
endif

#
# Decoder
#

executable(
  'run_decoder',
  'run_decoder.cxx',
  'DumpDecoderClient.cxx',
  include_directories: inc,
  dependencies: [
    log_dep,
    decoder_glue_dep,
    input_glue_dep,
    archive_glue_dep,
    cmdline_dep,
  ],
)

executable(
  'read_tags',
  'read_tags.cxx',
  include_directories: inc,
  dependencies: [
    decoder_glue_dep,
    input_glue_dep,
    archive_glue_dep,
    cmdline_dep,
  ],
)

executable(
  'ContainerScan',
  'ContainerScan.cxx',
  '../src/SongSave.cxx',
  '../src/TagSave.cxx',
  include_directories: inc,
  dependencies: [
      fmt_dep,
    decoder_glue_dep,
    input_glue_dep,
    archive_glue_dep,
  ],
)

executable(
  'ReadApeTags',
  'ReadApeTags.cxx',
  include_directories: inc,
  dependencies: [
    tag_dep,
    input_glue_dep,
    archive_glue_dep,
  ],
)

if libid3tag_dep.found()
  executable(
    'dump_rva2',
    'dump_rva2.cxx',
    include_directories: inc,
    dependencies: [
      tag_dep,
      input_glue_dep,
      archive_glue_dep,
      libid3tag_dep,
    ],
  )
endif

if ogg_dep.found()
  executable(
    'DumpOgg',
    'DumpOgg.cxx',
    include_directories: inc,
    dependencies: [
      ogg_dep,
      input_glue_dep,
      archive_glue_dep,
    ],
  )
endif
  
#
# Filter
#

test(
  'test_pcm',
  executable(
    'test_pcm',
    'TestAudioFormat.cxx',
    'test_pcm_dither.cxx',
    'test_pcm_pack.cxx',
    'test_pcm_channels.cxx',
    'test_pcm_format.cxx',
    'test_pcm_volume.cxx',
    'test_pcm_mix.cxx',
    'test_pcm_interleave.cxx',
    'test_pcm_export.cxx',
    include_directories: inc,
    dependencies: [
      pcm_dep,
      gtest_dep,
    ],
  ),
  protocol: 'gtest',
)

executable(
  'run_filter',
  'run_filter.cxx',
  'ReadFrames.cxx',
  include_directories: inc,
  dependencies: [
    filter_glue_dep,
    cmdline_dep,
  ],
)

executable(
  'software_volume',
  'software_volume.cxx',
  include_directories: inc,
  dependencies: [
    pcm_dep,
  ],
)

executable(
  'run_normalize',
  'run_normalize.cxx',
  include_directories: inc,
  dependencies: [
    pcm_dep,
  ],
)

executable(
  'run_convert',
  'run_convert.cxx',
  include_directories: inc,
  dependencies: [
    log_dep,
    pcm_dep,
    config_dep,
    cmdline_dep,
  ],
)

executable(
  'RunReplayGainAnalyzer',
  'RunReplayGainAnalyzer.cxx',
  'ReadFrames.cxx',
  include_directories: inc,
  dependencies: [
    pcm_dep,
    io_dep,
  ],
)

executable(
  'RunMixRampAnalyzer',
  'RunMixRampAnalyzer.cxx',
  'ReadFrames.cxx',
  include_directories: inc,
  dependencies: [
    pcm_dep,
    io_dep,
  ],
)

#
# Encoder
#

if need_encoder
  executable(
    'run_encoder',
    'run_encoder.cxx',
    include_directories: inc,
    dependencies: [
      encoder_glue_dep,
    ],
  )

  executable(
    'test_vorbis_encoder',
    'test_vorbis_encoder.cxx',
    include_directories: inc,
    dependencies: [
      encoder_glue_dep,
    ],
  )
endif
  
#
# Output
#

executable(
  'run_output',
  'run_output.cxx',
  include_directories: inc,
  dependencies: [
    output_registry_dep,
    encoder_glue_dep,
    event_dep,
    cmdline_dep,
  ],
)

#
# Mixer
#

if alsa_dep.found()
  # this debug program is still ALSA specific

  executable(
    'read_mixer',
    'read_mixer.cxx',
    include_directories: inc,
    dependencies: [
      mixer_plugins_dep,
    ],
  )
endif

subdir('fs')
