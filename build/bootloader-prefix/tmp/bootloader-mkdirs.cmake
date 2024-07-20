# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/ESP/Espressif/frameworks/esp-idf-v5.1.2/components/bootloader/subproject"
  "C:/Users/lucho/Desktop/Proyectos_ESP_en_C/mqtt_nodejs_grafana/build/bootloader"
  "C:/Users/lucho/Desktop/Proyectos_ESP_en_C/mqtt_nodejs_grafana/build/bootloader-prefix"
  "C:/Users/lucho/Desktop/Proyectos_ESP_en_C/mqtt_nodejs_grafana/build/bootloader-prefix/tmp"
  "C:/Users/lucho/Desktop/Proyectos_ESP_en_C/mqtt_nodejs_grafana/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/lucho/Desktop/Proyectos_ESP_en_C/mqtt_nodejs_grafana/build/bootloader-prefix/src"
  "C:/Users/lucho/Desktop/Proyectos_ESP_en_C/mqtt_nodejs_grafana/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/lucho/Desktop/Proyectos_ESP_en_C/mqtt_nodejs_grafana/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/lucho/Desktop/Proyectos_ESP_en_C/mqtt_nodejs_grafana/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
