echo ��������VS2010+�����в���Ҫ���ļ�
echo ��ȷ�����ļ������ڹ���Ŀ¼֮�в��ر�VS2010+
 echo ��ʼ�������Եȡ���
echo ����sdf�ļ�
del /q/a/f/s *.sdf
 echo ����ipch�ļ�
del /q/a/f/s ipch\*.*
 echo ����Debug�ļ�
del /q/a/f/s Debug\*.obj
 del /q/a/f/s Debug\*.tlog
 del /q/a/f/s Debug\*.log
 del /q/a/f/s Debug\*.idb
 del /q/a/f/s Debug\*.pdb
 del /q/a/f/s Debug\*.ilk
 del /q/a/f/s Debug\*.pch
 del /q/a/f/s Debug\*.bsc
 del /q/a/f/s Debug\*.sbr
 echo ����Release�ļ�
del /q/a/f/s Release\*.obj
 del /q/a/f/s Release\*.tlog
 del /q/a/f/s Release\*.log
 del /q/a/f/s Release\*.idb
 del /q/a/f/s Release\*.pdb
 del /q/a/f/s Release\*.ilk
 del /q/a/f/s Release\*.pch
 echo ����Temp�ļ�
del /q/a/f/s Temp\*.*

del /q/a/f/s *.m
rd /q/s Debug\ 
rd /q/s ipch\
 ECHO �ļ�������ϣ�
pause