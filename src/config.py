# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 GYM_Latest

from utils.database import Database
from utils.log import Log
import ctypes
import sys
import subprocess
import getpass
import time

def clear():
    subprocess.run('cls', shell=True)

def checkpasswd():
    while(True):
        clear()
        print(f'请输入管理密码\n')
        print(f'为了安全，输入的密码将不显示')
        password = getpass.getpass('>')
        if(password == db.PASSWORD):
            return True
        else:
            print("对不起，请重试")
            time.sleep(1)

def main():
    def is_admin():
        try:
            return ctypes.windll.shell32.IsUserAnAdmin()
        except:
            return False
    if not is_admin():
        ctypes.windll.shell32.ShellExecuteW(None, "runas",
                                            sys.executable,
                                            " ".join(sys.argv), None, 1)

    global db
    db = Database(r'.\guardian_config.db',r'.\data\config_backup.db',r'config_sha256')
    db.read_database()

if __name__ == "__main__":
    main()