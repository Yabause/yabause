$qtPath = 'D:\Desenvolvimento\Qt\Qt5.7.1\5.7\msvc2015_64'
$scriptDir = $PSScriptRoot

$vars = @()
$vars += [System.Tuple]::Create("QT_QMAKE_EXECUTABLE", "${qtPath}\bin\qmake.exe")
$vars += [System.Tuple]::Create("Qt5Core_DIR", "${qtPath}\lib\cmake\Qt5Core")
$vars += [System.Tuple]::Create("Qt5Gui_DIR", "${qtPath}\lib\cmake\Qt5Gui")
$vars += [System.Tuple]::Create("Qt5Multimedia_DIR", "${qtPath}\lib\cmake\Qt5Multimedia")
$vars += [System.Tuple]::Create("Qt5Network_DIR", "${qtPath}\lib\cmake\Qt5Network")
$vars += [System.Tuple]::Create("Qt5OpenGL_DIR", "${qtPath}\lib\cmake\Qt5OpenGL")
$vars += [System.Tuple]::Create("Qt5Widgets_DIR", "${qtPath}\lib\cmake\Qt5Widgets")
$vars += [System.Tuple]::Create("Qt5_DIR", "${qtPath}\lib\cmake\Qt5")
$vars += [System.Tuple]::Create("CMAKE_INSTALL_PREFIX", "${scriptDir}\..\build_install")
$vars += [System.Tuple]::Create("OPENAL_LIBRARY", "${scriptDir}\..\dependencies\OpenAL\libs\Win64\OpenAL32.lib")
$vars += [System.Tuple]::Create("OPENAL_INCLUDE_DIR", "${scriptDir}\..\dependencies\OpenAL\include")
$vars += [System.Tuple]::Create("SH2_DYNAREC", "1")
$vars += [System.Tuple]::Create("SH2_TRACE", "1")
$vars += [System.Tuple]::Create("SH2_UBC", "1")
$vars += [System.Tuple]::Create("YAB_NETWORK", "1")

$cmd = "cmake"
$args = "${scriptDir}"
$args += ' -G "Visual Studio 17 2022"'
foreach ($var in $vars) {
  $param = $var.Item1
  $value = Resolve-Path $var.Item2 -ErrorAction Ignore
  if (-not $value) {
    $value = $var.Item2
  }

  $args += " -D${param}=${value}"
}

Start-Process $cmd -ArgumentList $args -Wait -NoNewWindow