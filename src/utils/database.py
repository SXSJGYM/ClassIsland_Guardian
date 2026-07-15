# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 GYM_Latest

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import os
import shutil
import hashlib
import secrets
import sqlite3
from datetime import datetime
import time
import getpass
import sys
from .log import Log

# 封装数据库方法
class Database:
    def __init__(self,install_dir):
        self.install_dir = install_dir
        self.database_path = os.path.join(install_dir,'data','guardian_config.db')

        self.path = {}
        self.config = {}

    # 读取数据库，成功返回True，失败返回False
    def read_database(self):
        try:
            with sqlite3.connect(self.database_path) as conn:
                cursor = conn.cursor()
                cursor.execute('SELECT classisland_path, classisland_process_name, classisland_launcher_name FROM paths WHERE id=1')
                row = cursor.fetchone()
                if row:
                    self.path['classisland_path'] = row[0]
                    self.path['classisland_process_name'] = row[1] or 'ClassIsland.Desktop.exe'
                    self.path['classisland_launcher_name'] = row[2] or 'ClassIsland.exe'
                    if not row[0] or not os.path.isdir(row[0]):
                        Log.error(f'ClassIsland 路径无效或不存在: {row[0]}')
                        return False 
                        
                cursor.execute('SELECT password FROM config WHERE id=1')
                row = cursor.fetchone()
                if row:
                    self.config['password'] = row[0]
                
                return True

        except Exception as e:
            Log.error(f'读取配置失败: {e}')
            return False


    # 创建数据库，成功返回数据库路径，失败返回False
    def new_database(self,config_data):
        try:
            with sqlite3.connect(self.database_path) as conn:
                cursor = conn.cursor()
                cursor.executescript('''
                    CREATE TABLE IF NOT EXISTS paths(
                        id INTEGER PRIMARY KEY,
                        classisland_path TEXT,
                        classisland_process_name TEXT DEFAULT 'ClassIsland.Desktop.exe',
                        classisland_launcher_name TEXT DEFAULT 'ClassIsland.exe',
                        guardian_path TEXT         
                                );
                    CREATE TABLE IF NOT EXISTS config(
                        id INTEGER PRIMARY KEY,
                        password TEXT,
                        is_process_protect INTEGER,
                        is_prestart INTEGER,
                        is_prevent_deletion_protect INTEGER
                                );
                            ''')
                cursor.execute('''
                    INSERT OR REPLACE INTO paths (id, classisland_path, classisland_process_name, classisland_launcher_name, guardian_path)
                        VALUES (1, ?, ?, ?, ?)
                ''', (
                config_data.get('classisland_path', r'D:\ClassIsland'),
                config_data.get('classisland_process_name', 'ClassIsland.Desktop.exe'),
                config_data.get('classisland_launcher_name', 'ClassIsland.exe'),
                config_data.get('guardian_path')
                ))
                cursor.execute('''
                    INSERT OR REPLACE INTO config (id, password, is_process_protect, is_prestart, is_prevent_deletion_protect)
                        VALUES (1, ?, ?, ?, ?)
                ''', (
                config_data.get('password', ''),
                1 if config_data.get('is_process_protect', False) else 0,
                1 if config_data.get('is_prestart', False) else 0,
                1 if config_data.get('is_prevent_deletion_protect', False) else 0
                ))
                conn.commit()
                return self.database_path
            
        except Exception as e:
            Log.error(f'创建数据库时出错，错误为: {e}')
            return False
