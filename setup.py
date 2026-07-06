import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import os
import shutil
import hashlib
import secrets
import sqlite3
import psutil
from datetime import datetime
from prompt_toolkit import prompt
import time
import getpass
import ctypes
import sys
import subprocess

from utils.database import Database

version = 'v0.20260531.1'
maketime = '2026-5-31 9:08'

class config():
    def __init__(self):
        self.classisland_path = self.find_classisland()
        self.guardian_path = None
        self.classisland_backup_path = r'C:\install.wim'
        self.password = ''
        self.is_process_protect = False
        self.is_prevent_deletion_protect = False
        self.is_prestart = False

        self.path = {}

    def find_classisland(self):
        process_names = ['ClassIsland.Desktop.exe']
        for proc in psutil.process_iter(['name', 'exe']):
                if proc.info['name'] in process_names:
                    exe_path = proc.info['exe']
                    install_dir = os.path.dirname(os.path.dirname(exe_path))
                    return install_dir
        return None
    
    def set_config(self):
        pass

def clear():
    subprocess.run('cls', shell=True)

def install():
    clear()

    print('将在倒计时结束后开始安装.')
    for i in range(5,0,-1):
        print(i,end=" ",flush=True)
        time.sleep(1)
    
    print('\n')
    

def configure():
    # 起始页面
    while(True):
        clear()
        print(f'ClassIsland Guardian Installer')
        print(f'版本 {version}')
        print(f'生成于 {maketime}\n')
        print(f'欢迎，该配置向导会帮助你完成 ClassIsland Guardian 的安装与配置.\n')
        print(f'要开始，请输入 y 并按 ENTER')
        print(f'要退出向导，请输入 n 并按 ENTER')
        result = input('>')
        if(result ==  'y'):
            break
        elif(result ==  'n'):
            return

    # 选择classisland路径
    while(True):
        clear()
        print(f'请输入ClassIsland的路径')
        print(f'输入完成后 按 ENTER .')
        path = prompt('>',default=(config.find_classisland() or ''))
        if(path != ''):
            config.path[""] = path
            break
        else:
            print('请输入路径！')
            time.sleep(1)
    
    # 选择guardian路径
    while(True):
        clear()
        print(f'请输入程序安装路径,所有文件及配置都会存储到该目录下')
        print()
        print(f'输入完成后 按 ENTER .')
        path = prompt('>',default=r'C:\guardian')
        if(path != ''):
            config.guardian_path = path
            break
        else:
            print('请输入路径！')
            time.sleep(1)

    # 选择全量备份文件存储路径
    # 由于重构，所有数据都存在 安装目录\data 下，该功能废弃，发布时将删除
    # clear()
    # print(f'请输入全量备份文件存储路径，该文件用于存储ClassIsland全量备份，用于修复')
    # print(r'默认值: C:\install.wim')
    # print()
    # print(f'输入完成后 按 ENTER .')
    # print(f'若要使用默认值，按 ENTER .')
    # path = input('>')
    # if(path != ''):
    #     config.backup_path = path

    clear()
    # 选择是否开启预启动修复
    print(f'是否需要开启 预启动修复\n')
    print(f'要开启，请输入 y 并按 ENTER')
    print(f'要关闭，请输入 任何其他值  并按 ENTER')
    if(input('>') ==  'y'):
        config.is_prestart = True
    else:
        config.is_prestart = False

    # 选择密码保护
    while(True):
        clear()
        print(f'请设置管理密码（留空则不启用）\n')
        print(f'为了安全，输入的密码将不显示')
        password = getpass.getpass('>')
        if(password):
            print(f'确认密码')
            password_twice = getpass.getpass('>')
            if(password == password_twice):
                config.password = password
                break
            else:
                print(f'输入的两次密码不一致！请重试...')
                time.sleep(1)
        else:
            break
    
    # 开始安装
    while(True):
        clear()
        print(f'所有配置均已结束.\n')
        print(f'输入 install 并按 ENTER 以开始安装')
        print(f'安装过程中 ClassIsland 将短暂关闭')
        if(input('>') ==  'install'):
            install()
            break

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

    db = Database('.\\','','')
    global config
    config = config()
    configure()

if __name__ == "__main__":
    main()