import os
import zipfile
import shutil

from utils.exec import Exec
from utils.log import Log

class Snapshot:
    def __init__(self,db):
        self.db = db
        self.snapshot_dir = os.path.join(Exec.get_exe_dir(),'data','snapshot')
        
    def list_snapshot(self):
        '列出所有可用的快照 成功返回列表，失败返回False'
        try:
            filelist = os.listdir(self.list_snapshot)
            return filelist
        except:
            return False
        
    def restore_snapshot(self,name):
        '恢复到指定的快照 传入要恢复快照的文件名(string) 成功返回True，失败返回False'
        Exec.kill_process(self.db.path.get('classisland_process_name'))
        if os.path.exists(os.path.join(self.snapshot_dir,name)):
            try:
                try:
                    shutil.rmtree(self.db.path.get('classisland_path'))
                except:
                    pass
                os.mkdir(self.db.path.get('classisland_path'))
                with zipfile.ZipFile(os.path.join(self.snapshot_dir,name), 'r') as zf:
                    zf.extractall(path=self.db.path.get('classisland_path'))
                    Log.info(f'成功恢复到指定快照：{name}')
                    return True
            except Exception as e:
                Log.error(f'恢复时出错，错误为：{e}')
                return False
        else:
            Log.error(f'恢复时出错，指定的快照文件不存在')
            return False