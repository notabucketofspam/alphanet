@ECHO off
CALL build.bat
START "zapper" /D . "node" .
