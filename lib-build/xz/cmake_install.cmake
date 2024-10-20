# Install script for directory: /Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/res")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Library/Developer/CommandLineTools/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "liblzma_Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/liblzma.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/liblzma.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/liblzma.a")
    execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/liblzma.a")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "liblzma_Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/src/liblzma/api/" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "liblzma_Development" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/liblzma/liblzma-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/liblzma/liblzma-targets.cmake"
         "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/CMakeFiles/Export/6194817f435e7429e84a3ab7f926109c/liblzma-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/liblzma/liblzma-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/liblzma/liblzma-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/liblzma" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/CMakeFiles/Export/6194817f435e7429e84a3ab7f926109c/liblzma-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/liblzma" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/CMakeFiles/Export/6194817f435e7429e84a3ab7f926109c/liblzma-targets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "liblzma_Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/liblzma" TYPE FILE FILES
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/liblzma-config.cmake"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/liblzma-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "liblzma_Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/liblzma.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xzdec_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/xzdec")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xzdec" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xzdec")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/strip" -u -r "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xzdec")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "lzmadec_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/lzmadec")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lzmadec" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lzmadec")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/strip" -u -r "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lzmadec")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xzdec_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man1" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/src/xzdec/xzdec.1")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xzdec_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/man//man1")
                 foreach(L lzmadec)
                     file(CREATE_LINK "xzdec.1"
                                      "${D}/${L}.1"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "lzmainfo_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/lzmainfo")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lzmainfo" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lzmainfo")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/strip" -u -r "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lzmainfo")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "lzmainfo_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man1" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/src/lzmainfo/lzmainfo.1")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "lzmainfo_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/man//man1")
                 foreach(L )
                     file(CREATE_LINK "lzmainfo.1"
                                      "${D}/${L}.1"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ca/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/ca.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/cs/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/cs.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/da/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/da.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/de/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/de.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/eo/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/eo.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/es/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/es.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/fi/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/fi.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/fr/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/fr.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/hr/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/hr.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/hu/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/hu.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/it/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/it.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ko/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/ko.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/pl/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/pl.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/pt/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/pt.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/pt_BR/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/pt_BR.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/ro/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/ro.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/sr/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/sr.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/sv/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/sv.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/tr/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/tr.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/uk/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/uk.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/vi/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/vi.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/zh_CN/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/zh_CN.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/locale/zh_TW/LC_MESSAGES" TYPE FILE RENAME "xz.mo" FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/zh_TW.gmo")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/xz")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xz" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xz")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/strip" -u -r "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/xz")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin")
                 foreach(L unxz;xzcat;lzma;unlzma;lzcat)
                     file(CREATE_LINK "xz"
                                      "${D}/${L}"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man1" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/src/xz/xz.1")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "xz_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/man//man1")
                 foreach(L unxz;xzcat;lzma;unlzma;lzcat)
                     file(CREATE_LINK "xz.1"
                                      "${D}/${L}.1"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/xzdiff")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/xzgrep")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/xzmore")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE PROGRAM FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/xzless")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin")
                 foreach(L xzcmp;lzdiff;lzcmp)
                     file(CREATE_LINK "xzdiff"
                                      "${D}/${L}"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin")
                 foreach(L xzegrep;xzfgrep;lzgrep;lzegrep;lzfgrep)
                     file(CREATE_LINK "xzgrep"
                                      "${D}/${L}"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin")
                 foreach(L lzmore)
                     file(CREATE_LINK "xzmore"
                                      "${D}/${L}"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Runtime" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin")
                 foreach(L lzless)
                     file(CREATE_LINK "xzless"
                                      "${D}/${L}"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man1" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/src/scripts/xzdiff.1")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/man//man1")
                 foreach(L xzcmp;lzdiff;lzcmp)
                     file(CREATE_LINK "xzdiff.1"
                                      "${D}/${L}.1"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man1" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/src/scripts/xzgrep.1")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/man//man1")
                 foreach(L xzegrep;xzfgrep;lzgrep;lzegrep;lzfgrep)
                     file(CREATE_LINK "xzgrep.1"
                                      "${D}/${L}.1"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man1" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/src/scripts/xzmore.1")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/man//man1")
                 foreach(L lzmore)
                     file(CREATE_LINK "xzmore.1"
                                      "${D}/${L}.1"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man1" TYPE FILE FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/src/scripts/xzless.1")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "scripts_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  set(D "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/man//man1")
                 foreach(L lzless)
                     file(CREATE_LINK "xzless.1"
                                      "${D}/${L}.1"
                                      SYMBOLIC)
                 endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "liblzma_Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/xz" TYPE DIRECTORY FILES "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/doc/examples")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Documentation" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/xz" TYPE FILE FILES
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/AUTHORS"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/COPYING"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/COPYING.0BSD"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/COPYING.GPLv2"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/NEWS"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/README"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/THANKS"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/doc/faq.txt"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/doc/history.txt"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/doc/lzma-file-format.txt"
    "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib/xz/doc/xz-file-format.txt"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
  file(WRITE "/Users/valeriiapletneva/Desktop/testGMOne/GamesmanOne/lib-build/xz/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
