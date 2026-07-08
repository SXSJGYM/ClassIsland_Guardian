import psutil
import os
import subprocess
import time

from utils.log import Log
from utils.exec import Exec

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
        '启动Classisland。 成功返回True，失败返回False'
        return Exec.start(os.path.join(self.db.path.get('classisland_path'),self.db.get('classisland_launcher_name')))
        
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