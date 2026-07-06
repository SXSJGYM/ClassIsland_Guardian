import psutil
import subprocess
import os
import zipfile
import shutil
import time
import sys
from datetime import datetime 

from utils.log import log
from utils.database import database
from utils.process import process

# 获取当前运行目录 
def get_exe_dir():
    if getattr(sys, 'frozen', False):
        return os.path.dirname(sys.executable)
    else:
        return os.path.dirname(os.path.abspath(__file__))

# 设置计划任务
def register_self():
    task_name = "ClassIslandGuardian"
    exe_path = sys.executable if getattr(sys, 'frozen', False) else __file__

    try:
        result = subprocess.run(
            ["schtasks", "/Query", "/TN", task_name],
            capture_output=True, text=True
        )
        if result.returncode == 0:
            log.info('自启动项存在')
            return True
        
        log.info('自启动项不存在，创建')
        subprocess.run([
            "schtasks", "/Create",
            "/TN", task_name,
            "/TR", f'"{exe_path}"',
            "/SC", "ONLOGON",
            "/DELAY", "0001:00",
            "/RL", "HIGHEST",
            "/F"
        ], capture_output=True)
        log.info('创建成功')
        return True
    except Exception:
        log.info('')
        return False

# 进程丢失后处理函数
def process_missing():
    if process.start_classisland():
        log.warn('尝试拉起ClassIsland')
        time.sleep(30)
        status = process.classisland_status_check()
        if status == 1:
            log.info('拉起成功，ClassIsland进程正常')
            return
    
    log.warn('拉起失败，ClassIsland进程仍未在运行，尝试修复')
    if process.restore_from_backup():
        log.info('修复成功，尝试拉起ClassIsland')
        if process.start_classisland():
            log.info('拉起成功，ClassIsland进程正常')
            return

    log.error('修复失败')

def main():
    try: 
        global db
        global process

        db = database(os.path.joinget_exe_dir())
        db.read_database()
        process = process(db)
        log.info('Classisland Guardian 已启动')

        register_self()

        while(True):
            
            time.sleep(120)
            status = process.classisland_status_check()
            if status == 1:
                log.info('检查ClassIsland，进程正常')
            elif status == 0:
                process_missing()
            elif status >= 2:
                log.info(f'(Warning) 检测到 {status} 个ClassIsland进程，确认卡死，正在重启')
                process.reboot_classisland()
    except Exception as e:
            try:
                log.error(f'发生无法处理的错误：{e}')
            except:
                logfile = os.path.join(os.path.dirname(sys.executable) if getattr(sys, 'frozen', False) else os.path.dirname(__file__), 'guardian.log')
                with open(logfile, 'a') as f:
                    f.write(f'{datetime.now()}: {e}\n')
            time.sleep(5)
            os.execl(sys.executable, sys.executable, *sys.argv)

if __name__ == "__main__":
    main()