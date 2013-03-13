@echo off
:Test_Start
echo Test: Read Required Properties of Device Object %1
bacrp.exe %1 8 %1 75
bacrp.exe %1 8 %1 77
bacrp.exe %1 8 %1 79
bacrp.exe %1 8 %1 112
bacrp.exe %1 8 %1 121
bacrp.exe %1 8 %1 120
bacrp.exe %1 8 %1 70
bacrp.exe %1 8 %1 44
bacrp.exe %1 8 %1 12
bacrp.exe %1 8 %1 98
bacrp.exe %1 8 %1 139
bacrp.exe %1 8 %1 97
bacrp.exe %1 8 %1 96
bacrp.exe %1 8 %1 76 0
bacrp.exe %1 8 %1 76 
bacrp.exe %1 8 %1 62
bacrp.exe %1 8 %1 107
bacrp.exe %1 8 %1 11
bacrp.exe %1 8 %1 73
bacrp.exe %1 8 %1 30
bacrp.exe %1 8 %1 155

echo Test: Read Optional Properties of Device Object %1
bacrp.exe %1 8 %1 58
bacrp.exe %1 8 %1 28
bacrp.exe %1 8 %1 167
bacrp.exe %1 8 %1 122
bacrp.exe %1 8 %1 5
bacrp.exe %1 8 %1 57
bacrp.exe %1 8 %1 56
bacrp.exe %1 8 %1 119
bacrp.exe %1 8 %1 24
bacrp.exe %1 8 %1 10
bacrp.exe %1 8 %1 55
bacrp.exe %1 8 %1 116
bacrp.exe %1 8 %1 64
bacrp.exe %1 8 %1 63
bacrp.exe %1 8 %1 1
bacrp.exe %1 8 %1 154
bacrp.exe %1 8 %1 157
bacrp.exe %1 8 %1 153
bacrp.exe %1 8 %1 152
bacrp.exe %1 8 %1 172
bacrp.exe %1 8 %1 170
bacrp.exe %1 8 %1 169
bacrp.exe %1 8 %1 171
bacrp.exe %1 8 %1 168
