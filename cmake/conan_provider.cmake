set(cmake_conan_commit 76aa902fdc68450923aabaa1f8495431fc0752cf)
set(cmake_conan_sha256 63caaf42c06620d4f6139a0e17b3442738570fb9b123585ba8247afc322f8789)
set(cmake_conan_provider "${CMAKE_CURRENT_LIST_DIR}/../.cache/conan_provider-${cmake_conan_commit}.cmake")

if (NOT EXISTS "${cmake_conan_provider}")
  message(STATUS "Downloading cmake-conan ${cmake_conan_commit} to ${cmake_conan_provider}")
  file(DOWNLOAD
          "https://raw.githubusercontent.com/conan-io/cmake-conan/${cmake_conan_commit}/conan_provider.cmake"
          "${cmake_conan_provider}"
          EXPECTED_HASH SHA256=${cmake_conan_sha256}
          TLS_VERIFY ON
          STATUS cmake_conan_download_status)

  list(GET cmake_conan_download_status 0 cmake_conan_download_code)
  list(GET cmake_conan_download_status 1 cmake_conan_download_message)
  if (NOT cmake_conan_download_code EQUAL 0)
      # Remove potential partial download
      file(REMOVE "${cmake_conan_provider}")
      message(FATAL_ERROR "Failed to download cmake-conan ${cmake_conan_commit}: ${cmake_conan_download_message}")
  endif ()

  unset(cmake_conan_download_status)
  unset(cmake_conan_download_code)
  unset(cmake_conan_download_message)
endif ()

include("${cmake_conan_provider}")
