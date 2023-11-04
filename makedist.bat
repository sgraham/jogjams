rmdir /s/q dist
rmdir /s/q build
"C:\Program Files\ImageMagick-7.1.1-Q16-HDRI\convert.exe" logo2.png -define icon:auto-resize=256,48,32,24,16 logo2.ico
pyinstaller ^
  --add-data=logo2-32.png:. ^
  --add-binary=bin/ffmpeg/ffmpeg.exe:./bin/ffmpeg ^
  --add-binary=bin/wpd-mtp-helper.exe:./bin ^
  --icon=logo2.ico ^
  --windowed ^
  --onefile ^
  jogjams.py
