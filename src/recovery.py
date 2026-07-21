import os
import shutil
import string
import sys
import subprocess
import time

from utils.log import Log

def clear():
    subprocess.run('cls', shell=True)

def find_guardianrecovery_path():
    """返回 GuardianRecovery 目录的路径(String)。如果未找到，返回False。"""
    for drive in string.ascii_uppercase:
        root = f"{drive}:\\"
        backup_path = os.path.join(root, "GuardianRecovery")
        if os.path.isdir(backup_path):
            return backup_path
    return False


def main():
    guardianrecovery_path = find_guardianrecovery_path()
    drive, _ = os.path.splitdrive(guardianrecovery_path)
    guardian_path = os.path.join(drive, "Program Files", "Guardian")

    Log.logfile = os.path.join(guardianrecovery_path,'guardian.log')

    clear()
    Log.info('正在初始化预启动修复恢复环境...')
    if(not guardianrecovery_path):
        Log.error('未找到可用的恢复路径。程序将会退出。')
        sys.exit(0)
    Log.info(f'寻找到了可用的恢复目录：{guardianrecovery_path}')
    Log.info('初始化成功。')

    time.sleep(2)

    clear()
    print(r'''
   ___ _            ___    _              _    ___                  _ _             ___                             
  / __| |__ _ _____|_ _|__| |__ _ _ _  __| |  / __|_  _ __ _ _ _ __| (_)__ _ _ _   | _ \___ __ _____ _____ _ _ _  _ 
 | (__| / _` (_-<_-<| |(_-< / _` | ' \/ _` | | (_ | || / _` | '_/ _` | / _` | ' \  |   / -_) _/ _ \ V / -_) '_| || |
  \___|_\__,_/__/__/___/__/_\__,_|_||_\__,_|  \___|\_,_\__,_|_| \__,_|_\__,_|_||_| |_|_\___\__\___/\_/\___|_|  \_, |
                                                                                                               |__/ 
                         
[ClassIsland Guardian Recovery] 版本：0.20260721.1
正在进行修复，请勿关闭计算机......
                
''')
    
    time.sleep(5)

    if(os.path.exists(os.path.join(guardianrecovery_path,'Guardian'))):
        try:
            try:
                shutil.rmtree(guardian_path)
                Log.info(f'成功移除了Guardian目录。')
            except:
                pass
            def copy_and_log(src, dst):
                Log.info(f'正在修复：{dst}')
                shutil.copy2(src, dst)
            shutil.copytree(
                os.path.join(guardianrecovery_path, 'Guardian'), 
                guardian_path,
                copy_function=copy_and_log
            )
            Log.info(f'修复成功。')
        except Exception as e:
            Log.error(f'修复失败，错误为：{e}')


if __name__ == "__main__":
    main()