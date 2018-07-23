# -*- coding:utf-8 -*-
import wx
from wx.adv import Animation, AnimationCtrl
import sys
import time
import wx.gizmos as gizmos
import KeyboardRecorderr0
import os

class RedirectText(object):
    def __init__(self, aWxTextCtrl):
        self.out = aWxTextCtrl

    def write(self, string):
        self.out.WriteText(string)


class indexinit():
    def __init__(self, panel2):
        self.panel2 = panel2
        self.initpanel2(None)

    def initpanel2(self, event):
        self.panel2.DestroyChildren()
        # self.panel2.bacpic = wx.Image('confbac.jpg', wx.BITMAP_TYPE_JPEG, ).Scale(1100,295).ConvertToBitmap()
        # self.panel2.bk = wx.StaticBitmap(parent=self.panel2, bitmap=self.panel2.bacpic)
        self.tex = wx.TextCtrl(parent=self.panel2, size=(900, 500),
                               style=wx.TE_MULTILINE|wx.STAY_ON_TOP|wx.TE_READONLY)
        font = wx.Font(16, wx.ROMAN, wx.NORMAL, wx.NORMAL)
        self.tex.SetFont(font)
        self.tex.SetMaxLength(20)

        self.panelpic_gif = os.path.join(KeyboardRecorderr0.PIC_PATH,'anikey.gif')
        self.panelpic = wx.Image(os.path.join(KeyboardRecorderr0.PIC_PATH,'key.png'), wx.BITMAP_TYPE_PNG).Scale(1005, 305).ConvertToBitmap()
        ani = Animation(self.panelpic_gif)
        self.keybdbk = AnimationCtrl(self.panel2, -1,ani)
        self.keybdbk.Play()
        self.tex.SetBackgroundColour('black')
        self.tex.SetForegroundColour('white')
        self.LEDinit()

        self.Sizer = wx.BoxSizer(wx.VERTICAL)
        self.Sizer.Add(self.tex,0,wx.EXPAND)
        self.Sizer.Add(self.keybdbk,1,wx.EXPAND)
        self.panel2.SetSizer(self.Sizer)
        self.panel2.Fit()

        self.temp = sys.stdout
        redir = RedirectText(self.tex)
        sys.stdout = redir

        self.keybdbk.Bind(wx.EVT_LEFT_DCLICK, self.StopAndStart)

    # LED显示器
    def LEDinit(self):
        led = gizmos.LEDNumberCtrl(self.keybdbk, -1, (830, 15), (150, 40),
                                   gizmos.LED_ALIGN_CENTER)
        led.SetForegroundColour('white')
        led.SetBackgroundColour('black')
        self.clock = led
        self.OnTimer(None)
        self.timer = wx.Timer()
        self.timer.Start(1000)
        self.timer.Bind(wx.EVT_TIMER, self.OnTimer)

    # 刷新器
    def OnTimer(self, evt):
        t = time.localtime(time.time())
        st = time.strftime("%I-%M-%S", t)
        if self.clock:
            self.clock.SetValue(st)

    def StopAndStart(self, evt):
        if KeyboardRecorderr0.start:
            KeyboardRecorderr0.start = 0
            redir = RedirectText(self.tex)
            sys.stdout = redir
            print '\n> Recording restarted:-)'
            self.keybdbk.Destroy()
            ani = Animation(self.panelpic_gif)
            self.keybdbk = AnimationCtrl(self.panel2, -1, ani)
            self.keybdbk.Play()
            self.keybdbk.Bind(wx.EVT_LEFT_DCLICK, self.StopAndStart)
            self.Sizer.Add(self.keybdbk, 1, wx.EXPAND)
            self.panel2.SetSizer(self.Sizer)
            self.panel2.Fit()
            self.LEDinit()
        else:
            KeyboardRecorderr0.start = 1
            print '\n> Recording stopped.'
            sys.stdout = self.temp
            self.keybdbk.Destroy()
            self.keybdbk = wx.StaticBitmap(parent=self.panel2, bitmap=self.panelpic)
            self.keybdbk.Bind(wx.EVT_LEFT_DCLICK, self.StopAndStart)
            self.Sizer.Add(self.keybdbk, 1, wx.EXPAND)
            self.panel2.SetSizer(self.Sizer)
            self.panel2.Fit()
            self.LEDinit()
