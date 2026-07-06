import os
import sys
import subprocess
import winreg

class Exec:
    # 获取当前运行目录 
    def get_exe_dir(self):
        if getattr(sys, 'frozen', False):
            return os.path.dirname(sys.executable)
        else:
            return os.path.dirname(os.path.abspath(__file__))
    
        # 带映像劫持对抗的启动应用程序
    def start(self,path):
        '启动指定程序，并尝试删除可能存在的IFEO 传入要启动文件的目录(string)'
        name = os.path.basename(path)
        try:
            key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, f'SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\{name}', 0, winreg.KEY_WRITE)
        except FileNotFoundError:
            pass
        try:
            winreg.DeleteKey(key)
        except FileNotFoundError : 
            pass
        except PermissionError:
                return False
    
    # 结束指定进程
    def kill_process(self,name):
        '结束指定进程 传入要结束的进程名(string)'
        subprocess.run(
        ["TASKKILL", "/F", "/IM", name],
        capture_output=True
        )
