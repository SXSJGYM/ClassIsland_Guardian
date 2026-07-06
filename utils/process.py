import psutil
import os
import subprocess
import time
from utils.log import Log

class Process:
    def __init__(self,db):
        self.db = db

    # 检查classisland进程数量并返回
    def check_classisland_status(self):
        process_quantity = 0
        for proc in psutil.process_iter(['name']):
            if proc.info['name'] == self.db.path.get('classisland_process_name'):
                process_quantity += 1; 
        return process_quantity
    
    # 启动ClassIsland
    def start_classisland(self):
        exe_path = os.path.join(self.db.path.get('classisland_path'), self.db.path.get('classisland_launcher_name'))
        if os.path.exists(exe_path):
            subprocess.Popen(
                [exe_path],
                cwd=self.db.path.get('classisland_path')
            )
            return True
        else:
            return False
        
    # 重启ClassIsland
    def reboot_classisland(self):
        subprocess.run(
            ['TASKKILL', '/F', '/IM', self.db.path.get('classisland_process_name')],
            capture_output=True
        )
        time.sleep(3)
        if self.start_classisland():
            Log.info('重启成功')
            return
        Log.error('重启失败')