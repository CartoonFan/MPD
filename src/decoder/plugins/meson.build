decoder_plugins_sources = [
  'PcmDecoderPlugin.cxx',
]
decoder_plugins_dependencies = []

if get_option('dsd')
  decoder_plugins_sources += [
    'DsdiffDecoderPlugin.cxx',
    'DsfDecoderPlugin.cxx',
    'DsdLib.cxx',
  ]
  decoder_plugins_dependencies += libid3tag_dep
endif

if ffmpeg_dep.found()
  decoder_plugins_sources += [
    'FfmpegIo.cxx',
    'FfmpegMetaData.cxx',
    'FfmpegDecoderPlugin.cxx',
  ]
endif

adplug_dep = dependency('adplug', required: get_option('adplug'))
decoder_features.set('ENABLE_ADPLUG', adplug_dep.found())
if adplug_dep.found()
  decoder_plugins_sources += 'AdPlugDecoderPlugin.cxx'
endif

decoder_features.set('ENABLE_FLAC', flac_dep.found())
if flac_dep.found()
  decoder_plugins_sources += [
    'FlacDecoderPlugin.cxx',
    'FlacInput.cxx',
    'FlacPcm.cxx',
    'FlacDomain.cxx',
    'FlacCommon.cxx',
  ]
endif

decoder_features.set('ENABLE_VORBIS_DECODER', vorbis_dep.found())
decoder_features.set('HAVE_TREMOR', libvorbisidec_dep.found())
if vorbis_dep.found()
  decoder_plugins_sources += [
    'VorbisDecoderPlugin.cxx',
    'VorbisDomain.cxx',
  ]
endif

decoder_features.set('ENABLE_OPUS', libopus_dep.found())
if libopus_dep.found()
  decoder_plugins_sources += [
    'OpusDecoderPlugin.cxx',
    'OpusDomain.cxx',
    'OpusHead.cxx',
    'OpusTags.cxx',
  ]
endif

if ogg_dep.found()
  decoder_plugins_sources += 'OggDecoder.cxx'
endif

if xiph_dep.found()
  decoder_plugins_sources += 'OggCodec.cxx'
endif

fluidsynth_dep = dependency('fluidsynth', version: '>= 1.1', required: get_option('fluidsynth'))
decoder_features.set('ENABLE_FLUIDSYNTH', fluidsynth_dep.found())
if fluidsynth_dep.found()
  decoder_plugins_sources += 'FluidsynthDecoderPlugin.cxx'
endif

libaudiofile_dep = dependency('audiofile', version: '>= 0.3', required: get_option('audiofile'))
decoder_features.set('ENABLE_AUDIOFILE', libaudiofile_dep.found())
if libaudiofile_dep.found()
  decoder_plugins_sources += 'AudiofileDecoderPlugin.cxx'
endif

libfaad_dep = c_compiler.find_library('faad', required: get_option('faad'))
decoder_features.set('ENABLE_FAAD', libfaad_dep.found())
if libfaad_dep.found()
  decoder_plugins_sources += 'FaadDecoderPlugin.cxx'
endif

libgme_dep = dependency('libgme', version: '>= 0.6', required: get_option('gme'))
decoder_features.set('ENABLE_GME', libgme_dep.found())
if libgme_dep.found()
  decoder_plugins_sources += 'GmeDecoderPlugin.cxx'
endif

vgmstream_dep = c_compiler.find_library('vgmstream', required: get_option('vgmstream'))
decoder_features.set('ENABLE_VGMSTREAM', vgmstream_dep.found())
if vgmstream_dep.found()
  decoder_plugins_sources += 'VgmstreamDecoderPlugin.cxx'
endif

libmad_dep = c_compiler.find_library('mad', required: get_option('mad'))
decoder_features.set('ENABLE_MAD', libmad_dep.found())
if libmad_dep.found()
  decoder_plugins_sources += 'MadDecoderPlugin.cxx'
  decoder_plugins_dependencies += libid3tag_dep
endif

libmikmod_dep = dependency('libmikmod', version: '>= 3.2', required: get_option('mikmod'))
decoder_features.set('ENABLE_LIBMIKMOD', libmikmod_dep.found())
if libmikmod_dep.found()
  decoder_plugins_sources += 'MikmodDecoderPlugin.cxx'
endif

libmodplug_dep = dependency('libmodplug', required: get_option('modplug'))
decoder_features.set('ENABLE_MODPLUG', libmodplug_dep.found())
if libmodplug_dep.found()
  decoder_plugins_sources += [
    'ModplugDecoderPlugin.cxx',
    'ModCommon.cxx'
  ]
endif

libopenmpt_dep = dependency('libopenmpt', required: get_option('openmpt'))
decoder_features.set('ENABLE_OPENMPT', libopenmpt_dep.found())
if libopenmpt_dep.found()
  decoder_plugins_sources += [
    'OpenmptDecoderPlugin.cxx',
    'ModCommon.cxx'
  ]
endif

libmpcdec_dep = c_compiler.find_library('mpcdec', required: get_option('mpcdec'))
decoder_features.set('ENABLE_MPCDEC', libmpcdec_dep.found())
if libmpcdec_dep.found()
  decoder_plugins_sources += 'MpcdecDecoderPlugin.cxx'
endif

libmpg123_dep = dependency('libmpg123', required: get_option('mpg123'))
decoder_features.set('ENABLE_MPG123', libmpg123_dep.found())
if libmpg123_dep.found()
  decoder_plugins_sources += 'Mpg123DecoderPlugin.cxx'
endif

libsndfile_dep = dependency('sndfile', required: get_option('sndfile'))
decoder_features.set('ENABLE_SNDFILE', libsndfile_dep.found())
if libsndfile_dep.found()
  decoder_plugins_sources += 'SndfileDecoderPlugin.cxx'
endif

wavpack_dep = dependency('wavpack', version: '>= 5', required: get_option('wavpack'))
decoder_features.set('ENABLE_WAVPACK', wavpack_dep.found())
if wavpack_dep.found()
  decoder_plugins_sources += 'WavpackDecoderPlugin.cxx'
endif

wildmidi_required = get_option('wildmidi')
if wildmidi_required.enabled()
  # if the user has force-enabled WildMidi, allow the pkg-config test
  # to fail; after that, the find_library() check must succeed
  wildmidi_required = false
endif
wildmidi_dep = dependency('wildmidi', required: wildmidi_required)
if not wildmidi_dep.found()
  wildmidi_dep = c_compiler.find_library('WildMidi', required: get_option('wildmidi'))
endif
decoder_features.set('ENABLE_WILDMIDI', wildmidi_dep.found())
if wildmidi_dep.found()
  decoder_plugins_sources += 'WildmidiDecoderPlugin.cxx'
endif

libsidplay_dep = dependency('libsidplayfp', version: '>= 1.8', required: get_option('sidplay'))
decoder_features.set('ENABLE_SIDPLAY', libsidplay_dep.found())
if libsidplay_dep.found()
  decoder_plugins_sources += 'SidplayDecoderPlugin.cxx'
endif

decoder_plugins = static_library(
  'decoder_plugins',
  decoder_plugins_sources,
  include_directories: inc,
  dependencies: [
    decoder_plugins_dependencies,
    adplug_dep,
    ffmpeg_dep,
    flac_dep,
    fluidsynth_dep,
    libaudiofile_dep,
    libfaad_dep,
    libgme_dep,
    vgmstream_dep,
    libmad_dep,
    libmikmod_dep,
    libmodplug_dep,
    libopenmpt_dep,
    libmpcdec_dep,
    libmpg123_dep,
    libopus_dep,
    libsidplay_dep,
    libsndfile_dep,
    vorbis_dep,
    ogg_dep,
    wavpack_dep,
    wildmidi_dep,
    log_dep,
  ],
)

decoder_plugins_dep = declare_dependency(
  link_with: decoder_plugins,
  dependencies: [
    crypto_base64_dep,
    decoder_api_dep,
    pcm_dep,
  ],
)
