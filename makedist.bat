rmdir /s/q dist
rmdir /s/q build
pyinstaller ^
  --add-data=logo2-32.png:./logo2-32.png ^
  --add-binary=bin/ffmpeg/ffmpeg.exe:./bin/ffmpeg ^
  --add-binary=bin/wpd-mtp-helper.exe:./bin ^
  --windowed ^
  --onefile ^
  jogjams.py
