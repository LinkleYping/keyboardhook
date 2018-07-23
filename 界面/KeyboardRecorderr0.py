# -*- coding=utf-8 -*-
import wx
import wx.lib.buttons as buttons
import os
import win32process
import win32api
import thread
import ctypes
import subprocess
import shutil
import index
import Qpassword

CUR_PATH = os.path.dirname(__file__)
#CUR_PATH = 'C:/Users/hello/Desktop/KeyboardRecorderr0-ui'
PIC_PATH = CUR_PATH + '/pics/'
kerlogpath = 'C:/record_kernel.txt'
tranpath = 'C:/EnableCH.txt'
start = 0

# 主框架
class newframe(wx.Frame):
    def __init__(self):
        wx.Frame.__init__(self, None, -1, u'KeyboardRecorderr0', size=(1020,930))
        self.SetIcon(wx.Icon('logo.png',wx.BITMAP_TYPE_PNG))
        self.SetMaxSize((1020,930))
        self.splitterwindow()
        self.statusbar()
        self.initindex(None)
        self.cursorinit()
        self.panel1buttonadd()
        self.panel1buttonbind()
        self.Center()
        self.start()

        self.pos = 0    #记录文件读取位置
        self.chipos = 0     #记录中文记录文件读取位置
        self.size = 0
        self.chisize = 0
        self.hExe = None
        self.hExe2=None
        self.mod = 1

        #关闭事件
        self.Bind(wx.EVT_CLOSE, self.OnFormClosed, self)


    # 分割窗口
    def splitterwindow(self):
        self.sp = wx.SplitterWindow(self, style=wx.SP_LIVE_UPDATE)
        self.panel1 = wx.Panel(self.sp, -1)
        self.panel2 = wx.Panel(self.sp, -1)
        self.panel1.SetBackgroundColour('white')
        self.panel2.SetBackgroundColour('black')
        self.sp.SplitHorizontally(self.panel1, self.panel2)
        self.sp.SetMinimumPaneSize(50)

    # 状态栏
    def statusbar(self):
        self.statusbar = self.CreateStatusBar()
        self.statusbar.SetFieldsCount(3)
        self.statusbar.SetStatusText(u'[默认]中英输入法模式', 1)
        self.statusbar.SetStatusText(u'[默认]未开启进程保护', 2)

    # panel1按钮数据
    def buttondata(self):
        return [[os.path.join(PIC_PATH,'keyboard.png'), u'原始键盘模式'],
                [os.path.join(PIC_PATH, 'translate.png'), u'中英输入法模式'],
                [os.path.join(PIC_PATH, 'protect.png'), u'进程保护'],
                [os.path.join(PIC_PATH, 'cancelpro.png'), u'取消保护'],
                [os.path.join(PIC_PATH, 'qq.png'), u'QQ密码查看器'],
                [os.path.join(PIC_PATH, 'log.png'), u'ring3-输入法日志'],
                [os.path.join(PIC_PATH, 'presslog.png'), u'ring0-原始按键日志']]

    # panel1按钮创建
    def buttoncreate(self, index):
        pic = wx.Image(self.buttondata()[index][0], wx.BITMAP_TYPE_PNG).Scale(30,30).ConvertToBitmap()
        self.button = buttons.GenBitmapButton(self.panel1, -1, pic)
        self.button.SetBezelWidth(7)
        self.button.SetBackgroundColour('#eaf8f7')
        self.button.SetToolTip(self.buttondata()[index][1])
        return self.button

    # panel1按钮添加
    def panel1buttonadd(self):
        self.button1 = self.buttoncreate(0)
        self.button2 = self.buttoncreate(1)
        self.button3 = self.buttoncreate(2)
        self.button4 = self.buttoncreate(3)
        self.button5 = self.buttoncreate(4)
        self.button6 = self.buttoncreate(5)
        self.button7 = self.buttoncreate(6)
        sizer = wx.FlexGridSizer(rows=1, cols=7, hgap=0, vgap=0)
        sizer.Add(self.button1, 0)
        sizer.Add(self.button2, 0,flag = wx.RIGHT,border = 10)
        sizer.Add(self.button3, 0)
        sizer.Add(self.button4, 0,flag = wx.RIGHT,border = 10)
        sizer.Add(self.button5, 0,flag = wx.RIGHT,border = 10)
        sizer.Add(self.button6, 0)
        sizer.Add(self.button7, 0)
        sizer.AddGrowableCol(4, proportion=1)
        sizer.Layout()
        self.panel1.SetSizer(sizer)
        self.panel1.Fit()
        self.button2.Enable(False)
        self.button4.Enable(False)

    # 按钮事件绑定
    def panel1buttonbind(self):
        self.Bind(wx.EVT_BUTTON, self.kbHandler, self.button1)
        self.Bind(wx.EVT_BUTTON, self.tranHandler, self.button2)
        self.Bind(wx.EVT_BUTTON, self.proHandler, self.button3)
        self.Bind(wx.EVT_BUTTON, self.cancHandler, self.button4)
        self.Bind(wx.EVT_BUTTON, self.qqHandler, self.button5)
        self.Bind(wx.EVT_BUTTON, self.fileHandler1, self.button6)
        self.Bind(wx.EVT_BUTTON, self.fileHandler2, self.button7)

    # 自定义光标
    def cursorinit(self):
        cursorpic = wx.Image(os.path.join(PIC_PATH,'cursor.png'))
        self.cursor = wx.Cursor(cursorpic)
        self.SetCursor(self.cursor)

    def initindex(self, event):
        self.statusbar.SetStatusText(u'实时监测:[双击键盘]-[暂停/重启]', 0)
        self.index = index.indexinit(self.panel2)

    #开启键盘监控
    def start(self):
        self.hExe=win32process.CreateProcess(None,'KDriver.exe', None, None, 0,win32process.CREATE_NO_WINDOW , None, None, win32process.STARTUPINFO())
        self.hExe2=win32process.CreateProcess(None,'EnableCH.exe', None, None, 0, win32process.CREATE_NO_WINDOW, None, None, win32process.STARTUPINFO())
        dll = ctypes.WinDLL(os.path.join(CUR_PATH,'PDriver.dll'))
        dll.installpro()
        thread.start_new_thread(self.logread, ())

    # 读取动态记录
    def logread(self):
        fp = open(kerlogpath,'rb')
        chifp = open(tranpath,'rb')
        lastmod = self.mod
        self.index.tex.Clear()
        start = 1
        while start == 1: #当点击关闭按钮时变为0，退出循环
            if lastmod != self.mod:
                print '> Mod changed.'
                lastmod = self.mod
                self.index.tex.Clear()
            if self.mod == 0:
                sizetemp = os.path.getsize(kerlogpath)
                if sizetemp != self.size: #如果记录文件大小变化，则从上次读取结束位置开始读取产生的新的告警信息
                    self.size = sizetemp
                    fp.seek(self.pos, 0)
                    temp = fp.read()
                    self.index.tex.AppendText(temp.decode('gbk').encode('utf-8'))
                    self.pos = fp.tell()
            else:
                sizetemp = os.path.getsize(tranpath)
                if sizetemp != self.chisize:
                    self.chisize = sizetemp
                    chifp.seek(self.chipos, 0)
                    temp = chifp.read()
                    self.index.tex.AppendText(temp.decode('gbk').encode('utf-8'))
        fp.close()
        chifp.close()

    # 原始键盘模式
    def kbHandler(self, event):
        self.mod = 0
        self.button1.Enable(False)
        self.button2.Enable(True)
        self.statusbar.SetStatusText(u'已切换为原始键盘模式', 1)

    # 中英输入法模式
    def tranHandler(self, event):
        self.mod = 1
        self.button2.Enable(False)
        self.button1.Enable(True)
        self.statusbar.SetStatusText(u'已切换为中英输入法模式', 1)

    # 进程保护
    def proHandler(self, event):
        self.button3.Enable(False)
        self.button4.Enable(True)
        self.statusbar.SetStatusText(u'已开启进程保护', 2)
        dll = ctypes.WinDLL(os.path.join(CUR_PATH, 'Protect.dll'))
        print '> Protection enabled:-) Pid=', dll.addProtect('KeyboardRecorderr0.exe')

    # 关闭进程保护
    def cancHandler(self, event):
        self.button3.Enable(True)
        self.button4.Enable(False)
        self.statusbar.SetStatusText(u'已关闭进程保护', 2)
        dll = ctypes.WinDLL(os.path.join(CUR_PATH, 'Protect.dll'))
        print '> Protection closed! pid=', dll.cancleProtect('KeyboardRecorderr0.exe')

    # 关闭进程保护
    def qqHandler(self, event):
        #output = subprocess.Popen("Qpassword.exe", stdout=subprocess.PIPE).communicate()[0]
        output = Qpassword.getpassword()
        print output

    def fileHandler1(self, event):
        self.SysOpen(tranpath)
    def fileHandler2(self, event):
        shutil.copyfile(kerlogpath, "./ring0kblog.txt")
        self.SysOpen("ring0kblog.txt")

    def SysOpen(self,path):
        try:  # 打开外部可执行程序
            proc = subprocess.Popen(path, shell=True)
            proc.wait()  # 等待子进程结束
        except Exception, e:
            print e

    # 关闭窗体时结束进程，释放资源
    def OnFormClosed(self,event):
        dll = ctypes.WinDLL(os.path.join(CUR_PATH, 'PDriver.dll'))
        dll.uninstall()
        if self.hExe != None:
            print self.hExe
            try:
                win32api.TerminateProcess(self.hExe[0], -1)
            except:
                print 'KDriver.exe: TerminateProcess Error'
        self.hExe = None
        if self.hExe2 != None:
            print self.hExe2
            try:
                win32api.TerminateProcess(self.hExe2[0], -1)
            except:
                print 'EnableCH.exe: TerminateProcess Error'
        self.hExe2 = None
        try:
            os.remove("./ring0kblog.txt")
        except:
            pass
        self.Destroy()


if __name__ == '__main__':
    newapp = wx.App(False)
    frame = newframe()
    frame.Show()
    newapp.MainLoop()
