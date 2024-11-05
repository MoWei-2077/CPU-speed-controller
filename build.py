from time import time

from subprocess import run

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
