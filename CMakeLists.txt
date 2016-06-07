cmake_minimum_required(VERSION 3.5)
project(ModernMIDI LANGUAGES CXX)

option(MODERNMIDI_USE_JACK "Use JACK instead of ALSA as a backend on Linux" OFF)
option(MODERNMIDI_DEBUG "Enable some debugging features" OFF)

set(MODERNMIDI_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ModernMIDI")
set(RTMIDI_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rtmidi")

set(MODERNMIDI_HEADERS
"${MODERNMIDI_SRC_DIR}/concurrent_queue.h"
"${MODERNMIDI_SRC_DIR}/midi_event.h"
"${MODERNMIDI_SRC_DIR}/midi_file_reader.h"
"${MODERNMIDI_SRC_DIR}/midi_file_writer.h"
"${MODERNMIDI_SRC_DIR}/midi_input.h"
"${MODERNMIDI_SRC_DIR}/midi_message.h"
"${MODERNMIDI_SRC_DIR}/midi_output.h"
"${MODERNMIDI_SRC_DIR}/midi_utils.h"
"${MODERNMIDI_SRC_DIR}/modernmidi.h"
"${MODERNMIDI_SRC_DIR}/music_theory.h"
"${MODERNMIDI_SRC_DIR}/port_manager.h"
"${MODERNMIDI_SRC_DIR}/sequence_player.h"
"${MODERNMIDI_SRC_DIR}/timer.h"
)

set(MODERNMIDI_SRCS
"${MODERNMIDI_SRC_DIR}/midi_file_reader.cpp"
"${MODERNMIDI_SRC_DIR}/midi_file_writer.cpp"
"${MODERNMIDI_SRC_DIR}/midi_input.cpp"
"${MODERNMIDI_SRC_DIR}/midi_output.cpp"
"${MODERNMIDI_SRC_DIR}/midi_utils.cpp"
"${MODERNMIDI_SRC_DIR}/music_theory.cpp"
"${MODERNMIDI_SRC_DIR}/port_manager.cpp"
)
if(WIN32 OR APPLE)
set(MODERNMIDI_SRCS ${MODERNMIDI_SRCS}
"${MODERNMIDI_SRC_DIR}/sequence_player.cpp"
)
endif()

set(RTMIDI_HEADERS "${RTMIDI_SRC_DIR}/RtMidi.h")
set(RTMIDI_SRCS "${RTMIDI_SRC_DIR}/RtMidi.cpp")

add_library(ModernMIDI 
	${MODERNMIDI_SRCS} ${MODERNMIDI_HEADERS}
	${RTMIDI_SRCS} ${RTMIDI_HEADERS})

target_include_directories(ModernMIDI 
	PRIVATE ${MODERNMIDI_SRC_DIR}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MODERNMIDI_DEBUG)
  target_compile_definitions(ModernMIDI PRIVATE __RTMIDI_DEBUG__)
endif()

# Platform-specific setup
if(APPLE)
  target_compile_definitions(ModernMIDI PRIVATE __MACOSX_CORE__)

  find_library(COREMIDI_LIBRARY CoreMIDI)
  find_library(COREAUDIO_LIBRARY CoreAudio)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)

  target_link_libraries(ModernMIDI PRIVATE 
        ${COREFOUNDATION_LIBRARY} 
        ${COREAUDIO_LIBRARY} 
        ${COREMIDI_LIBRARY})

elseif(WIN32)
  target_compile_definitions(ModernMIDI PRIVATE __WINDOWS_MM__)
  target_link_libraries(ModernMIDI PRIVATE winmm)

elseif(UNIX)
  if(MODERNMIDI_USE_JACK)
    target_compile_definitions(ModernMIDI PRIVATE __UNIX_JACK__)
    target_link_libraries(ModernMIDI PRIVATE jack)
  else()
    target_compile_definitions(ModernMIDI PRIVATE __LINUX_ALSA__)
    target_link_libraries(ModernMIDI PRIVATE asound pthread)
  endif()
endif()