echo 正在清理VS2010+工程中不需要的文件
echo 请确保本文件放置在工程目录之中并关闭VS2010+
 echo 开始清理请稍等……
echo 清理sdf文件
del /q/a/f/s *.sdf
 echo 清理ipch文件
del /q/a/f/s ipch\*.*
 echo 清理Debug文件
del /q/a/f/s Debug\*.obj
 del /q/a/f/s Debug\*.tlog
 del /q/a/f/s Debug\*.log
 del /q/a/f/s Debug\*.idb
 del /q/a/f/s Debug\*.pdb
 del /q/a/f/s Debug\*.ilk
 del /q/a/f/s Debug\*.pch
 del /q/a/f/s Debug\*.bsc
 del /q/a/f/s Debug\*.sbr
 echo 清理Release文件
del /q/a/f/s Release\*.obj
 del /q/a/f/s Release\*.tlog
 del /q/a/f/s Release\*.log
 del /q/a/f/s Release\*.idb
 del /q/a/f/s Release\*.pdb
 del /q/a/f/s Release\*.ilk
 del /q/a/f/s Release\*.pch
 echo 清理Temp文件
del /q/a/f/s Temp\*.*

del /q/a/f/s *.m
rd /q/s Debug\ 
rd /q/s ipch\
 ECHO 文件清理完毕！
pause