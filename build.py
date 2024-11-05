from time import time

from subprocess import run

from shutil import copy2
from os import makedirs
from os.path import join,basename
from glob import glob
# 源文件路径
source_file = 'D:\AndroidStudio\AndroidProject\CSController\app\build\outputs\apk\release\CS-Controller*.apk'

# 目标文件夹路径
destination_folder = './magisk/'
def moveF():
    # 源文件夹路径
    source_folder = "D:\\AndroidStudio\\AndroidProject\\CSController\\app\\build\\outputs\\apk\\release\\"

    # 目标文件夹路径
    destination_folder = './magisk/'

    # 确保目标文件夹存在
    makedirs(destination_folder, exist_ok=True)

    # 使用通配符匹配文件
    apk_files = glob(join(source_folder, 'CS-Controller*.apk'))

    # 复制每个匹配的文件
    for apk_file in apk_files:
        destination_file = join(destination_folder, basename(apk_file))
        copy2(apk_file, destination_file)

def update(file_path, new_str , to_type):
    # 用于存储更新后的内容
    updated_lines = []

    # 读取文件
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()

    # 修改versionCode的值
    for line in lines:
        if line.startswith(f'{to_type}='):
            # 更新versionCode行的内容
            updated_line = f'{to_type}={new_str}\n'
            updated_lines.append(updated_line)
        else:
            updated_lines.append(line)

    # 写回文件
    with open(file_path, 'w', encoding='utf-8') as file:
        file.writelines(updated_lines)

# 使用方法
file_path = './magisk/module.prop'  # 文件路径
new_version_code = input("请输入要更改的版本号：")
new_version_name = input("请输入要更改的版本名：")
if input("请选择是否移入CSController(除了xshe谁也不要输入1)") == "1":
    moveF()
if new_version_code == "0" :
    new_version_code = round(time())
    print(f"时间戳版本号：{new_version_code}")
else:
    pass

update(file_path, new_version_code , "versionCode")

update(file_path, new_version_name , "version")

print("更新版本信息完毕，开始编译&打包")

# PowerShell脚本的路径
ps_script_path = './build_pack.ps1'

# 构建PowerShell命令
cmd = ['powershell', '-ExecutionPolicy', 'Bypass', '-File', ps_script_path]

# 执行PowerShell脚本
result = run(cmd, capture_output=True, text=True)

# 输出结果
print("编译脚本输出:")
print(result.stdout)

print("编译脚本报错:")
print(result.stderr)

if result.returncode != 0:
    print(f"错误码 {result.returncode}")
else:
    print("成功")
